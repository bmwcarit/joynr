#!/bin/bash

START=$(date +%s)

source /data/scripts/global.sh

log "ENVIRONMENT"
env

SUCCESS=0

cd /data/src/examples

# once updated to maven 3.3.1, execute the radio-app generation explicitly without
# java using @executionId. See http://jira.codehaus.org/browse/MNG-5768
mvn -P no-license-and-notice generate-sources

rm -rf /data/build/radio
mkdir /data/build/radio
cd /data/build/radio

cmake -DJOYNR_SERVER=localhost:8080 /data/src/examples/radio-app

time make -j 20

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Radio App build time: $DIFF seconds"

exit $SUCCESS