#!/bin/bash

# fail on first error
set -e

source /data/src/docker/joynr-base/scripts/global.sh

START=$(date +%s)

function usage
{
    echo "usage: cpp-install-muesli.sh [--jobs X]"
}

while [ "$1" != "" ]; do
    case $1 in
        --jobs )                shift
                                JOBS=$1
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

log "ENVIRONMENT"
env

log "INSTALL RAPIDJSON"
rm -rf /data/build/rapidjson
mkdir -p /data/build/rapidjson
cd /data/build/rapidjson

cmake -DRAPIDJSON_BUILD_DOC=OFF \
      -DRAPIDJSON_BUILD_EXAMPLES=OFF \
      -DRAPIDJSON_BUILD_TESTS=OFF \
      -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF \
      /data/src-rapidjson

make -j $JOBS
make install DESTDIR=/opt/rapidjson

log "INSTALL MUESLI"
rm -rf /data/build/muesli
mkdir -p /data/build/muesli
cd /data/build/muesli

cmake -DENABLE_CLANG_FORMATTER=OFF \
      -DUSE_PLATFORM_RAPIDJSON=ON \
      -DBUILD_MUESLI_TESTS=OFF \
      -DCMAKE_PREFIX_PATH=/opt/rapidjson/usr/local/lib/cmake/RapidJSON/ \
      /data/src-muesli

make -j $JOBS
make install DESTDIR=/opt/muesli

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Muesli build time: $DIFF seconds"
