#!/bin/bash

source /data/src/docker/joynr-base/scripts/ci/global.sh

log "ENVIRONMENT"
env

cd /data/src/tests/inter-language-test
ulimit -c unlimited
./run-inter-language-tests.sh -b /data/build/tests/inter-language-test
SUCCESS=$?
exit $SUCCESS
