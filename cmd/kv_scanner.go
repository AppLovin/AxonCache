// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package main

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"math"
	"os"
	"strconv"
	"time"

	"github.com/edsrzf/mmap-go"
)

var KvSeparator string = string(rune(30))

// Key Value file scanner
type KeyValueFileScanner struct {
	Path        string
	File        *os.File
	MappedBytes mmap.MMap
	Counter     *InsertCounter
	Cancel      context.CancelFunc

	// Dedup
	DeduplicateKeys          bool
	UniqueKeys               map[string]bool
	UniqueKeysWithoutQueryId map[string]bool
	CurrentPopulateQueryId   string

	// Stats
	// At minima we should implement:
	//
	// "min_key_length": 3,
	// "max_key_length": 263,
	// "average_key_length": 34.80221222383713,
	// "min_value_length": 0,
	// "max_value_length": 3247099,
	// "average_value_length": 28.421983276513867,
	//
	// FIXME: We should also implement stats for vector keys
	//        and report the queryId average size
	//
	KeysCount      int
	MinKeyLength   int
	MaxKeyLength   int
	MinValueLength int
	MaxValueLength int

	KeySpaceSize   int
	ValueSpaceSize int

	QueryIds map[int]bool
}

func NewKeyValueFileScanner() *KeyValueFileScanner {
	kvScanner := &KeyValueFileScanner{
		MinKeyLength:             math.MaxInt,
		MaxKeyLength:             0,
		MinValueLength:           math.MaxInt,
		MaxValueLength:           0,
		UniqueKeysWithoutQueryId: make(map[string]bool),
		QueryIds:                 make(map[int]bool),
	}

	return kvScanner
}

func (k *KeyValueFileScanner) EnableKeysDeduplication() {
	k.DeduplicateKeys = true
}

func (k *KeyValueFileScanner) SetPath(path string) error {
	k.Path = path

	k.File = nil
	k.MappedBytes = nil

	// Small progress bar to display how many entries we have processed
	ctx, cancel := context.WithCancel(context.Background())
	counter := &InsertCounter{Start: time.Now(), Label: fmt.Sprintf("processing [%s]", path)}
	handleInsertProgress(ctx, counter)

	k.Counter = counter
	k.Cancel = cancel

	// Open file so that we can mmap it
	f, err := os.OpenFile(path, os.O_RDONLY, 0644)
	if err != nil {
		return err
	}
	k.File = f

	// Memory map it
	b, err := mmap.Map(f, mmap.RDONLY, 0)
	if err != nil {
		return err
	}
	k.MappedBytes = b

	return nil
}

func (k *KeyValueFileScanner) Close() error {
	if len(k.Path) == 0 {
		return nil
	}

	if k.MappedBytes != nil {
		k.MappedBytes.Unmap()
	}

	if k.File != nil {
		k.File.Close()
	}

	k.Cancel()

	return nil
}

func (k *KeyValueFileScanner) UpdateStats(key, value []byte) error {
	k.KeysCount++

	k.KeySpaceSize += len(key)
	k.ValueSpaceSize += len(value)

	if len(key) < k.MinKeyLength {
		k.MinKeyLength = len(key)
	}
	if len(value) < k.MinValueLength {
		k.MinValueLength = len(value)
	}

	if len(key) > k.MaxKeyLength {
		k.MaxKeyLength = len(key)
	}
	if len(value) > k.MaxValueLength {
		k.MaxValueLength = len(value)
	}

	return nil
}

func (k *KeyValueFileScanner) isKeyDuplicated(key []byte) bool {
	keyStr := string(key)

	//
	// Extract queryId (123) from key
	// 123.foo=bar
	// queryId = 123
	// key = 123.foo
	// value = bar
	//
	var queryId []byte
	queryTokens := bytes.SplitN(key, []byte("."), 2)
	if len(queryTokens) == 1 {
		// This is rare, but happens for LAST_POPULATE_xxx=123 keys
		// Treat it specially, there won't be many of those
		if _, found := k.UniqueKeysWithoutQueryId[keyStr]; found {
			return true
		} else {
			k.UniqueKeysWithoutQueryId[keyStr] = true
			return false
		}
	}

	queryId = queryTokens[0]

	// Ignore empty queryId (most likely from LAST_POPULATE_xxx timestamps)
	if len(queryId) == 0 {
		return false
	}
	queryIdStr := string(queryId)

	if k.CurrentPopulateQueryId == "" {
		k.CurrentPopulateQueryId = queryIdStr
	}

	if queryIdStr != k.CurrentPopulateQueryId {
		k.CurrentPopulateQueryId = queryIdStr
		k.UniqueKeys = make(map[string]bool)
	}

	if _, found := k.UniqueKeys[keyStr]; found {
		return true
	} else {
		k.UniqueKeys[keyStr] = true
		return false
	}
}

func (k *KeyValueFileScanner) ExtractQueryId(key []byte) int {
	// Extract queryId
	offset := 0
	if key[0] == 1 {
		offset = 1
	}

	key2 := key[offset:]

	idx := bytes.IndexByte(key2, '.')
	if idx == -1 {
		return 0
	}

	val, err := strconv.Atoi(string(key2[:idx]))
	if err != nil {
		return 0
	} else {
		return val
	}
}

func (k *KeyValueFileScanner) ForEach(fn func(k, v []byte, queryId int) error) error {
	var key, value []byte

	start := 0
	last := len(k.MappedBytes) - 1

	if k.DeduplicateKeys {
		k.UniqueKeys = make(map[string]bool)
	}

	for i, char := range k.MappedBytes {
		if char == 30 || i == last {
			end := i
			if char != 30 {
				end++
			}

			line := k.MappedBytes[start:end]

			// Find key=value separator
			idx := bytes.IndexByte(line, '=')
			if idx == -1 {
				return errors.New(fmt.Sprintf("Invalid line '%s' missing = separator, format should be key=value", line))
			}

			key = line[:idx]
			value = line[idx+1:]

			queryId := k.ExtractQueryId(key)
			k.QueryIds[queryId] = true

			skipKey := false
			if k.DeduplicateKeys {
				skipKey = k.isKeyDuplicated(key)
			}

			if !skipKey {
				k.UpdateStats(key, value)

				if err := fn(key, value, queryId); err != nil {
					return err
				}
			}

			k.Counter.Incr()
			start = i + 1
		}
	}

	return nil
}

type ExtraWriter interface {
	Write(p []byte) (int, error)
	GetTotalRows() int
}

type DevNullWriter struct {
}

func (d *DevNullWriter) Write(p []byte) (int, error) {
	n := len(p)
	return n, nil
}

func (d *DevNullWriter) GetTotalRows() int {
	return 0
}

type RowsCounterWriter struct {
	TotalRows int
}

func (r *RowsCounterWriter) Write(p []byte) (int, error) {
	n := len(p)
	if n > 0 {
		r.TotalRows += bytes.Count(p, []byte(KvSeparator))
	}
	return n, nil
}

func (r *RowsCounterWriter) GetTotalRows() int {
	return r.TotalRows
}
