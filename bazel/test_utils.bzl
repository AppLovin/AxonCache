"""Test Utility Macros for AxonCache
    
This module provides automated test target generation for AxonCache.
It scans specified directories for test source files and creates individual
cc_test targets for each file, enabling parallel test execution and granular
failure reporting.

Key Features:
- Automatic test discovery from source directories
- Individual test targets for parallel execution
- Configurable exclusion of specific test files
- Consistent test configuration across all targets
- Integration with doctest framework

Usage:
    generate_test_suite(
        name = "all_tests",
        test_dirs = ["test/src"],
        exclude_files = ["test/src/main.cpp"],
    )
"""

load("@rules_cc//cc:defs.bzl", "cc_test")

def generate_test_suite(name, test_dirs, exclude_files = []):
    """Generates individual cc_test targets for all test files in specified directories.
    
    This function automatically discovers test source files and creates separate
    cc_test targets for each file. This approach provides several benefits:
    - Parallel test execution for faster CI builds
    - Granular failure reporting (know exactly which test file failed)
    - Individual test debugging capabilities
    - Automatic discovery of new test files

    Args:
        name: The name of the test suite that groups all generated tests
        test_dirs: List of directories to recursively search for test files
        exclude_files: List of specific test files to skip during generation
        
    Generated Targets:
        - Individual cc_test targets named after each test file
        - A test_suite target with the specified name containing all tests
    """

    # Common Test Configuration
    # Compiler options shared across all generated test targets
    # These settings ensure consistent behavior and compatibility
    common_copts = [
        "-std=c++20",                    # Use C++20 standard for modern features
        "-fno-strict-aliasing",          # Disable strict aliasing for performance code
        "-Wall",                         # Enable comprehensive warnings
        "-Wno-variadic-macros",          # Suppress variadic macro warnings (doctest uses these)
        "-Wno-deprecated-declarations",  # Suppress deprecated API warnings from dependencies
        "-DBAZEL_BUILD",                 # Define macro for Bazel-specific code paths
    ]
    
    # Common Dependencies
    # Dependencies shared across all generated test targets
    common_deps = [
        "//:axoncache",              # Main AxonCache library under test
        "@doctest//doctest:main",    # Doctest framework with main() function
    ]

    test_patterns = []
    for test_dir in test_dirs:
        test_patterns.extend([
            test_dir + "/**/*Tests.cpp",
            test_dir + "/**/*Test.cpp",
            test_dir + "/**/Test*.cpp",
        ])
    
    all_test_files = []
    for pattern in test_patterns:
        all_test_files.extend(native.glob([pattern], allow_empty = True))
    
    # Filter out excluded files and utility files
    exclude_patterns = ["TestUtils.cpp", "CacheTestUtils.cpp", "ResourceLoader.cpp", "main.cpp"] + exclude_files
    test_files = []
    for f in all_test_files:
        should_exclude = False
        for pattern in exclude_patterns:
            if pattern in f:
                should_exclude = True
                break
        if not should_exclude:
            test_files.append(f)
    
    test_targets = []
    
    for test_file in test_files:
        # Generate a clean target name
        target_name = test_file[:-4].replace("/", "_").replace("test_src_", "")

        # Determine extra dependencies
        extra_deps = []
        
        # Determine test data
        test_data = []
        if "FullCacheTest.cpp" in test_file or "CacheReaderCApiTest.cpp" in test_file:
            test_data.extend([
                "test/data/example_fast_data.dta",
                "test/data/bench_cache.1758220992251.cache",
            ])
        
        # Determine if test needs utility support
        test_srcs = [test_file]
        test_hdrs = []
        
        # Add ResourceLoader for all tests (it's commonly needed)
        test_srcs.append("test/src/ResourceLoader.cpp")
        test_hdrs.append("test/src/ResourceLoader.h")
        
        # Add CacheTestUtils for all tests (it's a common utility)
        test_hdrs.append("test/src/CacheTestUtils.h")

        # Create the test target
        native.cc_test(
            name = target_name,
            srcs = test_srcs + test_hdrs,
            includes = ["test/src", "test"],  # Add test directories to include path
            deps = common_deps + extra_deps,
            data = test_data,
            copts = common_copts,
            size = "small",  # Set default test size to avoid warnings
        )
        
        test_targets.append(":" + target_name)
    
    # Create a test suite that includes all tests
    native.test_suite(
        name = name,
        tests = test_targets,
    )
    
    return test_targets
