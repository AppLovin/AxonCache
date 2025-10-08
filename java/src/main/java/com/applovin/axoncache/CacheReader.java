// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.
package com.applovin.axoncache;

import java.nio.charset.StandardCharsets;

/**
 * Java wrapper for AxonCache Reader C API
 */
public class CacheReader implements AutoCloseable {
    
    static {
        System.loadLibrary("axoncache_jni");
    }

    private long nativeHandle;

    /**
     * Creates a new CacheReader instance
     */
    public CacheReader() {
        this.nativeHandle = nativeNewCacheReaderHandle();
    }

    /**
     * Initializes the cache reader
     * 
     * @param taskName The name of the task/cache
     * @param destinationFolder The folder where the cache file is located
     * @param timestamp The timestamp of the cache file
     * @param isPreloadMemoryEnabled Whether to preload the cache into memory
     * @return 0 on success, non-zero on error
     */
    public int initialize(String taskName, String destinationFolder, String timestamp, boolean isPreloadMemoryEnabled) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeInitialize(nativeHandle, taskName, destinationFolder, timestamp, isPreloadMemoryEnabled ? 1 : 0);
    }

    /**
     * Finalizes the cache reader (called automatically by close())
     */
    private void finalize_internal() {
        if (nativeHandle != 0) {
            nativeFinalize(nativeHandle);
        }
    }

    /**
     * Checks if a key exists in the cache
     * 
     * @param key The key to check
     * @return true if the key exists, false otherwise
     */
    public boolean containsKey(String key) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        return nativeContainsKey(nativeHandle, keyBytes, keyBytes.length) != 0;
    }

    /**
     * Gets the value for a string key
     * 
     * @param key The key to look up
     * @return The value as a string, or null if not found
     */
    public String getString(String key) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        return nativeGetKey(nativeHandle, keyBytes, keyBytes.length);
    }

    /**
     * Gets the value for a long key
     * 
     * @param key The key to look up
     * @param defaultValue The default value if key not found
     * @return The value as a long
     */
    public long getLong(String key, long defaultValue) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        return nativeGetLong(nativeHandle, keyBytes, keyBytes.length, defaultValue);
    }

    /**
     * Gets the value for an integer key
     * 
     * @param key The key to look up
     * @param defaultValue The default value if key not found
     * @return The value as an int
     */
    public int getInteger(String key, int defaultValue) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        return nativeGetInteger(nativeHandle, keyBytes, keyBytes.length, defaultValue);
    }

    /**
     * Gets the value for a double key
     * 
     * @param key The key to look up
     * @param defaultValue The default value if key not found
     * @return The value as a double
     */
    public double getDouble(String key, double defaultValue) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        return nativeGetDouble(nativeHandle, keyBytes, keyBytes.length, defaultValue);
    }

    /**
     * Gets the value for a boolean key
     * 
     * @param key The key to look up
     * @param defaultValue The default value if key not found
     * @return The value as a boolean
     */
    public boolean getBoolean(String key, boolean defaultValue) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        return nativeGetBool(nativeHandle, keyBytes, keyBytes.length, defaultValue ? 1 : 0) != 0;
    }

    /**
     * Gets a vector of strings for a key
     * 
     * @param key The key to look up
     * @return An array of strings, or null if not found
     */
    public String[] getVector(String key) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        return nativeGetVector(nativeHandle, keyBytes, keyBytes.length);
    }

    /**
     * Gets a vector of floats for a key
     * 
     * @param key The key to look up
     * @return An array of floats, or null if not found
     */
    public float[] getFloatVector(String key) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        return nativeGetFloatVector(nativeHandle, keyBytes, keyBytes.length);
    }

    /**
     * Gets the size of a vector for a key
     * 
     * @param key The key to look up
     * @return The size of the vector
     */
    public int getVectorSize(String key) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        return nativeGetVectorKeySize(nativeHandle, keyBytes, keyBytes.length);
    }

    /**
     * Gets a specific item from a vector
     * 
     * @param key The key to look up
     * @param index The index in the vector
     * @return The value at the index, or null if not found
     */
    public String getVectorItem(String key, int index) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        return nativeGetVectorKey(nativeHandle, keyBytes, keyBytes.length, index);
    }

    /**
     * Gets the type of a key
     * 
     * @param key The key to look up
     * @return The type as a string, or null if not found
     */
    public String getKeyType(String key) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        byte[] keyBytes = key.getBytes(StandardCharsets.UTF_8);
        return nativeGetKeyType(nativeHandle, keyBytes, keyBytes.length);
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
    private native long nativeNewCacheReaderHandle();
    private native int nativeInitialize(long handle, String taskName, String destinationFolder, String timestamp, int isPreloadMemoryEnabled);
    private native void nativeFinalize(long handle);
    private native void nativeDeleteCppObject(long handle);
    private native int nativeContainsKey(long handle, byte[] key, int keySize);
    private native String nativeGetKey(long handle, byte[] key, int keySize);
    private native long nativeGetLong(long handle, byte[] key, int keySize, long defaultValue);
    private native int nativeGetInteger(long handle, byte[] key, int keySize, int defaultValue);
    private native double nativeGetDouble(long handle, byte[] key, int keySize, double defaultValue);
    private native int nativeGetBool(long handle, byte[] key, int keySize, int defaultValue);
    private native String[] nativeGetVector(long handle, byte[] key, int keySize);
    private native float[] nativeGetFloatVector(long handle, byte[] key, int keySize);
    private native int nativeGetVectorKeySize(long handle, byte[] key, int keySize);
    private native String nativeGetVectorKey(long handle, byte[] key, int keySize, int index);
    private native String nativeGetKeyType(long handle, byte[] key, int keySize);
}
