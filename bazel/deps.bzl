"""External dependencies for AxonCache via WORKSPACE compatibility mode.

These dependencies are NOT available in BCR or need custom build configurations:
- cxxopts: CLI parsing library for main binary (not in BCR)

Dependencies handled via MODULE.bazel (BCR):
- rules_cc, platforms, doctest, abseil-cpp

Note: Core AxonCache library has no external dependencies (only bundled xxhash)
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def axoncache_deps():
    """Load minimal external dependencies for AxonCache via WORKSPACE compatibility mode."""
    
    # cxxopts 3.0.0 (CLI parsing for main binary) - using native Bazel support
    if not native.existing_rule("cxxopts"):
        git_repository(
            name = "cxxopts",
            remote = "https://github.com/jarro2783/cxxopts.git",
            tag = "v3.0.0",
        )


