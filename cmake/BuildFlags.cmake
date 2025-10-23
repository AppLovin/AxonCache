if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  set(X86_FLAGS
    "-march=native"
  )
endif()

if(NOT DEFINED BUILD_FLAGS OR BUILD_FLAGS STREQUAL "")
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # https://gcc.gnu.org/onlinedocs/gcc-11.3.0/gcc/Optimize-Options.html We explicitely use -O2
    # instead of -O3
    set(BUILD_FLAGS "${X86_FLAGS} -O2")
  else()
    set(BUILD_FLAGS "${X86_FLAGS} -ggdb -g3")
  endif()
endif()

if(PROJECT_IS_TOP_LEVEL AND ENABLE_TEST_COVERAGE)
  set(BUILD_FLAGS "${BUILD_FLAGS} -O0 -g -fprofile-arcs -ftest-coverage")
endif()

if(AL_SANITIZER MATCHES "address")
  set(BUILD_FLAGS "${BUILD_FLAGS} -fsanitize=address")
endif()

if(AL_SANITIZER MATCHES "thread")
  set(BUILD_FLAGS "${BUILD_FLAGS} -fsanitize=thread")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${BUILD_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${BUILD_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${BUILD_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${BUILD_FLAGS}")

set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} ${BUILD_FLAGS}")
set(CMAKE_LD_FLAGS_DEBUG "${CMAKE_LD_FLAGS_DEBUG} ${BUILD_FLAGS}")
set(CMAKE_LD_FLAGS_RELEASE "${CMAKE_LD_FLAGS_RELEASE} ${BUILD_FLAGS}")
