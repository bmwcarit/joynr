#!/bin/bash

source /data/src/docker/joynr-base/scripts/global.sh

START=$(date +%s)
JOBS=8

function usage
{
    echo "usage: cpp-radio-app.sh [--jobs X]"
    echo "default jobs is $JOBS"
}

while [ "$1" != "" ]; do
    case $1 in
        --jobs )                shift
                                JOBS=$1
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

log "CPP RADIO APP JOBS: $JOBS"

log "ENVIRONMENT"
env

# fail on first error
set -e

# radio currently also generating for Java, so install the java-generator too
# until we can exclude the Java build of radio using @executionId (see below)
cd /data/src/
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests \
--projects \
io.joynr.tools.generator:generator-framework,\
io.joynr.tools.generator:joynr-generator-maven-plugin,\
io.joynr.tools.generator:java-generator,\
io.joynr.tools.generator:cpp-generator,\
io.joynr.examples:radio-app

rm -rf /data/build/radio
mkdir /data/build/radio
cd /data/build/radio

cmake -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR -DJOYNR_SERVER=localhost:8080 /data/src/examples/radio-app

time make -j $JOBS

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Radio App build time: $DIFF seconds"
