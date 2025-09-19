// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package main

import (
	"fmt"
	"os"
	"os/signal"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"syscall"
	"time"

	axoncache "github.com/AppLovin/AxonCache"
	"github.com/AppLovin/AxonCache/internal/core"

	log "github.com/sirupsen/logrus"
	"github.com/spf13/cobra"
)

func RootCmd(cmd *cobra.Command, args []string) {
}

func KvScan(cmd *cobra.Command, args []string) {
	path := args[0]

	scanner := NewKeyValueFileScanner()
	defer scanner.Close()

	err := scanner.SetPath(path)
	if err != nil {
		log.Fatal(err)
	}

	queries := make(map[int]bool)

	err = scanner.ForEach(func(k, v []byte, queryId int) error {
		queries[queryId] = true
		return nil
	})
	if err != nil {
		log.Fatal(err)
	}

	keys := make([]int, 0, len(queries))
	for k := range queries {
		keys = append(keys, k)
	}
	sort.Ints(keys)

	for _, key := range keys {
		fmt.Println(key)
	}

	log.Printf("keys: %d", scanner.KeysCount)
	log.Printf("key space: %d", scanner.KeySpaceSize)
	log.Printf("value space: %d", scanner.ValueSpaceSize)
}

func KvSplit(cmd *cobra.Command, args []string) {
	path := args[0]

	scanner := NewKeyValueFileScanner()
	defer scanner.Close()

	err := scanner.SetPath(path)
	if err != nil {
		log.Fatal(err)
	}

	// A map to hold open file handles per queryId
	files := make(map[int]*os.File)

	// Ensure all files are closed when we're done
	defer func() {
		for _, f := range files {
			f.Close()
		}
	}()

	err = scanner.ForEach(func(k, v []byte, queryId int) error {
		// Check if we already have a file for this queryId
		f, exists := files[queryId]
		if !exists {
			filename := fmt.Sprintf("%s_%d.txt", path, queryId)
			var err error
			f, err = os.Create(filepath.Clean(filename)) // Safer filename handling
			if err != nil {
				return fmt.Errorf("failed to create file for queryId %d: %w", queryId, err)
			}
			files[queryId] = f
		}

		// Write k=v\n to the corresponding file
		_, err := fmt.Fprintf(f, "%s=%s\n", k, v)
		return err
	})

	if err != nil {
		log.Fatal(err)
	}
}

func parsePath(path string) (directory, cacheName, timestamp string, err error) {
	// Get the directory name
	directory = filepath.Dir(path)

	// Get the base name of the file
	fileName := filepath.Base(path)

	// Split the file name into parts based on "."
	parts := strings.Split(fileName, ".")
	if len(parts) != 3 {
		return "", "", "", fmt.Errorf("invalid path format: %s", path)
	}

	cacheName = parts[0]
	timestamp = parts[1]

	return directory, cacheName, timestamp, nil
}

func Comparo(cmd *cobra.Command, args []string) {
	path := args[0]
	cachePath := args[1]

	directory, cacheName, timestamp, err := parsePath(cachePath)
	if err != nil {
		log.Fatal(err)
	}

	cache, err := axoncache.NewCacheReader(
		&axoncache.CacheReaderOptions{
			TaskName:          cacheName,
			DestinationFolder: directory,
			UpdatePeriod:      time.Second})
	defer cache.Delete()

	if err != nil {
		log.Fatal(err)
	}

	err = cache.Update(timestamp)
	if err != nil {
		log.Fatal(err)
	}

	scanner := NewKeyValueFileScanner()
	scanner.EnableKeysDeduplication()
	defer scanner.Close()

	err = scanner.SetPath(path)
	if err != nil {
		log.Fatal(err)
	}

	mismatchingKeys := 0

	err = scanner.ForEach(func(k, v []byte, queryId int) error {

		key := string(k)
		// Vector keys insert a special character (1) before the actual key to denote their type
		if k[0] == 1 {
			key = string(k[1:])
		}

		keyType, err := cache.GetKeyType(key)
		if err != nil {
			log.Errorf("key not found: '%s'", key)
			return nil
		}

		switch keyType {
		case "String":
			{
				cacheValue, _ := cache.GetKey(key)

				if cacheValue != string(v) {
					mismatchingKeys += 1
					log.Errorf("key mismatch: %s\nGo Value: %s\nAxonCache Value %s", string(k), string(v), cacheValue)
				}
			}
		case "Double":
			{
				cacheValue, _ := cache.GetDouble(key)

				goValue, err := strconv.ParseFloat(string(v), 64)
				if err != nil {
					return fmt.Errorf("cannot convert %v to double, %s", k, err)
				}

				if cacheValue != goValue {
					mismatchingKeys += 1
					log.Errorf("key mismatch: %s\nGo Value: %f\nAxonCache Value %f", string(k), goValue, cacheValue)
				}
			}
		case "Int":
			{
				cacheValue, _ := cache.GetInt(key)

				goValue, err := strconv.Atoi(string(v))
				if err != nil {
					return fmt.Errorf("cannot convert %v to int, %s", k, err)
				}

				if cacheValue != goValue {
					mismatchingKeys += 1
					log.Errorf("key mismatch: %s\nGo Value: %d\nAxonCache Value %d", string(k), goValue, cacheValue)
				}
			}
		case "Int64":
			{
				cacheValue, _ := cache.GetLong(key)

				goValue, err := strconv.Atoi(string(v))
				if err != nil {
					return fmt.Errorf("cannot convert %v to int64, %s", k, err)
				}

				if cacheValue != int64(goValue) {
					mismatchingKeys += 1
					log.Errorf("key mismatch: %s\nGo Value: %d\nAxonCache Value %d", string(k), goValue, cacheValue)
				}
			}
		case "StringList":
			{
				vectorStrings, _ := cache.GetVector(key)

				parts := strings.Split(string(v), "|")
				badKey := false
				for i, str := range vectorStrings {
					if str != parts[i] {
						badKey = true
						log.Errorf("key mismatch: %s\nGo Value: %s\nAxonCache Value %s", string(k), parts[i], str)
					}
				}

				if badKey {
					mismatchingKeys += 1
				}
			}
		case "FloatList":
			{
				vectorFloats, _ := cache.GetVectorFloat(key)

				parts := strings.Split(string(v), ":")
				badKey := len(vectorFloats) != len(parts)
				if badKey {
					log.Errorf("Len doesn't match %v vs %v", len(vectorFloats), len(parts))
				} else {
					for i, floatVal := range vectorFloats {
						goValue, err := strconv.ParseFloat(parts[i], 32)
						if err != nil {
							badKey = true
							log.Errorf("cannot convert %v to double, %s", parts[i], err)
							break
						}
						if float32(goValue) != floatVal {
							badKey = true
							log.Errorf("key mismatch: %s\nGo Value: %s\nAxonCache Value %v", string(k), parts[i], floatVal)
							break
						}
					}
				}

				if badKey {
					mismatchingKeys += 1
				}
			}
		default:
			{
				mismatchingKeys += 1
				log.Debugf("Skip key %s of type '%s'", key, keyType)
				return nil
			}
		}

		return nil
	})

	if mismatchingKeys > 0 {
		log.Fatal(fmt.Errorf("%d mismatching keys", mismatchingKeys))
	}

	if err != nil {
		log.Fatal(err)
	}

	log.Printf("keys processed: %d", scanner.KeysCount)
}

func WaitForLocalFileUpdate(cmd *cobra.Command, args []string) {
	cachePath := args[0]

	directory, cacheName, timestamp, err := parsePath(cachePath)
	if err != nil {
		log.Fatal(err)
	}

	cache, err := axoncache.NewCacheReader(
		&axoncache.CacheReaderOptions{
			TaskName:          cacheName,
			DestinationFolder: directory,
			UpdatePeriod:      time.Second})
	defer cache.Delete()

	if err != nil {
		log.Fatal(err)
	}

	err = cache.Update(timestamp)
	if err != nil {
		log.Fatal(err)
	}

	// Capture OS signals for graceful shutdown
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)

	for {
		select {
		case <-sigChan:
			log.Info("Shuting down")
			return
		default:
			time.Sleep(1 * time.Second)
		}
	}
}

func WaitForRemoteFileUpdate(cmd *cobra.Command, args []string) {
	cacheName := args[0]
	directory := args[1]
	allUrls := args[2]

	cache, err := axoncache.NewCacheReader(
		&axoncache.CacheReaderOptions{
			TaskName:          cacheName,
			DestinationFolder: directory,
			UpdatePeriod:      3 * time.Second,
			BaseUrls:          allUrls,
			DownloadAtInit:    true})
	defer cache.Delete()

	if err != nil {
		log.Fatal(err)
	}

	// Capture OS signals for graceful shutdown
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)

	for {
		select {
		case <-sigChan:
			log.Info("Shuting down")
			return
		default:
			time.Sleep(1 * time.Second)
		}
	}
}

func GetKey(cmd *cobra.Command, args []string) {
	cachePath := args[0]
	key := args[1]

	directory, cacheName, timestamp, err := parsePath(cachePath)
	if err != nil {
		log.Fatal(err)
	}

	cache, _ := axoncache.NewCacheReader(
		&axoncache.CacheReaderOptions{
			TaskName:          cacheName,
			DestinationFolder: directory,
			UpdatePeriod:      time.Second})
	defer cache.Delete()

	err = cache.Update(timestamp)
	if err != nil {
		log.Fatal(err)
	}

	keyType, err := cache.GetKeyType(key)
	if err == axoncache.ErrNotFound {
		log.Printf("Key %s not found", key)
		return
	}

	log.Printf("Key type for %s -> %s", key, keyType)

	switch keyType {
	case "String":
		{
			cacheValue, err := cache.GetKey(key)
			log.Printf("Found: %t", err != axoncache.ErrNotFound)
			log.Printf("Value: %s", cacheValue)
		}
	case "Double":
		{
			cacheValue, err := cache.GetDouble(key)
			log.Printf("Found: %t", err != axoncache.ErrNotFound)
			log.Printf("Value: %f", cacheValue)
		}
	case "Int":
		{
			cacheValue, err := cache.GetInt(key)
			log.Printf("Found: %t", err != axoncache.ErrNotFound)
			log.Printf("Value: %d", cacheValue)
		}
	case "Int64":
		{
			cacheValue, err := cache.GetLong(key)
			log.Printf("Found: %t", err != axoncache.ErrNotFound)
			log.Printf("Value: %d", cacheValue)
		}
	case "StringList":
		{
			vectorStrings, err := cache.GetVector(key)
			log.Printf("Found: %t", err != axoncache.ErrNotFound)

			for i, str := range vectorStrings {
				log.Printf("[%d] = %s", i, str)
			}
		}
	case "FloatList":
		{
			vectorFloats, err := cache.GetVectorFloat(key)
			log.Printf("Found: %t", err != axoncache.ErrNotFound)

			for i, floatVal := range vectorFloats {
				log.Printf("[%d] = %f", i, floatVal)
			}
		}
	default:
		{
			log.Errorf("Invalid key type '%s'", keyType)
		}
	}
}

func Download(cmd *cobra.Command, args []string) {
	downloaderOptions := &core.DownloaderOptions{
		Basename:          "test_cache.cache",
		DestinationFolder: "/tmp",
		AllUrls:           "example.com", // FIXME - make this configurable
		Timeout:           time.Second,
	}

	downloader := core.NewDownloader(downloaderOptions)

	downloadStats, err := downloader.Run()
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("%v", downloadStats)
}

func Benchmark(cmd *cobra.Command, args []string) {
	RunBenchmark()
}

func main() {
	log.SetLevel(log.DebugLevel)

	var rootCmd = &cobra.Command{
		Use:   "axoncache_cli",
		Short: "synchronize caches across machines",
		Run:   RootCmd,
	}

	var kvScan = &cobra.Command{
		Use:   "kv_scan file",
		Run:   KvScan,
		Short: "scan a key value file",
		Args:  cobra.MinimumNArgs(1),
	}

	var kvSplit = &cobra.Command{
		Use:   "kv_split file",
		Run:   KvSplit,
		Short: "split a key value file into N files, one per query ID",
		Args:  cobra.MinimumNArgs(1),
	}

	var comparo = &cobra.Command{
		Use:   "comparo kv_file cache_file",
		Run:   Comparo,
		Short: "Compare a key value file, ",
		Args:  cobra.MinimumNArgs(2),
	}

	var waitForLocalFileUpdates = &cobra.Command{
		Use:   "wait_local file",
		Run:   WaitForLocalFileUpdate,
		Short: "wait for a local file update",
		Args:  cobra.MinimumNArgs(1),
	}

	var waitForRemoteFileUpdates = &cobra.Command{
		Use:   "wait_remote file",
		Run:   WaitForRemoteFileUpdate,
		Short: "wait for a remote file update",
		Args:  cobra.MinimumNArgs(3),
	}

	var getKey = &cobra.Command{
		Use:   "get file key",
		Run:   GetKey,
		Short: "wait for a file update",
		Args:  cobra.MinimumNArgs(1),
	}

	var download = &cobra.Command{
		Use:   "download axoncache",
		Run:   Download,
		Short: "wait for a file update",
		Args:  cobra.MinimumNArgs(0),
	}

	var benchmark = &cobra.Command{
		Use:   "benchmark",
		Run:   Benchmark,
		Short: "wait for a file update",
		Args:  cobra.MinimumNArgs(0),
	}

	rootCmd.AddCommand(kvScan)
	rootCmd.AddCommand(kvSplit)
	rootCmd.AddCommand(comparo)
	rootCmd.AddCommand(waitForLocalFileUpdates)
	rootCmd.AddCommand(waitForRemoteFileUpdates)
	rootCmd.AddCommand(getKey)
	rootCmd.AddCommand(download)
	rootCmd.AddCommand(benchmark)

	if err := rootCmd.Execute(); err != nil {
		log.Fatal("failed when executing root axoncache_cli command: " + err.Error())
	}
}
