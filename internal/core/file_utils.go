// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package core

import (
	"context"
	"fmt"
	"io"
	"math/rand"
	"os"
	"path"
	"syscall"
	"time"

	log "github.com/sirupsen/logrus"
)

func WriteStringToFile(path string, text string) error {
	tempFile := fmt.Sprintf("%s.tmp.%d", path, rand.Uint32())

	err := os.WriteFile(tempFile, []byte(text), 0644)
	if err != nil {
		return err
	}

	return os.Rename(tempFile, path)
}

// CopyFile copies the contents from src to dst atomically.
// If dst does not exist, CopyFile creates it with permissions perm.
// If the copy fails, CopyFile aborts and dst is preserved.
func CopyFile(src, dst string, perm os.FileMode) error {
	// Create the backing folder for the target file
	dstFolder := path.Dir(dst)
	err := os.MkdirAll(dstFolder, 0777)
	if err != nil {
		return fmt.Errorf("Cannot create destination dir %s for file %s : %s", dstFolder, dst, err)
	}

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	counter := &WriteCounter{Name: path.Base(src), Start: time.Now(), Label: fmt.Sprintf("mirror file to %s, ", path.Dir(dst))}
	handleWriterProgress(ctx, counter)

	in, err := os.Open(src)
	if err != nil {
		return err
	}
	defer in.Close()

	destination, err := os.Create(dst)
	if err != nil {
		return err
	}
	defer destination.Close()

	_, err = io.Copy(destination, io.TeeReader(in, counter))
	return err
}

type MMapInfo struct {
	ByteRange []byte
	Path      string
}

func MemoryMapFile(path string) (*MMapInfo, error) {
	f, err := os.Open(path)
	if err != nil {
		return &MMapInfo{}, err
	}
	defer f.Close()

	fd := int(f.Fd())

	stat, err := f.Stat()
	if err != nil {
		return &MMapInfo{}, err
	}
	size := int(stat.Size())

	b, err := syscall.Mmap(fd, 0, size, syscall.PROT_READ, MMapOptions())
	if err != nil {
		f.Close()
		return &MMapInfo{}, err
	}

	mmapInfo := &MMapInfo{ByteRange: b, Path: path}
	mmapInfo.Prefetch(fd)

	return mmapInfo, nil
}

func MemoryUnMapAndEvictFile(mmapInfo *MMapInfo) {
	log.Printf("🧹 🗺️  Memory unmap %s", mmapInfo.Path)
	err := syscall.Munmap(mmapInfo.ByteRange)
	if err != nil {
		log.WithError(err).Warnf("Cannot memory unmap file %s", mmapInfo.Path)
	}

	// Only try to evict if the file is still around. It could have been deleted already
	if _, err := os.Stat(mmapInfo.Path); !os.IsNotExist(err) {
		log.Printf("🚚 Evict file from OS cache %s", mmapInfo.Path)
		err = Evict(mmapInfo.Path)
		if err != nil {
			// This isn't a fatal error
			log.WithError(err).Warnf("😥 Cannot evict file")
		}
	}
}
