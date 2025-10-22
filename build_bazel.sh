#!/bin/bash
set -e

usage="$(basename "$0") [-htFvb:p:] -- build AxonCache with Bazel

Options:
-h  show this help message
-t  run tests after building
-F  fresh build (clean cache first)
-v  verbose output
-b  build type: Debug (default) or Release
-p  number of parallel jobs (default: auto-detected)
"

# Default values
BUILD_TYPE="Debug"
RUN_TESTS=false
FRESH_BUILD=false
VERBOSE=false
PROC_COUNT=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Parse command line options
while getopts "htFvb:p:" opt; do
    case $opt in
        h)
            echo "$usage"
            exit 0
            ;;
        t)
            RUN_TESTS=true
            ;;
        F)
            FRESH_BUILD=true
            ;;
        v)
            VERBOSE=true
            ;;
        b)
            BUILD_TYPE="$OPTARG"
            if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "Release" ]]; then
                echo "Error: Build type must be Debug or Release"
                exit 1
            fi
            ;;
        p)
            PROC_COUNT="$OPTARG"
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            echo "$usage" >&2
            exit 1
            ;;
    esac
done

# Check for Bazel/Bazelisk
BAZEL_BIN="bazel"
if command -v bazelisk >/dev/null 2>&1; then
    BAZEL_BIN="bazelisk"
    echo "Using bazelisk for automatic version management"
elif command -v bazel >/dev/null 2>&1; then
    echo "Using bazel"
else
    echo "Error: Neither bazel nor bazelisk found. Install with: brew install bazelisk"
    exit 1
fi

# Set Bazel config based on build type
CONFIG="local"
if [[ "$BUILD_TYPE" == "Release" ]]; then
    CONFIG="release"
fi

# Build arguments
BAZEL_ARGS="--config=$CONFIG --jobs=$PROC_COUNT"
if [[ "$VERBOSE" == true ]]; then
    BAZEL_ARGS="$BAZEL_ARGS --verbose_failures"
fi

echo "Building AxonCache with Bazel ($BUILD_TYPE mode)..."

# Clean if requested
if [[ "$FRESH_BUILD" == true ]]; then
    echo "Cleaning Bazel cache..."
    $BAZEL_BIN clean --expunge
fi

# Build the library
echo "Building //:axoncache..."
$BAZEL_BIN build $BAZEL_ARGS //:axoncache

# Run tests if requested
if [[ "$RUN_TESTS" == true ]]; then
    echo "Running tests..."
    $BAZEL_BIN test $BAZEL_ARGS //...
fi

echo "Build completed successfully!"
echo "Build artifacts are in bazel-bin/ directory"