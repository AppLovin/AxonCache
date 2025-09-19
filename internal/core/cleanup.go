// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package core

import (
	"fmt"
	"path"
	"sort"
	"strconv"
	"strings"

	log "github.com/sirupsen/logrus"
)

func GetSortedTimeStamps(destinationFolder string, prefix string, storageHandler StorageHandler) ([]int64, error) {
	globPattern := fmt.Sprintf("%s/%s.*", destinationFolder, prefix)
	paths, err := storageHandler.Glob(globPattern)
	if err != nil {
		return []int64{}, err
	}

	// Manually insert mmap file and friend in the list. They use a different naming scheme
	tokens := strings.Split(prefix, ".")
	if len(tokens) >= 2 {
		if tokens[1] == "mmap" || tokens[1] == "cache" {
			name := tokens[0]
			globPattern = fmt.Sprintf("%s/%s.*", destinationFolder, name)
			extraFiles, err := storageHandler.Glob(globPattern)

			if err != nil {
				log.WithError(err).Error("Globbing error")
			}

			for _, file := range extraFiles {
				paths = append(paths, file)
			}
		}
	}

	timestamps := make(map[int64]bool)
	for _, p := range paths {
		basename := path.Base(p)
		tokens := strings.Split(basename, ".")

		if len(tokens) > 1 {
			if timestamp, err := strconv.ParseInt(tokens[1], 10, 64); err == nil {
				timestamps[timestamp] = true
			} else {
				if len(tokens) > 2 {
					if timestamp, err := strconv.ParseInt(tokens[2], 10, 64); err == nil {
						timestamps[timestamp] = true
					}
				}
			}
		}
	}

	var sortedTimestamps []int64
	for k := range timestamps {
		sortedTimestamps = append(sortedTimestamps, k)
	}
	sort.Slice(sortedTimestamps, func(i, j int) bool { return sortedTimestamps[i] < sortedTimestamps[j] })

	return sortedTimestamps, nil
}

func ListFetchedFiles(destinationFolder string, basename string, timestamp string, storageHandler StorageHandler) []string {
	globPattern := fmt.Sprintf("%s/%s.%s*", destinationFolder, basename, timestamp)
	paths, err := storageHandler.Glob(globPattern)

	if err != nil {
		log.WithError(err).Error("Globbing error")
	}

	// Manually insert mmap file and friend in the list. They use a different naming scheme
	tokens := strings.Split(basename, ".")
	if len(tokens) >= 2 {
		extension := tokens[1]
		if extension == "mmap" || extension == "cache" {
			name := tokens[0]
			globPattern = fmt.Sprintf("%s/%s.%s.%s*", destinationFolder, name, timestamp, extension)
			extraFiles, err := storageHandler.Glob(globPattern)

			if err != nil {
				log.WithError(err).Error("Globbing error")
			}

			for _, file := range extraFiles {
				paths = append(paths, file)
			}
		}
	}

	return paths
}

func ListTempFetchedFiles(destinationFolder string, basename string, storageHandler StorageHandler) []string {
	// Some tmp files can be left around in some situation (crash, restarts), look for them as well.
	tmpPatterns := []string{
		"*.latest.tmp*",
		"*.http_download_tmp*",
	}

	paths := make([]string, 0)

	for _, tmpPattern := range tmpPatterns {
		globPattern := fmt.Sprintf("%s/%s.%s", destinationFolder, basename, tmpPattern)
		tmpFiles, err := storageHandler.Glob(globPattern)

		log.Printf("Search %s pattern, found %d hits", globPattern, len(tmpFiles))

		if err != nil {
			log.WithError(err).Error("Globbing error")
			continue
		}

		for _, file := range tmpFiles {
			paths = append(paths, file)
		}
	}

	return paths
}

func RemoveOldFiles(destinationFolder string, basename string, numRetainedDownloads int, callback func(string), storageHandler StorageHandler) error {
	paths := ListTempFetchedFiles(destinationFolder, basename, storageHandler)
	for _, path := range paths {
		callback(path)

		log.Printf("🧹 Removing %s", path)
		err := storageHandler.Remove(path)
		if err != nil {
			log.Errorf("Error removing %s : %s", path, err)
		}
	}

	timestamps, err := GetSortedTimeStamps(destinationFolder, basename, storageHandler)
	if err != nil {
		return err
	}

	// Display all the timestamps found
	var s strings.Builder
	for _, timestamp := range timestamps {
		s.WriteString(fmt.Sprintf("%d ", timestamp))
	}
	log.Printf("🧹 found %d timestamps: %s", len(timestamps), s.String())

	// Simple case, nothing to remove
	if len(timestamps) <= numRetainedDownloads {
		log.Printf("🧹 No old files to remove")
		return err
	}

	// Start from old timestamps, move towards more recent ones, and stop when we have
	// reached the ones we want to keep.
	for i := 0; i < (len(timestamps) - numRetainedDownloads); i++ {
		paths := ListFetchedFiles(destinationFolder, basename, fmt.Sprint(timestamps[i]), storageHandler)

		log.Printf("🧹 Found %d old files to remove", len(paths))
		if len(paths) == 0 {
			continue
		}

		for _, path := range paths {
			callback(path)

			log.Printf("🧹 Removing %s", path)
			err = storageHandler.Remove(path)
			if err != nil {
				log.Errorf("Error removing %s : %s", path, err)
			}
		}
	}

	return nil
}
func RemoveLastDownloadedFilesAfterError(destinationFolder string, basename string, timestamp string, storageHandler StorageHandler) error {
	paths := ListTempFetchedFiles(destinationFolder, basename, storageHandler)
	for _, path := range paths {
		log.Printf("🧹 Removing %s", path)
		err := storageHandler.Remove(path)
		if err != nil {
			log.Errorf("Error removing %s : %s", path, err)
		}
	}

	if timestamp == "" {
		log.Printf("🧹 Nothing to clean since no valid timestamp was provided, the middle mode servers are probably down")
		return nil
	}

	paths = ListFetchedFiles(destinationFolder, basename, timestamp, storageHandler)

	for _, path := range paths {
		log.Printf("🧹 Removing %s", path)
		err := storageHandler.Remove(path)
		if err != nil {
			log.Errorf("Error removing %s : %s", path, err)
		}
	}
	return nil
}
