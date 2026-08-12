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

CPMAddPackage(
  NAME absl
  GITHUB_REPOSITORY abseil/abseil-cpp
  GIT_TAG 20240722.1
  OPTIONS
  "ABSL_USE_SYSTEM_INCLUDES ON"
  "CMAKE_UNITY_BUILD OFF"
)

# ---- Create standalone executable ----

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(${PROJECT_MAIN_NAME} ${sources})

set_target_properties(${PROJECT_MAIN_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_MAIN_NAME}")

target_link_libraries(${PROJECT_MAIN_NAME} ${PROJECT_LIB_NAME} cxxopts absl::flat_hash_map)
