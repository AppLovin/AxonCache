// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.
package com.applovin.axoncache;

import java.nio.charset.StandardCharsets;

/**
 * Java wrapper for AxonCache Writer C API
 */
public class CacheWriter implements AutoCloseable {
    
    static {
        System.loadLibrary("axoncache_jni");
    }

    private long nativeHandle;

    /**
     * Creates a new CacheWriter instance
     */
    public CacheWriter() {
        this.nativeHandle = nativeNewCacheWriterHandle();
    }

    /**
     * Initializes the cache writer
     * 
     * @param taskName The name of the task/cache
     * @param settingsLocation The location of the settings file
     * @param numberOfKeySlots The number of key slots to allocate
     * @return 0 on success, non-zero on error
     */
    public byte initialize(String taskName, String settingsLocation, long numberOfKeySlots) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        return nativeInitialize(nativeHandle, taskName, settingsLocation, numberOfKeySlots);
    }

    /**
     * Finalizes the cache writer (called automatically by close())
     */
    private void finalize_internal() {
        if (nativeHandle != 0) {
            nativeFinalize(nativeHandle);
        }
    }

    /**
     * Inserts a key-value pair into the cache
     * 
     * @param key The key
     * @param value The value
     * @param keyType The type of the value
     * @return 0 on success, non-zero on error
     */
    public byte insertKey(String key, String value, byte keyType) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        byte[] valueBytes = value.getBytes(StandardCharsets.UTF_8);
        return nativeInsertKey(nativeHandle, keyBytes, keyBytes.length, valueBytes, valueBytes.length, keyType);
    }

    /**
     * Inserts a key-value pair into the cache with byte arrays
     * 
     * @param key The key as byte array
     * @param value The value as byte array
     * @param keyType The type of the value
     * @return 0 on success, non-zero on error
     */
    public byte insertKey(byte[] key, byte[] value, byte keyType) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        return nativeInsertKey(nativeHandle, key, key.length, value, value.length, keyType);
    }

    /**
     * Adds a duplicate value to the cache
     * 
     * @param value The value
     * @param keyType The type of the value
     */
    public void addDuplicateValue(String value, byte keyType) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        nativeAddDuplicateValue(nativeHandle, value, keyType);
    }

    /**
     * Finishes adding duplicate values
     * 
     * @return 0 on success, non-zero on error
     */
    public byte finishAddDuplicateValues() {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        return nativeFinishAddDuplicateValues(nativeHandle);
    }

    /**
     * Gets the cache version
     * 
     * @return The version string
     */
    public String getVersion() {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        return nativeGetVersion(nativeHandle);
    }

    /**
     * Gets the hash function name
     * 
     * @return The hash function name
     */
    public String getHashFunction() {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        return nativeGetHashFunction(nativeHandle);
    }

    /**
     * Gets the number of unique keys
     * 
     * @return The number of unique keys
     */
    public long getUniqueKeys() {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        return nativeGetUniqueKeys(nativeHandle);
    }

    /**
     * Gets the maximum number of collisions
     * 
     * @return The maximum collision count
     */
    public long getMaxCollisions() {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        return nativeGetMaxCollisions(nativeHandle);
    }

    /**
     * Gets the collisions counter
     * 
     * @return The collisions counter as a string
     */
    public String getCollisionsCounter() {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        return nativeGetCollisionsCounter(nativeHandle);
    }

    /**
     * Finishes cache creation and writes to disk
     * 
     * @return 0 on success, non-zero on error
     */
    public byte finishCacheCreation() {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        return nativeFinishCacheCreation(nativeHandle);
    }

    /**
     * Sets the cache type
     * 
     * @param cacheType The cache type
     */
    public void setCacheType(int cacheType) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        nativeSetCacheType(nativeHandle, cacheType);
    }

    /**
     * Sets the offset bits
     * 
     * @param offsetBits The offset bits
     */
    public void setOffsetBits(int offsetBits) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        nativeSetOffsetBits(nativeHandle, offsetBits);
    }

    /**
     * Gets the last error message
     * 
     * @return The last error message
     */
    public String getLastError() {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheWriter has been closed");
        }
        return nativeGetLastError(nativeHandle);
    }

    @Override
    public void close() {
        if (nativeHandle != 0) {
            finalize_internal();
            nativeDeleteCppObject(nativeHandle);
            nativeHandle = 0;
        }
    }

    // Native method declarations
    private native long nativeNewCacheWriterHandle();
    private native byte nativeInitialize(long handle, String taskName, String settingsLocation, long numberOfKeySlots);
    private native void nativeFinalize(long handle);
    private native void nativeDeleteCppObject(long handle);
    private native byte nativeInsertKey(long handle, byte[] key, int keySize, byte[] value, int valueSize, byte keyType);
    private native void nativeAddDuplicateValue(long handle, String value, byte keyType);
    private native byte nativeFinishAddDuplicateValues(long handle);
    private native String nativeGetVersion(long handle);
    private native String nativeGetHashFunction(long handle);
    private native long nativeGetUniqueKeys(long handle);
    private native long nativeGetMaxCollisions(long handle);
    private native String nativeGetCollisionsCounter(long handle);
    private native byte nativeFinishCacheCreation(long handle);
    private native void nativeSetCacheType(long handle, int cacheType);
    private native void nativeSetOffsetBits(long handle, int offsetBits);
    private native String nativeGetLastError(long handle);
}
