// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package core

import (
	"bufio"
	"context"
	"fmt"
	"io"
	"math/rand"
	"os"
	"path"
	"path/filepath"
	"strings"
	"time"

	"github.com/cespare/xxhash"
	"github.com/klauspost/compress/zstd"
	"github.com/zeebo/xxh3"

	log "github.com/sirupsen/logrus"
)

func WriteDecompressedFileToDiskFromReader(f io.Reader, compressedPath string, compressionMethod string, checksumExtension string, remoteCksum string, uncompressedSize int64, extraWriter ExtraWriter, progressBarLabel string) (string, int64, error) {
	log.Printf("💾 Decompressing from memory or %s and writing to disk...", compressedPath)

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Decompress to a temp file which we will move the real file in an atomic fashion with
	// os.rename, so that we don't leave broken file around in case of crashes
	decompressedPath := strings.TrimSuffix(compressedPath, filepath.Ext(compressedPath))
	basename := path.Base(decompressedPath)
	tokens := strings.Split(basename, ".")
	if len(tokens) > 2 {
		extension := tokens[1]
		if extension == "cache" {
			dirName := path.Dir(decompressedPath)
			name := tokens[0]
			timestamp := tokens[2]
			decompressedPath = fmt.Sprintf("%s/%s.%s.%s", dirName, name, timestamp, extension)
		}
	}

	tempDecompressedPath := fmt.Sprintf("%s.tmp.%d", decompressedPath, rand.Uint32())

	of, err := os.Create(tempDecompressedPath)
	if err != nil {
		return "", -1, err
	}
	defer of.Close()

	// fallocate the output size, if possible
	if uncompressedSize > 0 {
		err = Fallocate(int(of.Fd()), uncompressedSize)
		if err != nil {
			return "", -1, err
		}
	}

	counter := &WriteCounter{Name: path.Base(decompressedPath), Start: time.Now(), Label: progressBarLabel}
	handleWriterProgress(ctx, counter)

	writer := bufio.NewWriter(of)

	checksum := "invalid-checksum"

	if compressionMethod == "zst" && checksumExtension == "xxh" {
		h := xxhash.New()
		w := io.MultiWriter(writer, h, extraWriter)

		zr, err := zstd.NewReader(f)
		if err != nil {
			return "", -1, err
		}

		if _, err = io.Copy(w, io.TeeReader(zr, counter)); err != nil {
			return "", -1, err
		}
		checksum = fmt.Sprintf("%x", h.Sum64()) // Notice we use hex for xxh64, we used to use decimal
	} else if compressionMethod == "zst" && checksumExtension == "xxh3" {
		h := xxh3.New()
		w := io.MultiWriter(writer, h, extraWriter)

		zr, err := zstd.NewReader(f)
		if err != nil {
			return "", -1, err
		}

		if _, err = io.Copy(w, io.TeeReader(zr, counter)); err != nil {
			return "", -1, err
		}
		checksum = fmt.Sprintf("%x", h.Sum64()) // Notice we use hex for xxh3
	} else {
		return "", -1, fmt.Errorf("Unsupported compressionMethod %s and checksumExtension %s", compressionMethod, checksumExtension)
	}

	// Explicitely flush the buffer to OS disk cache, and close the file descriptor,
	// so that the subsequent 'stat' call (to get the decompressed size),
	// and the file rename that follow are safe
	writer.Flush()

	// it is safe to call close twice (the second close will be from the defer), see go docs
	of.Close()

	if checksum != remoteCksum {
		return "", -1, fmt.Errorf("invalid computed cksum '%s' != pre-computed remote cksum '%s'", checksum, remoteCksum)
	}

	var st os.FileInfo
	if st, err = os.Stat(tempDecompressedPath); os.IsNotExist(err) {
		return "", -1, err
	}
	decompressedSize := st.Size()

	return decompressedPath, decompressedSize, os.Rename(tempDecompressedPath, decompressedPath)
}
