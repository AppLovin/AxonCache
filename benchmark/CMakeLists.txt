set(PROJECT_BENCHMARK_NAME "${PROJECT_NAME}Benchmark")

# ---- Options ----

cpmaddpackage(
  NAME
  benchmark
  GITHUB_REPOSITORY
  google/benchmark
  VERSION
  ${benchmark_version}
  OPTIONS
  "BENCHMARK_ENABLE_TESTING Off")

# ---- Create binary ----

file(GLOB_RECURSE benchmark_sources CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_BENCHMARK_NAME} ${benchmark_sources})
target_link_libraries(${PROJECT_BENCHMARK_NAME} benchmark ${PROJECT_LIB_NAME})
target_include_directories(${PROJECT_BENCHMARK_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---- Add ${PROJECT_BENCHMARK_NAME} ----
