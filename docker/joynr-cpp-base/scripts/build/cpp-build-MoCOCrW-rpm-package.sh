#!/bin/bash

# fail on first error
set -e

source /data/src/docker/joynr-base/scripts/global.sh

log "### start cpp-build-MoCOCrW-rpm-package.sh ###"

START=$(date +%s)

log "ENVIRONMENT"
env

JOBS=4
MOCOCRW_BUILD_DIR=/data/build/MoCOCrW
MOCOCRW_SRCS=/tmp/MoCOCrW
MOCOCRW_VERSION=master

# clone MoCOCrW
rm -rf $MOCOCRW_SRCS
git clone https://github.com/bmwcarit/MoCOCrW.git $MOCOCRW_SRCS
cd $MOCOCRW_SRCS

git checkout $MOCOCRW_VERSION
mkdir $MOCOCRW_BUILD_DIR
cd $MOCOCRW_BUILD_DIR
cmake -DBUILD_TESTING=Off ..
make install -j $JOBS

# prepare RPM environment
mkdir $MOCOCRW_BUILD_DIR/package
mkdir $MOCOCRW_BUILD_DIR/package/RPM
mkdir $MOCOCRW_BUILD_DIR/package/RPM/BUILD
mkdir $MOCOCRW_BUILD_DIR/package/RPM/BUILDROOT
mkdir $MOCOCRW_BUILD_DIR/package/RPM/RPMS
mkdir $MOCOCRW_BUILD_DIR/package/RPM/SOURCES
mkdir $MOCOCRW_BUILD_DIR/package/RPM/SRPMS
mkdir $MOCOCRW_BUILD_DIR/package/RPM/MoCOCrW

cd $MOCOCRW_BUILD_DIR

DESTDIR=$MOCOCRW_BUILD_DIR/package/RPM/MoCOCrW
log "BUILD RPM PACKAGE for MoCOCrW"
rpm_with_flags=""
cd $MOCOCRW_BUILD_DIR/package/RPM/SPECS
rpmbuild -bb ${rpm_with_flags} --buildroot $DESTDIR

END=$(date +%s)
DIFF=$(( $END - $START ))
log "MoCOCrW build time: $DIFF seconds"

log "### end cpp-build-MoCOCrW-rpm-package.sh ###"
