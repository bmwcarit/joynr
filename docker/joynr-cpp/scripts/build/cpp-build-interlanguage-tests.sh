#!/bin/bash

source /data/src/docker/joynr-base/scripts/global.sh

START=$(date +%s)
JOBS=$(nproc)

function usage
{
    echo "usage: cpp-build-interlanguage-tests.sh [--jobs X]"
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

log "CPP BUILD INTER-LANGUAGE TESTS JOBS: $JOBS"

log "ENVIRONMENT"
env

# fail on first error
set -e

# inter-language tests currently also require generating for Java and Javascript, so
# install the java- und javascripts generators
cd /data/src/
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests \
--projects \
io.joynr.tools.generator:generator-framework,\
io.joynr.tools.generator:joynr-generator-maven-plugin,\
io.joynr.tools.generator:java-generator,\
io.joynr.tools.generator:js-generator,\
io.joynr.tools.generator:cpp-generator,\
io.joynr.tests:inter-language-test

rm -rf /data/build/inter-language-test
mkdir /data/build/inter-language-test
cd /data/build/inter-language-test

cmake -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR -DJOYNR_SERVER=localhost:8080 /data/src/tests/inter-language-test

time make -j $JOBS

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Inter-language Test build time: $DIFF seconds"
