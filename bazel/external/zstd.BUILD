load("@rules_cc//cc:defs.bzl", "cc_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "zstd",
    srcs = glob([
        "lib/common/*.c",
        "lib/compress/*.c", 
        "lib/decompress/*.c",
        "lib/dictBuilder/*.c",
    ]) + select({
        "@platforms//os:macos": glob(["lib/decompress/*.S"]),
        "@platforms//os:linux": glob(["lib/decompress/*.S"]),
        "//conditions:default": [],
    }),
    hdrs = glob([
        "lib/*.h",
        "lib/**/*.h",
    ]),
    includes = [
        "lib",
        "lib/common",
    ],
    copts = [
        "-DZSTD_MULTITHREAD",
        "-w",  # Suppress all warnings for external dependency
        "-Wno-error",
        "-Wno-macro-redefined",
    ],
    linkopts = select({
        "@platforms//os:linux": ["-lpthread"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)
