# --- Import dependencies ----

cpmaddpackage(
  GITHUB_REPOSITORY
  jarro2783/cxxopts
  VERSION
  ${cxxopts_version}
  OPTIONS
  "CXXOPTS_BUILD_EXAMPLES NO"
  "CXXOPTS_BUILD_TESTS NO"
  "CXXOPTS_ENABLE_INSTALL YES")

cpmaddpackage(
  NAME spdlog
  OPTIONS
  "SPDLOG_BUILD_EXAMPLE OFF"
  "SPDLOG_BUILD_BENCH OFF"
  "SPDLOG_BUILD_TESTS OFF"
  "SPDLOG_INSTALL YES"
  URL https://github.com/gabime/spdlog/archive/refs/tags/v${spdlog_version}.tar.gz
)

CPMAddPackage(
  NAME absl
  GITHUB_REPOSITORY abseil/abseil-cpp
  GIT_TAG 20240722.1
  OPTIONS
  "ABSL_USE_SYSTEM_INCLUDES ON"

  # turn off warnings when Abseil itself compiles
  "CMAKE_C_FLAGS -w"
  "CMAKE_CXX_FLAGS -w"

  # make sure warnings aren't promoted to errors inside Abseil
  "CMAKE_COMPILE_WARNING_AS_ERROR OFF"

  # optional: speed up by not building tests
  "ABSL_BUILD_TESTING OFF"
)

# ---- Create standalone executable ----

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${PROJECT_MAIN_NAME} ${sources})

set_target_properties(${PROJECT_MAIN_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_MAIN_NAME}")

target_link_libraries(${PROJECT_MAIN_NAME} ${PROJECT_LIB_NAME} spdlog cxxopts absl::flat_hash_map)
