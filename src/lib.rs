// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

//! # AxonCache Rust Bindings
//! 
//! This crate provides safe Rust bindings for the AxonCache C++ library.
//! AxonCache is a high-performance key-value cache system.

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::ptr;
use std::sync::atomic::{AtomicBool, Ordering};
use std::time::Duration;

/// Error types for AxonCache operations
#[derive(Debug, thiserror::Error)]
pub enum AxonCacheError {
    #[error("Key was not found")]
    NotFound,
    #[error("Cache is not initialized")]
    UnInitialized,
    #[error("Empty key provided")]
    EmptyKey,
    #[error("C API error: {0}")]
    ApiError(i32),
    #[error("Offset bits error: {0}")]
    OffsetBitsError(i32),
    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),
    #[error("UTF-8 conversion error: {0}")]
    Utf8Error(#[from] std::str::Utf8Error),
    #[error("UTF-8 conversion error: {0}")]
    FromUtf8Error(#[from] std::string::FromUtf8Error),
    #[error("Nul byte error: {0}")]
    NulError(#[from] std::ffi::NulError),
}

/// Result type for AxonCache operations
pub type Result<T> = std::result::Result<T, AxonCacheError>;

/// Cache reader for reading from AxonCache files
pub struct CacheReader {
    handle: *mut CacheReaderHandle,
    task_name: String,
    destination_folder: String,
    base_urls: String,
    most_recent_file_timestamp: std::sync::atomic::AtomicI64,
    update_period: Duration,
    stop_signal: std::sync::mpsc::Sender<()>,
    update_callback: Option<Box<dyn Fn(&CacheReader) -> Result<()> + Send + Sync>>,
    timestamp: String,
    is_preload_memory_enabled: bool,
}

/// Options for creating a new CacheReader
#[derive(Debug, Clone)]
pub struct CacheReaderOptions {
    pub task_name: String,
    pub destination_folder: String,
    pub update_period: Duration,
    pub base_urls: String,
    pub download_at_init: bool,
    pub timestamp: String,
    pub is_preload_memory_enabled: bool,
}

impl Default for CacheReaderOptions {
    fn default() -> Self {
        Self {
            task_name: String::new(),
            destination_folder: String::new(),
            update_period: Duration::from_secs(60),
            base_urls: String::new(),
            download_at_init: false,
            timestamp: String::new(),
            is_preload_memory_enabled: false,
        }
    }
}

/// Cache writer for creating AxonCache files
pub struct CacheWriter {
    handle: *mut CacheWriterHandle,
    task_name: String,
    destination_folder: String,
    generate_timestamp_file: bool,
    rename_cache_file: bool,
    offset_bits: i32,
    is_initialized: AtomicBool,
}

/// Options for creating a new CacheWriter
#[derive(Debug, Clone)]
pub struct CacheWriterOptions {
    pub task_name: String,
    pub destination_folder: String,
    pub settings_location: String,
    pub number_of_key_slots: u64,
    pub generate_timestamp_file: bool,
    pub rename_cache_file: bool,
    pub cache_type: i32,
    pub offset_bits: i32,
}

impl Default for CacheWriterOptions {
    fn default() -> Self {
        Self {
            task_name: String::new(),
            destination_folder: String::new(),
            settings_location: String::new(),
            number_of_key_slots: 1000000,
            generate_timestamp_file: true,
            rename_cache_file: true,
            cache_type: 5,
            offset_bits: 35,
        }
    }
}

/// Key types for cache operations
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i8)]
pub enum KeyType {
    String = 0,
    Integer = 1,
    Long = 2,
    Double = 3,
    Bool = 4,
    Vector = 5,
    FloatVector = 6,
}

// FFI bindings
#[repr(C)]
struct CacheReaderHandle {
    _private: [u8; 0],
}

#[repr(C)]
struct CacheWriterHandle {
    _private: [u8; 0],
}

extern "C" {
    // CacheReader functions
    fn NewCacheReaderHandle() -> *mut CacheReaderHandle;
    fn CacheReader_Initialize(
        handle: *mut CacheReaderHandle,
        task_name: *const c_char,
        destination_folder: *const c_char,
        timestamp: *const c_char,
        is_preload_memory_enabled: c_int,
    ) -> c_int;
    fn CacheReader_Finalize(handle: *mut CacheReaderHandle);
    fn CacheReader_DeleteCppObject(handle: *mut CacheReaderHandle);
    fn CacheReader_SetCacheType(handle: *mut CacheReaderHandle, cache_type: c_int);
    fn CacheReader_GetVectorKeySize(handle: *mut CacheReaderHandle, key: *const c_char, key_size: usize) -> c_int;
    fn CacheReader_ContainsKey(handle: *mut CacheReaderHandle, key: *const c_char, key_size: usize) -> c_int;
    fn CacheReader_GetLong(
        handle: *mut CacheReaderHandle,
        key: *const c_char,
        key_size: usize,
        is_exist: *mut c_int,
        default_value: i64,
    ) -> i64;
    fn CacheReader_GetInteger(
        handle: *mut CacheReaderHandle,
        key: *const c_char,
        key_size: usize,
        is_exist: *mut c_int,
        default_value: c_int,
    ) -> c_int;
    fn CacheReader_GetDouble(
        handle: *mut CacheReaderHandle,
        key: *const c_char,
        key_size: usize,
        is_exist: *mut c_int,
        default_value: f64,
    ) -> f64;
    fn CacheReader_GetBool(
        handle: *mut CacheReaderHandle,
        key: *const c_char,
        key_size: usize,
        is_exist: *mut c_int,
        default_value: c_int,
    ) -> c_int;
    fn CacheReader_GetVector(
        handle: *mut CacheReaderHandle,
        key: *const c_char,
        key_size: usize,
        vector_size: *mut c_int,
        value_sizes: *mut *mut c_int,
    ) -> *mut *mut c_char;
    fn CacheReader_GetFloatVector(
        handle: *mut CacheReaderHandle,
        key: *const c_char,
        key_size: usize,
        vector_size: *mut c_int,
    ) -> *mut f32;
    fn CacheReader_GetKey(
        handle: *mut CacheReaderHandle,
        key: *const c_char,
        key_size: usize,
        is_exist: *mut c_int,
        value_size: *mut c_int,
    ) -> *mut c_char;
    fn CacheReader_GetVectorKey(
        handle: *mut CacheReaderHandle,
        key: *const c_char,
        key_size: usize,
        index: i32,
        value_size: *mut c_int,
    ) -> *mut c_char;
    fn CacheReader_GetKeyType(
        handle: *mut CacheReaderHandle,
        key: *const c_char,
        key_size: usize,
        value_size: *mut c_int,
    ) -> *mut c_char;

    // CacheWriter functions
    fn NewCacheWriterHandle() -> *mut CacheWriterHandle;
    fn CacheWriter_Initialize(
        handle: *mut CacheWriterHandle,
        task_name: *const c_char,
        settings_location: *const c_char,
        number_of_key_slots: u64,
    ) -> i8;
    fn CacheWriter_Finalize(handle: *mut CacheWriterHandle);
    fn CacheWriter_DeleteCppObject(handle: *mut CacheWriterHandle);
    fn CacheWriter_InsertKey(
        handle: *mut CacheWriterHandle,
        key: *const c_char,
        key_size: usize,
        value: *const c_char,
        value_size: usize,
        key_type: i8,
    ) -> i8;
    fn CacheWriter_AddDuplicateValue(handle: *mut CacheWriterHandle, value: *const c_char, key_type: i8);
    fn CacheWriter_FinishAddDuplicateValues(handle: *mut CacheWriterHandle) -> i8;
    fn CacheWriter_GetVersion(handle: *mut CacheWriterHandle) -> *const c_char;
    fn CacheWriter_GetHashFunction(handle: *mut CacheWriterHandle) -> *const c_char;
    fn CacheWriter_GetUniqueKeys(handle: *mut CacheWriterHandle) -> u64;
    fn CacheWriter_GetMaxCollisions(handle: *mut CacheWriterHandle) -> u64;
    fn CacheWriter_GetCollisionsCounter(handle: *mut CacheWriterHandle) -> *mut c_char;
    fn CacheWriter_FinishCacheCreation(handle: *mut CacheWriterHandle) -> i8;
    fn CacheWriter_GetLastError(handle: *mut CacheWriterHandle) -> *mut c_char;

    // Standard C library functions
    fn free(ptr: *mut c_void);
}

impl CacheReader {
    /// Create a new CacheReader with the given options
    pub fn new(options: CacheReaderOptions) -> Result<Self> {
        let handle = unsafe { NewCacheReaderHandle() };
        if handle.is_null() {
            return Err(AxonCacheError::ApiError(-1));
        }

        let (stop_tx, _stop_rx) = std::sync::mpsc::channel();

        let reader = Self {
            handle,
            task_name: options.task_name.clone(),
            destination_folder: options.destination_folder.clone(),
            base_urls: options.base_urls.clone(),
            most_recent_file_timestamp: std::sync::atomic::AtomicI64::new(0),
            update_period: options.update_period,
            stop_signal: stop_tx,
            update_callback: None,
            timestamp: options.timestamp.clone(),
            is_preload_memory_enabled: options.is_preload_memory_enabled,
        };

        // Ensure destination folder exists
        std::fs::create_dir_all(&options.destination_folder)?;

        if options.download_at_init {
            reader.maybe_download()?;
        }

        let latest_timestamp = if options.timestamp.is_empty() {
            reader.get_latest_timestamp()?
        } else {
            options.timestamp.parse::<i64>().map_err(|_| {
                AxonCacheError::ApiError(-1)
            })?
        };

        reader.update(&latest_timestamp.to_string())?;

        Ok(reader)
    }

    /// Update the cache with a new timestamp
    pub fn update(&self, timestamp: &str) -> Result<()> {
        if self.handle.is_null() {
            return Err(AxonCacheError::UnInitialized);
        }

        let most_recent_file_timestamp = timestamp.parse::<i64>().map_err(|_| {
            AxonCacheError::ApiError(-1)
        })?;

        let task_name_c = CString::new(self.task_name.as_str())?;
        let destination_folder_c = CString::new(self.destination_folder.as_str())?;
        let timestamp_c = CString::new(timestamp)?;

        let ret = unsafe {
            CacheReader_Initialize(
                self.handle,
                task_name_c.as_ptr(),
                destination_folder_c.as_ptr(),
                timestamp_c.as_ptr(),
                if self.is_preload_memory_enabled { 1 } else { 0 },
            )
        };

        if ret != 0 {
            return Err(AxonCacheError::ApiError(ret));
        }

        self.most_recent_file_timestamp.store(most_recent_file_timestamp, Ordering::SeqCst);

        Ok(())
    }

    /// Check if the cache is initialized
    pub fn is_initialized(&self) -> bool {
        self.most_recent_file_timestamp.load(Ordering::SeqCst) > 0
    }

    /// Get the latest timestamp from the cache file
    fn get_latest_timestamp(&self) -> Result<i64> {
        let path = format!("{}/{}.cache.timestamp.latest", self.destination_folder, self.task_name);
        let content = std::fs::read_to_string(&path)?;
        let timestamp = content.trim().parse::<i64>().map_err(|_| {
            AxonCacheError::ApiError(-1)
        })?;
        Ok(timestamp)
    }

    /// Attempt to download the cache file if base URLs are provided
    fn maybe_download(&self) -> Result<()> {
        if self.base_urls.is_empty() {
            return Ok(());
        }
        // TODO: Implement download logic similar to Go version
        // This would require implementing the downloader functionality
        Ok(())
    }

    /// Check if a key exists in the cache
    pub fn contains_key(&self, key: &str) -> Result<bool> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        let key_bytes = key.as_bytes();
        let has_key = unsafe {
            CacheReader_ContainsKey(
                self.handle,
                key_bytes.as_ptr() as *const c_char,
                key_bytes.len(),
            )
        };

        Ok(has_key != 0)
    }

    /// Get a string value from the cache
    pub fn get_key(&self, key: &str) -> Result<String> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        let key_bytes = key.as_bytes();
        let mut is_exists = 0;
        let mut size = 0;

        let cstring = unsafe {
            CacheReader_GetKey(
                self.handle,
                key_bytes.as_ptr() as *const c_char,
                key_bytes.len(),
                &mut is_exists,
                &mut size,
            )
        };

        if is_exists == 0 {
            return Err(AxonCacheError::NotFound);
        }

        let result = unsafe {
            let slice = std::slice::from_raw_parts(cstring, size as usize);
            let bytes: Vec<u8> = slice.iter().map(|&x| x as u8).collect();
            String::from_utf8(bytes)?
        };

        unsafe { free(cstring as *mut c_void) };

        Ok(result)
    }

    /// Get the type of a key
    pub fn get_key_type(&self, key: &str) -> Result<String> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        let key_bytes = key.as_bytes();
        let mut size = 0;

        let cstring = unsafe {
            CacheReader_GetKeyType(
                self.handle,
                key_bytes.as_ptr() as *const c_char,
                key_bytes.len(),
                &mut size,
            )
        };

        if cstring.is_null() {
            return Err(AxonCacheError::NotFound);
        }

        let result = unsafe {
            let slice = std::slice::from_raw_parts(cstring, size as usize);
            let bytes: Vec<u8> = slice.iter().map(|&x| x as u8).collect();
            String::from_utf8(bytes)?
        };

        unsafe { free(cstring as *mut c_void) };

        Ok(result)
    }

    /// Get a boolean value from the cache
    pub fn get_bool(&self, key: &str) -> Result<bool> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        let key_bytes = key.as_bytes();
        let mut is_exists = 0;

        let val = unsafe {
            CacheReader_GetBool(
                self.handle,
                key_bytes.as_ptr() as *const c_char,
                key_bytes.len(),
                &mut is_exists,
                0,
            )
        };

        if is_exists == 0 {
            return Err(AxonCacheError::NotFound);
        }

        Ok(val != 0)
    }

    /// Get an integer value from the cache
    pub fn get_int(&self, key: &str) -> Result<i32> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        let key_bytes = key.as_bytes();
        let mut is_exists = 0;

        let val = unsafe {
            CacheReader_GetInteger(
                self.handle,
                key_bytes.as_ptr() as *const c_char,
                key_bytes.len(),
                &mut is_exists,
                0,
            )
        };

        if is_exists == 0 {
            return Err(AxonCacheError::NotFound);
        }

        Ok(val)
    }

    /// Get a long value from the cache
    pub fn get_long(&self, key: &str) -> Result<i64> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        let key_bytes = key.as_bytes();
        let mut is_exists = 0;

        let val = unsafe {
            CacheReader_GetLong(
                self.handle,
                key_bytes.as_ptr() as *const c_char,
                key_bytes.len(),
                &mut is_exists,
                0,
            )
        };

        if is_exists == 0 {
            return Err(AxonCacheError::NotFound);
        }

        Ok(val)
    }

    /// Get a double value from the cache
    pub fn get_double(&self, key: &str) -> Result<f64> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        let key_bytes = key.as_bytes();
        let mut is_exists = 0;

        let val = unsafe {
            CacheReader_GetDouble(
                self.handle,
                key_bytes.as_ptr() as *const c_char,
                key_bytes.len(),
                &mut is_exists,
                0.0,
            )
        };

        if is_exists == 0 {
            return Err(AxonCacheError::NotFound);
        }

        Ok(val)
    }

    /// Get a vector of strings from the cache
    pub fn get_vector(&self, key: &str) -> Result<Vec<String>> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        let key_bytes = key.as_bytes();
        let mut count = 0;
        let mut c_sizes: *mut c_int = ptr::null_mut();

        let c_strings = unsafe {
            CacheReader_GetVector(
                self.handle,
                key_bytes.as_ptr() as *const c_char,
                key_bytes.len(),
                &mut count,
                &mut c_sizes,
            )
        };

        if c_strings.is_null() {
            return Err(AxonCacheError::NotFound);
        }

        let mut result = Vec::new();
        for i in 0..count {
            let size = unsafe { *c_sizes.add(i as usize) };
            let c_str = unsafe { *c_strings.add(i as usize) };
            
            if !c_str.is_null() {
                let slice = unsafe { std::slice::from_raw_parts(c_str, size as usize) };
                let bytes: Vec<u8> = slice.iter().map(|&x| x as u8).collect();
                if let Ok(s) = String::from_utf8(bytes) {
                    result.push(s);
                }
                unsafe { free(c_str as *mut c_void) };
            }
        }

        unsafe {
            free(c_strings as *mut c_void);
            free(c_sizes as *mut c_void);
        }

        Ok(result)
    }

    /// Get a vector of floats from the cache
    pub fn get_vector_float(&self, key: &str) -> Result<Vec<f32>> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        let key_bytes = key.as_bytes();
        let mut c_size = 0;

        let c_floats = unsafe {
            CacheReader_GetFloatVector(
                self.handle,
                key_bytes.as_ptr() as *const c_char,
                key_bytes.len(),
                &mut c_size,
            )
        };

        if c_floats.is_null() {
            return Err(AxonCacheError::NotFound);
        }

        let mut result = Vec::new();
        for i in 0..c_size {
            let val = unsafe { *c_floats.add(i as usize) };
            result.push(val);
        }

        unsafe { free(c_floats as *mut c_void) };

        Ok(result)
    }
}

impl Drop for CacheReader {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe {
                CacheReader_DeleteCppObject(self.handle);
            }
            self.handle = ptr::null_mut();
        }
    }
}

impl CacheWriter {
    /// Create a new CacheWriter with the given options
    pub fn new(options: CacheWriterOptions) -> Result<Self> {
        let handle = unsafe { NewCacheWriterHandle() };
        if handle.is_null() {
            return Err(AxonCacheError::ApiError(-1));
        }

        let settings_location = if options.settings_location.is_empty() {
            format!("{}/{}.properties", options.destination_folder, options.task_name)
        } else {
            options.settings_location
        };

        // Create properties file if it doesn't exist
        if !std::path::Path::new(&settings_location).exists() {
            let mut properties = std::collections::HashMap::new();
            properties.insert("ccache.destination_folder".to_string(), options.destination_folder.clone());
            properties.insert("ccache.type".to_string(), options.cache_type.to_string());
            properties.insert("ccache.offset.bits".to_string(), options.offset_bits.to_string());
            
            // Write properties file
            let mut content = String::new();
            for (key, value) in properties {
                content.push_str(&format!("{}={}\n", key, value));
            }
            std::fs::write(&settings_location, content)?;
        }

        let task_name_c = CString::new(options.task_name.as_str())?;
        let settings_location_c = CString::new(settings_location.as_str())?;

        let ret = unsafe {
            CacheWriter_Initialize(
                handle,
                task_name_c.as_ptr(),
                settings_location_c.as_ptr(),
                options.number_of_key_slots,
            )
        };

        if ret != 0 {
            return Err(AxonCacheError::ApiError(ret as i32));
        }

        Ok(Self {
            handle,
            task_name: options.task_name,
            destination_folder: options.destination_folder,
            generate_timestamp_file: options.generate_timestamp_file,
            rename_cache_file: options.rename_cache_file,
            offset_bits: options.offset_bits,
            is_initialized: AtomicBool::new(true),
        })
    }

    /// Check if the writer is initialized
    pub fn is_initialized(&self) -> bool {
        self.is_initialized.load(Ordering::SeqCst)
    }

    /// Insert a key-value pair into the cache
    pub fn insert_key(&self, key: &[u8], value: &[u8], key_type: KeyType) -> Result<()> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        if key.is_empty() {
            return Err(AxonCacheError::EmptyKey);
        }

        let ret = if value.is_empty() {
            unsafe {
                CacheWriter_InsertKey(
                    self.handle,
                    key.as_ptr() as *const c_char,
                    key.len(),
                    ptr::null(),
                    0,
                    key_type as i8,
                )
            }
        } else {
            unsafe {
                CacheWriter_InsertKey(
                    self.handle,
                    key.as_ptr() as *const c_char,
                    key.len(),
                    value.as_ptr() as *const c_char,
                    value.len(),
                    key_type as i8,
                )
            }
        };

        if ret != 0 {
            if ret == 2 {
                return Err(AxonCacheError::OffsetBitsError(self.offset_bits));
            } else {
                return Err(AxonCacheError::ApiError(ret as i32));
            }
        }

        Ok(())
    }

    /// Add a duplicate value to the cache
    pub fn add_duplicate_value(&self, value: &str, query_type: KeyType) -> Result<()> {
        let value_c = CString::new(value)?;
        unsafe {
            CacheWriter_AddDuplicateValue(self.handle, value_c.as_ptr(), query_type as i8);
        }
        Ok(())
    }

    /// Finish adding duplicate values
    pub fn finish_add_duplicate_values(&self) -> Result<()> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        let ret = unsafe { CacheWriter_FinishAddDuplicateValues(self.handle) };
        if ret != 0 {
            return Err(AxonCacheError::ApiError(ret as i32));
        }
        Ok(())
    }

    /// Finish cache creation
    pub fn finish_cache_creation(&self) -> Result<String> {
        if !self.is_initialized() {
            return Err(AxonCacheError::UnInitialized);
        }

        let ret = unsafe { CacheWriter_FinishCacheCreation(self.handle) };
        if ret != 0 {
            return Err(AxonCacheError::ApiError(ret as i32));
        }

        let timestamp = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_millis()
            .to_string();

        if self.generate_timestamp_file {
            let path = format!("{}/{}.cache.timestamp.latest", self.destination_folder, self.task_name);
            std::fs::write(&path, &timestamp)?;
        }

        if self.rename_cache_file {
            let cache_without_timestamp = format!("{}/{}.cache", self.destination_folder, self.task_name);
            let cache_with_timestamp = format!("{}/{}.{}.cache", self.destination_folder, self.task_name, timestamp);
            std::fs::rename(&cache_without_timestamp, &cache_with_timestamp)?;
        }

        Ok(timestamp)
    }

    /// Get version information
    pub fn get_version(&self) -> String {
        unsafe {
            let version_ptr = CacheWriter_GetVersion(self.handle);
            if version_ptr.is_null() {
                String::new()
            } else {
                CStr::from_ptr(version_ptr).to_string_lossy().into_owned()
            }
        }
    }

    /// Get hash function information
    pub fn get_hash_function(&self) -> String {
        unsafe {
            let hash_ptr = CacheWriter_GetHashFunction(self.handle);
            if hash_ptr.is_null() {
                String::new()
            } else {
                CStr::from_ptr(hash_ptr).to_string_lossy().into_owned()
            }
        }
    }

    /// Get number of unique keys
    pub fn get_unique_keys(&self) -> u64 {
        unsafe { CacheWriter_GetUniqueKeys(self.handle) }
    }

    /// Get maximum collisions
    pub fn get_max_collisions(&self) -> u64 {
        unsafe { CacheWriter_GetMaxCollisions(self.handle) }
    }

    /// Get collisions counter
    pub fn get_collisions_counter(&self) -> String {
        unsafe {
            let counter_ptr = CacheWriter_GetCollisionsCounter(self.handle);
            if counter_ptr.is_null() {
                String::new()
            } else {
                let result = CStr::from_ptr(counter_ptr).to_string_lossy().into_owned();
                free(counter_ptr as *mut c_void);
                result
            }
        }
    }

    /// Get last error message
    pub fn get_last_error(&self) -> String {
        unsafe {
            let error_ptr = CacheWriter_GetLastError(self.handle);
            if error_ptr.is_null() {
                String::new()
            } else {
                let result = CStr::from_ptr(error_ptr).to_string_lossy().into_owned();
                free(error_ptr as *mut c_void);
                result
            }
        }
    }
}

impl Drop for CacheWriter {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe {
                CacheWriter_DeleteCppObject(self.handle);
            }
            self.handle = ptr::null_mut();
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cache_reader_options_default() {
        let options = CacheReaderOptions::default();
        assert_eq!(options.task_name, "");
        assert_eq!(options.destination_folder, "");
        assert_eq!(options.update_period, Duration::from_secs(60));
    }

    #[test]
    fn test_cache_writer_options_default() {
        let options = CacheWriterOptions::default();
        assert_eq!(options.task_name, "");
        assert_eq!(options.destination_folder, "");
        assert_eq!(options.number_of_key_slots, 1000000);
        assert_eq!(options.cache_type, 5);
        assert_eq!(options.offset_bits, 35);
    }

    #[test]
    fn test_key_type_values() {
        assert_eq!(KeyType::String as i8, 0);
        assert_eq!(KeyType::Integer as i8, 1);
        assert_eq!(KeyType::Long as i8, 2);
        assert_eq!(KeyType::Double as i8, 3);
        assert_eq!(KeyType::Bool as i8, 4);
        assert_eq!(KeyType::Vector as i8, 5);
        assert_eq!(KeyType::FloatVector as i8, 6);
    }
}
