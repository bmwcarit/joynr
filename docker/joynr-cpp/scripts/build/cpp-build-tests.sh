#!/bin/bash

# fail on first error
set -e

source /data/src/docker/joynr-base/scripts/global.sh

START=$(date +%s)
JOBS=$(nproc)

TESTS=(inter-language-test performance-test robustness-test system-integration-test)

function join_strings
{
    local delimiter=$1
    shift
    printf "%s" "${@/#/$delimiter}"
}

function usage
{
    local joined_tests=$(join_strings " | " "${TESTS[@]}")
    echo "usage: cpp-build-tests.sh all$joined_tests [--jobs X]"
    echo "default jobs is $JOBS"
}

SELECTED_TEST=$1
shift

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

# check which test to build
MAVEN_PROJECT=
MAVEN_PREFIX=",io.joynr.tests:"
SRC_FOLDER=/data/src/tests
if [[ "all" == ${SELECTED_TEST} ]]
then
  log "building all tests"
  MAVEN_PROJECT="$(join_strings "${MAVEN_PREFIX}" "${TESTS[@]}")"
elif [[ " ${TESTS[@]} " =~ " ${SELECTED_TEST} " ]]
then
  log "building ${SELECTED_TEST} test"
  MAVEN_PROJECT="${MAVEN_PREFIX}${SELECTED_TEST}"
  SRC_FOLDER="${SRC_FOLDER}/${SELECTED_TEST}"
else
  usage
  exit 1
fi

log "CPP BUILD TESTS JOBS: $JOBS"

log "ENVIRONMENT"
env

cd /data/src/
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests \
--projects \
io.joynr.tools.generator:generator-framework,\
io.joynr.tools.generator:joynr-generator-maven-plugin,\
io.joynr.tools.generator:java-generator,\
io.joynr.tools.generator:js-generator,\
io.joynr.tools.generator:cpp-generator\
${MAVEN_PROJECT}

rm -rf /data/build/tests
mkdir /data/build/tests
cd /data/build/tests

cmake -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR -DJOYNR_SERVER=localhost:8080 ${SRC_FOLDER}

time make -j $JOBS

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Test build time: $DIFF seconds"
