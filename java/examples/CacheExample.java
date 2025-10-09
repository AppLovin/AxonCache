// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

import com.applovin.axoncache.CacheReader;
import com.applovin.axoncache.CacheWriter;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;

/**
 * Comprehensive test and example for AxonCache Java bindings
 * Tests all write and read operations
 */
public class CacheExample {
    
    private static int testsPassed = 0;
    private static int testsFailed = 0;
    
    public static void main(String[] args) {
        System.out.println("=== AxonCache Java JNI Test Suite ===\n");
        
        // Setup test environment
        setupTestDirectory();
        
        // Run comprehensive tests
        System.out.println("--- Write Operations ---");
        String timestamp = testWriteOperations();
        
        if (timestamp == null) {
            System.err.println("\nWrite operations failed, aborting tests");
            System.exit(1);
        }
        
        System.out.println("\n" + "=".repeat(60) + "\n");
        System.out.println("--- Read Operations ---");
        testReadOperations(timestamp);
        
        printSummary();
        
        System.exit(testsFailed == 0 ? 0 : 1);
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
    
    /**
     * Test all write operations
     * @return The timestamp of the created cache, or null on failure
     */
    private static String testWriteOperations() {
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
                100000L
            );
            assertTest("Writer Initialize", result == 0, "Failed with code: " + result);
            
            if (result != 0) {
                String error = writer.getLastError();
                if (error != null && !error.isEmpty()) {
                    System.err.println("  Error: " + error);
                }
                return null;
            }
            
            // Test: Insert multiple data types
            int insertCount = 0;
            
            // Strings
            if (writer.insertKey("user:1001:name", "Alice Johnson", (byte)0) == 0) insertCount++;
            if (writer.insertKey("user:1002:name", "Bob Smith", (byte)0) == 0) insertCount++;
            if (writer.insertKey("user:1003:name", "Charlie Brown", (byte)0) == 0) insertCount++;
            if (writer.insertKey("test:empty", "", (byte)0) == 0) insertCount++;
            
            // Integers
            if (writer.insertKey("user:1001:age", "28", (byte)3) == 0) insertCount++;
            if (writer.insertKey("user:1002:age", "35", (byte)3) == 0) insertCount++;
            if (writer.insertKey("user:1003:age", "42", (byte)3) == 0) insertCount++;
            
            // Doubles
            if (writer.insertKey("user:1001:score", "95.5", (byte)1) == 0) insertCount++;
            if (writer.insertKey("user:1002:score", "87.3", (byte)1) == 0) insertCount++;
            if (writer.insertKey("user:1003:score", "92.8", (byte)1) == 0) insertCount++;
            if (writer.insertKey("test:pi", "3.14159", (byte)1) == 0) insertCount++;
            
            // Booleans
            if (writer.insertKey("user:1001:active", "true", (byte)2) == 0) insertCount++;
            if (writer.insertKey("user:1002:active", "true", (byte)2) == 0) insertCount++;
            if (writer.insertKey("user:1003:active", "false", (byte)2) == 0) insertCount++;
            
            // Long
            if (writer.insertKey("test:bignum", "9223372036854775807", (byte)3) == 0) insertCount++;
            
            // String Vectors
            byte[] vectorKey1 = new byte[] { 0x01, 't', 'e', 's', 't', ':', 'v', 'e', 'c', 't', 'o', 'r' };
            byte[] vectorValue1 = "apple|banana|cherry".getBytes(java.nio.charset.StandardCharsets.UTF_8);
            if (writer.insertKey(vectorKey1, vectorValue1, (byte)1) == 0) insertCount++;
            
            byte[] vectorKey2 = new byte[] { 0x01, 'u', 's', 'e', 'r', ':', 't', 'a', 'g', 's' };
            byte[] vectorValue2 = "java|python|cpp".getBytes(java.nio.charset.StandardCharsets.UTF_8);
            if (writer.insertKey(vectorKey2, vectorValue2, (byte)1) == 0) insertCount++;
            
            // Float Vectors
            if (writer.insertKey("test:floatvector", "1.5:2.7:3.9:4.2", (byte)7) == 0) insertCount++;
            if (writer.insertKey("test:embeddings", "0.1:0.2:0.3:0.4:0.5", (byte)7) == 0) insertCount++;
            
            assertTest("Insert Multiple Types", insertCount == 19, 
                      "Only " + insertCount + "/19 inserts succeeded");
            
            // Test: Duplicate values
            writer.addDuplicateValue("duplicate_value", (byte)0);
            writer.addDuplicateValue("another_dup", (byte)0);
            result = writer.finishAddDuplicateValues();
            assertTest("Duplicate Values", result == 0, "Failed with code: " + result);
            
            // Test: Finalize
            result = writer.finishCacheCreation();
            assertTest("Finish Cache Creation", result == 0, "Failed with code: " + result);
            
            if (result != 0) {
                return null;
            }
            
            // Rename cache file with timestamp
            String cacheTimestamp = String.valueOf(System.currentTimeMillis());
            File cacheDir = new File("./cache_output");
            File oldFile = new File(cacheDir, "example_cache.cache");
            File newFile = new File(cacheDir, "example_cache." + cacheTimestamp + ".cache");
            
            if (oldFile.exists()) {
                if (oldFile.renameTo(newFile)) {
                    System.out.println("  Cache file: " + newFile.getName());
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
    private static void testReadOperations(String timestamp) {
        try (CacheReader reader = new CacheReader()) {
            // Test: Initialize
            int result = reader.initialize("example_cache", "./cache_output", timestamp, true);
            assertTest("Reader Initialize", result == 0, "Failed with code: " + result);
            
            if (result != 0) {
                fail("Cannot proceed with read tests");
                return;
            }
            
            // Test: ContainsKey
            boolean exists = reader.containsKey("user:1001:name");
            boolean notExists = reader.containsKey("nonexistent");
            assertTest("ContainsKey", exists && !notExists, 
                      "exists=" + exists + ", !exists=" + notExists);
            
            // Test: GetString
            String name = reader.getString("user:1001:name");
            assertTest("GetString", 
                      name != null && name.equals("Alice Johnson"),
                      "Expected 'Alice Johnson', got '" + name + "'");
            
            // Test: GetString (empty)
            String empty = reader.getString("test:empty");
            assertTest("GetString (empty)", 
                      empty != null && empty.equals(""),
                      "Expected empty string, got '" + empty + "'");
            
            // Test: GetInteger
            Integer age = reader.getInteger("user:1001:age");
            assertTest("GetInteger",
                      age != null && age == 28,
                      "Expected 28, got " + age);
            
            // Test: GetLong
            Long bignum = reader.getLong("test:bignum");
            assertTest("GetLong", 
                      bignum != null && bignum == 9223372036854775807L,
                      "Expected max long, got " + bignum);
            
            // Test: GetDouble
            Double pi = reader.getDouble("test:pi");
            Double score = reader.getDouble("user:1001:score");
            boolean piOk = pi != null && Math.abs(pi - 3.14159) < 0.00001;
            boolean scoreOk = score != null && Math.abs(score - 95.5) < 0.01;
            assertTest("GetDouble", piOk && scoreOk,
                      "pi=" + pi + ", score=" + score);
            
            // Test: GetBoolean
            Boolean trueVal = reader.getBoolean("user:1001:active");
            Boolean falseVal = reader.getBoolean("user:1003:active");
            assertTest("GetBoolean",
                      trueVal != null && trueVal && falseVal != null && !falseVal,
                      "true=" + trueVal + ", false=" + falseVal);
            
            // Test: GetKeyType
            String type = reader.getKeyType("user:1001:name");
            assertTest("GetKeyType", type != null, "Returned null");
            
            // Test: GetVector (String Vector)
            String[] vector1 = reader.getVector("test:vector");
            boolean vectorOk = vector1 != null && vector1.length == 3;
            if (vectorOk) {
                // Vectors are sorted, so check sorted order
                java.util.Arrays.sort(new String[]{"apple", "banana", "cherry"});
                java.util.Arrays.sort(vector1);
                vectorOk = java.util.Arrays.equals(vector1, new String[]{"apple", "banana", "cherry"});
            }
            assertTest("GetVector (String Vector)", vectorOk,
                      "Expected [apple, banana, cherry], got " + 
                      (vector1 != null ? java.util.Arrays.toString(vector1) : "null"));
            
            String[] vector2 = reader.getVector("user:tags");
            boolean vector2Ok = vector2 != null && vector2.length == 3;
            if (vector2Ok) {
                java.util.Arrays.sort(vector2);
                String[] expected = {"cpp", "java", "python"};
                java.util.Arrays.sort(expected);
                vector2Ok = java.util.Arrays.equals(vector2, expected);
            }
            assertTest("GetVector (user tags)", vector2Ok,
                      "Expected [cpp, java, python], got " + 
                      (vector2 != null ? java.util.Arrays.toString(vector2) : "null"));
            
            // Test: GetVectorSize
            int vecSize1 = reader.getVectorSize("test:vector");
            assertTest("GetVectorSize", vecSize1 == 3, 
                      "Expected 3, got " + vecSize1);
            
            // Test: GetVectorItem (individual items from vector)
            String item0 = reader.getVectorItem("test:vector", 0);
            String item1 = reader.getVectorItem("test:vector", 1);
            String item2 = reader.getVectorItem("test:vector", 2);
            boolean itemsOk = item0 != null && item1 != null && item2 != null;
            assertTest("GetVectorItem", itemsOk,
                      "Got items: [" + item0 + ", " + item1 + ", " + item2 + "]");
            
            // Test: GetFloatVector
            float[] floatVec1 = reader.getFloatVector("test:floatvector");
            boolean floatVecOk = floatVec1 != null && floatVec1.length == 4;
            if (floatVecOk) {
                floatVecOk = Math.abs(floatVec1[0] - 1.5f) < 0.01f &&
                            Math.abs(floatVec1[1] - 2.7f) < 0.01f &&
                            Math.abs(floatVec1[2] - 3.9f) < 0.01f &&
                            Math.abs(floatVec1[3] - 4.2f) < 0.01f;
            }
            assertTest("GetFloatVector", floatVecOk,
                      "Expected [1.5, 2.7, 3.9, 4.2], got " + 
                      (floatVec1 != null ? java.util.Arrays.toString(floatVec1) : "null"));
            
            float[] floatVec2 = reader.getFloatVector("test:embeddings");
            boolean embedOk = floatVec2 != null && floatVec2.length == 5;
            if (embedOk) {
                embedOk = Math.abs(floatVec2[0] - 0.1f) < 0.01f &&
                         Math.abs(floatVec2[1] - 0.2f) < 0.01f &&
                         Math.abs(floatVec2[2] - 0.3f) < 0.01f &&
                         Math.abs(floatVec2[3] - 0.4f) < 0.01f &&
                         Math.abs(floatVec2[4] - 0.5f) < 0.01f;
            }
            assertTest("GetFloatVector (embeddings)", embedOk,
                      "Expected [0.1, 0.2, 0.3, 0.4, 0.5], got " + 
                      (floatVec2 != null ? java.util.Arrays.toString(floatVec2) : "null"));
            
            // Test: Non-existent keys return null
            String nonExist1 = reader.getString("nonexistent");
            Integer nonExist2 = reader.getInteger("nonexistent");
            Long nonExist3 = reader.getLong("nonexistent");
            Double nonExist4 = reader.getDouble("nonexistent");
            Boolean nonExist5 = reader.getBoolean("nonexistent");
            String[] nonExist6 = reader.getVector("nonexistent");
            float[] nonExist7 = reader.getFloatVector("nonexistent");
            
            assertTest("Non-existent Keys (null return)",
                      nonExist1 == null && nonExist2 == null && nonExist3 == null &&
                      nonExist4 == null && nonExist5 == null && nonExist6 == null && nonExist7 == null,
                      "Some methods didn't return null");
            
            // Additional verification: read all user data
            System.out.println("\n  --- Detailed Data Verification ---");
            String[] userIds = {"user:1001", "user:1002", "user:1003"};
            int dataChecks = 0;
            int dataSuccess = 0;
            
            for (String userId : userIds) {
                System.out.println("  " + userId + ":");
                
                String userName = reader.getString(userId + ":name");
                Integer userAge = reader.getInteger(userId + ":age");
                Double userScore = reader.getDouble(userId + ":score");
                Boolean userActive = reader.getBoolean(userId + ":active");
                
                if (userName != null) { dataSuccess++; System.out.println("    Name: " + userName); }
                if (userAge != null) { dataSuccess++; System.out.println("    Age: " + userAge); }
                if (userScore != null) { dataSuccess++; System.out.println("    Score: " + userScore); }
                if (userActive != null) { dataSuccess++; System.out.println("    Active: " + userActive); }
                
                dataChecks += 4;
            }
            
            System.out.println("  Data verification: " + dataSuccess + "/" + dataChecks + " fields retrieved");
            
        } catch (Exception e) {
            fail("Read exception: " + e.getMessage());
            e.printStackTrace();
        }
    }
    
    private static void assertTest(String testName, boolean condition, String failMessage) {
        if (condition) {
            pass(testName);
        } else {
            fail(testName + ": " + failMessage);
        }
    }
    
    private static void pass(String testName) {
        System.out.println("  [PASS] " + testName);
        testsPassed++;
    }
    
    private static void fail(String message) {
        System.out.println("  [FAIL] " + message);
        testsFailed++;
    }
    
    private static void printSummary() {
        System.out.println("\n" + "=".repeat(60));
        System.out.println("Test Results:");
        System.out.println("  Passed: " + testsPassed);
        System.out.println("  Failed: " + testsFailed);
        System.out.println("  Total:  " + (testsPassed + testsFailed));
        
        if (testsFailed == 0) {
            System.out.println("\nAll tests passed!");
        } else {
            System.out.println("\nSome tests failed!");
        }
    }
}
