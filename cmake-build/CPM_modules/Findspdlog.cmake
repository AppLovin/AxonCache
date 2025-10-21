include("/Users/erolkeskiner/Playground/bidderserver-bazelification/AxonCache/cmake-build/cmake/CPM_0.36.0.cmake")
CPMAddPackage("NAME;spdlog;OPTIONS;SPDLOG_BUILD_EXAMPLE OFF;SPDLOG_BUILD_BENCH OFF;SPDLOG_BUILD_TESTS OFF;SPDLOG_INSTALL YES;URL;https://github.com/gabime/spdlog/archive/refs/tags/v1.13.0.tar.gz")
set(spdlog_FOUND TRUE)