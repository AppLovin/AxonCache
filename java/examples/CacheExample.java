// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

import com.applovin.axoncache.CacheReader;
import com.applovin.axoncache.CacheWriter;

/**
 * Example demonstrating how to use AxonCache Java bindings
 */
public class CacheExample {
    
    public static void main(String[] args) {
        System.out.println("=== AxonCache Java JNI Integration Test ===\n");
        
        // Write cache and get timestamp
        String timestamp = writeCacheExample();
        if (timestamp == null) {
            System.err.println("Failed to write cache, aborting test");
            System.exit(1);
        }
        
        System.out.println("\n" + "=".repeat(60) + "\n");
        
        // Read from the cache we just wrote
        readCacheExample(timestamp);
        
        System.out.println("\n=== All Tests Passed! ===");
    }
    
    /**
     * Example of writing data to cache
     * @return The timestamp of the created cache, or null on failure
     */
    private static String writeCacheExample() {
        System.out.println("=== Part 1: Writing Cache ===");
        
        try (CacheWriter writer = new CacheWriter()) {
            // Initialize the cache writer
            System.out.println("Initializing cache writer...");
            byte result = writer.initialize(
                "example_cache",              // taskName
                "./settings.properties",      // settings location
                100000L                       // number of key slots
            );
            
            if (result != 0) {
                System.err.println("Failed to initialize cache writer: " + result);
                String error = writer.getLastError();
                if (error != null && !error.isEmpty()) {
                    System.err.println("Error details: " + error);
                }
                return null;
            }
            
            System.out.println("Cache writer initialized successfully");
            
            // Set cache configuration
            writer.setCacheType(5);  // LINEAR_PROBE_DEDUP
            writer.setOffsetBits(35);
            
            // Insert some sample data
            System.out.println("\nInserting sample data...");
            
            // String values (type 0)
            writer.insertKey("user:1001:name", "Alice Johnson", (byte)0);
            writer.insertKey("user:1002:name", "Bob Smith", (byte)0);
            writer.insertKey("user:1003:name", "Charlie Brown", (byte)0);
            
            // Integer values (type 3)
            writer.insertKey("user:1001:age", "28", (byte)3);
            writer.insertKey("user:1002:age", "35", (byte)3);
            writer.insertKey("user:1003:age", "42", (byte)3);
            
            // Double values (type 1)
            writer.insertKey("user:1001:score", "95.5", (byte)1);
            writer.insertKey("user:1002:score", "87.3", (byte)1);
            writer.insertKey("user:1003:score", "92.8", (byte)1);
            
            // Boolean values (type 2)
            writer.insertKey("user:1001:active", "true", (byte)2);
            writer.insertKey("user:1002:active", "true", (byte)2);
            writer.insertKey("user:1003:active", "false", (byte)2);
            
            System.out.println("Inserted 12 key-value pairs");
            
            // Finish cache creation and write to disk
            System.out.println("\nFinalizing cache...");
            byte finishResult = writer.finishCacheCreation();
            if (finishResult != 0) {
                System.err.println("Failed to finish cache creation: " + finishResult);
                String error = writer.getLastError();
                if (error != null && !error.isEmpty()) {
                    System.err.println("Error details: " + error);
                }
                return null;
            }
            
            // Print statistics
            System.out.println("\n=== Cache Statistics ===");
            System.out.println("Version: " + writer.getVersion());
            System.out.println("Hash Function: " + writer.getHashFunction());
            System.out.println("Unique Keys: " + writer.getUniqueKeys());
            System.out.println("Max Collisions: " + writer.getMaxCollisions());
            System.out.println("Collisions Counter: " + writer.getCollisionsCounter());
            
            System.out.println("\n  Cache written successfully!");
            
            // Find the cache file and add timestamp if needed
            // The writer may create: example_cache.cache (no timestamp)
            // But the reader expects: example_cache.<timestamp>.cache
            java.io.File cacheDir = new java.io.File("./cache_output");
            String cacheTimestamp = String.valueOf(System.currentTimeMillis());
            
            if (cacheDir.exists() && cacheDir.isDirectory()) {
                java.io.File cacheFile = new java.io.File(cacheDir, "example_cache.cache");
                
                if (cacheFile.exists()) {
                    // Rename to include timestamp
                    String timestampedName = "example_cache." + cacheTimestamp + ".cache";
                    java.io.File timestampedFile = new java.io.File(cacheDir, timestampedName);
                    
                    if (cacheFile.renameTo(timestampedFile)) {
                        System.out.println("  Cache file: cache_output/" + timestampedName);
                        System.out.println("  Timestamp: " + cacheTimestamp);
                    } else {
                        System.err.println("  Warning: Could not rename cache file");
                        cacheTimestamp = "";
                    }
                } else {
                    java.io.File[] files = cacheDir.listFiles((dir, name) -> 
                        name.startsWith("example_cache") && name.endsWith(".cache"));
                    
                    if (files != null && files.length > 0) {
                        String fileName = files[0].getName();
                        System.out.println("  Cache file: cache_output/" + fileName);
                        
                        // Extract timestamp
                        if (fileName.matches("example_cache\\.\\d+\\.cache")) {
                            cacheTimestamp = fileName.substring("example_cache.".length(), 
                                                               fileName.length() - ".cache".length());
                            System.out.println("  Timestamp: " + cacheTimestamp);
                        }
                    }
                }
            }
            
            return cacheTimestamp;
            
        } catch (Exception e) {
            System.err.println("Error during cache writing:");
            e.printStackTrace();
            return null;
        }
    }
    
    /**
     * Example of reading data from cache
     * @param timestamp The timestamp of the cache to read (empty string for files without timestamp)
     */
    private static void readCacheExample(String timestamp) {
        System.out.println("=== Part 2: Reading Cache ===");
        
        try (CacheReader reader = new CacheReader()) {
            // Initialize the cache reader
            System.out.println("Initializing cache reader...");
            
            // The cache file is now: example_cache.<timestamp>.cache
            // Reader will construct: taskName + "." + timestamp + ".cache"
            int result = reader.initialize(
                "example_cache",                   // taskName  
                "./cache_output",                  // destinationFolder (relative to examples/)
                timestamp,                         // timestamp from writer
                true                               // preload into memory
            );
            
            if (result != 0) {
                System.err.println("Failed to initialize cache reader: " + result);
                System.err.println("Cache file location: ./cache_output/example_cache.cache");
                return;
            }
            
            System.out.println("  Cache reader initialized successfully");
            
            // Read sample data that we just wrote
            System.out.println("\n--- Verifying Written Data ---");
            
            String[] userIds = {"user:1001", "user:1002", "user:1003"};
            int successCount = 0;
            int totalChecks = 0;
            
            for (String userId : userIds) {
                System.out.println("\nReading " + userId + ":");
                
                // Check name
                String nameKey = userId + ":name";
                totalChecks++;
                if (reader.containsKey(nameKey)) {
                    String name = reader.getString(nameKey);
                    System.out.println("   Name: " + name);
                    successCount++;
                } else {
                    System.out.println("   Name: NOT FOUND");
                }
                
                // Check age
                String ageKey = userId + ":age";
                totalChecks++;
                int age = reader.getInteger(ageKey, -1);
                if (age != -1) {
                    System.out.println("   Age: " + age);
                    successCount++;
                } else {
                    System.out.println("   Age: NOT FOUND");
                }
                
                // Check score
                String scoreKey = userId + ":score";
                totalChecks++;
                double score = reader.getDouble(scoreKey, 0.0);
                if (reader.containsKey(scoreKey)) {
                    System.out.println("   Score: " + score);
                    successCount++;
                } else {
                    System.out.println("   Score: NOT FOUND");
                }
                
                // Check active
                String activeKey = userId + ":active";
                totalChecks++;
                if (reader.containsKey(activeKey)) {
                    boolean active = reader.getBoolean(activeKey, false);
                    System.out.println("   Active: " + active);
                    successCount++;
                } else {
                    System.out.println("   Active: NOT FOUND");
                }
            }
            
            System.out.println("\n" + "=".repeat(60));
            System.out.println("Results: " + successCount + "/" + totalChecks + " values successfully read");
            
            if (successCount == totalChecks) {
                System.out.println(" All data verified successfully!");
            } else {
                System.out.println(" Some data could not be read");
            }
            
        } catch (Exception e) {
            System.err.println("Error during cache reading:");
            e.printStackTrace();
        }
    }
}
