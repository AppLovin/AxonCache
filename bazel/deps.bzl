"""External dependencies for AxonCache via WORKSPACE compatibility mode.

These dependencies are NOT available in BCR or need custom build configurations:
- spdlog: Custom build config for compiled lib mode (needed by CLI and tests)
- cxxopts: CLI parsing library for main binary (not in BCR)

Dependencies handled via MODULE.bazel (BCR):
- rules_cc, platforms, doctest, abseil-cpp

Note: Core AxonCache library has no external dependencies (only bundled xxhash)
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def axoncache_deps():
    """Load minimal external dependencies for AxonCache via WORKSPACE compatibility mode."""
    
    # spdlog 1.13.0 (custom build for compiled lib mode - needed by CLI and tests)
    if not native.existing_rule("spdlog"):
        http_archive(
            name = "spdlog",
            build_file = "@//bazel/external:spdlog.BUILD",
            sha256 = "534f2ee1a4dcbeb22249856edfb2be76a1cf4f708a20b0ac2ed090ee24cfdbc9",
            strip_prefix = "spdlog-1.13.0",
            urls = ["https://github.com/gabime/spdlog/archive/refs/tags/v1.13.0.tar.gz"],
        )

    # cxxopts 3.0.0 (CLI parsing for main binary) - using native Bazel support
    if not native.existing_rule("cxxopts"):
        git_repository(
            name = "cxxopts",
            remote = "https://github.com/jarro2783/cxxopts.git",
            tag = "v3.0.0",
        )


