// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

//! Integration tests for AxonCache Rust bindings

use axoncache::{
    CacheReader, CacheReaderOptions, CacheWriter, CacheWriterOptions, 
    AxonCacheError, KeyType, Result
};
use std::fs;
use std::path::Path;
use tempfile::TempDir;

#[test]
fn test_cache_writer_creation() -> Result<()> {
    let temp_dir = TempDir::new()?;
    let cache_dir = temp_dir.path().join("cache");
    fs::create_dir_all(&cache_dir)?;

    let options = CacheWriterOptions {
        task_name: "test_cache".to_string(),
        destination_folder: cache_dir.to_string_lossy().to_string(),
        number_of_key_slots: 100,
        ..Default::default()
    };

    let writer = CacheWriter::new(options)?;
    assert!(writer.is_initialized());
    
    Ok(())
}

#[test]
fn test_cache_reader_creation() -> Result<()> {
    let temp_dir = TempDir::new()?;
    let cache_dir = temp_dir.path().join("cache");
    fs::create_dir_all(&cache_dir)?;

    // First create a cache
    let writer_options = CacheWriterOptions {
        task_name: "test_cache".to_string(),
        destination_folder: cache_dir.to_string_lossy().to_string(),
        number_of_key_slots: 100,
        ..Default::default()
    };

    let writer = CacheWriter::new(writer_options)?;
    let timestamp = writer.finish_cache_creation()?;

    // Then create a reader
    let reader_options = CacheReaderOptions {
        task_name: "test_cache".to_string(),
        destination_folder: cache_dir.to_string_lossy().to_string(),
        timestamp,
        ..Default::default()
    };

    let reader = CacheReader::new(reader_options)?;
    assert!(reader.is_initialized());
    
    Ok(())
}

#[test]
fn test_string_operations() -> Result<()> {
    let (writer, reader) = create_test_cache()?;

    // Insert string data
    writer.insert_key(b"name", b"John Doe", KeyType::String)?;
    writer.insert_key(b"email", b"john@example.com", KeyType::String)?;
    let timestamp = writer.finish_cache_creation()?;

    // Update reader with new timestamp
    reader.update(&timestamp)?;

    // Test string retrieval
    assert!(reader.contains_key("name")?);
    assert!(reader.contains_key("email")?);
    assert!(!reader.contains_key("nonexistent")?);

    assert_eq!(reader.get_key("name")?, "John Doe");
    assert_eq!(reader.get_key("email")?, "john@example.com");

    // Test error handling
    match reader.get_key("nonexistent") {
        Err(AxonCacheError::NotFound) => {},
        _ => panic!("Expected NotFound error"),
    }

    Ok(())
}

#[test]
fn test_numeric_operations() -> Result<()> {
    let (writer, reader) = create_test_cache()?;

    // Insert numeric data
    writer.insert_key(b"age", &32i32.to_le_bytes(), KeyType::Integer)?;
    writer.insert_key(b"timestamp", &1234567890i64.to_le_bytes(), KeyType::Long)?;
    writer.insert_key(b"score", &95.5f64.to_le_bytes(), KeyType::Double)?;
    writer.insert_key(b"active", &true.to_le_bytes(), KeyType::Bool)?;
    
    let timestamp = writer.finish_cache_creation()?;
    reader.update(&timestamp)?;

    // Test numeric retrieval
    assert_eq!(reader.get_int("age")?, 32);
    assert_eq!(reader.get_long("timestamp")?, 1234567890);
    assert_eq!(reader.get_double("score")?, 95.5);
    assert_eq!(reader.get_bool("active")?, true);

    Ok(())
}

#[test]
fn test_vector_operations() -> Result<()> {
    let (writer, reader) = create_test_cache()?;

    // Insert vector data
    writer.add_duplicate_value("apple", KeyType::String)?;
    writer.add_duplicate_value("banana", KeyType::String)?;
    writer.add_duplicate_value("cherry", KeyType::String)?;
    writer.finish_add_duplicate_values()?;
    writer.insert_key(b"fruits", b"", KeyType::Vector)?;

    // Insert float vector
    writer.insert_key(b"float_vector", &[1.0f32, 2.0f32, 3.0f32].concat(), KeyType::FloatVector)?;
    
    let timestamp = writer.finish_cache_creation()?;
    reader.update(&timestamp)?;

    // Test vector retrieval
    let fruits = reader.get_vector("fruits")?;
    assert_eq!(fruits.len(), 3);
    assert!(fruits.contains(&"apple".to_string()));
    assert!(fruits.contains(&"banana".to_string()));
    assert!(fruits.contains(&"cherry".to_string()));

    let floats = reader.get_vector_float("float_vector")?;
    assert_eq!(floats.len(), 3);
    assert_eq!(floats[0], 1.0);
    assert_eq!(floats[1], 2.0);
    assert_eq!(floats[2], 3.0);

    Ok(())
}

#[test]
fn test_key_types() -> Result<()> {
    let (writer, reader) = create_test_cache()?;

    // Insert different key types
    writer.insert_key(b"string_key", b"value", KeyType::String)?;
    writer.insert_key(b"int_key", &42i32.to_le_bytes(), KeyType::Integer)?;
    writer.insert_key(b"long_key", &1234567890i64.to_le_bytes(), KeyType::Long)?;
    writer.insert_key(b"double_key", &3.14f64.to_le_bytes(), KeyType::Double)?;
    writer.insert_key(b"bool_key", &true.to_le_bytes(), KeyType::Bool)?;
    
    let timestamp = writer.finish_cache_creation()?;
    reader.update(&timestamp)?;

    // Test key types
    assert_eq!(reader.get_key_type("string_key")?, "String");
    assert_eq!(reader.get_key_type("int_key")?, "Integer");
    assert_eq!(reader.get_key_type("long_key")?, "Long");
    assert_eq!(reader.get_key_type("double_key")?, "Double");
    assert_eq!(reader.get_key_type("bool_key")?, "Bool");

    Ok(())
}

#[test]
fn test_error_handling() -> Result<()> {
    let temp_dir = TempDir::new()?;
    let cache_dir = temp_dir.path().join("cache");
    fs::create_dir_all(&cache_dir)?;

    // Test uninitialized reader
    let reader_options = CacheReaderOptions {
        task_name: "nonexistent".to_string(),
        destination_folder: cache_dir.to_string_lossy().to_string(),
        timestamp: "1234567890".to_string(),
        ..Default::default()
    };

    let reader = CacheReader::new(reader_options)?;
    
    // Should get UnInitialized error
    match reader.get_key("test") {
        Err(AxonCacheError::UnInitialized) => {},
        _ => panic!("Expected UnInitialized error"),
    }

    Ok(())
}

#[test]
fn test_empty_key_handling() -> Result<()> {
    let temp_dir = TempDir::new()?;
    let cache_dir = temp_dir.path().join("cache");
    fs::create_dir_all(&cache_dir)?;

    let options = CacheWriterOptions {
        task_name: "test_cache".to_string(),
        destination_folder: cache_dir.to_string_lossy().to_string(),
        number_of_key_slots: 100,
        ..Default::default()
    };

    let writer = CacheWriter::new(options)?;

    // Test empty key
    match writer.insert_key(b"", b"value", KeyType::String) {
        Err(AxonCacheError::EmptyKey) => {},
        _ => panic!("Expected EmptyKey error"),
    }

    Ok(())
}

#[test]
fn test_cache_statistics() -> Result<()> {
    let temp_dir = TempDir::new()?;
    let cache_dir = temp_dir.path().join("cache");
    fs::create_dir_all(&cache_dir)?;

    let options = CacheWriterOptions {
        task_name: "test_cache".to_string(),
        destination_folder: cache_dir.to_string_lossy().to_string(),
        number_of_key_slots: 1000,
        ..Default::default()
    };

    let writer = CacheWriter::new(options)?;

    // Insert some data
    for i in 0..10 {
        let key = format!("key_{}", i);
        let value = format!("value_{}", i);
        writer.insert_key(key.as_bytes(), value.as_bytes(), KeyType::String)?;
    }

    let timestamp = writer.finish_cache_creation()?;

    // Test statistics
    assert!(!writer.get_version().is_empty());
    assert!(!writer.get_hash_function().is_empty());
    assert_eq!(writer.get_unique_keys(), 10);
    assert!(writer.get_max_collisions() >= 0);

    Ok(())
}

#[test]
fn test_concurrent_readers() -> Result<()> {
    let temp_dir = TempDir::new()?;
    let cache_dir = temp_dir.path().join("cache");
    fs::create_dir_all(&cache_dir)?;

    // Create cache
    let writer_options = CacheWriterOptions {
        task_name: "concurrent_test".to_string(),
        destination_folder: cache_dir.to_string_lossy().to_string(),
        number_of_key_slots: 100,
        ..Default::default()
    };

    let writer = CacheWriter::new(writer_options)?;
    writer.insert_key(b"shared_key", b"shared_value", KeyType::String)?;
    let timestamp = writer.finish_cache_creation()?;

    // Create multiple readers
    let reader_options = CacheReaderOptions {
        task_name: "concurrent_test".to_string(),
        destination_folder: cache_dir.to_string_lossy().to_string(),
        timestamp,
        ..Default::default()
    };

    let reader1 = CacheReader::new(reader_options.clone())?;
    let reader2 = CacheReader::new(reader_options)?;

    // Both readers should be able to read the same data
    assert_eq!(reader1.get_key("shared_key")?, "shared_value");
    assert_eq!(reader2.get_key("shared_key")?, "shared_value");

    Ok(())
}

// Helper function to create a test cache
fn create_test_cache() -> Result<(CacheWriter, CacheReader)> {
    let temp_dir = TempDir::new()?;
    let cache_dir = temp_dir.path().join("cache");
    fs::create_dir_all(&cache_dir)?;

    let writer_options = CacheWriterOptions {
        task_name: "test_cache".to_string(),
        destination_folder: cache_dir.to_string_lossy().to_string(),
        number_of_key_slots: 100,
        ..Default::default()
    };

    let writer = CacheWriter::new(writer_options)?;

    let reader_options = CacheReaderOptions {
        task_name: "test_cache".to_string(),
        destination_folder: cache_dir.to_string_lossy().to_string(),
        timestamp: String::new(),
        ..Default::default()
    };

    let reader = CacheReader::new(reader_options)?;

    Ok((writer, reader))
}
