#!/bin/bash

# fail on first error
set -e

source /data/src/docker/joynr-base/scripts/global.sh

START=$(date +%s)

log "ENVIRONMENT"
env

cd /
ARCHIVE=MoCOCrW.tar.gz
tar cvzf /data/build/$ARCHIVE usr/local/include/MoCOCrW-* usr/local/lib64/libmococrw* usr/local/lib64/cmake/MoCOCrW-*

END=$(date +%s)
DIFF=$(( $END - $START ))
log "$ARCHIVE build time: $DIFF seconds"
