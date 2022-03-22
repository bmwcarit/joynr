#!/bin/bash

START=$(date +%s)

source /data/src/docker/joynr-ilt-gcc/scripts/ci/global.sh

log "ENVIRONMENT"
env

set -e
ulimit -c unlimited

log "running robustness-test-env"

cd /data/src/tests/robustness-test-env
./run-prov-cons-robustness-tests.sh -b /data/build/tests

log "running mqtt-cc-robustness-test"

cd /data/src/tests/robustness-test
./run-mqtt-cc-robustness-tests.sh -b /data/build/tests -s /data/src

log "running robustness-test"

./run-robustness-tests.sh -b /data/build/tests -s /data/src

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ robustness tests time: $DIFF seconds"
