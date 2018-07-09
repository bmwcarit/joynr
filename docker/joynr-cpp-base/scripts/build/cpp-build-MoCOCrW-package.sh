#!/bin/bash

# fail on first error
set -e

source /data/src/docker/joynr-base/scripts/global.sh

log "### start cpp-build-MoCOCrW-package.sh ###"

START=$(date +%s)

log "ENVIRONMENT"
env

JOBS=4
MOCOCRW_BUILD_DIR=/data/src/docker/build/MoCOCrW
MOCOCRW_SRCS=/tmp/MoCOCrW
MOCOCRW_VERSION=master

# clone MoCOCrW
log "INSTALL MoCOCrW"
rm -rf $MOCOCRW_SRCS
git clone https://github.com/bmwcarit/MoCOCrW.git $MOCOCRW_SRCS
cd $MOCOCRW_SRCS

git checkout $MOCOCRW_VERSION
rm -fr $MOCOCRW_BUILD_DIR
mkdir -p $MOCOCRW_BUILD_DIR
cd $MOCOCRW_BUILD_DIR
cmake -DBUILD_TESTING=Off -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr $MOCOCRW_SRCS
make -j $JOBS

DESTDIR=$MOCOCRW_BUILD_DIR
make install DESTDIR=$DESTDIR


cd /data/src/docker/build/MoCOCrW
ARCHIVE=MoCOCrW.tar.gz
tar cvzf /data/src/docker/build/$ARCHIVE usr/include/MoCOCrW-* usr/lib64/libmococrw* usr/lib64/cmake/MoCOCrW-*

END=$(date +%s)
DIFF=$(( $END - $START ))
log "MoCOCrW build time: $DIFF seconds"

log "### end cpp-build-MoCOCrW-package.sh ###"
