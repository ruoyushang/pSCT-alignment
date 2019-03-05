#!/bin/bash

CONFIGURATIONS=""

# get command line options
usage="./buildServer.sh -c<CONFIG> -s<SHARED_OBJECT> -t<TOOLCHAIN>"
usage1="CONFIGURATIONS : The configuration to build (possible is \"Release\", \"Debug\", \"AMD64\", / default: \"Release\")"
usage2="BUILD_SHARED_LIBS : Set this option to ON to build the SDK in shared object mode (default: OFF)"
usage3="TOOLCHAIN : Set the full path to the CMake toolchain file to cross-compile (e.g.: /home/user/work/toolchain.cmake)"
usage4="JOBS : Specifies the number of jobs (commands) to run simultaneously (e.g.: ./buildExamples.sh -j 3)."
while getopts "c:s:t:j:h" options; do
    case $options in
        c ) export CONFIGURATIONS=$OPTARG
            echo "Setting up the config to build CONFIGURATIONS=$CONFIGURATIONS";;
        s ) export BUILD_SHARED_LIBS=$OPTARG
            echo "Setting up the config to build BUILD_SHARED_LIBS=$BUILD_SHARED_LIBS";;
        t ) export TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=$OPTARG"
            echo "Setting up toolchain";;
        j ) export JOBS="-j$OPTARG"
            echo "Setting up jobs for parallel build"
            echo "Building with $JOBS parallel threads.";;
        h ) echo $usage
            echo $usage1
            echo $usage2
            echo $usage3
            exit 1;;
        \? ) echo $usage
             exit 1;;
        * ) echo $usage
            echo "Invalid Parameter $options"
            exit 1;;
    esac
done

UASDKDIR=`pwd`
# install target path
CMAKE_INSTALL_PREFIX=$UASDKDIR

# Configurations
if [ "$CONFIGURATIONS" == "" ]; then
    CONFIGURATIONS="Release"
    echo "No config was set - setting up the config to default build CONFIGURATIONS=$CONFIGURATIONS"
fi
MODULES="server"

# shared objects?
if [ "$BUILD_SHARED_LIBS" == "" ]; then
    BUILD_SHARED_LIBS=OFF
    echo "Setting up default to static libraries BUILD_SHARED_LIBS=$BUILD_SHARED_LIBS"
fi

# parallel build?
if [ "$JOBS" == "" ]; then
    JOBS="-j1"
    echo "Setting up default default build ('make -j1')."
fi

# build configuration options
OPTION=""

# toolchain file option
if [ "$TOOLCHAIN" == "" ]; then
    TOOLCHAIN=""
fi

# check if the system is 32 or 64 Bit
SYSTEM=`getconf LONG_BIT`
if [ "$TOOLCHAIN" == "" ]; then
    if [ "$SYSTEM" == 32  ]; then
        # configure compiler for generic i686
        export CC=gcc
        export CXX=g++
        export CFLAGS='-march=native -fPIC -fno-strict-aliasing -g'
        export CXXFLAGS=$CFLAGS
        export CC CXX CFLAGS CXXFLAGS
    else
        # configure compiler for x64
        export CC=gcc
        export CXX=g++
        export CFLAGS='-march=native -mtune=generic -fPIC -fno-strict-aliasing -g -Og'
        export CXXFLAGS=$CFLAGS
        export CC CXX CFLAGS CXXFLAGS
    fi
fi

rm -rf ./buildServerRelease ./buildServerReleaseSIMMODE

export UASDKDIR
# build all configurations
for config in $CONFIGURATIONS; do
    if [[ $config = "AMD64" ]]; then
        config="Release"
        suffix="AMD64"
        OPTION=${OPTION}" -DAMD64:bool=true"
    fi
    if [[ $config = "SIMMODE" ]]; then #Ruo
        config="Release"
        suffix="SIMMODE"
        OPTION=${OPTION}" -DAMD64:bool=true"
        OPTION=${OPTION}" -DSIMMODE:bool=true"
    fi
    # build all modules
    for module in $MODULES; do
        # create build directory
        mkdir -p $UASDKDIR/buildServer$config$suffix/$module || { echo "mkdir failed."; exit 1; }
        # enter build directory
        cd $UASDKDIR/buildServer$config$suffix/$module || { echo "cd failed."; exit 1; }
        # create the Makefile using CMake
        cmake -DCMAKE_BUILD_TYPE=$config -DBUILD_SHARED_LIBS=$BUILD_SHARED_LIBS -DCMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX ${OPTION} $UASDKDIR/../$module || { echo "cmake failed."; exit 1; }
        # build
        make "$JOBS" || { echo "make failed."; exit 1; }
        # install
        make install || { echo "make install failed."; exit 1; }
    done
done


# VIM Configuration
# vim: set enc=utf-8:
# vim: set fenc=utf-8:
# vim: set ff=unix:
# vim: set tabstop=4:
# vim: set shiftwidth=4:
# vim: expandtab:
