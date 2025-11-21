// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package axoncache

import (
	"io"
	"log"
	"os"
	"path/filepath"
	"testing"

	"fmt"
	"time"

	"github.com/AppLovin/AxonCache/internal/core"
	"github.com/stretchr/testify/assert"
)

func TestCacheReader(t *testing.T) {
	var err error
	assert := assert.New(t)

	cache, err := NewCacheReader(
		&CacheReaderOptions{
			TaskName:          "fast_cache",
			DestinationFolder: "test_data",
			UpdatePeriod:      time.Second})
	defer cache.Delete()

	assert.Equal(nil, err)

	var keyType string
	var boolValue bool
	var stringValue string
	var doubleValue float64
	var intValue int
	var longValue int64
	var stringVector []string
	var floatVector []float32
	var contained bool

	// Key Existence
	contained, err = cache.ContainsKey("267.bar")
	assert.Equal(true, contained)
	contained, err = cache.ContainsKey("267.that_does_not_exists_for_sure")
	assert.Equal(false, contained)

	_, err = cache.GetKeyType("123456789.NOTFOUND")
	assert.Equal(ErrNotFound, err)

	// Strings
	keyType, _ = cache.GetKeyType("267.bar")
	assert.Equal("String", keyType)

	stringValue, _ = cache.GetKey("267.foo")
	assert.Equal("foo", stringValue)

	stringValue, _ = cache.GetKey("267.bar")
	assert.Equal("bar", stringValue)

	_, err = cache.GetKey("267.NOTFOUND")
	assert.Equal(ErrNotFound, err)

	// Booleans
	keyType, _ = cache.GetKeyType("992.yyy")
	assert.Equal("Bool", keyType)

	boolValue, _ = cache.GetBool("992.yyy")
	assert.Equal(true, boolValue)

	boolValue, _ = cache.GetBool("992.zzz")
	assert.Equal(false, boolValue)

	_, err = cache.GetLong("1690.NOTFOUND")
	assert.Equal(ErrNotFound, err)

	// Double
	keyType, _ = cache.GetKeyType("1401.xxx")
	assert.Equal("Double", keyType)

	doubleValue, _ = cache.GetDouble("1401.xxx")
	assert.Equal(123.456, doubleValue)

	_, err = cache.GetDouble("1401.NOTFOUND")
	assert.Equal(ErrNotFound, err)

	// Integer
	keyType, _ = cache.GetKeyType("1690.xxx")
	assert.Equal("Int64", keyType)

	intValue, _ = cache.GetInt("1690.xxx")
	assert.Equal(1234567890, intValue)

	_, err = cache.GetInt("1690.NOTFOUND")
	assert.Equal(ErrNotFound, err)

	// Long
	keyType, _ = cache.GetKeyType("1690.xxx")
	assert.Equal("Int64", keyType)

	longValue, _ = cache.GetLong("1690.xxx")
	assert.Equal(int64(1234567890), longValue)

	_, err = cache.GetLong("1690.NOTFOUND")
	assert.Equal(ErrNotFound, err)

	// String[]
	keyType, _ = cache.GetKeyType("999.vec1")
	assert.Equal("StringList", keyType)

	stringVector, _ = cache.GetVector("999.vec1")
	assert.Equal("slot0", stringVector[0])
	assert.Equal("slot1", stringVector[1])
	assert.Equal("slot2", stringVector[2])

	_, err = cache.GetVector("999.NOTFOUND")
	assert.Equal(ErrNotFound, err)

	// float32[]
	keyType, _ = cache.GetKeyType("1909.xxx")
	assert.Equal("FloatList", keyType)

	floatVector, _ = cache.GetVectorFloat("1909.xxx")
	expectedVector := []float32{3642, 1161, 371, 595, 183, 60, 287, 88, 29, 668, 221, 79, 9, 2, 1, 9, 2, 0, 0.078803, 0.075797}
	assert.Equal(len(floatVector), len(expectedVector))
	for i := 0; i < len(expectedVector); i++ {
		assert.Equal(expectedVector[i], floatVector[i])
	}

	_, err = cache.GetVectorFloat("1909.NOTFOUND")
	assert.Equal(ErrNotFound, err)
}

// Golang 1.23 has a copy folder routine but it does not work as expected, so bring in our own
func copyFolder(src, dest string) error {
	return filepath.Walk(src, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		// Construct the destination path
		relPath, err := filepath.Rel(src, path)
		if err != nil {
			return err
		}
		destPath := filepath.Join(dest, relPath)

		// Create directories
		if info.IsDir() {
			return os.MkdirAll(destPath, info.Mode())
		}

		// Copy files
		return copyFile(path, destPath)
	})
}

func copyFile(src, dest string) error {
	// Open the source file
	srcFile, err := os.Open(src)
	if err != nil {
		return err
	}
	defer srcFile.Close()

	// Create the destination file
	destFile, err := os.Create(dest)
	if err != nil {
		return err
	}
	defer destFile.Close()

	// Copy the file contents
	_, err = io.Copy(destFile, srcFile)
	if err != nil {
		return err
	}

	// Copy the file mode
	srcInfo, err := os.Stat(src)
	if err != nil {
		return err
	}
	return os.Chmod(dest, srcInfo.Mode())
}

func TestCacheReaderUpdates(t *testing.T) {
	assert := assert.New(t)

	dir := t.TempDir()
	// dir := "/tmp/my_new_folder"

	// Create the destination directory if it doesn't exist
	if err := os.MkdirAll(dir, os.ModePerm); err != nil {
		fmt.Println("Error creating destination directory:", err)
		return
	}

	// Copy input test dir to our temporary folder
	err := copyFolder("test_data/cache_updates", dir)
	assert.Equal(nil, err)

	// the foo variable is used inside the callback.
	capturedVariable := 123
	cb := func(cache CacheReaderInterface) error {
		log.Printf("Cache updated, capturedVariable = %d", capturedVariable)

		stringValue, _ := cache.GetKey("key_333")
		log.Printf("Cache value after update: %s", stringValue)
		return nil
	}

	cache, err := NewCacheReader(
		&CacheReaderOptions{
			TaskName:          "test_cache",
			DestinationFolder: dir,
			UpdatePeriod:      10 * time.Millisecond,
			UpdateCallback:    cb})
	assert.Equal(nil, err)
	defer cache.Delete()

	if err != nil {
		t.Fatal(err)
	}

	var stringValue string
	stringValue, _ = cache.GetKey("key_333")
	assert.Equal("val_333", stringValue)

	time.Sleep(100 * time.Millisecond)

	newPath := fmt.Sprintf("%s/test_cache.cache.timestamp.latest", dir)
	os.WriteFile(newPath, []byte("1700522842046"), 0644)

	time.Sleep(100 * time.Millisecond)

	stringValue, _ = cache.GetKey("key_333")
	assert.Equal("val_331", stringValue) // value changed

	assert.Equal(int64(1700522842046), cache.MostRecentFileTimestamp)
}

func TestCacheReaderUninitialized(t *testing.T) {
	var err error
	assert := assert.New(t)

	cache, err := NewCacheReader(
		&CacheReaderOptions{
			TaskName:          "fast_cache",
			DestinationFolder: "test_data_missing",
			UpdatePeriod:      time.Second})
	defer cache.Delete()

	_, err = cache.GetKeyType("267.bar")
	assert.Equal(ErrUnInitialized, err)

	_, err = cache.GetKey("267.foo")
	assert.Equal(ErrUnInitialized, err)

	_, err = cache.GetBool("992.yyy")
	assert.Equal(ErrUnInitialized, err)

	_, err = cache.GetLong("1690.NOTFOUND")
	assert.Equal(ErrUnInitialized, err)

	_, err = cache.GetDouble("1401.xxx")
	assert.Equal(ErrUnInitialized, err)

	_, err = cache.GetInt("1690.xxx")
	assert.Equal(ErrUnInitialized, err)

	_, err = cache.GetLong("1690.xxx")
	assert.Equal(ErrUnInitialized, err)

	_, err = cache.GetVector("999.vec1")
	assert.Equal(ErrUnInitialized, err)

	_, err = cache.GetVectorFloat("1909.xxx")
	assert.Equal(ErrUnInitialized, err)
}

func TestCacheReaderBadUpdate(t *testing.T) {
	var err error
	assert := assert.New(t)

	cache, err := NewCacheReader(
		&CacheReaderOptions{
			TaskName:          "test_cache",
			DestinationFolder: "test_data",
			UpdatePeriod:      time.Second})
	defer cache.Delete()

	assert.Equal(nil, err)

	var stringValue string

	stringValue, _ = cache.GetKey("key_123")
	assert.Equal("val_123", stringValue)

	// Make a bad update by trying to pick a file that does not exists
	err = cache.Update("0000000000000000000000")
	assert.NotEqual(nil, err) // should fail

	// But our lookup should still succeed. The previous good file should be retained
	stringValue, _ = cache.GetKey("key_123")
	assert.Equal("val_123", stringValue)
}

func TestCacheReaderUseAfterDelete(t *testing.T) {
	var err error
	assert := assert.New(t)

	cache, err := NewCacheReader(
		&CacheReaderOptions{
			TaskName:          "fast_cache",
			DestinationFolder: "test_data",
			UpdatePeriod:      time.Second})
	assert.Equal(nil, err)

	var stringValue string

	stringValue, _ = cache.GetKey("267.foo")
	assert.Equal("foo", stringValue)

	// Delete the cache file
	cache.Delete()

	// Our lookup should fail now
	stringValue, err = cache.GetKey("267.foo")
	assert.Equal(ErrUnInitialized, err)

	// Update should fail too, and not crash
	err = cache.Update("0000000000000000000000")
	assert.NotEqual(nil, err) // should fail
}

func TestCacheWriterGenerateTimestampFile(t *testing.T) {
	// path -> /tmp/ads_event_cache.1730393701512.cache
	// err := cache.Update("ads_event_cache", "/tmp", "1730393701512")
	var err error
	assert := assert.New(t)

	// Insert a few keys

	taskName := "super_cache"
	// extension := ".cache"

	destinationFolder := t.TempDir()

	cache, err := NewCacheWriter(
		&CacheWriterOptions{
			TaskName:              taskName,
			DestinationFolder:     destinationFolder,
			SettingsLocation:      "",
			NumberOfKeySlots:      10000,
			GenerateTimestampFile: true,
			RenameCacheFile:       true,
		})
	defer cache.Delete()

	assert.Equal(nil, err)

	const keys = 1000
	for i := 1; i < keys; i++ {
		key := fmt.Sprintf("key_%d", i)
		val := fmt.Sprintf("val_%d", i)
		if err = cache.InsertKey([]byte(key), []byte(val), 0); err != nil {
			t.Fatal(err)
		}
	}

	err, _ = cache.FinishCacheCreation()
	if err != nil {
		t.Fatal(err)
	}

	rcache, err := NewCacheReader(
		&CacheReaderOptions{
			TaskName:          taskName,
			DestinationFolder: destinationFolder,
			UpdatePeriod:      time.Second})
	defer rcache.Delete()

	assert.Equal(nil, err)

	// Now do one lookup
	var stringValue string
	stringValue, _ = rcache.GetKey("key_1")
	assert.Equal("val_1", stringValue)
}

func TestCacheWriterDoNotGenerateTimestampFile(t *testing.T) {
	// path -> /tmp/ads_event_cache.1730393701512.cache
	// err := cache.Update("ads_event_cache", "/tmp", "1730393701512")
	var err error
	assert := assert.New(t)

	// Insert a few keys

	taskName := "super_cache"
	// extension := ".cache"

	destinationFolder := t.TempDir()

	// Write a properties file
	settingsLocation := fmt.Sprintf("%s/%s.properties", destinationFolder, taskName)
	ccacheProperties := make(map[string]string)
	ccacheProperties["ccache.destination_folder"] = destinationFolder
	core.WritePropertiesFile(ccacheProperties, settingsLocation)

	cache, err := NewCacheWriter(
		&CacheWriterOptions{
			TaskName:              taskName,
			DestinationFolder:     destinationFolder,
			SettingsLocation:      settingsLocation,
			NumberOfKeySlots:      10000,
			GenerateTimestampFile: false,
			RenameCacheFile:       true,
		})
	defer cache.Delete()

	assert.Equal(nil, err)

	cache.AddDuplicateValue("val_1", 0)
	cache.FinishAddDuplicateValues()

	const keys = 1000
	for i := 1; i < keys; i++ {
		key := fmt.Sprintf("key_%d", i)
		val := fmt.Sprintf("val_%d", i)
		if err = cache.InsertKey([]byte(key), []byte(val), 0); err != nil {
			t.Fatal(err)
		}
	}

	err, timestamp := cache.FinishCacheCreation()
	if err != nil {
		t.Fatal(err)
	}

	rcache, err := NewCacheReader(
		&CacheReaderOptions{
			TaskName:          taskName,
			DestinationFolder: destinationFolder,
			UpdatePeriod:      time.Second,
			Timestamp:         timestamp})
	defer rcache.Delete()

	assert.Equal(nil, err)

	// Now do one lookup
	var stringValue string
	stringValue, _ = rcache.GetKey("key_1")
	assert.Equal("val_1", stringValue)

	stringValue, _ = rcache.GetKey("key_2")
	assert.Equal("val_2", stringValue)
}

func TestCacheWriterSetCacheTypeBucketChain(t *testing.T) {
	var err error
	assert := assert.New(t)

	// Insert a few keys

	taskName := "super_cache"
	// extension := ".cache"

	destinationFolder := t.TempDir()

	// Write a properties file
	settingsLocation := fmt.Sprintf("%s/%s.properties", destinationFolder, taskName)
	ccacheProperties := make(map[string]string)
	ccacheProperties["ccache.destination_folder"] = destinationFolder
	core.WritePropertiesFile(ccacheProperties, settingsLocation)

	cache, err := NewCacheWriter(
		&CacheWriterOptions{
			TaskName:              taskName,
			DestinationFolder:     destinationFolder,
			SettingsLocation:      "", // settingsLocation,
			NumberOfKeySlots:      10000,
			GenerateTimestampFile: false,
			RenameCacheFile:       true,
			CacheType:             2, // Bucket chain
		})
	defer cache.Delete()

	assert.Equal(nil, err)

	cache.AddDuplicateValue("val_1", 0)
	cache.FinishAddDuplicateValues()

	const keys = 1000
	for i := 1; i < keys; i++ {
		key := fmt.Sprintf("key_%d", i)
		val := fmt.Sprintf("val_%d", i)
		if err = cache.InsertKey([]byte(key), []byte(val), 0); err != nil {
			t.Fatal(err)
		}
	}

	err, timestamp := cache.FinishCacheCreation()
	if err != nil {
		t.Fatal(err)
	}

	rcache, err := NewCacheReader(
		&CacheReaderOptions{
			TaskName:          taskName,
			DestinationFolder: destinationFolder,
			UpdatePeriod:      time.Second,
			Timestamp:         timestamp})
	defer rcache.Delete()

	assert.Equal(nil, err)

	// Now do one lookup
	var stringValue string
	stringValue, _ = rcache.GetKey("key_1")
	assert.Equal("val_1", stringValue)

	stringValue, _ = rcache.GetKey("key_2")
	assert.Equal("val_2", stringValue)
}

func TestCacheWriterSetOffsetBitsTooSmallForDataSize(t *testing.T) {
	var err error
	assert := assert.New(t)

	taskName := "super_cache"
	destinationFolder := t.TempDir()

	cache, err := NewCacheWriter(
		&CacheWriterOptions{
			TaskName:              taskName,
			DestinationFolder:     destinationFolder,
			SettingsLocation:      "",
			NumberOfKeySlots:      10000,
			GenerateTimestampFile: false,
			RenameCacheFile:       true,
			OffsetBits:            16, // Too small for 5000 keys
			// Min value 16, max 38
		})
	defer cache.Delete()

	assert.Equal(nil, err)

	cache.AddDuplicateValue("val_1", 0)
	cache.FinishAddDuplicateValues()

	expectedError := false
	const keys = 5000 // with offsetbits = 16, not enough space

	for i := 1; i < keys; i++ {
		key := fmt.Sprintf("key_%d", i)
		val := fmt.Sprintf("val_%d", i)
		if err = cache.InsertKey([]byte(key), []byte(val), 0); err != nil {
			expectedError = true
			switch err.(type) {
			case *OffSetBitsError:
				fmt.Println("Expected OffsetBits error")
			default:
				t.Fatal("We should have errored")
			}

			break
		}
	}

	if !expectedError {
		t.Fatal("We should have errored")
	}
}

func TestCacheWriterSetOffsetBitsBelowLimits(t *testing.T) {
	var err error
	assert := assert.New(t)

	taskName := "super_cache"
	destinationFolder := t.TempDir()

	cache, err := NewCacheWriter(
		&CacheWriterOptions{
			TaskName:              taskName,
			DestinationFolder:     destinationFolder,
			SettingsLocation:      "",
			NumberOfKeySlots:      10000,
			GenerateTimestampFile: false,
			RenameCacheFile:       true,
			OffsetBits:            15, // Too small for 5000 keys
			// Min value 16, max 38
		})
	defer cache.Delete()

	assert.NotEqual(nil, err)

	cache.AddDuplicateValue("val_1", 0)
	err = cache.FinishAddDuplicateValues()
	assert.Equal(ErrUnInitialized, err)

	const keys = 10

	for i := 1; i < keys; i++ {
		key := fmt.Sprintf("key_%d", i)
		val := fmt.Sprintf("val_%d", i)
		err = cache.InsertKey([]byte(key), []byte(val), 0)
		assert.Equal(ErrUnInitialized, err)
	}
}

func TestCacheWriterKeyspaceFull(t *testing.T) {
	var err error
	assert := assert.New(t)

	taskName := "super_cache"
	destinationFolder := t.TempDir()

	cache, err := NewCacheWriter(
		&CacheWriterOptions{
			TaskName:              taskName,
			DestinationFolder:     destinationFolder,
			SettingsLocation:      "",
			NumberOfKeySlots:      1000,
			GenerateTimestampFile: false,
			RenameCacheFile:       true,
			OffsetBits:            27,
			// Min value 16, max 38
		})
	defer cache.Delete()

	assert.Equal(nil, err)

	cache.AddDuplicateValue("val_1", 0)
	cache.FinishAddDuplicateValues()

	expectedError := false
	const keys = 5000 // with offsetbits = 16, not enough space

	for i := 1; i < keys; i++ {
		key := fmt.Sprintf("key_%d", i)
		val := fmt.Sprintf("val_%d", i)
		if err = cache.InsertKey([]byte(key), []byte(val), 0); err != nil {
			expectedError = true

			switch err.(type) {
			case *KeySpaceIsFullError:
				fmt.Println("Expected KeySpaceIsFullError error")
			default:
				t.Fatal("We should have errored")
			}

			break
		}
	}

	if !expectedError {
		t.Fatal("We should have errored")
	}
}
