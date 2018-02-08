#!/bin/bash

START=$(date +%s)

source /data/scripts/global.sh

log "ENVIRONMENT"
env

set -e
ulimit -c unlimited

cd /data/src/tests/robustness-test-env
./run-prov-cons-robustness-tests.sh -b /data/build/tests

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ robustness tests time: $DIFF seconds"
