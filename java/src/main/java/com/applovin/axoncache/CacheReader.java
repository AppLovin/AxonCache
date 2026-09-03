// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.
package com.applovin.axoncache;

import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Java wrapper for AxonCache Reader C API
 */
public class CacheReader implements AutoCloseable {

    static {
        NativeLibraryLoader.load();
    }

    // AtomicLong rather than a plain long so that (a) a close() on one thread is guaranteed to be
    // visible to get*()/initialize() calls on other threads instead of racing on a plain field, and
    // (b) close() itself is safe to call more than once, or concurrently, without double-freeing the
    // native handle: getAndSet(0) makes at most one caller ever see the previous non-zero value.
    private final AtomicLong nativeHandle = new AtomicLong();

    /**
     * Creates a new CacheReader instance
     */
    public CacheReader() {
        this.nativeHandle.set(nativeNewCacheReaderHandle());
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
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeInitialize(handle, taskName, destinationFolder, timestamp, isPreloadMemoryEnabled ? 1 : 0);
    }

    /**
     * Checks if a key exists in the cache
     *
     * @param key The key to check
     * @return true if the key exists, false otherwise
     */
    public boolean containsKey(String key) {
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeContainsKey(handle, key);
    }

    /**
     * Gets the value for a string key
     *
     * @param key The key to look up
     * @return The value as a string, or null if not found
     */
    public String getString(String key) {
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetKey(handle, key);
    }

    /**
     * Gets the value for a long key
     *
     * @param key The key to look up
     * @return The value as Long, or null if not found
     */
    public Long getLong(String key) {
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetLong(handle, key);
    }

    /**
     * Gets the value for an integer key
     *
     * @param key The key to look up
     * @return The value as Integer, or null if not found
     */
    public Integer getInteger(String key) {
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetInteger(handle, key);
    }

    /**
     * Gets the value for a double key
     *
     * @param key The key to look up
     * @return The value as Double, or null if not found
     */
    public Double getDouble(String key) {
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetDouble(handle, key);
    }

    /**
     * Gets the value for a boolean key
     *
     * @param key The key to look up
     * @return The value as Boolean, or null if not found
     */
    public Boolean getBoolean(String key) {
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetBool(handle, key);
    }

    /**
     * Gets a vector of strings for a key
     *
     * @param key The key to look up
     * @return An array of strings, or null if not found
     */
    public String[] getVector(String key) {
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetVector(handle, key);
    }

    /**
     * Gets a vector of floats for a key
     *
     * @param key The key to look up
     * @return An array of floats, or null if not found
     */
    public float[] getFloatVector(String key) {
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetFloatVector(handle, key);
    }

    /**
     * Gets the size of a vector for a key
     *
     * @param key The key to look up
     * @return The size of the vector
     */
    public int getVectorSize(String key) {
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetVectorKeySize(handle, key);
    }

    /**
     * Gets a specific item from a vector
     *
     * @param key The key to look up
     * @param index The index in the vector
     * @return The value at the index, or null if not found
     */
    public String getVectorItem(String key, int index) {
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetVectorKey(handle, key, index);
    }

    /**
     * Gets the type of a key
     *
     * @param key The key to look up
     * @return The type as a string, or null if not found
     */
    public String getKeyType(String key) {
        long handle = nativeHandle.get();
        if (handle == 0) {
            throw new IllegalStateException("CacheReader has been closed");
        }
        return nativeGetKeyType(handle, key);
    }

    @Override
    public void close() {
        long handle = nativeHandle.getAndSet(0);
        if (handle != 0) {
            nativeFinalize(handle);
            nativeDeleteCppObject(handle);
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
