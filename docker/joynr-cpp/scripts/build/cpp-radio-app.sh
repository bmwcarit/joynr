#!/bin/bash

START=$(date +%s)

source /data/scripts/global.sh

log "ENVIRONMENT"
env

# fail on first error
set -e
(
     # radio currently also generating for Java, so install the java-generator too
     # until we can exclude the Java build of radio using @executionId (see below)
    cd /data/src/java/java-generator
    mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests
)

cd /data/src/examples/radio-app

# once updated to maven 3.3.1, execute the radio-app generation explicitly without
# java using @executionId. See http://jira.codehaus.org/browse/MNG-5768
mvn -P no-license-and-notice generate-sources

rm -rf /data/build/radio
mkdir /data/build/radio
cd /data/build/radio

cmake -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR -DJOYNR_SERVER=localhost:8080 /data/src/examples/radio-app

time make -j 20

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Radio App build time: $DIFF seconds"