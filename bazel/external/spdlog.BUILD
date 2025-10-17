load("@rules_cc//cc:defs.bzl", "cc_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "spdlog",
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = ["include"],
    copts = [
        "-std=c++20",
        "-DSPDLOG_COMPILED_LIB",
        "-w",
        "-Wno-error",
    ],
    linkopts = [
        "-lpthread",
    ],
    visibility = ["//visibility:public"],
)
