// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

package core

import (
	"context"
	"errors"
	"fmt"
	"io"
	"math/rand"
	"net/http"
	"net/url"
	"os"
	"path"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	"github.com/dustin/go-humanize"
	"github.com/jpillora/backoff"

	"github.com/sirupsen/logrus"
	log "github.com/sirupsen/logrus"
)

// The String() function below needs to be updated when adding entries
type DownloadReason int

const (
	UpToDate                      DownloadReason = iota // 0
	NoTimestamp                                         // 1
	MissingLatestTimestamp                              // 2
	MissingRemoteInputs                                 // 3
	MissingLocalInputs                                  // 4
	EmptyRemoteInputs                                   // 5
	EmptyLocalInputs                                    // 6
	SimilarRemoteAndLocalInputs                         // 7
	DifferentRemoteAndLocalInputs                       // 8
	NotEnoughFiles                                      // 9
)

func (reason DownloadReason) String() string {
	return []string{
		"UpToDate",
		"NoTimestamp",
		"MissingLatestTimestamp",
		"MissingRemoteInputs",
		"MissingLocalInputs",
		"EmptyRemoteInputs",
		"EmptyLocalInputs",
		"SimilarRemoteAndLocalInputs",
		"DifferentRemoteAndLocalInputs",
		"NotEnoughFiles"}[reason]
}

type DownloadError struct {
	Url string
	Err error
}

func (e *DownloadError) Error() string {
	return fmt.Sprintf("✋ download error: %s for url %s", e.Err, e.Url)
}

type DownloadStats struct {
	Timestamp               string  `json:"timestamp"`
	Delay                   string  `json:"delay"`
	DownloadMethod          string  `json:"download_method"`
	Prefetch                string  `json:"pre_fetch"`
	Download                string  `json:"download"`
	DownloadSeconds         float64 `json:"download_seconds"`
	DownloadSpeed           string  `json:"download_speed"`
	DiskWrite               string  `json:"decompress_checksum_disk_write"`
	DiskWriteSeconds        float64 `json:"decompress_checksum_disk_write_seconds"`
	MMap                    string  `json:"mmap"`
	MMapSeconds             float64 `json:"mmap_seconds"`
	Total                   string  `json:"total"`
	TotalSeconds            float64 `json:"total_seconds"`
	CompressedSize          string  `json:"compressed_size"`
	CompressedPath          string  `json:"compressed_path"`
	DecompressedSize        string  `json:"decompressed_size"`
	DecompressedSizeInBytes int64   `json:"decompressed_size_bytes"`
	DecompressedPath        string  `json:"decompressed_path"`
	BaseUrl                 string  `json:"base_url"`
	TotalBytesDownloaded    int64   `json:"total_bytes_downloaded"`
	TotalBytesWrittenToDisk int64   `json:"total_bytes_written_to_disk"`
	TotalRows               int     `json:"total_rows"`

	SavedFiles []string `json:"saved_files"`
}

type RemoteSettings struct {
	compressionMethod string
	checksumExtension string
	downloadMethod    string
	cacheProducer     string
	cacheType         int
}

type CoreDownloader interface {
	Get(url string) (*http.Response, error)
}

type Downloader struct {
	Basename          string
	BasenameNoExt     string
	DestinationFolder string
	AllUrls           string
	DownloadMethod    string
	Log               *logrus.Entry
	MMapInfo          *MMapInfo
	RemoteSettings    RemoteSettings

	// Clients used to be *http.Client before GCS support
	HttpClient          CoreDownloader
	HttpClientNoTimeout CoreDownloader

	// Bail out after a certain amount of time.
	NoProgressTimeout int

	DefaultCompressionMethod string
	DefaultChecksumExtension string

	// Memory map a file after download, so that it is ready and fully loaded in memory for
	// servers to utilize it right away. We do this in process with ccache V2, so this is only
	// enabled for ccache V1 files
	MMapFileAfterDownload bool
}

type DownloaderOptions struct {
	Basename          string
	DestinationFolder string
	AllUrls           string
	Timeout           time.Duration
}

func NewDownloader(downloaderOptions *DownloaderOptions) *Downloader {
	downloadMethod := "http"

	basename := downloaderOptions.Basename
	destinationFolder := downloaderOptions.DestinationFolder
	allUrls := downloaderOptions.AllUrls

	logger := log.WithFields(logrus.Fields{
		"basename": basename,
	})

	mmapFileAfterDownload := true

	baseUrls := buildBaseUrlList(allUrls)

	// Used for large downloads. We should probably have a large timeout (say 5 minutes)
	// but for some remote machines
	httpClientNoTimeout, _ := MakeHttpClient(downloaderOptions, baseUrls, false)

	// Used for quick download of tiny files (settings, checksum etc...)
	httpClient, _ := MakeHttpClient(downloaderOptions, baseUrls, true)

	downloader := &Downloader{
		Basename:          basename,
		BasenameNoExt:     strings.SplitN(basename, ".", 2)[0],
		DestinationFolder: destinationFolder,
		DownloadMethod:    downloadMethod,
		Log:               logger,
		AllUrls:           allUrls,

		HttpClient:          httpClient,
		HttpClientNoTimeout: httpClientNoTimeout,

		DefaultCompressionMethod: "zst",
		DefaultChecksumExtension: "xxh3",
		MMapFileAfterDownload:    mmapFileAfterDownload,
	}
	return downloader
}

func MakeHttpClient(downloaderOptions *DownloaderOptions, baseUrls []string, useTimeout bool) (CoreDownloader, error) {
	// Check
	httpClient := &http.Client{}

	if len(baseUrls) > 0 {
		baseUrl := baseUrls[0]

		u, err := url.Parse(baseUrl)
		if err != nil {
			return httpClient, nil
		}

		//
		// This file:// mode could be use with GCS / Fuse mode or if populate
		// and datamover are running on the same host.
		// Fuse doc -> https://cloud.google.com/storage/docs/gcs-fuse
		// See also this
		// https://cloud.google.com/filestore
		//
		if u.Scheme == "file" {
			t := &http.Transport{}
			t.RegisterProtocol("file", http.NewFileTransport(http.Dir("/")))

			httpClient := &http.Client{Transport: t}
			return httpClient, nil
		}

		// A GCS path/url is formed like this:
		//
		// gs://datamover-bucket/test/ccache_seed/fast_cache.mmap.timestamp
		// the host is the bucket name, in our case the current one being "datamover-bucket"
		//
		if u.Scheme == "gs" {
			bucketName := u.Host
			return NewGCSClient(bucketName)
		}
	}

	if useTimeout {
		httpClient.Timeout = downloaderOptions.Timeout
	}

	return httpClient, nil
}

// We reshuffle the base urls so that we try url that matter first
// Otherwise we get a bunch of failed attempt to get timestamps
func buildBaseUrlList(allUrls string) []string {
	// Our result
	var baseUrls []string

	allUrlsSplitted := strings.Split(allUrls, ",")

	// First pass where we select the urls we care and put them first
	for _, baseUrl := range allUrlsSplitted {
		if len(baseUrl) > 0 {
			baseUrlWithScheme := fmt.Sprintf("http://%s", baseUrl)
			if strings.HasPrefix(baseUrl, "file://") { // Is the url a file based one ?
				baseUrlWithScheme = baseUrl
			} else if strings.HasPrefix(baseUrl, "gs://") { // Is the url a gcs based one
				baseUrlWithScheme = baseUrl
			}

			baseUrls = append(baseUrls, baseUrlWithScheme)
		}
	}

	return baseUrls
}

func (d *Downloader) GetBaseUrlAndTimestamp() (string, string, error) {
	baseUrls := buildBaseUrlList(d.AllUrls)

	var upTodateBaseUrl string
	var upTodateTimestamp int64 = 0

	for i, baseUrl := range baseUrls {
		url := fmt.Sprintf("%s/%s.timestamp", baseUrl, d.Basename)
		d.Log.Printf("Checking base url #%d, %s", i, url)

		resp, err := d.HttpClient.Get(url)
		if err != nil {
			d.Log.WithError(err).Warn("Cannot fetch timestamp")
			continue
		}
		defer resp.Body.Close()

		if resp.StatusCode != 200 {
			d.Log.WithFields(logrus.Fields{
				"code":    resp.StatusCode,
				"url":     url,
				"baseUrl": baseUrl,
			}).Warn("Cannot fetch timestamp")
			continue
		}

		body, err := io.ReadAll(resp.Body)
		if err != nil {
			d.Log.WithError(err).Warn("Cannot read timestamp response")
			continue
		}

		// Convert and parse timestamp to a date
		timestamp := string(body)

		// Trim trailing \n
		timestamp = strings.TrimSuffix(timestamp, "\n")

		timestampMillis, err := strconv.ParseInt(timestamp, 10, 64)
		if err != nil {
			d.Log.Print(err)
			continue
		}

		// Display timestamp as a date for debugging
		timestampSeconds := timestampMillis / 1000
		d.Log.Printf("⏰ remote file creation %s = %q", timestamp, time.Unix(timestampSeconds, 0))

		// Is this timestamp the most recent timestamp ?
		if timestampMillis > upTodateTimestamp {
			upTodateTimestamp = timestampMillis
			upTodateBaseUrl = baseUrl
		}
	}

	if len(upTodateBaseUrl) > 0 {
		return upTodateBaseUrl, fmt.Sprint(upTodateTimestamp), nil
	} else {
		return "", "", errors.New(fmt.Sprintf("Cannot retrieve any timestamp, tried %d base urls", len(baseUrls)))
	}
}

// $ curl http://dls-datamoverstage1.alcfd.com/ccache_seed/mediation_cache.mmap.settings
// checksum.extension=xxh
// compression.method=zst
// data.files=mediation_data
func (d *Downloader) fetchRemoteSettings(baseUrl string, timestamp string) (Properties, error) {
	properties := make(Properties)

	url := fmt.Sprintf("%s/%s.settings", baseUrl, d.Basename)
	if len(timestamp) > 0 {
		url = fmt.Sprintf("%s/%s.%s.settings", baseUrl, d.Basename, timestamp)
	}

	d.Log.Printf("remote settings url: %s", url)
	resp, err := d.HttpClient.Get(url)
	if err != nil {
		return properties, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		return properties, fmt.Errorf("Non 200 response status code, %d", resp.StatusCode)
	}

	return loadProperties(resp.Body)
}

func (d *Downloader) makeSettingsFromProperties(properties Properties) RemoteSettings {
	remoteSettings := RemoteSettings{
		compressionMethod: d.DefaultCompressionMethod,
		checksumExtension: d.DefaultChecksumExtension,
		downloadMethod:    d.DownloadMethod,
	}
	// gz, lz4 or zstd
	remoteSettings.compressionMethod = GetStringProperty(properties, "compression.method", d.DefaultCompressionMethod)

	// xxh or sha1
	remoteSettings.checksumExtension = GetStringProperty(properties, "checksum.extension", d.DefaultChecksumExtension)

	// http or torrent in theory, but when set, should always be http
	remoteSettings.downloadMethod = GetStringProperty(properties, "download.method", d.DownloadMethod)

	return remoteSettings
}

// $ curl http://nuq-datamoverstage1.alcfd.com/ccache_seed/mediation_cache.mmap.1608138249734.xxh3
// 8956642779699444393
func (d *Downloader) fetchChecksum(baseUrl string, timestamp string, checksumExtension string) (string, error) {
	url := fmt.Sprintf("%s/%s.%s.%s", baseUrl, d.Basename, timestamp, checksumExtension)
	d.Log.Printf("Remote checksum url: %s", url)
	resp, err := d.HttpClient.Get(url)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		return "", fmt.Errorf("Non 200 response status code, %d", resp.StatusCode)
	}

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}

	return string(body), nil
}

func (d *Downloader) fetchUncompressedSize(baseUrl string, timestamp string) (int64, error) {
	url := fmt.Sprintf("%s/%s.%s.size", baseUrl, d.Basename, timestamp)
	d.Log.Printf("Remote uncompressed size url: %s", url)
	resp, err := d.HttpClient.Get(url)
	if err != nil {
		return -1, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		return -1, fmt.Errorf("Non 200 response status code, %d", resp.StatusCode)
	}

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return -1, err
	}

	size, err := strconv.ParseInt(string(body), 10, 64)
	if err != nil {
		return -1, err
	}

	return size, nil
}

func (d *Downloader) fetchRemoteInputsFile(baseUrl string) ([]byte, error) {
	url := fmt.Sprintf("%s/%s.inputs", baseUrl, d.Basename)
	d.Log.Printf("remote inputs url: %s", url)
	resp, err := d.HttpClient.Get(url)
	if err != nil {
		return []byte{}, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		return []byte{}, errors.New(fmt.Sprintf("Invalid HTTP response %d for input file request", resp.StatusCode))
	}

	body, err := io.ReadAll(resp.Body)
	return body, err
}

// DownloadFile will download a url to a local file. It's efficient because it will
// write as it downloads and not load the whole file into memory. We pass an io.TeeReader
// into Copy() to report progress on the download.
func (d *Downloader) DownloadFile(filepath string, url string) error {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	// Get the data
	resp, err := d.HttpClientNoTimeout.Get(url)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	// Create the file, but give it a tmp file extension, this means we won't overwrite a
	// file until it's downloaded, but we'll remove the tmp extension once downloaded.
	tempFile := fmt.Sprintf("%s.http_download_tmp.%d", filepath, rand.Uint32())
	out, err := os.Create(tempFile)
	if err != nil {
		return err
	}

	counter := &WriteCounter{Name: path.Base(filepath), Start: time.Now(), Label: "downloading"}
	contentLength := resp.Header.Get("Content-Length")
	val, err := strconv.ParseInt(contentLength, 10, 64)
	if err == nil {
		counter.ContentLength = val
	} else {
		val = -1
	}

	handleWriterProgress(ctx, counter)

	// Create our progress reporter and pass it to be used alongside our writer
	written, err := io.Copy(out, io.TeeReader(resp.Body, counter))
	if err != nil {
		out.Close()
		return err
	}

	// Close the file without defer so it can happen before Rename()
	out.Close()

	if val != -1 && written != counter.ContentLength {
		if err = os.Remove(tempFile); err != nil {
			log.WithError(err).Warn("Cannot delete temp file")
		}
		return errors.New("http transfer error - not all bytes were read")
	}

	return os.Rename(tempFile, filepath)
}

func (d *Downloader) hasAtLeastOneDownloadedFileAlready(timestamp string) bool {

	// Count how many associated files exists. If there are none maybe we are in a situation
	// where only a .latest got created for some reason, but we still need to sync files
	//
	// For example on that mks box, only one file was present, and the ccache file which
	// needed the adserver_goog_data to be present was never generated.
	//
	// ops@mks-datamover2:/var/lib/applovin$ find . | grep adserver_goog_data
	// ./datamover/adserver_goog_data.timestamp.latest
	//
	// In a more recent occurence, it looked like this:
	//
	// "43 B     Feb  5 07:07:42 adserver_goog_data.inputs",
	// "118 B    Feb  5 07:07:42 adserver_goog_data.settings",
	// "13 B     Feb  5 07:07:42 adserver_goog_data.timestamp",
	// "13 B     Feb  5 07:07:42 adserver_goog_data.timestamp.latest",
	// FIXME: a more robust way would be to download the .settings file, extract the filesize
	//        and verify that the local downloaded file has the same file size. It isn't as
	//        reliable as a full cksum check but it's still better than the current heuristic
	//        which does not handle well corrupted local files.
	//
	globPattern := fmt.Sprintf("%s/%s.*%s*", d.DestinationFolder, d.BasenameNoExt, timestamp)
	paths, err := filepath.Glob(globPattern)
	if err != nil {
		return false
	}

	if len(paths) <= 2 {
		d.Log.Printf("⏰ no files matching %s", globPattern)
		return false
	}

	return true
}

// shouldDownload tells whether a file is out of date and need to be
// re-fetched from the remote server
func (d *Downloader) shouldDownload() (bool, DownloadReason) {
	baseUrl, timestamp, err := d.GetBaseUrlAndTimestamp()
	if err != nil {
		return true, NoTimestamp
	}

	timestampPathLatest := fmt.Sprintf("%s/%s.timestamp.latest", d.DestinationFolder, d.Basename)
	content, err := os.ReadFile(timestampPathLatest)
	if err != nil {
		d.Log.WithError(err).Warnf("Cannot read %s from disk; maybe it is the first datamover run", timestampPathLatest)
		return true, MissingLatestTimestamp
	}

	if string(content) != timestamp {
		remoteInputs, err := d.fetchRemoteInputsFile(baseUrl)
		if err != nil {
			d.Log.WithError(err).Warn("Cannot fetch remote inputs file; maybe it is the first datamover run")
			return true, MissingRemoteInputs
		}

		if len(remoteInputs) == 0 {
			d.Log.WithError(err).Warn("Discard empty remote inputs file")
			return true, EmptyRemoteInputs
		}

		inputsPath := fmt.Sprintf("%s/%s.inputs", d.DestinationFolder, d.Basename)
		localInputs, err := os.ReadFile(inputsPath)
		if err != nil {
			d.Log.WithError(err).Warn("Cannot read inputs file from disk; maybe it is the first datamover run")
			return true, MissingLocalInputs
		}

		if len(localInputs) == 0 {
			d.Log.WithError(err).Warn("Discard empty local inputs file")
			return true, EmptyLocalInputs
		}

		if string(localInputs) == string(remoteInputs) {

			// Always do this check before returnining false
			if !d.hasAtLeastOneDownloadedFileAlready(timestamp) {
				return true, NotEnoughFiles
			} else {
				d.Log.Print("Remote and local inputs files are the same. No need to update.")
				return false, SimilarRemoteAndLocalInputs
			}
		}

		// remote file is different from local, an update is required
		d.Log.Printf("Inputs file is different: local %s, remote %s", string(localInputs), string(remoteInputs))
		return true, DifferentRemoteAndLocalInputs
	}

	// Always do this check before returnining false
	if !d.hasAtLeastOneDownloadedFileAlready(timestamp) {
		return true, NotEnoughFiles
	} else {
		return false, UpToDate
	}
}

func (d *Downloader) waitUntilNewFileIsAvailable(timeout time.Duration) error {
	// Use an exponential backoff to not retry to download an up to date asset all the time,
	// which is very noisy for platform_data
	b := &backoff.Backoff{
		Min:    5 * time.Second,
		Max:    30 * time.Second,
		Factor: 2,
		Jitter: true,
	}

	startTime := time.Now()

	for {
		elapsed := time.Now().Sub(startTime)
		if timeout > 0 && elapsed > timeout {
			return errors.New(fmt.Sprintf("Reached timeout %s waiting for a new file for %s", timeout, d.Basename))
		}

		if needsUpdate, reason := d.shouldDownload(); !needsUpdate {
			sleepTime := b.Duration()
			d.Log.Debugf("🏖️  file %s already up to date, reason %s, no need to download it ; will retry in %s", d.Basename, reason, sleepTime)
			time.Sleep(sleepTime)
			continue
		} else {
			d.Log.Debugf("file %s is out of date and should be synced ; reason: %s", d.Basename, reason)
		}
		break
	}

	return nil
}

// We use an io.Writer to count how many rows there are in a file when decompressing it
// It only make sense to compute the number of rows in a file when that file is a key/value file
func (d *Downloader) makeExtraWriter() ExtraWriter {
	return &DevNullWriter{}
}

func (d *Downloader) recordSavedFile(filepath string, downloadStats *DownloadStats) {
	downloadStats.SavedFiles = append(downloadStats.SavedFiles, filepath)
}

func (d *Downloader) writeStringToFile(path string, content string, downloadStats *DownloadStats) error {
	// Write local file first
	if err := WriteStringToFile(path, content); err != nil {
		return err
	}

	d.recordSavedFile(path, downloadStats)
	return nil
}

func (d *Downloader) mmapFileAfterDownload(downloadStats *DownloadStats) {
	// Memory map files to keep them in kernel memory,
	// so that servers are fast at reloading ccache files
	// and no page faults happen. To look at page faults the
	// `dstat --vm` command can be used.
	if d.MMapFileAfterDownload {
		mmapStartTime := time.Now()
		d.Log.Printf("🗺️  Memory map %s", downloadStats.DecompressedPath)
		mmapInfo, err := MemoryMapFile(downloadStats.DecompressedPath)
		if err != nil {
			d.Log.WithError(err).Error("Cannot mmap file")
		}

		// It is important to wait a bit (60 seconds found empirically) before
		// unmapping the file
		go func(mmapInfo *MMapInfo) {
			if mmapInfo != nil {
				<-time.After(60 * time.Second)
				MemoryUnMapAndEvictFile(mmapInfo)
			}
		}(d.MMapInfo)

		d.MMapInfo = mmapInfo

		downloadStats.MMap = fmt.Sprintf("%s", time.Now().Sub(mmapStartTime))
		downloadStats.MMapSeconds = time.Now().Sub(mmapStartTime).Seconds()
	} else {
		downloadStats.MMap = "not_memory_mapped"
	}
}

func (d *Downloader) downloadInternal(downloadStats *DownloadStats, extraWriter ExtraWriter, baseUrl string, downloadMethod string, timestamp string, compressionMethod string, checksumExtension string, uncompressedSize int64, remoteCksum string) (string, error) {
	//
	// Start downloading
	//
	downloadStats.DownloadMethod = downloadMethod

	downloadStartTime := time.Now()

	var decompressedPath string
	var decompressedSize int64
	var diskWriteStartTime time.Time

	var err error

	//
	// HTTP download code path
	//
	fileUrl := fmt.Sprintf("%s/%s.%s.%s", baseUrl, d.Basename, timestamp, compressionMethod)
	compressedPath := fmt.Sprintf("%s/%s.%s.%s", d.DestinationFolder, d.Basename, timestamp, compressionMethod)

	resp, err := d.HttpClientNoTimeout.Get(fileUrl)
	if err != nil {
		return decompressedPath, &DownloadError{Url: fileUrl, Err: err}
	}
	defer resp.Body.Close()

	// Retrieve content-length to know the compressed size
	contentLength := resp.Header.Get("Content-Length")
	compressedSize, err := strconv.ParseInt(contentLength, 10, 64)
	if err != nil {
		compressedSize = -1
	}

	downloadStats.CompressedSize = humanize.Bytes(uint64(compressedSize))
	diskWriteStartTime = time.Now()

	decompressedPath, decompressedSize, err = WriteDecompressedFileToDiskFromReader(resp.Body, compressedPath, compressionMethod, checksumExtension, remoteCksum, uncompressedSize, extraWriter, "downloading, decompressing and writing to disk")
	if err != nil {
		return decompressedPath, &DownloadError{Url: fileUrl, Err: err}
	}

	// Some of those statistics are not relevant since we do many steps in parallel
	// We keep them around anyway to avoid having odd numbers or empty values.
	downloadStats.Download = fmt.Sprintf("%s", time.Now().Sub(downloadStartTime))
	downloadStats.DownloadSeconds = time.Now().Sub(downloadStartTime).Seconds()
	downloadSpeed := float64(compressedSize) / downloadStats.DownloadSeconds
	downloadStats.DownloadSpeed = humanize.Bytes(uint64(downloadSpeed)) + "/s"

	if decompressedSize != uncompressedSize {
		d.Log.Warnf("Decompressed size mismatch: %d bytes versus %d bytes", decompressedSize, uncompressedSize)

		fi, err := os.Stat(decompressedPath)
		if err != nil {
			return decompressedPath, fmt.Errorf("✋ Cannot stat decompressed file %s : %s", decompressedPath, err)
		}
		d.Log.Warnf("File size on disk: %d bytes", fi.Size())
	}

	downloadStats.DecompressedPath = decompressedPath
	downloadStats.DecompressedSize = humanize.Bytes(uint64(decompressedSize))
	downloadStats.DecompressedSizeInBytes = decompressedSize
	d.recordSavedFile(downloadStats.DecompressedPath, downloadStats)

	downloadStats.TotalBytesWrittenToDisk += decompressedSize
	downloadStats.DiskWrite = fmt.Sprintf("%s", time.Now().Sub(diskWriteStartTime))
	downloadStats.DiskWriteSeconds = time.Now().Sub(diskWriteStartTime).Seconds()

	// Create a size file. This file is used to allocate disk space when decompressing, search for Fallocate
	sizePath := fmt.Sprintf("%s/%s.%s.%s", d.DestinationFolder, d.Basename, timestamp, "size")
	err = d.writeStringToFile(sizePath, fmt.Sprintf("%d", decompressedSize), downloadStats)
	if err != nil {
		return decompressedPath, fmt.Errorf("error creating size file: %s", err)
	}

	// yeah !
	return decompressedPath, nil
}

func (d *Downloader) download() (DownloadStats, error) {
	downloadStats := DownloadStats{}
	downloadStats.SavedFiles = make([]string, 0)
	startTime := time.Now()
	prefetchStartTime := time.Now()

	baseUrl, timestamp, err := d.GetBaseUrlAndTimestamp()
	if err != nil {
		return downloadStats, fmt.Errorf("✋ Error downloading timestamp and baseUrl for %s : %s", d.Basename, err)
	}

	downloadStats.Timestamp = timestamp
	downloadStats.BaseUrl = baseUrl

	// Try to download per timestamp settings
	remoteProperties, err := d.fetchRemoteSettings(baseUrl, timestamp)
	if err != nil {
		d.Log.Info("No per timestamp settings available")

		// Fallback to per file settings
		remoteProperties, err = d.fetchRemoteSettings(baseUrl, "")
		if err != nil {
			msg := fmt.Sprintf("✋ Error downloading remote settings for %s : %s", d.Basename, err)
			return downloadStats, errors.New(msg)
		}
	}

	// Save the settings file to disk so that java services, for parallel jcache loading,
	// and ccache generation to optimize file size, can use it
	path := fmt.Sprintf("%s/%s.%s.settings", d.DestinationFolder, d.Basename, timestamp)
	if err = WritePropertiesFile(remoteProperties, path); err != nil {
		return downloadStats, fmt.Errorf("✋ Error writing settings file: %s", err)
	}

	d.RemoteSettings = d.makeSettingsFromProperties(remoteProperties)

	compressionMethod := d.RemoteSettings.compressionMethod
	checksumExtension := d.RemoteSettings.checksumExtension
	downloadMethod := d.RemoteSettings.downloadMethod

	// Fetch the size of the uncompressed file
	uncompressedSize, err := d.fetchUncompressedSize(baseUrl, timestamp)
	if err != nil {
		d.Log.WithError(err).Errorf("😥 Error fetching size file for %s : %s", d.Basename, err)
		uncompressedSize = -1
	}
	d.Log.Printf("Uncompressed size %d", uncompressedSize)

	// Fetch the remote checksum, before checksuming the downloaded file, since checksuming can take
	// some time and the remote checksum file could disappear in the meantime
	remoteCksum, err := d.fetchChecksum(baseUrl, timestamp, checksumExtension)
	if err != nil {
		return downloadStats, fmt.Errorf("✋ Error downloading %s checksum: %s", d.Basename, err)
	}
	d.Log.Printf("Remote %s cksum %s", checksumExtension, remoteCksum)

	downloadStats.Prefetch = fmt.Sprintf("%s", time.Now().Sub(prefetchStartTime))

	// Actual download
	extraWriter := d.makeExtraWriter()
	decompressedPath, err := d.downloadInternal(&downloadStats, extraWriter, baseUrl, downloadMethod, timestamp, compressionMethod, checksumExtension, uncompressedSize, remoteCksum)
	if err != nil {
		return downloadStats, err
	}

	// Write the checksum to disk, mostly for middle mode
	checksumPath := fmt.Sprintf("%s/%s.%s.%s", d.DestinationFolder, d.Basename, timestamp, checksumExtension)
	err = d.writeStringToFile(checksumPath, remoteCksum, &downloadStats)
	if err != nil {
		return downloadStats, fmt.Errorf("✋ Error writing checksum file %s : %s", checksumPath, err)
	}

	d.mmapFileAfterDownload(&downloadStats)

	// Now that data has been validated, write timestamp files to disk.
	// Those timestamps files are searched by servers so they should be written last
	timestampPaths := []string{
		// write one timestamp file per file
		fmt.Sprintf("%s/%s.timestamp", d.DestinationFolder, d.Basename),

		// used to make sure we are not re-downloading a previously fetched file
		fmt.Sprintf("%s/%s.timestamp.latest", d.DestinationFolder, d.Basename),
	}

	if len(decompressedPath) > 0 {
		// used and required by C-Cache to load .mmap files
		timestampPaths = append(timestampPaths, decompressedPath+".timestamp")
	}

	for _, timestampPath := range timestampPaths {
		err = d.writeStringToFile(timestampPath, timestamp, &downloadStats)
		if err != nil {
			return downloadStats, fmt.Errorf("✋ Error writing timestamp timestampPath %s to disk : %s", timestampPath, err)
		}
	}

	// Finally, compute how much time was spent between this download since the original
	// file was created on the other node (middle box or populate box)
	timestampAsNumber, err := strconv.ParseInt(timestamp, 10, 64)
	if err == nil {
		populateTime := time.Unix(timestampAsNumber/1000, 0)
		delay := time.Now().Sub(populateTime)
		downloadStats.Delay = fmt.Sprintf("%s", delay)
	}

	d.Log.Print("🍺 Download successfull!")

	downloadStats.Total = fmt.Sprintf("%s", time.Now().Sub(startTime))
	downloadStats.TotalSeconds = time.Now().Sub(startTime).Seconds()

	return downloadStats, nil
}

func (d *Downloader) Run() (DownloadStats, error) {
	if needsUpdate, reason := d.shouldDownload(); !needsUpdate {
		d.Log.Debugf("🏖️  file %s already up to date, reason %s, no need to download it", d.Basename, reason)
		return DownloadStats{}, nil
	}

	downloadStats, err := d.download()
	if err != nil {
		// Always garbage collect old files, regardless of errors
		RemoveLastDownloadedFilesAfterError(d.DestinationFolder, d.Basename, downloadStats.Timestamp, &LocalFStorageHandler{})

		return downloadStats, err
	}

	//
	// Garbage collect old files
	//
	numRetainedDownloads := 1

	callback := func(path string) {}
	err = RemoveOldFiles(d.DestinationFolder, d.Basename, numRetainedDownloads, callback, &LocalFStorageHandler{})
	if err != nil {
		d.Log.WithError(err).Error("Cannot remove old files") // Serious error
	}

	return downloadStats, nil
}
