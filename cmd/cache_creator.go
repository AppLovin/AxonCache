package main

import (
	"bytes"
	"errors"
	"fmt"

	axoncache "github.com/AppLovin/AxonCache"
	"github.com/AppLovin/AxonCache/internal/core"
)

type OffSetBitsError struct {
	Err        error
	OffsetBits int
}

func (e *OffSetBitsError) Error() string {
	return fmt.Sprintf("✋ offset bits error: %s", e.Err)
}

type OutOfDataSpaceError struct {
	Err error
}

func (e *OutOfDataSpaceError) Error() string {
	return fmt.Sprintf("✋ out of dataspace error: %s", e.Err)
}

const (
	// enum from "alcache/domain/CacheValue.h"
	StringValueType     int8 = 0
	StringListValueType      = 1
	BoolValueType            = 2
	Int64ValueType           = 3
	DoubleValueType          = 4
	FloatListValueType       = 7

	// Special type to bring old C-Cache behavior
	StringNoNullType = 127
)

type CacheCreator struct {
	Properties              core.Properties
	DestinationFolder       string
	CCacheDestinationFolder string

	GotOffsetBitsError     bool
	GotOutOfDataSpaceError bool

	HasLastPopulateKey bool

	KeyTypes map[int]int8
}

func NewCacheCreator() *CacheCreator {
	cacheCreator := &CacheCreator{}

	return cacheCreator
}

func (c *CacheCreator) InsertAllKeysAndValues(cacheWriter axoncache.CacheWriter, scanner *KeyValueFileScanner, path string, dataFile string) error {
	var previousKey []byte

	err := scanner.SetPath(path)
	defer scanner.Close()
	if err != nil {
		return err
	}

	err = scanner.ForEach(func(key, value []byte, queryId int) error {
		if bytes.HasPrefix(key, []byte("LAST_POPULATE")) {
			c.HasLastPopulateKey = true
		}

		fmt.Printf("Inserting %s\n", string(key))

		// Skip consecutive duplicate keys, by comparing the previous key to the current one
		// This simple key removal behavior is backward compatible with the
		// previous "insert all given keys" one, as the first key inserted is the one being looked up
		if !bytes.Equal(key, previousKey) {
			previousKey = key

			keyType := c.KeyTypes[queryId]
			if keyType != StringValueType && len(value) == 0 {
				// skip empty value for bool/double/int64
				return nil
			}

			err := cacheWriter.InsertKey(key, value, keyType)
			if err != nil {
				return err
			}
		}

		return nil
	})

	if err != nil {
		return err
	}

	if !c.HasLastPopulateKey {
		return errors.New(fmt.Sprintf("Invalid key value file '%s' : missing LAST_POPULATE key", dataFile))
	}

	return nil
}
