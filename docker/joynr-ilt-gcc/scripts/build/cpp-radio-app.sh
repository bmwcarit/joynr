#!/bin/bash

source /data/src/docker/joynr-ilt-gcc/scripts/ci/global.sh

START=$(date +%s)
ADDITIONAL_CMAKE_ARGS=''
DLT='ON'
RUN_MAVEN='OFF'
USE_NINJA='OFF'

function usage
{
    echo "usage: cpp-radio-app.sh
        [--additionalcmakeargs <args> Default: empty string]
        [--jobs X Default $JOBS]
        [--run-maven ON|OFF Default: $RUN_MAVEN]
        [--use-ninja ON|OFF Default: $USE_NINJA]
        [--dlt ON|OFF Default: $DLT]"
}

while [ "$1" != "" ]; do
    case $1 in
        --additionalcmakeargs ) shift
                                ADDITIONAL_CMAKE_ARGS=$1
                                ;;
        --dlt )                 shift
                                DLT=$1
                                ;;
        --jobs )                shift
                                JOBS=$1
                                ;;
        --run-maven )           shift
                                RUN_MAVEN=$1
                                ;;
        --use-ninja )           shift
                                USE_NINJA=$1
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

log "CPP RADIO APP JOBS: $JOBS"
log "USE_NINJA: $USE_NINJA"

log "ENVIRONMENT"
env
echo "ADDITIONAL_CMAKE_ARGS is $ADDITIONAL_CMAKE_ARGS"

# fail on first error
set -e

if [ ${RUN_MAVEN} == "ON" ]
then
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
fi

rm -rf /data/build/radio
mkdir /data/build/radio
cd /data/build/radio

GENERATOR_VAR=''
GENERATOR_SPECIFIC_ARGUMENTS=''
if [ ${USE_NINJA} == "ON" ]; then
    log "RUN CMAKE with ninja build system"
    GENERATOR_VAR="-GNinja"
else
    log "RUN CMAKE"
    GENERATOR_SPECIFIC_ARGUMENTS="-- -j $JOBS"
fi

cmake $GENERATOR_VAR \
      -DJOYNR_ENABLE_DLT_LOGGING=$DLT \
      -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR \
      -DJOYNR_SERVER=localhost:8080 \
      $ADDITIONAL_CMAKE_ARGS \
      /data/src/examples/radio-app

time cmake --build . --target all $GENERATOR_SPECIFIC_ARGUMENTS

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Radio App build time: $DIFF seconds"
