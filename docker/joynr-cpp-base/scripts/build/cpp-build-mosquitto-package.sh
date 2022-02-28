#!/bin/bash

# fail on first error
set -e -u

source /data/src/docker/joynr-base/scripts/ci/global.sh

log "### start cpp-build-mosquitto-package.sh ###"

START=$(date +%s)

log "ENVIRONMENT"
env

# this runs in the cpp-gcc docker image where mosquitto is already
# installed, we just gather the required contents and put them into
# a zipfile

cd /
ARCHIVE=mosquitto.tar.gz
tar cvzf /data/src/docker/build/$ARCHIVE usr/include/mosquitto* usr/lib/libmosquitto* usr/sbin/mosquitto usr/bin/mosquitto* usr/lib/pkgconfig/libmosquitto*

END=$(date +%s)
DIFF=$(( $END - $START ))
log "mosquitto build time: $DIFF seconds"

log "### end cpp-build-mosquitto-package.sh ###"
