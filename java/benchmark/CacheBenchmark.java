// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

import com.applovin.axoncache.CacheReader;
import com.applovin.axoncache.CacheWriter;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Random;

import java.text.NumberFormat;

import java.util.HashMap;
import java.util.Map;

public class CacheBenchmark {
    
    private static int testsPassed = 0;
    private static int testsFailed = 0;
    
    public static void main(String[] args) {
        int maxKeys = 1000 * 1000;
        
        // Step 1: create the array
        String[] keys = new String[maxKeys];
        String[] vals = new String[maxKeys];

        for (int i = 0; i < maxKeys; i++) {
            keys[i] = "key_" + i;
            vals[i] = "val_" + i;
        }
        
        benchJavaMap(maxKeys, keys, vals);
        benchAxonCache(maxKeys, keys, vals);
    }

    private static void benchAxonCache(int maxKeys, String[] keys, String[] vals) {
        System.out.println("Using AxonCache");
        
        // Setup test environment
        setupTestDirectory();

        // Run comprehensive tests
        String timestamp = generateAxonCache(maxKeys, keys, vals);
        
        if (timestamp == null) {
            System.err.println("\nWrite operations failed, aborting tests");
            System.exit(1);
        }
        
        lookupAllKeys(timestamp, maxKeys, keys);
        System.out.println("");
    }

    private static void benchJavaMap(int maxKeys, String[] keys, String[] vals) {
        System.out.println("Using HashMap");
        
        // Declare and create the HashMap
        Map<String, String> map = new HashMap<>();

        long start = System.nanoTime();
        
        // Test: Insert multiple data types
        int insertCount = 0;

        for ( int i = 0 ; i < maxKeys ; ++i )
        {
            map.put(keys[i], vals[i]);
        }

        double elapsed = (System.nanoTime() - start) / 1_000_000_000.0;
        double qps = maxKeys / elapsed;

        NumberFormat nf = NumberFormat.getInstance(); // adds commas

        System.out.printf(
            "Inserted %s keys in %.2f seconds (%s keys/sec)%n",
            nf.format(maxKeys),
            elapsed,
            nf.format((long) qps)
        );

        // Lookups
        start = System.nanoTime();

        // Shuffle the array in place (Fisher–Yates algorithm)
        Random rand = new Random(); // or new Random(42) for reproducibility
        for (int i = maxKeys - 1; i > 0; i--) {
            int j = rand.nextInt(i + 1);
            String tmp = keys[i];
            keys[i] = keys[j];
            keys[j] = tmp;
        }

        for ( int i = 0 ; i < maxKeys ; ++i )
        {
            String result = map.get(keys[i]);
        }

        elapsed = (System.nanoTime() - start) / 1_000_000_000.0;
        qps = maxKeys / elapsed;

        System.out.printf(
            "Looked up %s keys in %.2f seconds (%s keys/sec)%n",
            nf.format(maxKeys),
            elapsed,
            nf.format((long) qps)
        );

        System.out.println("");
    }
    
    private static void setupTestDirectory() {
        File outputDir = new File("./cache_output");
        if (!outputDir.exists()) {
            outputDir.mkdirs();
        }
    }
    
    /**
     * Create a temporary properties file with cache configuration
     */
    private static String createTempPropertiesFile() {
        try {
            File tempFile = File.createTempFile("axoncache_", ".properties");
            tempFile.deleteOnExit(); // Auto-cleanup
            
            try (PrintWriter out = new java.io.PrintWriter(tempFile)) {
                out.println("ccache.destination_folder=./cache_output");
                out.println("ccache.type=5");
                out.println("ccache.offset.bits=35");
            }
            
            return tempFile.getAbsolutePath();
        } catch (IOException e) {
            System.err.println("Failed to create temp properties file: " + e.getMessage());
            return null;
        }
    }

    private static void fail(String message) {
        System.out.println("  [FAIL] " + message);
    }
    
    /**
     * Test all write operations
     * @return The timestamp of the created cache, or null on failure
     */
    private static String generateAxonCache(int maxKeys, String[] keys, String[] vals) {
        // Create temp properties file
        String propertiesFile = createTempPropertiesFile();
        if (propertiesFile == null) {
            fail("Failed to create temporary properties file");
            return null;
        }
        
        try (CacheWriter writer = new CacheWriter()) {
            // Test: Initialize
            byte result = writer.initialize(
                "example_cache",
                propertiesFile,
                2 * maxKeys
            );
            assertTest("Writer Initialize", result == 0, "Failed with code: " + result);
            
            if (result != 0) {
                String error = writer.getLastError();
                if (error != null && !error.isEmpty()) {
                    System.err.println("  Error: " + error);
                }
                return null;
            }

            long start = System.nanoTime();
            
            // Test: Insert multiple data types
            int insertCount = 0;

            for ( int i = 0 ; i < maxKeys ; ++i )
            {
                writer.insertKey(keys[i], vals[i], (byte)0);
            }

            // Test: Finalize
            result = writer.finishCacheCreation();
            assertTest("Finish Cache Creation", result == 0, "Failed with code: " + result);
            
            if (result != 0) {
                return null;
            }

            double elapsed = (System.nanoTime() - start) / 1_000_000_000.0;
            double qps = maxKeys / elapsed;

            NumberFormat nf = NumberFormat.getInstance(); // adds commas

            System.out.printf(
                "Inserted %s keys in %.2f seconds (%s keys/sec)%n",
                nf.format(maxKeys),
                elapsed,
                nf.format((long) qps)
            );
            
            // Rename cache file with timestamp
            String cacheTimestamp = String.valueOf(System.currentTimeMillis());
            File cacheDir = new File("./cache_output");
            File oldFile = new File(cacheDir, "example_cache.cache");
            File newFile = new File(cacheDir, "example_cache." + cacheTimestamp + ".cache");
            
            if (oldFile.exists()) {
                if (oldFile.renameTo(newFile)) {
                    // System.out.println("  Cache file: " + newFile.getName());
                    return cacheTimestamp;
                } else {
                    fail("Failed to rename cache file");
                    return null;
                }
            } else {
                fail("Cache file not found");
                return null;
            }
            
        } catch (Exception e) {
            fail("Write exception: " + e.getMessage());
            e.printStackTrace();
            return null;
        }
    }
    
    /**
     * Test all read operations
     * @param timestamp The timestamp of the cache to read
     */
    private static void lookupAllKeys(String timestamp, int maxKeys, String[] keys) {
        try (CacheReader reader = new CacheReader()) {
            // Test: Initialize
            int result = reader.initialize("example_cache", "./cache_output", timestamp, true);
            assertTest("Reader Initialize", result == 0, "Failed with code: " + result);
            
            if (result != 0) {
                fail("Cannot proceed with read tests");
                return;
            }

            long start = System.nanoTime();

            // Shuffle the array in place (Fisher–Yates algorithm)
            Random rand = new Random(); // or new Random(42) for reproducibility
            for (int i = maxKeys - 1; i > 0; i--) {
                int j = rand.nextInt(i + 1);
                String tmp = keys[i];
                keys[i] = keys[j];
                keys[j] = tmp;
            }

            for ( int i = 0 ; i < maxKeys ; ++i )
            {
                String val = reader.getString(keys[i]);
            }

            
            double elapsed = (System.nanoTime() - start) / 1_000_000_000.0;
            double qps = maxKeys / elapsed;

            NumberFormat nf = NumberFormat.getInstance(); // adds commas

            System.out.printf(
                "Looked up %s keys in %.2f seconds (%s keys/sec)%n",
                nf.format(maxKeys),
                elapsed,
                nf.format((long) qps)
            );
        } catch (Exception e) {
            fail("Read exception: " + e.getMessage());
            e.printStackTrace();
        }
    }
    
    private static void assertTest(String testName, boolean condition, String failMessage) {
        if (!condition) {
            fail(testName + ": " + failMessage);
        }
    }
}
