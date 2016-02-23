#!/bin/bash

source /data/src/docker/joynr-base/scripts/global.sh

START=$(date +%s)
JOBS=8

function usage
{
    echo "usage: cpp-build-joynr-system-integration-test.sh [--jobs X]"
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

log "CPP joynr-system-integration-test JOBS: $JOBS"

log "ENVIRONMENT"
env

# fail on first error
set -e

cd /data/src/
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests \
--projects \
io.joynr.tools.generator:generator-framework,\
io.joynr.tools.generator:joynr-generator-maven-plugin,\
io.joynr.tools.generator:cpp-generator,\
io.joynr.examples:joynr-system-integration-test

rm -rf /data/build/joynr-system-integration-test/
mkdir /data/build/joynr-system-integration-test
cd /data/build/joynr-system-integration-test

cmake -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR -DJOYNR_SERVER=localhost:8080 /data/src/examples/joynr-system-integration-test

time make -j $JOBS

END=$(date +%s)
DIFF=$(( $END - $START ))
log "joynr-system-integration-test build time: $DIFF seconds"
