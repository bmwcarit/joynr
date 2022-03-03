#!/bin/bash

START=$(date +%s)

# fail on first error
set -e

source /data/src/docker/joynr-base/scripts/ci/global.sh

cd /data/src
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests

END=$(date +%s)
DIFF=$(( $END - $START ))
log "c++ generate sources took $DIFF seconds"
