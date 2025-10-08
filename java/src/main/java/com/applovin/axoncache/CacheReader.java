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
        return nativeContainsKey(nativeHandle, key);
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
        return nativeGetKey(nativeHandle, key);
    }

    /**
     * Gets the value for a long key
     * 
     * @param key The key to look up
     * @return The value as Long, or null if not found
     */
    public Long getLong(String key) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetLong(nativeHandle, key);
    }

    /**
     * Gets the value for an integer key
     * 
     * @param key The key to look up
     * @return The value as Integer, or null if not found
     */
    public Integer getInteger(String key) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetInteger(nativeHandle, key);
    }

    /**
     * Gets the value for a double key
     * 
     * @param key The key to look up
     * @return The value as Double, or null if not found
     */
    public Double getDouble(String key) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetDouble(nativeHandle, key);
    }

    /**
     * Gets the value for a boolean key
     * 
     * @param key The key to look up
     * @return The value as Boolean, or null if not found
     */
    public Boolean getBoolean(String key) {
        if (nativeHandle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetBool(nativeHandle, key);
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
        return nativeGetVector(nativeHandle, key);
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
        return nativeGetFloatVector(nativeHandle, key);
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
        return nativeGetVectorKeySize(nativeHandle, key);
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
        return nativeGetVectorKey(nativeHandle, key, index);
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
        return nativeGetKeyType(nativeHandle, key);
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
    private native boolean nativeContainsKey(long handle, String key);
    private native String nativeGetKey(long handle, String key);
    private native Long nativeGetLong(long handle, String key);
    private native Integer nativeGetInteger(long handle, String key);
    private native Double nativeGetDouble(long handle, String key);
    private native Boolean nativeGetBool(long handle, String key);
    private native String[] nativeGetVector(long handle, String key);
    private native float[] nativeGetFloatVector(long handle, String key);
    private native int nativeGetVectorKeySize(long handle, String key);
    private native String nativeGetVectorKey(long handle, String key, int index);
    private native String nativeGetKeyType(long handle, String key);
}
