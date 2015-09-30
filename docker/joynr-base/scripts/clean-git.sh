#!/bin/bash

cd $SRC_DIR

source /data/src/docker/joynr-base/scripts/global.sh

log "CLEAN GIT FOLDER"
git clean -f -d -x
