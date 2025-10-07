"""Test utility macros for AxonCache."""

load("@rules_cc//cc:defs.bzl", "cc_test")

def generate_test_suite(name, test_dirs, exclude_files = []):
    """Generates cc_test targets for all test files in specified directories.

    Args:
        name: The name of the test suite.
        test_dirs: A list of directories to search for test files.
        exclude_files: A list of test files to exclude from auto-generation.
    """

    # Common test configuration
    common_copts = [
        "-std=c++20",
        "-fno-strict-aliasing", 
        "-Wall",
        "-Wno-variadic-macros",
        "-Wno-deprecated-declarations",
        "-DBAZEL_BUILD",
    ]
    
    common_deps = [
        "//:axoncache_lib",  # Updated target name
        "@doctest//doctest:main",
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
