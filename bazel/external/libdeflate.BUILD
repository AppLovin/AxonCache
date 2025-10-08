load("@rules_cc//cc:defs.bzl", "cc_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "libdeflate",
    srcs = glob([
        "lib/*.c",
        "lib/**/*.c",
    ]),
    hdrs = glob([
        "libdeflate.h",
        "lib/*.h",
        "lib/**/*.h",
        "common_defs.h",
        "common/*.h",
    ]),
    includes = [
        ".",
        "lib",
        "common",
    ],
    copts = [
        "-DLIBDEFLATE_STATIC",
        "-Iexternal/libdeflate",
        "-Iexternal/libdeflate/lib",
        "-Iexternal/libdeflate/common",
        "-w",  # Suppress all warnings for external dependency
        "-Wno-error",
        "-Wno-unused-parameter",
    ],
    visibility = ["//visibility:public"],
)
