#!/bin/bash
set -e

usage="$(basename "$0") [-hvcdtwe] [-p -b -s -g] -- build AxonCache with Bazel (complete CMake parity)
where:
-p  number of threads to build with (default is number of available cores)
-b  Build type, options are Release and Debug (defaults to Debug)
-s  Build with specified sanitizer (e.g. address, thread, memory, undefined)
-F  Fresh build (clean cache)
-c  Clean build (bazel clean)
-w  Run clang tidy on whole project
-d  Run clang tidy on diff from master to current HEAD (placeholder)
-g  Build deb using build number (placeholder)
-v  verbose build
-t  run tests
-e  run perf tests (benchmarks)
-h  print this message
-j  generate compile commands (placeholder)
-f  run clang format
-z  run fuzz test (placeholder)
-x  Build test tools (main binary)
-u  unity build (no-op for Bazel - always optimized)
-U  unity build off (no-op for Bazel)
-l  link time optimization (no-op for Bazel - handled by --config=release)
-C  on Linux, fetch a recent version of clang (no-op)
-P  run unittest serially
-n  build with ninja (no-op for Bazel)
-S  build with static libs (no-op for Bazel - always static)
-T  enable testing but don't run
-o  generate documentation
-r  generate coverage report
"

# Setup default values
SOURCE_DIR=$PWD
PROJECT_NAME=axoncache
BUILD_TYPE=Debug
RUN_TESTS=OFF
RUN_PARALLEL_TESTS=ON
RUN_PERF_TESTS=OFF
RUN_FUZZ_TESTS=OFF
BUILD_TOOLS=OFF
BUILD_NUMBER=0
PROC_COUNT=8
CLANG_TIDY=0
CLANG_DIFF_TIDY=0
CLANG_FORMAT=0
UNITY_BUILD=ON
LTO_BUILD=OFF
CTEST_PARALLEL_LEVEL=16
USE_NINJA=ON
BUILD_SHARED_LIBS=ON
VERBOSE_FLAGS=
FRESH_FLAG=
ENABLE_TESTING=OFF
SANITIZER=""
GENERATE_DOCS=OFF
GENERATE_COVERAGE=OFF

case `uname -s` in
    Linux)
        PROC_COUNT=`grep -c ^processor /proc/cpuinfo`
    ;;
    Darwin)
        PROC_COUNT=`sysctl -n hw.ncpu`
    ;;
esac

#############################################################################################################
# Check Bazel-specific dependencies on macOS
#############################################################################################################
check_bazel_dependencies ()
{
    test `uname` = Darwin && {
        # Check for bazel or bazelisk
        if ! command -v bazelisk >/dev/null 2>&1 && ! command -v bazel >/dev/null 2>&1; then
            echo "Neither bazel nor bazelisk is installed on your machine."
            echo "Install bazelisk (recommended) with:"
            echo "    brew install bazelisk"
            echo "Or install bazel with:"
            echo "    brew install bazel"
            exit 1
        fi
        
        # Optional tools warnings
        if [ $GENERATE_DOCS = ON ] && ! command -v doxygen >/dev/null 2>&1; then
            echo "Warning: doxygen not found. Install with: brew install doxygen"
        fi
        
        if [ $GENERATE_COVERAGE = ON ] && ! command -v lcov >/dev/null 2>&1; then
            echo "Warning: lcov not found. Install with: brew install lcov"
        fi
        
        if [ $CLANG_TIDY = 1 ] && ! command -v clang-tidy >/dev/null 2>&1; then
            echo "Warning: clang-tidy not found. Install with: brew install llvm"
        fi
        
        if [ $CLANG_FORMAT = 1 ] && ! command -v clang-format >/dev/null 2>&1; then
            echo "Warning: clang-format not found. Install with: brew install llvm"
        fi
    }
}

#############################################################################################################
# Build project with Bazel
#############################################################################################################
build_project_bazel ()
{
    echo "Building AxonCache with Bazel..."
    
    # Determine Bazel binary (prefer bazelisk if available)
    BAZEL_BIN="bazel"
    if command -v bazelisk >/dev/null 2>&1; then
        BAZEL_BIN="bazelisk"
        echo "Using bazelisk for automatic version management"
    else
        echo "Using bazel (consider installing bazelisk for automatic version management)"
    fi
    
    # Determine config based on build type
    BAZEL_CONFIG="local"
    if [[ "${BUILD_TYPE}" == "Release" ]]; then
        BAZEL_CONFIG="release"
    fi
    
    # Add sanitizer config if specified
    if [[ -n "${SANITIZER}" ]]; then
        case "${SANITIZER}" in
            address)
                BAZEL_CONFIG="${BAZEL_CONFIG} --config=asan"
                ;;
            thread)
                BAZEL_CONFIG="${BAZEL_CONFIG} --config=tsan"
                ;;
            memory)
                BAZEL_CONFIG="${BAZEL_CONFIG} --config=msan"
                ;;
            undefined)
                BAZEL_CONFIG="${BAZEL_CONFIG} --config=ubsan"
                ;;
            *)
                echo "Unknown sanitizer: ${SANITIZER}"
                echo "Supported sanitizers: address, thread, memory, undefined"
                exit 1
                ;;
        esac
    fi
    
    # Build arguments
    BAZEL_ARGS="--config=${BAZEL_CONFIG}"
    
    # Add job count
    BAZEL_ARGS="${BAZEL_ARGS} --jobs=${PROC_COUNT}"
    
    # Add verbose flags if requested
    if [[ -n "${VERBOSE_FLAGS}" ]]; then
        BAZEL_ARGS="${BAZEL_ARGS} --verbose_failures --show_timestamps"
    fi
    
    # Clean build if requested
    if [[ "$FRESH_FLAG" == "--fresh" ]]; then
        echo "Cleaning Bazel cache..."
        $BAZEL_BIN clean --expunge
    fi
    
    # Build the main library
    echo "Building //:axoncache_lib..."
    $BAZEL_BIN build $BAZEL_ARGS //:axoncache_lib
    
    # Build CLI if requested
    if [[ $BUILD_TOOLS == "ON" ]]; then
        echo "Building CLI binary..."
        $BAZEL_BIN build $BAZEL_ARGS //:axoncache_cli
    fi
    
    # Build Python module
    echo "Building Python module..."
    $BAZEL_BIN build $BAZEL_ARGS //:axoncache
    
    # Run tests if requested
    if [[ $RUN_TESTS == "ON" || $ENABLE_TESTING == "ON" ]]; then
        echo "Running tests..."
        
        # Add coverage config if generating coverage
        TEST_ARGS="$BAZEL_ARGS"
        if [[ $GENERATE_COVERAGE == "ON" ]]; then
            TEST_ARGS="$TEST_ARGS --config=coverage"
        fi
        
        if [[ $RUN_PARALLEL_TESTS == "ON" && $CTEST_PARALLEL_LEVEL -gt 1 ]]; then
            $BAZEL_BIN test $TEST_ARGS --jobs=${CTEST_PARALLEL_LEVEL} //:all_tests //:axoncacheTest
        else
            $BAZEL_BIN test $TEST_ARGS --jobs=1 //:all_tests //:axoncacheTest
        fi
    fi
    
    # Run performance tests if requested
    if [[ $RUN_PERF_TESTS == "ON" ]]; then
        echo "Building benchmarks..."
        $BAZEL_BIN build $BAZEL_ARGS //:benchmarks
        echo "Benchmark binaries available in bazel-bin/"
    fi
    
    # Generate documentation if requested
    if [[ $GENERATE_DOCS == "ON" ]]; then
        echo "Generating documentation..."
        $BAZEL_BIN build $BAZEL_ARGS //:generate_docs
        echo "Documentation generated in bazel-bin/docs/html/"
    fi
    
    # Generate coverage report if requested
    if [[ $GENERATE_COVERAGE == "ON" ]]; then
        echo "Generating coverage report..."
        $BAZEL_BIN build $BAZEL_ARGS //:coverage_report
        echo "Coverage report generated in bazel-bin/coverage/"
    fi
    
    echo "Bazel build completed successfully!"
    echo "Build artifacts are in bazel-bin/ directory"
    
    return 0
}

#############################################################################################################
# Run clang tidy
#############################################################################################################
run_clang_tidy ()
{
    echo "Running clang-tidy analysis..."
    
    # Determine Bazel binary
    BAZEL_BIN="bazel"
    if command -v bazelisk >/dev/null 2>&1; then
        BAZEL_BIN="bazelisk"
    fi
    
    BAZEL_CONFIG="local"
    if [[ "${BUILD_TYPE}" == "Release" ]]; then
        BAZEL_CONFIG="release"
    fi
    
    $BAZEL_BIN build --config=${BAZEL_CONFIG} //:clang_tidy
    echo "Clang-tidy analysis complete. Report: bazel-bin/clang_tidy_report.txt"
    
    return 0
}

#############################################################################################################
# Run clang format
#############################################################################################################
run_clang_format ()
{
    echo "Running clang-format..."
    
    # Determine Bazel binary
    BAZEL_BIN="bazel"
    if command -v bazelisk >/dev/null 2>&1; then
        BAZEL_BIN="bazelisk"
    fi
    
    BAZEL_CONFIG="local"
    if [[ "${BUILD_TYPE}" == "Release" ]]; then
        BAZEL_CONFIG="release"
    fi
    
    # Check first, then apply if requested
    $BAZEL_BIN build --config=${BAZEL_CONFIG} //:clang_format_check
    echo "Format check complete. Report: bazel-bin/clang_format_report.txt"
    
    # Optionally apply formatting
    read -p "Apply formatting fixes? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        $BAZEL_BIN build --config=${BAZEL_CONFIG} //:clang_format_fix
        echo "Formatting applied successfully"
    fi
    
    return 0
}

while getopts "::h ::v ::t ::e ::c ::w ::d ::f ::z ::j ::x ::u ::U ::l ::C ::P ::n ::S ::F ::T ::o ::r :g: :s: :b: :p:" opt; do
  case $opt in
    p)
      PROC_COUNT="$OPTARG"
      ;;
    b)
      BUILD_TYPE=$OPTARG
      if [[ "${BUILD_TYPE}" != "Release" && "${BUILD_TYPE}" != "Debug" ]]; then
          echo "Invalid option for -b, valid ones are Release and Debug"
          exit
      fi
      ;;
    s)
      SANITIZER="$OPTARG"
      ;;
    v)
      VERBOSE_FLAGS="--verbose_failures --show_timestamps"
      ;;
    t)
      RUN_TESTS=ON
      ENABLE_TESTING=ON
      ;;
    P)
      CTEST_PARALLEL_LEVEL=1
      RUN_TESTS=ON
      RUN_PARALLEL_TESTS=OFF
      ;;
    e)
      # Perf tests should always be run as Release
      BUILD_TYPE=Release
      RUN_TESTS=OFF
      RUN_PERF_TESTS=ON
      ;;
    z)
      # Fuzz tests - placeholder for now
      echo "Fuzz testing not yet implemented for Bazel build"
      RUN_FUZZ_TESTS=ON
      ;;
    x)
      BUILD_TOOLS=ON
      ;;
    c)
      # Clean build
      rm -rf bazel-*
      ;;
    F)
      FRESH_FLAG="--fresh"
      ;;
    w)
      CLANG_TIDY=1
      ;;
    d)
      CLANG_TIDY=1
      CLANG_DIFF_TIDY=1
      echo "Clang-tidy diff mode not yet implemented - running full analysis"
      ;;
    f)
      CLANG_FORMAT=1
      ;;
    h)
      echo "$usage" >&2
      exit
      ;;
    g)
      BUILD_NUMBER=$OPTARG
      echo "DEB packaging not yet implemented for Bazel build"
      ;;
    u)
      # Unity build - no-op for Bazel (always optimized)
      UNITY_BUILD=ON
      ;;
    U)
      # Unity build off - no-op for Bazel
      UNITY_BUILD=OFF
      ;;
    l)
      # LTO - no-op for Bazel (handled by release config)
      LTO_BUILD=ON
      ;;
    C)
      # Fetch clang - no-op
      echo "Clang fetching not needed for Bazel build"
      ;;
    j)
      echo "Compile commands generation not yet implemented for Bazel build"
      ;;
    n)
      # Ninja - no-op for Bazel
      USE_NINJA=ON
      ;;
    S)
      # Static libs - no-op for Bazel (always static)
      BUILD_SHARED_LIBS=OFF
      ;;
    T)
      RUN_TESTS=OFF
      ENABLE_TESTING=ON
      ;;
    o)
      GENERATE_DOCS=ON
      ;;
    r)
      GENERATE_COVERAGE=ON
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit
      ;;
  esac
done

# Check dependencies
check_bazel_dependencies

# Choose build system - always Bazel for this script
build_project_bazel

if [[ $CLANG_TIDY == 1 ]]
then
    run_clang_tidy
fi

if [[ $CLANG_FORMAT == 1 ]]
then
    run_clang_format
fi
