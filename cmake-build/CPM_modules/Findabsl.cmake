include("/Users/erolkeskiner/Playground/bidderserver-bazelification/AxonCache/cmake-build/cmake/CPM_0.36.0.cmake")
CPMAddPackage("NAME;absl;GITHUB_REPOSITORY;abseil/abseil-cpp;GIT_TAG;20230125.1;OPTIONS;ABSL_USE_SYSTEM_INCLUDES ON;CMAKE_UNITY_BUILD OFF")
set(absl_FOUND TRUE)