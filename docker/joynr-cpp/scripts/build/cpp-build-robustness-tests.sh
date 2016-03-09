#!/bin/bash

source /data/src/docker/joynr-base/scripts/global.sh

START=$(date +%s)
JOBS=$(nproc)

function usage
{
    echo "usage: cpp-build-robustness-tests.sh [--jobs X]"
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

log "CPP BUILD ROBUSTNESS TESTS JOBS: $JOBS"

log "ENVIRONMENT"
env

# fail on first error
set -e

# robustness tests currently also require generating for C++ Javascript, so
# install the C++- und javascripts generators
cd /data/src/
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests \
--projects \
io.joynr.tools.generator:generator-framework,\
io.joynr.tools.generator:joynr-generator-maven-plugin,\
io.joynr.tools.generator:js-generator,\
io.joynr.tools.generator:cpp-generator,\
io.joynr.tests:robustness

rm -rf /data/build/robustness
mkdir /data/build/robustness
cd /data/build/robustness

cmake -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR -DJOYNR_SERVER=localhost:8080 /data/src/tests/robustness

time make -j $JOBS

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Robustness Tests build time: $DIFF seconds"
