load("@rules_cc//cc:defs.bzl", "cc_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utf8cpp",
    hdrs = glob([
        "source/*.h",
        "source/**/*.h",
    ]),
    includes = ["source"],
    copts = [
        "-std=c++20",
        "-w",  # Suppress warnings for external dependency
        "-Wno-error",
    ],
    visibility = ["//visibility:public"],
)
