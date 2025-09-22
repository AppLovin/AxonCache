// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package main

import (
	"fmt"
	"math/rand"
	"os"
	"path/filepath"
	"time"

	axoncache "github.com/AppLovin/AxonCache"
	"github.com/bmatsuo/lmdb-go/lmdb"
	"github.com/dustin/go-humanize"
	"github.com/syndtr/goleveldb/leveldb"
	bolt "go.etcd.io/bbolt"

	"bufio"
	"errors"
	"runtime"
	"strconv"
	"strings"

	log "github.com/sirupsen/logrus"
	"golang.org/x/sys/unix"
)

// PeakRSS returns the process-wide peak resident set size in bytes.
// Works on Linux and macOS. No sampling needed (kernel tracks the peak).
func PeakRSS() (uint64, error) {
	switch runtime.GOOS {
	case "linux":
		// Read VmHWM (High Water Mark) from /proc/self/status (in kB)
		f, err := os.Open("/proc/self/status")
		if err != nil {
			return 0, err
		}
		defer f.Close()
		sc := bufio.NewScanner(f)
		for sc.Scan() {
			line := sc.Text()
			if strings.HasPrefix(line, "VmHWM:") {
				// Example: "VmHWM:	   123456 kB"
				fields := strings.Fields(line)
				if len(fields) >= 2 {
					kb, err := strconv.ParseUint(fields[1], 10, 64)
					if err != nil {
						return 0, err
					}
					return kb * 1024, nil
				}
			}
		}
		if err := sc.Err(); err != nil {
			return 0, err
		}
		return 0, errors.New("VmHWM not found")
	default:
		// macOS & BSDs: unix.Getrusage.RuMaxrss is in BYTES on Darwin, KB on many BSDs.
		// We handle Darwin explicitly; for others we assume KB if not Darwin.
		var ru unix.Rusage
		if err := unix.Getrusage(unix.RUSAGE_SELF, &ru); err != nil {
			return 0, err
		}
		maxrss := uint64(ru.Maxrss)
		if runtime.GOOS == "darwin" {
			return maxrss, nil // bytes on macOS
		}
		return maxrss * 1024, nil // most others report KB
	}
}

func RunBenchmark() {
	const keysCount = 1 * 1000 * 1000

	keys := make([][]byte, keysCount)
	vals := make([][]byte, keysCount)
	for i := range keysCount {
		key := fmt.Sprintf("key_%d", i+1)
		val := fmt.Sprintf("val_%d", i+1)
		keys[i] = []byte(key)
		vals[i] = []byte(val)
	}

	RunBenchmarkNativeMap(keysCount, keys, vals)
	log.Println()
	RunBenchmarkAxonCache(keysCount, keys, vals)
	log.Println()
	RunBenchmarkLmdb(keysCount, keys, vals)
	log.Println()
	// Commented, got really slow at insertion time for some reason ...
	// RunBenchmarkBoltDB(keysCount, keys, vals)
	// log.Println()
	RunBenchmarkLevelDB(keysCount, keys, vals)
	log.Println()

	maxRss, _ := PeakRSS()
	log.Printf("Peak RSS: %s\n", humanize.Bytes(maxRss))
}

func RunBenchmarkNativeMap(keysCount int, keys, vals [][]byte) {
	log.Println("Using native go maps")
	start := time.Now()

	cache := make(map[string][]byte)

	for i, key := range keys {
		cache[string(key)] = []byte(vals[i])
	}

	elapsed := time.Since(start).Seconds()
	qps := float64(keysCount) / elapsed
	log.Printf("Inserted %s keys in %.3fs (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))

	// Randomize lookup order
	rand.Shuffle(len(keys), func(i, j int) { keys[i], keys[j] = keys[j], keys[i] })
	stringKeys := make([]string, 0)
	for _, key := range keys {
		stringKeys = append(stringKeys, string(key))
	}

	start = time.Now()

	for _, stringKey := range stringKeys {
		_, _ = cache[stringKey]
	}

	elapsed = time.Since(start).Seconds()
	qps = float64(keysCount) / elapsed

	log.Printf("Looked up %s keys in %.2f seconds (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))
}

func RunBenchmarkAxonCache(keysCount int, keys, vals [][]byte) {
	log.Println("Using AxonCache")
	start := time.Now()

	destinationFolder := "./"
	taskName := "bench_cache"

	if err := os.MkdirAll(destinationFolder, os.ModePerm); err != nil {
		log.Fatal(err)
	}

	cache, err := axoncache.NewCacheWriter(
		&axoncache.CacheWriterOptions{
			TaskName:              taskName,
			DestinationFolder:     destinationFolder,
			SettingsLocation:      "",
			NumberOfKeySlots:      uint64(keysCount * 2),
			GenerateTimestampFile: true,
			RenameCacheFile:       true,
			CacheType:             5,
		})
	defer cache.Delete()
	if err != nil {
		log.Fatal(err)
	}

	for i, key := range keys {
		if err = cache.InsertKey([]byte(key), []byte(vals[i]), 0); err != nil {
			log.Fatal(err)
		}
	}

	err, _ = cache.FinishCacheCreation()
	if err != nil {
		log.Fatal(err)
	}

	elapsed := time.Since(start).Seconds()
	qps := float64(keysCount) / elapsed
	log.Printf("Inserted %s keys in %.3fs (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))

	rcache, err := axoncache.NewCacheReader(
		&axoncache.CacheReaderOptions{
			TaskName:               taskName,
			DestinationFolder:      destinationFolder,
			IsPreloadMemoryEnabled: true,
			UpdatePeriod:           time.Second})
	defer rcache.Delete()
	if err != nil {
		log.Fatal(err)
	}

	// Randomize lookup order
	rand.Shuffle(len(keys), func(i, j int) { keys[i], keys[j] = keys[j], keys[i] })

	start = time.Now()

	for _, key := range keys {
		// log.Printf("Looking up key %s", key)
		_, _ = rcache.GetKey(string(key))
	}

	elapsed = time.Since(start).Seconds()
	qps = float64(keysCount) / elapsed

	log.Printf("Looked up %s keys in %.2f seconds (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))
}

func must(err error) {
	if err != nil {
		log.Fatal(err)
	}
}

func RunBenchmarkLmdb(keysCount int, keys, vals [][]byte) {
	log.Println("Using AxonCache")

	// temp DB path
	path := filepath.Join(os.TempDir(), "lmdb_ro_bench")
	_ = os.RemoveAll(path)
	if err := os.MkdirAll(path, 0o755); err != nil {
		log.Fatalf("mkdir: %v", err)
	}

	// ---------- Write phase ----------
	{
		env, err := lmdb.NewEnv()
		must(err)
		// Ensure close even on panic
		defer env.Close()

		// Set BEFORE Open
		// For 10k—millions of small K/V, 64–512 MiB is fine; bump as needed.
		must(env.SetMapSize(1 << 32)) // 4 GiB
		must(env.SetMaxDBs(2))
		must(env.Open(path, 0, 0o644))

		start := time.Now()
		must(env.Update(func(txn *lmdb.Txn) error {
			dbi, err := txn.OpenDBI("kv", lmdb.Create)
			if err != nil {
				return err
			}
			for i := 0; i < keysCount; i++ {
				if err := txn.Put(dbi, keys[i], vals[i], 0); err != nil {
					return err
				}
			}
			return nil
		}))
		// ensure durability before closing
		must(env.Sync(true))
		elapsed := time.Since(start).Seconds()
		qps := float64(keysCount) / elapsed
		log.Printf("Inserted %s keys in %.3fs (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))

		// Close the RW env before reopening RO
		env.Close()
	}

	// ---------- Read-only lookups ----------
	{
		env, err := lmdb.NewEnv()
		must(err)
		defer env.Close()

		// Must still be set before Open (even in RO)
		must(env.SetMapSize(1 << 32)) // 4 GiB
		must(env.SetMaxDBs(2))
		must(env.Open(path, lmdb.Readonly, 0o644))

		start := time.Now()
		missed := 0
		err = env.View(func(txn *lmdb.Txn) error {
			dbi, err := txn.OpenDBI("kv", 0)
			if err != nil {
				return err
			}
			for i := range keysCount {
				v, err := txn.Get(dbi, keys[i])
				if lmdb.IsNotFound(err) {
					missed++
					continue
				}
				if err != nil {
					return err
				}
				_ = v[0] // touch to prevent optimization
			}
			return nil
		})
		must(err)
		elapsed := time.Since(start).Seconds()
		qps := float64(keysCount) / elapsed
		log.Printf("Looked up %s keys in %.2f seconds (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))
	}

	// cleanup temp dir
	_ = os.RemoveAll(path)
}

func RunBenchmarkBoltDB(keysCount int, keys, vals [][]byte) {
	log.Println("Using BoltDB")
	start := time.Now()

	const (
		dbPath = "bolt.db"
		bucket = "main"
	)

	// Open (creates file if it doesn't exist). 1s timeout avoids "database is locked" on slow FS.
	db, err := bolt.Open(dbPath, 0o600, &bolt.Options{Timeout: time.Second})
	must(err)
	defer db.Close()

	// Create bucket if needed.
	must(db.Update(func(tx *bolt.Tx) error {
		_, err := tx.CreateBucketIfNotExists([]byte(bucket))
		return err
	}))

	// INSERT: batch in one write transaction for speed.
	must(db.Update(func(tx *bolt.Tx) error {
		b := tx.Bucket([]byte(bucket))
		for i := range keysCount {
			if err := b.Put(keys[i], vals[i]); err != nil {
				return err
			}
		}
		return nil
	}))

	elapsed := time.Since(start).Seconds()
	qps := float64(keysCount) / elapsed
	log.Printf("Inserted %s keys in %.3fs (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))

	// Randomize lookup order (optional).
	rand.Shuffle(len(keys), func(i, j int) { keys[i], keys[j] = keys[j], keys[i] })

	// LOOKUP: read-only transaction.
	start = time.Now()
	must(db.View(func(tx *bolt.Tx) error {
		b := tx.Bucket([]byte(bucket))
		missed := 0
		for _, k := range keys {
			v := b.Get(k)
			if v == nil {
				missed++
			}
			// use v if needed
		}
		if missed > 0 {
			log.Printf("missed %d keys\n", missed)
		}
		return nil
	}))
	elapsed = time.Since(start).Seconds()
	qps = float64(keysCount) / elapsed
	log.Printf("Looked up %s keys in %.2f seconds (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))
}

// RunLevelDBOnce opens/creates a DB at dbPath, inserts key_<n> -> val_<n>,
// then looks them up in random order. Returns (insertDuration, lookupDuration).
func RunBenchmarkLevelDB(keysCount int, keys, vals [][]byte) error {
	log.Println("Using LevelDB")
	start := time.Now()

	dbPath := "level.db"

	db, err := leveldb.OpenFile(dbPath, nil)
	if err != nil {
		return err
	}
	defer db.Close()

	// INSERT (batched)
	batch := new(leveldb.Batch)
	for i := range keys {
		batch.Put(keys[i], vals[i])
	}
	if err := db.Write(batch, nil); err != nil {
		return err
	}

	elapsed := time.Since(start).Seconds()
	qps := float64(keysCount) / elapsed
	log.Printf("Inserted %s keys in %.3fs (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))

	// Randomize lookup order
	rand.Shuffle(len(keys), func(i, j int) { keys[i], keys[j] = keys[j], keys[i] })

	// LOOKUP
	start = time.Now()
	for _, k := range keys {
		_, _ = db.Get(k, nil) // ignore value + errors
	}

	elapsed = time.Since(start).Seconds()
	qps = float64(keysCount) / elapsed
	log.Printf("Inserted %s keys in %.3fs (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))

	return nil
}
