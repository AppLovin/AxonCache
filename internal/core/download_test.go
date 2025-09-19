// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package core

import (
	"os"
	"testing"
	"time"
)

func TestDownloadFromLocalDisk(t *testing.T) {

	rootDir := "test_data/TestValidDownloadFromLocalDisk"

	pwd, err := os.Getwd()
	if err != nil {
		t.Fatal(err)
	}

	downloaderOptions := &DownloaderOptions{
		Basename:          "bench_cache.cache",
		DestinationFolder: t.TempDir(),
		AllUrls:           "file://" + pwd + "/" + rootDir,
		Timeout:           time.Second,
	}

	downloader := NewDownloader(downloaderOptions)
	downloadStats, err := downloader.Run()

	if err != nil {
		t.Fatal(err)
	}

	// Check that the downloaded file exists on disk
	path := downloadStats.DecompressedPath
	if _, err := os.Stat(path); os.IsNotExist(err) {
		t.Fatalf("missing file : %s", path)
	}
}
