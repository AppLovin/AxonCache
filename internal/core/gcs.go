// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package core

import (
	"context"
	"fmt"
	"io"
	"net/http"
	"os"
	"path"
	"strings"
	"time"

	"cloud.google.com/go/storage"
	"google.golang.org/api/iterator"

	"github.com/tzvetkoff-go/fnmatch"
)

type GCSClient struct {
	Client     *storage.Client
	BucketName string
}

func NewGCSClient(bucketName string) (*GCSClient, error) {
	c := &GCSClient{}

	c.BucketName = bucketName

	ctx := context.Background()
	// Use Google Application Default Credentials to authorize and authenticate the client.
	// More information about Application Default Credentials and how to enable is at
	// https://developers.google.com/identity/protocols/application-default-credentials.
	client, err := storage.NewClient(ctx)
	if err != nil {
		return c, err
	}

	c.Client = client
	return c, nil
}

// We implement the HTTP GET interface to retrieve an object using the GCS APIs
func (c *GCSClient) Get(url string) (*http.Response, error) {
	response := &http.Response{}

	bkt := c.Client.Bucket(c.BucketName)

	//
	// Extract file path from url
	// If url is gs://my-bucket/test/ccache_seed/test_cache.mmap.timestamp
	// then path is /test/ccache_seed/test_cache.mmap.timestamp
	//
	path := strings.TrimPrefix(url, fmt.Sprintf("gs://%s/", c.BucketName))

	// Get a handle to the object
	obj := bkt.Object(path)

	ctx := context.Background()

	// Fetch object attributes, which is not always necessary (for small downloaded files)
	objAttrs, err := obj.Attrs(ctx)
	if err != nil {
		return response, err
	}
	response.Header = make(http.Header)
	response.Header.Set("Content-Length", fmt.Sprintf("%d", objAttrs.Size))

	r, err := obj.NewReader(ctx)
	if err != nil {
		return response, err
	}

	response.Body = r

	// A fake successful error code, as we were able to locate and open the bucket
	response.StatusCode = 200

	return response, nil
}

func (c *GCSClient) CopyFile(src, dst string, perm os.FileMode) error {
	bkt := c.Client.Bucket(c.BucketName)

	//
	// Extract file path from url
	// If url is gs://my-bucket/test/ccache_seed/test_cache.mmap.timestamp
	// then path is /test/ccache_seed/test_cache.mmap.timestamp
	//
	objectPath := strings.TrimPrefix(dst, fmt.Sprintf("gs://%s/", c.BucketName))

	// Get a handle to the object
	obj := bkt.Object(objectPath)

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	counter := &WriteCounter{Name: path.Base(src), Start: time.Now(), Label: fmt.Sprintf("mirror file to %s, ", path.Dir(dst))}
	handleWriterProgress(ctx, counter)

	in, err := os.Open(src)
	if err != nil {
		return err
	}
	defer in.Close()

	destination := obj.NewWriter(ctx)
	defer destination.Close()

	_, err = io.Copy(destination, io.TeeReader(in, counter))
	return err
}

func (c *GCSClient) Glob(pattern string) ([]string, error) {
	bkt := c.Client.Bucket(c.BucketName)

	prefixWithGlob := strings.TrimPrefix(pattern, fmt.Sprintf("gs://%s/", c.BucketName))

	// Try to extract a prefix without globs, to speed up the query.
	var sb strings.Builder
	for _, c := range prefixWithGlob {
		if c == rune('*') || c == rune('?') || c == rune('[') {
			break
		}
		sb.WriteRune(c)
	}
	query := &storage.Query{Prefix: sb.String()}

	query.SetAttrSelection([]string{"Name"})

	ctx := context.Background()

	var names []string
	it := bkt.Objects(ctx, query)
	for {
		attrs, err := it.Next()
		if err == iterator.Done {
			break
		}
		if err != nil {
			return []string{}, err
		}

		if fnmatch.Match(prefixWithGlob, attrs.Name, 0) {
			names = append(names, fmt.Sprintf("gs://%s/%s", c.BucketName, attrs.Name))
		}
	}

	return names, nil
}

func (c *GCSClient) Remove(file string) error {
	bkt := c.Client.Bucket(c.BucketName)

	//
	// Extract file path from url
	// If url is gs://my-bucket/test/ccache_seed/test_cache.mmap.timestamp
	// then path is /test/ccache_seed/test_cache.mmap.timestamp
	//
	objectPath := strings.TrimPrefix(file, fmt.Sprintf("gs://%s/", c.BucketName))

	// Get a handle to the object
	obj := bkt.Object(objectPath)

	ctx := context.Background()

	return obj.Delete(ctx)
}
