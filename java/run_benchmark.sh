#!/bin/bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 AppLovin. All rights reserved.

# Script to run the Java example

set -e  # Exit on error

# Get the script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Check if build exists
if [ ! -f "$BUILD_DIR/java/axoncache.jar" ]; then
    echo "Error: JAR file not found. Please run build_and_run.sh first."
    exit 1
fi

if [ ! -f "$BUILD_DIR/java/CacheBenchmark.jar" ]; then
    echo "Error: Example JAR file not found. Please run build_and_run.sh first."
    exit 1
fi

# Set library path (JNI library is in build/java/, C++ library in build/src/)
if [[ "$OSTYPE" == "darwin"* ]]; then
    export DYLD_LIBRARY_PATH="$BUILD_DIR/java:$BUILD_DIR/src:$DYLD_LIBRARY_PATH"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    export LD_LIBRARY_PATH="$BUILD_DIR/java:$BUILD_DIR/src:$LD_LIBRARY_PATH"
fi

# Run the integrated example (write + read)
echo "Running integrated write/read test..."
cd "$SCRIPT_DIR/examples"
java -cp "$BUILD_DIR/java/CacheBenchmark.jar:$BUILD_DIR/java/axoncache.jar" \
     -Djava.library.path="$BUILD_DIR/java:$BUILD_DIR/src" \
     CacheBenchmark
