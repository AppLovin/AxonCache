"""External dependencies for AxonCache via WORKSPACE compatibility mode.

These dependencies are NOT available in BCR or need custom build configurations:
- cpp_commons: Our Bazel-migrated branch (feat/bazel-build) 
- spdlog: Custom build config for compiled lib mode (matches Cpp-Commons)
- xxhash: Simple header-only library (bundled with AxonCache)
- cxxopts: CLI parsing library for main binary
- doctest: Testing framework (matches existing CMake setup)
- absl: Google Abseil (used by existing CMake build)

Dependencies handled via MODULE.bazel (BCR):
- rules_cc, googletest
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def axoncache_deps():
    """Load external dependencies for AxonCache via WORKSPACE compatibility mode."""
    
    # Cpp-Commons from our Bazel-migrated branch
    if not native.existing_rule("cpp_commons"):
        git_repository(
            name = "cpp_commons",
            remote = "https://github.com/AppLovin/Cpp-Commons.git",
            branch = "feat/bazel-build",  # Our Bazel-migrated branch
        )

    # === Cpp-Commons Dependencies (required by our migrated branch) ===
    
    # RapidJSON v1.1.0 (custom build for include paths)
    if not native.existing_rule("rapidjson"):
        http_archive(
            name = "rapidjson",
            build_file = "@//bazel/external:rapidjson.BUILD",
            sha256 = "bf7ced29704a1e696fbccf2a2b4ea068e7774fa37f6d7dd4039d0787f8bed98e",
            strip_prefix = "rapidjson-1.1.0",
            urls = ["https://github.com/Tencent/rapidjson/archive/refs/tags/v1.1.0.tar.gz"],
        )

        # JsonCpp 1.9.5 (using native Bazel build)
        if not native.existing_rule("jsoncpp"):
            git_repository(
                name = "jsoncpp",
                remote = "https://github.com/open-source-parsers/jsoncpp.git",
                tag = "1.9.5",
            )

    # libdeflate 1.21 (not in BCR)
    if not native.existing_rule("libdeflate"):
        http_archive(
            name = "libdeflate",
            build_file = "@//bazel/external:libdeflate.BUILD",
            sha256 = "50827d312c0413fbd41b0628590cd54d9ad7ebf88360cba7c0e70027942dbd01",
            strip_prefix = "libdeflate-1.21",
            urls = ["https://github.com/ebiggers/libdeflate/archive/refs/tags/v1.21.tar.gz"],
        )

    # zstd 1.5.6 (not in BCR)
    if not native.existing_rule("zstd"):
        http_archive(
            name = "zstd",
            build_file = "@//bazel/external:zstd.BUILD",
            sha256 = "30f35f71c1203369dc979ecde0400ffea93c27391bfd2ac5a9715d2173d92ff7",
            strip_prefix = "zstd-1.5.6",
            urls = ["https://github.com/facebook/zstd/archive/refs/tags/v1.5.6.tar.gz"],
        )

        # magic_enum v0.9.5 (using native Bazel build)
        if not native.existing_rule("magic_enum"):
            git_repository(
                name = "magic_enum",
                remote = "https://github.com/Neargye/magic_enum.git",
                tag = "v0.9.5",
            )

    # utf8cpp v4.0.6 (not in BCR)
    if not native.existing_rule("utf8cpp"):
        http_archive(
            name = "utf8cpp",
            build_file = "@//bazel/external:utf8cpp.BUILD",
            sha256 = "6920a6a5d6a04b9a89b2a89af7132f8acefd46e0c2a7b190350539e9213816c0",
            strip_prefix = "utfcpp-4.0.6",
            urls = ["https://github.com/nemtrif/utfcpp/archive/refs/tags/v4.0.6.tar.gz"],
        )

    # === AxonCache-Specific Dependencies ===

    # spdlog 1.13.0 (custom build for compiled lib mode - matches Cpp-Commons)
    if not native.existing_rule("spdlog"):
        http_archive(
            name = "spdlog",
            build_file = "@//bazel/external:spdlog.BUILD",
            sha256 = "534f2ee1a4dcbeb22249856edfb2be76a1cf4f708a20b0ac2ed090ee24cfdbc9",
            strip_prefix = "spdlog-1.13.0",
            urls = ["https://github.com/gabime/spdlog/archive/refs/tags/v1.13.0.tar.gz"],
        )

    # Note: xxhash is bundled with AxonCache source (src/axoncache/common/xxhash.c)

    # cxxopts 3.0.0 (CLI parsing for main binary) - using native Bazel support
    if not native.existing_rule("cxxopts"):
        git_repository(
            name = "cxxopts",
            remote = "https://github.com/jarro2783/cxxopts.git",
            tag = "v3.0.0",
        )


