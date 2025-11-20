// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package axoncache

/*
#cgo CXXFLAGS: -std=c++2a -O3 -Iinclude -Ibuild/_deps/xxhash-src -DXXH_NAMESPACE=AXONCACHE_
#cgo linux LDFLAGS: -static-libstdc++ -static-libgcc -static

#include "include/axoncache/capi/CacheReaderCApi.h"
#include "include/axoncache/capi/CacheWriterCApi.h"
*/
import "C"
import (
	"errors"
	"fmt"
	"os"
	"strconv"
	"sync/atomic"
	"time"
	"unsafe"

	"github.com/AppLovin/AxonCache/internal/core"
	log "github.com/sirupsen/logrus"
)

var ErrNotFound = errors.New("Key was not found")
var ErrUnInitialized = errors.New("No AxonCache file is loaded")

// CacheReader is an interface that defines the GetKey method.
type CacheReaderInterface interface {
	GetKey(key string) (string, error)
}

type UpdateCallback func(CacheReaderInterface) error

// CacheReader is a struct that implements the CacheReader interface.
type CacheReader struct {
	Handle *C.CacheReaderHandle

	// Used for updates
	TaskName                string
	DestinationFolder       string
	BaseUrls                string
	MostRecentFileTimestamp int64

	// Used for one time cache load
	Timestamp              string
	IsPreloadMemoryEnabled int

	UpdatePeriod time.Duration

	Stop           chan struct{}
	UpdateCallback UpdateCallback
}

type CacheReaderOptions struct {
	TaskName               string
	DestinationFolder      string
	UpdatePeriod           time.Duration
	BaseUrls               string
	DownloadAtInit         bool
	UpdateCallback         UpdateCallback
	Timestamp              string
	IsPreloadMemoryEnabled bool
}

func ensureDestinationFolderExists(folderPath string) error {
	if _, err := os.Stat(folderPath); err != nil {
		if os.IsNotExist(err) {
			if err := os.MkdirAll(folderPath, os.ModePerm); err != nil {
				return fmt.Errorf("error creating destination folder %s: %v", folderPath, err)
			}
		} else {
			return fmt.Errorf("error checking destination folder %s: %v", folderPath, err)
		}
	}
	return nil
}

func NewCacheReader(options *CacheReaderOptions) (*CacheReader, error) {
	handle := C.NewCacheReaderHandle()

	// Create a channel to signal the goroutine to stop
	stop := make(chan struct{})

	isPreloadMemoryEnabled := 0
	if options.IsPreloadMemoryEnabled {
		isPreloadMemoryEnabled = 1
	}

	alcacheReader := &CacheReader{
		Handle:                 handle,
		TaskName:               options.TaskName,
		DestinationFolder:      options.DestinationFolder,
		BaseUrls:               options.BaseUrls,
		UpdatePeriod:           options.UpdatePeriod,
		Stop:                   stop,
		UpdateCallback:         options.UpdateCallback,
		Timestamp:              options.Timestamp,
		IsPreloadMemoryEnabled: isPreloadMemoryEnabled,
	}

	if err := ensureDestinationFolderExists(options.DestinationFolder); err != nil {
		return nil, err
	}

	if options.DownloadAtInit {
		err := alcacheReader.maybeDownload()
		if err != nil {
			return alcacheReader, fmt.Errorf("Error downloading alcache file %s: %v", alcacheReader.TaskName, err)
		}
	}

	var err error
	var latestTimestamp int64
	checkForNewFiles := true
	if len(options.Timestamp) == 0 {
		latestTimestamp, err = alcacheReader.getLatestTimestamp()
		if err != nil {
			return alcacheReader, fmt.Errorf("Error retrieving latest timestamp: %s", err)
		}
	} else {
		checkForNewFiles = false
		latestTimestamp, err = strconv.ParseInt(options.Timestamp, 10, 64)
		if err != nil {
			return alcacheReader, fmt.Errorf("Cannot parse timestamp as int from value %s, error: %s", options.Timestamp, err)
		}
	}

	err = alcacheReader.Update(fmt.Sprintf("%d", latestTimestamp))
	if err != nil {
		return alcacheReader, err
	}

	// A file was found, we can go check for new files now
	if checkForNewFiles {
		alcacheReader.checkForNewFiles()
	}

	return alcacheReader, nil
}

func (c *CacheReader) Update(timestamp string) error {
	if c.Handle == nil {
		return fmt.Errorf("C handle is nil")
	}

	mostRecentFileTimestamp, err := strconv.ParseInt(timestamp, 10, 64)
	if err != nil {
		return fmt.Errorf("Cannot convert timestamp %s to int, %s", timestamp, err)
	}

	cstring1 := C.CString(c.TaskName)
	defer C.free(unsafe.Pointer(cstring1))

	cstring2 := C.CString(c.DestinationFolder)
	defer C.free(unsafe.Pointer(cstring2))

	cstring3 := C.CString(timestamp)
	defer C.free(unsafe.Pointer(cstring3))

	ret := C.CacheReader_Initialize(c.Handle, cstring1, cstring2, cstring3, C.int(c.IsPreloadMemoryEnabled))
	if ret != 0 {
		return fmt.Errorf("Error initializing reader, error code: %d", ret)
	}

	atomic.StoreInt64(&c.MostRecentFileTimestamp, mostRecentFileTimestamp)

	if c.UpdateCallback != nil {
		err := c.UpdateCallback(c)
		if err != nil {
			return fmt.Errorf("Error calling update callback: %s", err)
		}
	}

	return nil
}

func (c *CacheReader) isInitialized() bool {
	return atomic.LoadInt64(&c.MostRecentFileTimestamp) > 0
}

func (c *CacheReader) getLatestTimestamp() (int64, error) {
	// ads_event_data.timestamp.latest
	path := fmt.Sprintf("%s/%s.cache.timestamp.latest", c.DestinationFolder, c.TaskName)

	content, err := os.ReadFile(path)
	if err != nil {
		return -1, fmt.Errorf("Cannot read file %s, error %s", path, err)
	}

	timestamp, err := strconv.ParseInt(string(content), 10, 64)
	if err != nil {
		return -1, fmt.Errorf("Cannot parse timestamp as int from file %s, error: %s", path, err)
	}

	return timestamp, nil
}

// If we have a non empty base urls, we will try to fetch the file ourself directly
// from datamover-proxy (or anywhere)
func (c *CacheReader) maybeDownload() error {
	if len(c.BaseUrls) == 0 {
		return nil
	}
	downloaderOptions := &core.DownloaderOptions{
		Basename:          c.TaskName + ".cache",
		DestinationFolder: c.DestinationFolder,
		AllUrls:           c.BaseUrls,
		Timeout:           time.Second,
	}

	downloader := core.NewDownloader(downloaderOptions)
	_, err := downloader.Run()
	return err
}

func (c *CacheReader) checkForNewFiles() {
	// Start a goroutine to check a condition every N seconds (configured with c.UpdatePeriod)
	go func() {
		for {
			select {
			case <-c.Stop:
				log.Info("Stopping condition checker...")
				return
			default:
				err := c.maybeDownload()
				if err != nil {
					log.Errorf("Error downloading alcache file %s: %v", c.TaskName, err)
				}

				// Check your condition here
				latestTimestamp, err := c.getLatestTimestamp()
				if err != nil {
					log.Errorf("Error computing latest timestamp: %s", err)
				} else {
					if latestTimestamp > atomic.LoadInt64(&c.MostRecentFileTimestamp) {
						log.Infof("Found a new timestamp, latest %d, most recent %d\n", latestTimestamp, c.MostRecentFileTimestamp)
						err := c.Update(fmt.Sprintf("%d", latestTimestamp))
						if err != nil {
							log.Errorf("Error updating to the latest timestamp %d: %s\n", latestTimestamp, err)
						}
					}
				}

				time.Sleep(c.UpdatePeriod)
			}
		}
	}()
}

func (c *CacheReader) Delete() {
	// Reset the MostRecentFileTimestamp to 0, so that lookups will fail
	atomic.StoreInt64(&c.MostRecentFileTimestamp, 0)

	C.CacheReader_DeleteCppObject(c.Handle)

	c.Handle = nil

	close(c.Stop)
}

func (c *CacheReader) ContainsKey(key string) (bool, error) {
	if !c.isInitialized() {
		return false, ErrUnInitialized
	}

	k := []byte(key)

	hasKey := C.CacheReader_ContainsKey(c.Handle,
		(*C.char)(unsafe.Pointer(&k[0])), C.size_t(len(k)))

	return hasKey != 0, nil
}

// GetKey is a method of CacheReader that satisfies the CacheReader interface.
func (c *CacheReader) GetKey(key string) (string, error) {
	if !c.isInitialized() {
		return "", ErrUnInitialized
	}

	k := []byte(key)

	size := C.int(0)
	isExists := C.int(0)
	cstring := C.CacheReader_GetKey(c.Handle,
		(*C.char)(unsafe.Pointer(&k[0])), C.size_t(len(k)),
		(*C.int)(unsafe.Pointer(&isExists)), (*C.int)(unsafe.Pointer(&size)))

	if isExists == 0 {
		return "", ErrNotFound
	}

	output := C.GoStringN(cstring, size)

	// Free memory
	C.free(unsafe.Pointer(cstring))

	return output, nil
}

func (c *CacheReader) GetKeyType(key string) (string, error) {
	if !c.isInitialized() {
		return "", ErrUnInitialized
	}

	k := []byte(key)

	size := C.int(0)
	cstring := C.CacheReader_GetKeyType(c.Handle,
		(*C.char)(unsafe.Pointer(&k[0])), C.size_t(len(k)),
		(*C.int)(unsafe.Pointer(&size)))

	if cstring == nil {
		return "", ErrNotFound
	}

	output := C.GoStringN(cstring, size)

	// Free memory
	C.free(unsafe.Pointer(cstring))

	return output, nil
}

func (c *CacheReader) GetBool(key string) (bool, error) {
	if !c.isInitialized() {
		return false, ErrUnInitialized
	}

	k := []byte(key)

	isExists := C.int(0)

	val := C.CacheReader_GetBool(c.Handle,
		(*C.char)(unsafe.Pointer(&k[0])), C.size_t(len(k)),
		(*C.int)(unsafe.Pointer(&isExists)), C.int(0))

	if isExists == 0 {
		return false, ErrNotFound
	}

	return val != 0, nil
}

func (c *CacheReader) GetInt(key string) (int, error) {
	if !c.isInitialized() {
		return 0, ErrUnInitialized
	}

	k := []byte(key)

	isExists := C.int(0)

	val := C.CacheReader_GetInteger(c.Handle,
		(*C.char)(unsafe.Pointer(&k[0])), C.size_t(len(k)),
		(*C.int)(unsafe.Pointer(&isExists)), C.int(0))

	if isExists == 0 {
		return 0, ErrNotFound
	}

	return int(val), nil
}

func (c *CacheReader) GetLong(key string) (int64, error) {
	if !c.isInitialized() {
		return 0, ErrUnInitialized
	}

	k := []byte(key)

	isExists := C.int(0)

	val := C.CacheReader_GetLong(c.Handle,
		(*C.char)(unsafe.Pointer(&k[0])), C.size_t(len(k)),
		(*C.int)(unsafe.Pointer(&isExists)), C.int64_t(0))

	if isExists == 0 {
		return 0, ErrNotFound
	}

	return int64(val), nil
}

func (c *CacheReader) GetDouble(key string) (float64, error) {
	if !c.isInitialized() {
		return 0, ErrUnInitialized
	}

	k := []byte(key)

	isExists := C.int(0)

	val := C.CacheReader_GetDouble(c.Handle,
		(*C.char)(unsafe.Pointer(&k[0])), C.size_t(len(k)),
		(*C.int)(unsafe.Pointer(&isExists)), C.double(0))

	if isExists == 0 {
		return 0., ErrNotFound
	}

	return float64(val), nil
}

func (c *CacheReader) GetVector(key string) ([]string, error) {
	if !c.isInitialized() {
		return []string{}, ErrUnInitialized
	}

	k := []byte(key)

	// Variables to hold the count and sizes
	var count C.int
	var cSizes *C.int

	cStrings := C.CacheReader_GetVector(c.Handle,
		(*C.char)(unsafe.Pointer(&k[0])), C.size_t(len(k)),
		&count, &cSizes)

	if cStrings == nil {
		C.free(unsafe.Pointer(cStrings)) // Free the array
		C.free(unsafe.Pointer(cSizes))   // Free the sizes array
		return []string{}, ErrNotFound
	}

	// Convert the `char**` and sizes to Go data structures
	goStrings := charPtrArrayToSliceWithSizes(cStrings, cSizes, int(count))

	// Free allocated memory in C
	for i := range count {
		cStr := *(**C.char)(unsafe.Pointer(uintptr(unsafe.Pointer(cStrings)) + uintptr(i)*unsafe.Sizeof(uintptr(0))))
		C.free(unsafe.Pointer(cStr)) // Free each string
	}
	C.free(unsafe.Pointer(cStrings)) // Free the array
	C.free(unsafe.Pointer(cSizes))   // Free the sizes array

	return goStrings, nil
}

// Helper function to convert `char**` and sizes to a Go slice of strings
func charPtrArrayToSliceWithSizes(cStrings **C.char, cSizes *C.int, count int) []string {
	goStrings := make([]string, count)

	for i := range count {
		// Get the size of the current string
		size := int(*(*C.int)(unsafe.Pointer(uintptr(unsafe.Pointer(cSizes)) + uintptr(i)*unsafe.Sizeof(C.int(0)))))

		// Get the pointer to the current string
		cStr := *(**C.char)(unsafe.Pointer(uintptr(unsafe.Pointer(cStrings)) + uintptr(i)*unsafe.Sizeof(uintptr(0))))

		// Convert the C string with size to a Go string
		goStrings[i] = C.GoStringN(cStr, C.int(size))
	}

	return goStrings
}

func (c *CacheReader) GetVectorFloat(key string) ([]float32, error) {
	if !c.isInitialized() {
		return []float32{}, ErrUnInitialized
	}

	k := []byte(key)

	// Variables to hold the size
	var cSize C.int

	cFloats := C.CacheReader_GetFloatVector(c.Handle,
		(*C.char)(unsafe.Pointer(&k[0])), C.size_t(len(k)),
		&cSize)

	if cFloats == nil {
		C.free(unsafe.Pointer(cFloats)) // Free the array
		return []float32{}, ErrNotFound
	}

	goFloats := make([]float32, cSize)
	for i := range int(cSize) {
		goFloats[i] = float32(*(*C.float)(unsafe.Pointer(uintptr(unsafe.Pointer(cFloats)) + uintptr(i)*unsafe.Sizeof(C.float(0)))))
	}
	C.free(unsafe.Pointer(cFloats)) // Free the array

	return goFloats, nil
}

// CacheWriter
type CacheWriter struct {
	Handle                *C.CacheWriterHandle
	TaskName              string
	DestinationFolder     string
	GenerateTimestampFile bool
	RenameCacheFile       bool
	OffsetBits            int
	IsInitialized         bool
}

type CacheWriterOptions struct {
	TaskName              string
	DestinationFolder     string
	SettingsLocation      string
	NumberOfKeySlots      uint64
	GenerateTimestampFile bool
	RenameCacheFile       bool
	CacheType             int
	OffsetBits            int
}

type OffSetBitsError struct {
	Err        error
	OffsetBits int
}

func (e *OffSetBitsError) Error() string {
	return fmt.Sprintf("✋ offset bits error: %s", e.Err)
}

type KeySpaceIsFullError struct {
	Err error
}

func (e *KeySpaceIsFullError) Error() string {
	return fmt.Sprintf("✋ keyspace is full error: %s", e.Err)
}

func NewCacheWriter(options *CacheWriterOptions) (*CacheWriter, error) {
	handle := C.NewCacheWriterHandle()

	cacheType := options.CacheType
	if cacheType == 0 {
		cacheType = 5 // default value
	}

	offsetBits := options.OffsetBits
	if offsetBits == 0 {
		offsetBits = 35 // default value
	}

	alcacheWriter := &CacheWriter{
		Handle:                handle,
		TaskName:              options.TaskName,
		DestinationFolder:     options.DestinationFolder,
		GenerateTimestampFile: options.GenerateTimestampFile,
		RenameCacheFile:       options.RenameCacheFile,
		OffsetBits:            offsetBits,
	}

	if len(options.SettingsLocation) == 0 {
		options.SettingsLocation = fmt.Sprintf("%s/%s.properties", options.DestinationFolder, options.TaskName)
		ccacheProperties := make(map[string]string)
		ccacheProperties["ccache.destination_folder"] = options.DestinationFolder
		ccacheProperties["ccache.type"] = fmt.Sprintf("%d", cacheType)
		ccacheProperties["ccache.offset.bits"] = fmt.Sprintf("%d", offsetBits)
		core.WritePropertiesFile(ccacheProperties, options.SettingsLocation) // to be used by InitializeReader()
	}

	cstring1 := C.CString(options.TaskName)
	defer C.free(unsafe.Pointer(cstring1))

	cstring2 := C.CString(options.SettingsLocation)
	defer C.free(unsafe.Pointer(cstring2))

	ret := C.CacheWriter_Initialize(handle, cstring1, cstring2, C.uint64_t(options.NumberOfKeySlots))
	if ret != 0 {
		return alcacheWriter, fmt.Errorf("Error initializing writer, error code: %d", ret)
	}

	alcacheWriter.IsInitialized = true
	return alcacheWriter, nil
}

func (c *CacheWriter) Delete() {
	C.CacheWriter_DeleteCppObject(c.Handle)

	c.Handle = nil
}

func (c *CacheWriter) isInitialized() bool {
	return c.IsInitialized
}

func (c *CacheWriter) InsertKey(k []byte, v []byte, keyType int8) error {
	if !c.isInitialized() {
		return ErrUnInitialized
	}

	if len(k) == 0 {
		return errors.New("Empty key")
	}

	var ret C.int8_t
	if len(v) == 0 {
		ret = C.CacheWriter_InsertKey(c.Handle,
			(*C.char)(unsafe.Pointer(&k[0])), C.size_t(len(k)),
			nil, C.size_t(0),
			C.int8_t(keyType))
	} else {
		ret = C.CacheWriter_InsertKey(c.Handle,
			(*C.char)(unsafe.Pointer(&k[0])), C.size_t(len(k)),
			(*C.char)(unsafe.Pointer(&v[0])), C.size_t(len(v)),
			C.int8_t(keyType))
	}

	if ret != 0 {
		msg := fmt.Sprintf("Error inserting key '%s' : error code %d", string(k), ret)
		err := errors.New(msg)

		// Special case error so that we can act on it
		if ret == 2 {
			return &OffSetBitsError{Err: err, OffsetBits: c.OffsetBits}
		} else if ret == 3 {
			return &KeySpaceIsFullError{Err: err}
		} else {
			return err
		}
	}

	return nil
}

func (c *CacheWriter) AddDuplicateValue(value string, queryType int8) {
	cstring1 := C.CString(value)
	defer C.free(unsafe.Pointer(cstring1))

	C.CacheWriter_AddDuplicateValue(c.Handle, cstring1, C.int8_t(queryType))
}

func (c *CacheWriter) FinishAddDuplicateValues() error {
	if !c.isInitialized() {
		return ErrUnInitialized
	}

	ret := C.CacheWriter_FinishAddDuplicateValues(c.Handle)
	if ret != 0 {
		return errors.New("Error setting duplicate values")
	}
	return nil
}

func (c *CacheWriter) FinishCacheCreation() (error, string) {
	if !c.isInitialized() {
		return ErrUnInitialized, ""
	}

	ret := C.CacheWriter_FinishCacheCreation(c.Handle)
	if ret != 0 {
		return fmt.Errorf("error finishing cache creation: %d", ret), ""
	}

	timestamp := fmt.Sprint(time.Now().UnixMilli())

	if c.GenerateTimestampFile {
		// Create a timestamp file as well
		newPath := fmt.Sprintf("%s/%s.cache.timestamp.latest", c.DestinationFolder, c.TaskName)
		if err := os.WriteFile(newPath, []byte(timestamp), 0644); err != nil {
			return fmt.Errorf("error writing timestamp file : %s", err), timestamp
		}
	}

	if c.RenameCacheFile {
		// Rename cache file
		cacheWithoutTimestamp := fmt.Sprintf("%s/%s.cache", c.DestinationFolder, c.TaskName)
		cacheWithTimestamp := fmt.Sprintf("%s/%s.%s.cache", c.DestinationFolder, c.TaskName, timestamp)
		if err := os.Rename(cacheWithoutTimestamp, cacheWithTimestamp); err != nil {
			return fmt.Errorf("error renaming file : %s", err), timestamp
		}
	}

	return nil, timestamp
}
