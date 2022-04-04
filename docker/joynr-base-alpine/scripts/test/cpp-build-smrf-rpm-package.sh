#!/bin/bash

# fail on first error
set -e

source /data/src/docker/joynr-base-alpine/scripts/ci/global.sh

log "### start cpp-build-smrf-rpm-package.sh ###"

START=$(date +%s)

log "ENVIRONMENT"
env

JOBS=4
SMRF_BUILD_DIR=/data/build/smrf
SMRF_SRCS=/tmp/smrf
SMRF_VERSION=0.3.4

# clone smrf
cd /tmp
rm -fr smrf
git clone https://github.com/bmwcarit/smrf.git
cd smrf
git checkout $SMRF_VERSION
rm -fr $SMRF_BUILD_DIR
mkdir -p $SMRF_BUILD_DIR
cd $SMRF_BUILD_DIR
cmake -DBUILD_TESTS=Off -DCMAKE_INSTALL_PREFIX=/usr $ADDITIONAL_CMAKE_ARGS $SMRF_SRCS
make -j $JOBS
cd /tmp

log "Enable core dumps"
ulimit -c unlimited

# prepare RPM environment
mkdir $SMRF_BUILD_DIR/package
mkdir $SMRF_BUILD_DIR/package/RPM
mkdir $SMRF_BUILD_DIR/package/RPM/BUILD
mkdir $SMRF_BUILD_DIR/package/RPM/BUILDROOT
mkdir $SMRF_BUILD_DIR/package/RPM/RPMS
mkdir $SMRF_BUILD_DIR/package/RPM/SOURCES
mkdir $SMRF_BUILD_DIR/package/RPM/SPECS
mkdir $SMRF_BUILD_DIR/package/RPM/SRPMS
mkdir $SMRF_BUILD_DIR/package/RPM/smrf

# copy RPM spec file
RPMSPEC=smrf.spec
cp $SMRF_SRCS/cpp/distribution/$RPMSPEC $SMRF_BUILD_DIR/package/RPM/SPECS
# disable generation of debug package as workaround for broken rpmbuild in
# Fedora 27 complaining about empty %files in BUILD/debugsourcefiles.list
# which is automatically created during build
# see https://bugzilla.redhat.com/show_bug.cgi?id=1479198 for more info
sed -i 's/%debug_package/%global debug_package %{nil}/' $SMRF_BUILD_DIR/package/RPM/SPECS/$RPMSPEC
RPMSPEC_BASENAME=`basename $SMRF_SRCS/cpp/distribution/$RPMSPEC`
cd $SMRF_BUILD_DIR

DESTDIR=$SMRF_BUILD_DIR/package/RPM/smrf
log "BUILD RPM PACKAGE"
make -j $JOBS install DESTDIR=$DESTDIR
rpm_with_flags=""
cd $SMRF_BUILD_DIR/package/RPM/SPECS
rpmbuild -bb ${rpm_with_flags} --buildroot $DESTDIR $RPMSPEC_BASENAME

END=$(date +%s)
DIFF=$(( $END - $START ))
log "SMRF build time: $DIFF seconds"

log "### end cpp-build-smrf-rpm-package.sh ###"
