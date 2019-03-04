#!/bin/bash

CONFIGURATIONS=""

# get command line options
usage="./buildServClient.sh -c<CONFIG> -s<SHARED_OBJECT> -t<TOOLCHAIN>"
usage1="CONFIGURATIONS : The configuration to build (possible is \"Release\", \"Debug\", \"RelWithDebInfo\" and \"RelMinSize\" / default: \"Debug Release\")"
usage2="BUILD_SHARED_LIBS : Set this option to ON to build the SDK in shared object mode (default: OFF)"
usage3="TOOLCHAIN : Set the full path to the CMake toolchain file to cross-compile (e.g.: /home/user/work/toolchain.cmake)"
while getopts "c:s:t:h" options; do
    case $options in
        c ) export CONFIGURATIONS=$OPTARG
            echo "Setting up the config to build CONFIGURATIONS=$CONFIGURATIONS";;
        s ) export BUILD_SHARED_LIBS=$OPTARG
            echo "Setting up the config to build BUILD_SHARED_LIBS=$BUILD_SHARED_LIBS";;
        t ) export TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=$OPTARG"
            echo "Setting up toolchain";;
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
    CONFIGURATIONS="Debug Release"
    echo "No config was set - setting up the config to default build CONFIGURATIONS=$CONFIGURATIONS"
fi
MODULES="client"

# shared objects?
if [ "$BUILD_SHARED_LIBS" == "" ]; then
    BUILD_SHARED_LIBS=OFF
    echo "Setting up default to static libraries BUILD_SHARED_LIBS=$BUILD_SHARED_LIBS"
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
        export CFLAGS='-march=i686 -fPIC -fno-strict-aliasing'
        export CXXFLAGS=$CFLAGS
        export CC CXX CFLAGS CXXFLAGS
    else
        # configure compiler for x64
        export CC=gcc
        export CXX=g++
        export CFLAGS='-march=x86-64 -mtune=generic -fPIC -fno-strict-aliasing'
        export CXXFLAGS=$CFLAGS
        export CC CXX CFLAGS CXXFLAGS
        echo ${CXXFLAGS}
    fi
fi

# create fresh build directories
rm -rf buildClientDebug buildClientRelease

export UASDKDIR
# build all configurations
for config in $CONFIGURATIONS; do
    # build all modules
    for module in $MODULES; do
        # create build directory
        mkdir -p $UASDKDIR/buildClient$config/$module || { echo "mkdir failed."; exit 1; }
        # enter build directory
        cd $UASDKDIR/buildClient$config/$module || { echo "cd failed."; exit 1; }
        # create the Makefile using CMake
        cmake "$TOOLCHAIN" "$OPTION" -DCMAKE_BUILD_TYPE=$config -DBUILD_SHARED_LIBS=$BUILD_SHARED_LIBS -DCMAKE_INSTALL_PREFIX=$CMAKE_INSTALL_PREFIX $UASDKDIR/../$module || { echo "cmake failed."; exit 1; }
        # build
        cmake --build . -- -j5 || { echo "make failed."; exit 1; }
        # install
        cmake --build . --target install
    done
done


# VIM Configuration
# vim: set enc=utf-8:
# vim: set fenc=utf-8:
# vim: set ff=unix:
# vim: set tabstop=4:
# vim: set shiftwidth=4:
# vim: expandtab:
