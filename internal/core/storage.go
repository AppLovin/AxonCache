// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package core

import (
	"net/url"
	"os"
	"path/filepath"
)

// Those are the basic operations that are needed to manage files pushed to GCS (or a mirror).
type StorageHandler interface {
	// Need to copy files over
	CopyFile(src, dst string, perm os.FileMode) error

	// Need to do directory listing remotely
	Glob(pattern string) ([]string, error)

	// So that we can remove some files (typically the old ones)
	Remove(file string) error
}

// Local filesystem
type LocalFStorageHandler struct {
}

func (l *LocalFStorageHandler) CopyFile(src, dst string, perm os.FileMode) error {
	return CopyFile(src, dst, perm)
}

func (l *LocalFStorageHandler) Glob(pattern string) ([]string, error) {
	return filepath.Glob(pattern)
}

func (l *LocalFStorageHandler) Remove(file string) error {
	return os.Remove(file)
}

// GCS
type GCSStorageHandler struct {
	BucketName string
	Client     *GCSClient
}

func (g *GCSStorageHandler) CopyFile(src, dst string, perm os.FileMode) error {
	return g.Client.CopyFile(src, dst, perm)
}

func (g *GCSStorageHandler) Glob(pattern string) ([]string, error) {
	return g.Client.Glob(pattern)
}

func (g *GCSStorageHandler) Remove(file string) error {
	return g.Client.Remove(file)
}

// Utility code to make a storage handler
func MakeStorageHandler(path string) StorageHandler {
	u, err := url.Parse(path)
	if err != nil {
		handler := &LocalFStorageHandler{}
		return handler
	}

	// A GCS path/url is formed like this:
	//
	// gs://datamover-bucket/test/ccache_seed/fast_cache.mmap.timestamp
	// the host is the bucket name, in our case the current one being "datamover-bucket"
	//
	if u.Scheme == "gs" {
		bucketName := u.Host

		gcsClient, _ := NewGCSClient(bucketName)
		handler := &GCSStorageHandler{BucketName: bucketName, Client: gcsClient}
		return handler
	}

	handler := &LocalFStorageHandler{}
	return handler
}
