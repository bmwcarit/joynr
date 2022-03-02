#!/bin/bash

source /data/src/docker/joynr-base/scripts/global.sh

log "### start cpp-clean-build.sh ###"

GCOV='OFF'
ENABLE_CLANG_FORMATTER='ON'
BUILD_TESTS='ON'
ADDITIONAL_CMAKE_ARGS=''
BUILD_TYPE='Debug'
USE_NINJA='OFF'

function usage
{
    echo "usage: cpp-clean-build.sh
            --jobs X
            --enableclangformatter ON|OFF
            --buildtests ON|OFF
            [--buildtype [Debug|Release|RelWithDebInfo|MinSizeRel] <default: Debug>]
            [--gcov ON|OFF <default: OFF>]
            [--use-ninja ON|OFF <Default: $USE_NINJA>]
            [--additionalcmakeargs <args> <default: "">]"
}

while [ "$1" != "" ]; do
    case $1 in
        --gcov )                 shift
                                 GCOV=$1
                                 ;;
        --jobs )                 shift
                                 JOBS=$1
                                 ;;
        --enableclangformatter ) shift
                                 ENABLE_CLANG_FORMATTER=$1
                                 ;;
        --buildtests )           shift
                                 BUILD_TESTS=$1
                                 ;;
        --buildtype )            shift
                                 BUILD_TYPE=$1
                                 ;;
        --additionalcmakeargs )  shift
                                 ADDITIONAL_CMAKE_ARGS=$1
                                 ;;
        --use-ninja )            shift
                                 USE_NINJA=$1
                                 ;;
        * )                      usage
                                 exit 1
    esac
    shift
done

log "CPP CLEAN BUILD GCOV: $GCOV JOBS: $JOBS"

log "Enable core dumps"
ulimit -c unlimited

START=$(date +%s)

log "ENVIRONMENT"
env
log "ADDITIONAL_CMAKE_ARGS is $ADDITIONAL_CMAKE_ARGS"
log "USE_NINJA is $USE_NINJA"

log "CLEAN BUILD DIRECTORY"
rm -rf ~/.cmake/packages
rm -rf /data/build/joynr
mkdir -p /data/build/joynr

cd /data/build/joynr

GENERATOR_VAR=''
GENERATOR_SPECIFIC_ARGUMENTS=''
if [ ${USE_NINJA} == "ON" ]; then
    log "RUN CMAKE with ninja build system"
    GENERATOR_VAR="-GNinja"
else
    log "RUN CMAKE"
    GENERATOR_SPECIFIC_ARGUMENTS="-- -j $JOBS"
fi

# fail on first error
set -e -x
cmake $GENERATOR_VAR \
      -DENABLE_GCOV=$GCOV \
      -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
      -DENABLE_CLANG_FORMATTER=$ENABLE_CLANG_FORMATTER \
      -DBUILD_TESTS=$BUILD_TESTS \
      -DCMAKE_INSTALL_SYSCONFDIR=/etc \
      -DCMAKE_INSTALL_PREFIX=/usr \
      $ADDITIONAL_CMAKE_ARGS \
      /data/src/cpp

if [ "$GCOV" == "ON" ] ; then
    echo "run coverage build with ninja"
    time cmake --build . --target UnitCoverageTarget $GENERATOR_SPECIFIC_ARGUMENTS
    time cmake --build . --target UnitCoverageHtml $GENERATOR_SPECIFIC_ARGUMENTS
fi

time cmake --build . --target all $GENERATOR_SPECIFIC_ARGUMENTS

log "BUILD C++ JOYNR DOXYGEN DOCUMENTATION"
log "doxygen is disabled"
# make doxygen

tar czf ../joynr-clean-build.tar.gz .
mv ../joynr-clean-build.tar.gz .

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ build time: $DIFF seconds"

log "### end cpp-clean-build.sh ###"
