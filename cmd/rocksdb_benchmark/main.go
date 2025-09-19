// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package main

import (
	"bufio"
	"errors"
	"fmt"
	"math/rand"
	"os"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"time"

	log "github.com/sirupsen/logrus"

	"github.com/dustin/go-humanize"
	gorocksdb "github.com/linxGnu/grocksdb"
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
	const keysCount = 5 * 1000 * 1000

	keys := make([][]byte, keysCount)
	vals := make([][]byte, keysCount)
	for i := 0; i < keysCount; i++ {
		keys[i] = []byte(fmt.Sprintf("key_%d", i+1))
		vals[i] = []byte(fmt.Sprintf("key_%d", i+1))
	}

	RunBenchmarkRocksDB(keysCount, keys, vals)
	log.Println()

	maxRss, _ := PeakRSS()
	log.Printf("Peak RSS: %s\n", humanize.Bytes(maxRss))
}

func RunBenchmarkRocksDB(keysCount int, keys, vals [][]byte) {
	log.Println("Using GoRocksDB")
	start := time.Now()

	const (
		cacheMB = 2048 // block cache MB (tune to your RAM)
	)

	tmpdir, err := os.MkdirTemp("", "rocksdb-pointget")
	if err != nil {
		panic(err)
	}
	defer os.RemoveAll(tmpdir)
	dbPath := filepath.Join(tmpdir, "db")
	_ = os.RemoveAll(dbPath)
	if err := os.MkdirAll(dbPath, 0o755); err != nil {
		panic(err)
	}

	// ----- Options tuned for point lookups -----
	opts := gorocksdb.NewDefaultOptions()
	defer opts.Destroy()
	opts.SetCreateIfMissing(true)
	opts.IncreaseParallelism(8)
	opts.SetMaxBackgroundCompactions(8)
	opts.SetMaxBackgroundFlushes(4)
	opts.SetMaxOpenFiles(-1) // keep table handles open
	// 	opts.SetCompression(gorocksdb.NoCompression) // avoid CPU on reads
	opts.SetWriteBufferSize(128 << 20) // bigger memtables to form larger sorted runs
	opts.SetLevel0FileNumCompactionTrigger(8)
	opts.SetTargetFileSizeBase(64 << 20)
	opts.SetMaxBytesForLevelBase(512 << 20)

	// Block-based table tuned for fast point gets
	bbto := gorocksdb.NewDefaultBlockBasedTableOptions()
	defer bbto.Destroy()
	// blockCache := gorocksdb.NewLRUCache(int(cacheMB) << 20)
	blockCache := gorocksdb.NewLRUCache(uint64(cacheMB) << 20)
	defer blockCache.Destroy()
	filter := gorocksdb.NewBloomFilter(10) // 10 bits per key
	defer filter.Destroy()

	bbto.SetBlockCache(blockCache)
	bbto.SetFilterPolicy(filter)
	bbto.SetCacheIndexAndFilterBlocks(true)        // keep index+filter in cache
	bbto.SetPinL0FilterAndIndexBlocksInCache(true) // pin hottest levels
	bbto.SetWholeKeyFiltering(true)                // whole-key Bloom for point gets
	bbto.SetBlockSize(4 << 10)                     // smaller blocks -> fewer extraneous bytes per point get
	bbto.SetPartitionFilters(true)                 // partitioned filters scale better
	// You can try hash index for seek-heavy workloads; binary search is usually fine for gets.
	// bbto.SetIndexType(gorocksdb.KBinarySearch) // default; uncomment if exposed in your version

	opts.SetBlockBasedTableFactory(bbto)

	// Open DB
	db, err := gorocksdb.OpenDb(opts, dbPath)
	if err != nil {
		panic(err)
	}
	defer db.Close()

	// Write options for fast bulk load
	wo := gorocksdb.NewDefaultWriteOptions()
	defer wo.Destroy()
	wo.SetSync(false)
	wo.DisableWAL(true) // faster initial load; not durable

	// Read options for point gets
	ro := gorocksdb.NewDefaultReadOptions()
	defer ro.Destroy()
	ro.SetFillCache(true)        // we want to warm the cache
	ro.SetVerifyChecksums(false) // skip checksum on hot-path

	for i := 0; i < keysCount; i++ {
		if err := db.Put(wo, keys[i], vals[i]); err != nil {
			panic(err)
		}
	}

	// Make data query-friendly: flush + compact to reduce overlap and improve cache locality.
	db.Flush(gorocksdb.NewDefaultFlushOptions())
	db.CompactRange(gorocksdb.Range{})

	elapsed := time.Since(start).Seconds()
	qps := float64(keysCount) / elapsed
	log.Printf("Inserted %s keys in %.3fs (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))

	// Randomize lookup order
	rand.Shuffle(len(keys), func(i, j int) { keys[i], keys[j] = keys[j], keys[i] })

	start = time.Now()

	for _, key := range keys {
		// log.Printf("Looking up key %s", key)
		_, _ = db.Get(ro, key)
	}

	elapsed = time.Since(start).Seconds()
	qps = float64(keysCount) / elapsed

	log.Printf("Looked up %s keys in %.2f seconds (%s keys/sec)\n", humanize.Comma(int64(keysCount)), elapsed, humanize.Comma(int64(qps)))
}

func main() {
	RunBenchmark()
}
