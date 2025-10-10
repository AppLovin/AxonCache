# options
option(AL_WITH_DOCS "Generate docs" OFF)
option(AL_WITH_MAIN "Enable main executable" ON)
option(AL_WITH_TESTS "Enable unit tests" ON)
option(AL_WITH_PERF_TESTS "Enable benchmark tests" OFF)
option(AL_WITH_JAVA "Enable Java JNI bindings" OFF)
option(USE_CCACHE_BY_DEFAULT "Enable ccache" ON)
option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

include(CMakeDependentOption)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
cmake_dependent_option(BUILD_STATIC_LIBS "Build static libraries" ON "NOT BUILD_SHARED_LIBS" OFF)

# prioritize static libs if building statically
if(BUILD_STATIC_LIBS)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

  # apple doesn't support fully static binaries
  # all system libraries will still be dynamic
  if(NOT APPLE)
    SET(CMAKE_EXE_LINKER_FLAGS "-static")
  endif()
endif()

# always export compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# enable ccache by default if available
if(USE_CCACHE_BY_DEFAULT)
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    set(USE_CCACHE ON CACHE BOOL "" FORCE)
  endif(CCACHE_FOUND)
endif()

# Set a default for unity build if none was specified
set(default_unity_build ON)

if(NOT DEFINED CMAKE_UNITY_BUILD)
  message(STATUS "Setting unity build to '${default_unity_build}' as none was specified.")
  set(CMAKE_UNITY_BUILD "${default_unity_build}" CACHE
    STRING "Choose the unity build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_UNITY_BUILD PROPERTY STRINGS
    ON OFF)
endif()
