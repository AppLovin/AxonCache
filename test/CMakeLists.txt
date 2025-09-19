# ---- Dependencies ----

CPMAddPackage("gh:doctest/doctest#v${doctest_version}")

cpmaddpackage(
  NAME spdlog
  OPTIONS
  "SPDLOG_BUILD_EXAMPLE OFF"
  "SPDLOG_BUILD_BENCH OFF"
  "SPDLOG_BUILD_TESTS OFF"
  "SPDLOG_INSTALL YES"
  URL https://github.com/gabime/spdlog/archive/refs/tags/v${spdlog_version}.tar.gz
)

# ---- Create binary ----

file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_TEST_NAME} ${sources})

# even when publicly linking our lib, it becomes private for some reason
# this requires us to list dl here, which is also a bit of a mystery
target_link_libraries(${PROJECT_TEST_NAME} spdlog doctest::doctest ${PROJECT_LIB_NAME} dl)
target_include_directories(${PROJECT_TEST_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 13.0)
        set(DISABLE_DEPRECATED_COPY -Wno-deprecated-copy)
      endif()
    endif()
    target_compile_options(${PROJECT_TEST_NAME} PUBLIC
      -g3
      -ggdb
      -O0
      -DDEBUG
      -fdiagnostics-color=always
      -fno-strict-aliasing
      -Wall
      -Warray-bounds
      -Wdate-time
      -Wdisabled-optimization
      -Werror
      -Wextra
      -Wformat=2
      -Wformat-security
      -Wimplicit-fallthrough
      -Winvalid-pch
      -Wmissing-declarations
      -Wno-deprecated-declarations
      -Wno-variadic-macros
      -Wno-variadic-macros
      ${DISABLE_DEPRECATED_COPY}
      -Wnull-dereference
      -Woverloaded-virtual
      -Wpacked
      -Wshadow
      -Wshift-overflow
      -Wunknown-pragmas
      -Wunused-parameter
      -Wvla
    )
  elseif(MSVC)
    target_compile_options(${PROJECT_TEST_NAME} PUBLIC /W4 /WX)
    target_compile_definitions(${PROJECT_TEST_NAME}Tests PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endif()

# ---- Add ${PROJECT_TEST_NAME}Tests ----

enable_testing()

# Note: doctest and similar testing frameworks can automatically configure CMake tests. For other
# testing frameworks add the tests target instead: add_test(NAME projectNameTests COMMAND
# ${PROJECT_TEST_NAME})

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(${PROJECT_TEST_NAME})

# TODO(dpayne): remove once we have something like coverity setup (or something within jenkins)
if(ENABLE_TEST_COVERAGE)
  include(CodeCoverage)
  set(COVERAGE_EXCLUDES
    '/usr/*' '${PROJECT_SOURCE_DIR}/test/*' '${PROJECT_SOURCE_DIR}/benchmark/*'
    '${PROJECT_SOURCE_DIR}/main/*' '${PROJECT_SOURCE_DIR}/build/*'
    '${PROJECT_SOURCE_DIR}/_deps/*'
  )
  setup_target_for_coverage(
    NAME coverage EXECUTABLE ${PROJECT_TEST_NAME} DEPENDENCIES ${PROJECT_TEST_NAME}
  )
endif()
