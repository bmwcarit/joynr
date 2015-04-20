#!/bin/bash

cd $SRC_DIR

source /data/scripts/global.sh

log "CLEAN GIT FOLDER"
git clean -f -d -x
