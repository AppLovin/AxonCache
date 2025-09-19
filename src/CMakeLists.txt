# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may
# not detect new files automatically. Keep that in mind when changing files, or
# explicitly mention them here.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# ---- Create library ----

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and
# create an interface target: add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_LIB_NAME} ${headers} ${sources})
add_library(app::${PROJECT_LIB_NAME} ALIAS ${PROJECT_LIB_NAME})

# being a cross-platform target, we enforce standards conformance on MSVC
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES
    "GNU")
    target_compile_options(
      ${PROJECT_LIB_NAME}
      PRIVATE "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
      -fno-strict-aliasing -DNDEBUG -Wall -O3 -Wno-variadic-macros)
  else()
    target_compile_options(
      ${PROJECT_LIB_NAME}
      PRIVATE "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
  endif()
  target_compile_definitions(${PROJECT_LIB_NAME} PUBLIC NDEBUG)
else()
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES
    "GNU")

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 13.0)
        set(DISABLE_DEPRECATED_COPY -Wno-deprecated-copy)
      endif()
    endif()

    target_compile_options(
      ${PROJECT_LIB_NAME}
      PRIVATE "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
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
      -Wvla)
  else()
    target_compile_options(
      ${PROJECT_LIB_NAME}
      PRIVATE "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
  endif()
  target_compile_definitions(${PROJECT_LIB_NAME} PUBLIC DEBUG)
endif()

# ------ Link dependencies --------
include(ProjectLibraries)

# setup a generated header directory for version.h
set(GENERATED_HEADER_DIR ${PROJECT_BINARY_DIR}/include)

target_include_directories(${PROJECT_LIB_NAME}
  PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${GENERATED_HEADER_DIR}>
  $<INSTALL_INTERFACE:include>)

# ---- Create an installable target ----
# this allows users to run cmake --install <build-dir>
# TODO install headers properly

# installing library and creating export set
install(TARGETS ${PROJECT_LIB_NAME}
  EXPORT ${PROJECT_LIB_NAME}-targets
)

# installing export set
# not required for our use case, but good for debugging
install(EXPORT ${PROJECT_LIB_NAME}-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_LIB_NAME}
  NAMESPACE app::
)
