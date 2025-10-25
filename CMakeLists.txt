cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(axoncache VERSION 2.5 LANGUAGES CXX)

# C++ defaults
if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# do not use append, as it will conflict with parent projects
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(versions.cmake)
include(RpathConfig)
include(Options)

# ---- Project ----
# using variables here is discouraged
# PROJECT_NAME must be copied to another variable for sub builds to work
set(PROJECT_LIB_NAME ${PROJECT_NAME})
set(PROJECT_MAIN_NAME ${PROJECT_NAME}_cli)
set(PROJECT_TEST_NAME ${PROJECT_NAME}Test)
set(PROJECT_DOCS_NAME ${PROJECT_NAME}Docs)

# ---- General Compile Time Flags ----
include(BuildFlags)

# ---- Add dependencies via CPM ----
include(CPM)

add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL)
  include(Tools)

  if(AL_WITH_MAIN)
    add_subdirectory(main)
  endif()

  if(AL_WITH_DOCS)
    add_subdirectory(docs)
  endif()

  if(AL_WITH_TESTS)
    # needed to generate test target
    enable_testing()
    add_subdirectory(test)
  endif()

  if(AL_WITH_PERF_TESTS)
    add_subdirectory(benchmark)
  endif()

  if(AL_WITH_JAVA)
    add_subdirectory(java)
  endif()

  if(AL_WITH_RUST)
    add_subdirectory(rust)
  endif()
endif()
