# SPDX-License-Identifier: MIT
# Copyright (c) 2025 AppLovin. All rights reserved.

cmake_minimum_required(VERSION 3.16)

# Find Java and JNI
find_package(Java REQUIRED)
find_package(JNI REQUIRED)
include(UseJava)

# Add utf8cpp dependency for proper UTF-8/UTF-16 conversion
include(CPM)
CPMAddPackage(
    NAME utf8cpp
    GITHUB_REPOSITORY nemtrif/utfcpp
    VERSION 3.2.5
    OPTIONS
        "UTF8_TESTS OFF"
        "UTF8_SAMPLES OFF"
)

if(NOT JNI_FOUND)
    message(FATAL_ERROR "JNI not found. Please install JDK.")
endif()

message(STATUS "Java version: ${Java_VERSION}")
message(STATUS "Java compiler: ${Java_JAVAC_EXECUTABLE}")
message(STATUS "Java archive tool: ${Java_JAR_EXECUTABLE}")
message(STATUS "JNI include dirs: ${JNI_INCLUDE_DIRS}")

# Build JAR with native header generation
add_jar(axoncache_java
    VERSION 2.5.0
    SOURCES
        src/main/java/com/applovin/axoncache/CacheReader.java
        src/main/java/com/applovin/axoncache/CacheWriter.java
        src/main/java/com/applovin/axoncache/NativeLibraryLoader.java
    GENERATE_NATIVE_HEADERS axoncache_java-native
    OUTPUT_NAME axoncache
)

# Build JNI shared library
add_library(axoncache_jni SHARED
    jni/CacheReaderJNI.cpp
    jni/CacheWriterJNI.cpp
)

target_include_directories(axoncache_jni
    PRIVATE
        ${JNI_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/jni
)

target_link_libraries(axoncache_jni
    PRIVATE
        axoncache
        axoncache_java-native
        utf8cpp
)

set_target_properties(axoncache_jni PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
    UNITY_BUILD OFF  # Disable unity build due to JNI helper functions
)

# Detect platform and copy native library to resources directory for Maven
if(APPLE)
    set(PLATFORM_DIR "osx-aarch64")
    set(LIB_SUFFIX "dylib")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(PLATFORM_DIR "linux-aarch64")
    set(LIB_SUFFIX "so")
else()
    set(PLATFORM_DIR "linux-x86_64")
    set(LIB_SUFFIX "so")
endif()

add_custom_command(TARGET axoncache_jni POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:axoncache>
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/natives/${PLATFORM_DIR}/libaxoncache.${LIB_SUFFIX}
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:axoncache_jni>
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main/resources/natives/${PLATFORM_DIR}/libaxoncache_jni.${LIB_SUFFIX}
    COMMENT "Copying core and JNI native libraries to Maven resources directory"
)

# Optional: Build examples
option(BUILD_JAVA_EXAMPLES "Build Java examples" ON)

if(BUILD_JAVA_EXAMPLES)
    add_jar(CacheExample
        SOURCES examples/CacheExample.java
        INCLUDE_JARS axoncache_java
        OUTPUT_NAME CacheExample
    )
    
    add_custom_target(java_examples ALL
        DEPENDS CacheExample
    )
endif()

# Make axoncache_jar depend on JAR, JNI library, and optionally examples
if(BUILD_JAVA_EXAMPLES)
    add_custom_target(axoncache_jar ALL
        DEPENDS axoncache_java axoncache_jni java_examples
    )
else()
    add_custom_target(axoncache_jar ALL
        DEPENDS axoncache_java axoncache_jni
    )
endif()

# Install targets
install_jar(axoncache_java lib)

install(TARGETS axoncache_jni
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
