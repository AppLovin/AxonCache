load("@rules_cc//cc:defs.bzl", "cc_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "xxhash",
    hdrs = glob([
        "*.h",
        "xxhash.h",
    ]),
    srcs = glob([
        "xxhash.c",
    ]),
    includes = ["."],
    copts = [
        "-DXXH_NAMESPACE=AXONCACHE_",  # Namespace as used in AxonCache
        "-w",  # Suppress warnings for external dependency
        "-Wno-error",
    ],
    visibility = ["//visibility:public"],
)
