#!/bin/bash

START=$(date +%s)

cd /data/src

# fail on first error
set -e

source /data/src/docker/joynr-base/scripts/ci/global.sh

mvn clean install -P javascript,no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests

END=$(date +%s)
DIFF=$(( $END - $START ))
log "all maven build took $DIFF seconds"
