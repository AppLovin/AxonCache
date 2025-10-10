#!/bin/bash
set -e

usage="$(basename "$0") [-hvcdtwe] [-p -b -s -g] -- build project
where:
-p  number of threads to build with (default is number of available cores)
-b  Build type, options are Release and Debug (defaults to Debug)
-s  Build with specified sanitizer (e.g. address or thread)
-F  Fresh build (clear cmake cache)
-c  Clean build (rm -rf build)
-w  Run clang tidy on whole project
-d  Run clang tidy on diff from master to current HEAD
-g  Build deb using build number
-v  verbose build
-t  run tests
-e  run perf tests
-h  print this message
-j  generate compile commands
-f  run clang format
-z  run fuzz test
-x  Build test tools
-u  unity build
-U  unity build off
-l  link time optimization
-C  on Linux, fetch a recent version of clang
-P  run unittest serially
-n  build with ninja
-S  build with static libs
-i  run tests and then switch branch and run backward compatibility tests
-J  build with Java JNI bindings
"

# Setup default values
SOURCE_DIR=$PWD
BUILD_DIR=${SOURCE_DIR}/build
PROJECT_NAME=alcommons
PROJECT_VERSION=1.0
BUILD_TYPE=Debug
RUN_TESTS=OFF
RUN_PARALLEL_TESTS=ON
WITH_OLD_GCC=OFF
RUN_PERF_TESTS=OFF
RUN_FUZZ_TESTS=OFF
BUILD_TOOLS=OFF
BUILD_NUMBER=0
PROC_COUNT=8
CLANG_FORMAT=0
UNITY_BUILD=OFF
CTEST_PARALLEL_LEVEL=16
USE_NINJA=ON
BUILD_SHARED_LIBS=ON
GENERATE_TESTS=OFF
BACKWARD_COMPATIBILITY_TESTS=OFF
PROD_GIT_REF=master
VERBOSE_FLAGS=
FRESH_FLAG=
BUILD_JAVA=OFF

case `uname -s` in
    Linux)
        PROC_COUNT=`grep -c ^processor /proc/cpuinfo`
    ;;
    Darwin)
        PROC_COUNT=`sysctl -n hw.ncpu`
    ;;
esac

#############################################################################################################
# On macOS, check that some basic packages are installed, as the build will fail without them
# homebrew, cmake, openssl (needed for core libraries)
# bison, mcrypt, boost (for servers)
#############################################################################################################
test `uname` = Darwin && {
    CELLAR=
    case `uname -m` in
        x86_64)
            CELLAR=/usr/local/Cellar
        ;;
        arm64)
            CELLAR=/opt/homebrew/Cellar
        ;;
    esac

    type brew || {
        echo "Homebrew is not installed on your machine."
        echo "Install it by following the instructions at https://brew.sh/"
        exit 1
    }

    for package in cmake 
    do
        test -d $CELLAR/$package || {
            echo "$package is not installed on your machine."
            echo "Install it with:"
            echo "    brew install $package"
            exit 1
        }
    done

    test ! -d $CELLAR/openssl@1.1 -a ! -d $CELLAR/openssl@3 && {
        echo "openssl is not installed on your machine."
        echo "Install it with:"
        echo "    brew install openssl@1.1"
        exit 1
    }
}

#############################################################################################################
# Build project
#############################################################################################################
build_project ()
{
    BUILD_OPTIONS="-DCMAKE_INSTALL_MESSAGE=LAZY -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF"

    BUILD_OPTIONS="${BUILD_OPTIONS} -DAL_WITH_TESTS=${RUN_TESTS}"
    BUILD_OPTIONS="${BUILD_OPTIONS} -DAL_WITH_PERF_TESTS=${RUN_PERF_TESTS}"
    BUILD_OPTIONS="${BUILD_OPTIONS} -DAL_WITH_MAIN=${BUILD_TOOLS}"
    BUILD_OPTIONS="${BUILD_OPTIONS} -DAL_WITH_JAVA=${BUILD_JAVA}"
    BUILD_OPTIONS="${BUILD_OPTIONS} -DCMAKE_UNITY_BUILD=${UNITY_BUILD}"
    BUILD_OPTIONS="${BUILD_OPTIONS} -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=${LTO_BUILD}"
    BUILD_OPTIONS="${BUILD_OPTIONS} -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}"
    BUILD_OPTIONS="${BUILD_OPTIONS} -DBUILD_FLAGS=${BUILD_FLAGS}" # env var
    BUILD_OPTIONS="${BUILD_OPTIONS} ${VERBOSE_FLAGS}"
    BUILD_OPTIONS="${BUILD_OPTIONS} ${FRESH_FLAG}"

    CMAKE_GENERATOR="Unix Makefiles"
    test "$USE_NINJA" = ON && {
        CMAKE_GENERATOR="Ninja"
    }

    # Configure
    cmake -Bbuild ${SANITIZER} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -G"${CMAKE_GENERATOR}" ${BUILD_OPTIONS}

    # Build
    cmake --build build -j${PROC_COUNT}
    test $RUN_TESTS = ON && {
        env CTEST_PARALLEL_LEVEL=$CTEST_PARALLEL_LEVEL cmake --build build -j${PROC_COUNT} --target test
    }

    return 0
}

#############################################################################################################
# Run clang tidy
#############################################################################################################
run_clang_tidy ()
{
    # Go back up to root directory so that we use the .clang-tidy file in the root directory
    cd ${SOURCE_DIR}
    if [ -z $CLANG_TIDY_BINARY ]; then CLANG_TIDY_BINARY=`which clang-tidy`; fi
    if [ -z $CLANG_TIDY_DIFF_BINARY ]; then CLANG_TIDY_BINARY="./build_resources/clang-tidy-diff.py"; fi

    if [ -z $CLANG_TIDY_DIFF_BASE ]; then CLANG_TIDY_DIFF_BASE="master"; fi
    if [ -z $CLANG_TIDY_DIFF_HEAD ]; then CLANG_TIDY_DIFF_HEAD="HEAD"; fi

    if [[ $CLANG_DIFF_TIDY == 1 ]]
    then
        # Assumes clang-tidy-diff.py is in the executable path, source: https://github.com/llvm-mirror/clang-tools-extra/blob/master/clang-tidy/tool/clang-tidy-diff.py
        git diff ${CLANG_TIDY_DIFF_BASE}...${CLANG_TIDY_DIFF_HEAD} | sed -E 's/ (a|b)\// /g' | ${CLANG_TIDY_DIFF_BINARY} -path ${SOURCE_DIR} -clang-tidy-binary=${CLANG_TIDY_BINARY}
    else
        # Assumes run_clang_tidy.py is in the executable path, source: https://github.com/llvm-mirror/clang-tools-extra/blob/master/clang-tidy/tool/run-clang-tidy.py
        ./build_resources/run_clang_tidy.py -p ${SOURCE_DIR} -header-filter='.*'
    fi

    return 0
}

#############################################################################################################
# Run clang format
#############################################################################################################
run_clang_format ()
{
    # find benchmark src include test -name '*.cpp' -exec clang-format -i {} \;
    # find benchmark src include test -name '*.h' -exec clang-format -i {} \;

    clang-format --version
    # find all .cpp/.h files and check formatting
    files=$(find . -name '*.cpp' -o -name '*.h')
    for f in $files; do
        echo "Formatting $f"
        diff -u "$f" <(clang-format "$f") || (
          echo "::error file=$f::needs reformatting"
          exit 1
        )
    done

    return 0
}

while getopts "::h ::v ::t ::e ::c ::w ::d ::f ::z ::j :g: :s: :b: :p: ::x ::u ::U ::l ::C ::P ::n ::S ::F ::i ::J" opt; do
  case $opt in
    p)
      PROC_COUNT="$OPTARG"
      ;;
    b)
      BUILD_TYPE=$OPTARG
      if [[ "${BUILD_TYPE}" != "Release" && "${BUILD_TYPE}" != "Debug" ]]; then
          echo "Invalid option for -b, valid ones are Release and Debug"
          exit
      fi
      ;;
    s)
      SANITIZER="-DAL_SANITIZER=$OPTARG"
      ;;
    v)
      VERBOSE_FLAGS="--trace-expand -DCMAKE_VERBOSE_MAKEFILE=ON"
      ;;
    t)
      RUN_TESTS=ON
      ;;
    P)
      CTEST_PARALLEL_LEVEL=1
      RUN_TESTS=ON
      ;;
    m)
      BUILD_MODEL_SYNC=ON
      ;;
    e)
      # Perf tests should always be run as Release
      BUILD_TYPE=Release
      RUN_TESTS=OFF
      RUN_PERF_TESTS=ON
      ;;
    z)
      BUILD_TYPE=Debug
      RUN_PERF_TESTS=OFF
      RUN_TESTS=OFF
      RUN_FUZZ_TESTS=ON
      export CC=clang
      export CXX=clang++
      ;;
    x)
      BUILD_TOOLS=ON
      ;;
    c)
      rm -rf ${BUILD_DIR}
      ;;
    F)
      FRESH_FLAG="--fresh"
      ;;
    f)
      CLANG_FORMAT=1
      ;;
    h)
      echo "$usage" >&2
      exit
      ;;
    g)
      BUILD_NUMBER=$OPTARG
      ;;
    u)
      UNITY_BUILD=ON
      ;;
    U)
      UNITY_BUILD=OFF
      ;;
    C)
      test `uname` = Linux && fetch_clang
      ;;
    n)
      USE_NINJA=ON
      ;;
    S)
      BUILD_SHARED_LIBS=OFF
      ;;
    i)
      RUN_TESTS=ON
      BACKWARD_COMPATIBILITY_TESTS=ON
      ;;
    J)
      BUILD_JAVA=ON
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit 
      ;;
  esac
done

build_project

if [[ $CLANG_FORMAT == 1 ]]
then
    run_clang_format
fi


