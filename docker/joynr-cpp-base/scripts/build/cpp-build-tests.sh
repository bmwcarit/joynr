#!/bin/bash

# fail on first error
set -exo pipefail
source /data/src/docker/joynr-base/scripts/global.sh

log "### start cpp-build-tests.sh ###"

START=$(date +%s)
CLANGFORMATTER='ON'
BUILDTYPE='Debug'
ARCHIVEBINARIES='OFF'
ADDITIONAL_CMAKE_ARGS=''
RUN_MAVEN='OFF'
USE_NINJA='OFF'

TESTS=(inter-language-test performance-test robustness-test robustness-test-env system-integration-test)

function join_strings
{
    local delimiter=$1
    shift
    printf "%s" "${@/#/$delimiter}"
}

function usage
{
    local joined_tests=$(join_strings " | " "${TESTS[@]}")
    echo "usage: cpp-build-tests.sh all$joined_tests
        [--additionalcmakeargs <args> Default empty string]
        [--archivebinaries ON|OFF Default $ARCHIVEBINARIES]
        [--buildtype DEBUG|RELEASE|RELWITHDEBINFO|MINSIZEREL Default: $BUILDTYPE]
        [--clangformatter ON|OFF Default $CLANGFORMATTER]
        [--jobs X Default $JOBS]
        [--use-ninja ON|OFF Default: $USE_NINJA]
        [--run-maven ON|OFF Default: $RUN_MAVEN]"
    echo "default: jobs is $JOBS, clangformatter is $CLANGFORMATTER, buildtype is \
    $BUILDTYPE, archivebinaries is $ARCHIVEBINARIES and additionalcmakeargs is $ADDITIONAL_CMAKE_ARGS"
}

SELECTED_TEST=$1
shift

while [ "$1" != "" ]; do
    case $1 in
        --additionalcmakeargs ) shift
                                ADDITIONAL_CMAKE_ARGS=$1
                                ;;
        --archivebinaries )     shift
                                ARCHIVEBINARIES=$1
                                ;;
        --buildtype )           shift
                                BUILDTYPE=$1
                                ;;
        --clangformatter )      shift
                                CLANGFORMATTER=$1
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

# check which test to build
MAVEN_PROJECT=
MAVEN_PREFIX=",io.joynr.tests:"
SRC_FOLDER=/data/src/tests
if [[ "all" == ${SELECTED_TEST} ]]
then
  log "building all tests"
  MAVEN_PROJECT="$(join_strings "${MAVEN_PREFIX}" "${TESTS[@]}")"
elif [[ "system-integration-test" == ${SELECTED_TEST} ]]
then
  log "building system-integration-test"
  MAVEN_PROJECT=",io.joynr.tests.system-integration-test:sit-cpp-app"
  SRC_FOLDER="${SRC_FOLDER}/system-integration-test/sit-cpp-app"
elif [[ "robustness-test-env" == ${SELECTED_TEST} ]]
then
  log "building robustness-test-env"
  MAVEN_PROJECT=",io.joynr.tests:robustness-test-env"
  SRC_FOLDER="${SRC_FOLDER}/robustness-test-env"
elif [[ " ${TESTS[@]} " =~ " ${SELECTED_TEST} " ]]
then
  log "building ${SELECTED_TEST} test"
  MAVEN_PROJECT="${MAVEN_PREFIX}${SELECTED_TEST}"
  SRC_FOLDER="${SRC_FOLDER}/${SELECTED_TEST}"
else
  usage
  exit 1
fi

echo "ADDITIONAL_CMAKE_ARGS: $ADDITIONAL_CMAKE_ARGS"
echo "CPP BUILD TESTS JOBS: $JOBS"
echo "USE_NINJA: $USE_NINJA"
echo "MAVEN_PROJECT: $MAVEN_PROJECT"
echo "RUN_MAVEN: $RUN_MAVEN"
echo "SRC_FOLDER: $SRC_FOLDER"
echo "SELECTED_TEST: $SELECTED_TEST"

log "ENVIRONMENT:"
env

if [ ${RUN_MAVEN} == "ON" ]
then
    cd /data/src/
    mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests \
    --projects \
    io.joynr.tools.generator:generator-framework,\
    io.joynr.tools.generator:joynr-generator-maven-plugin,\
    io.joynr.tools.generator:java-generator,\
    io.joynr.tools.generator:js-generator,\
    io.joynr.tools.generator:cpp-generator\
    ${MAVEN_PROJECT}
fi

GENERATOR_VAR=''
GENERATOR_SPECIFIC_ARGUMENTS=''
if [ ${USE_NINJA} == "ON" ]; then
    log "RUN CMAKE with ninja build system"
    GENERATOR_VAR="-GNinja"
else
    log "RUN CMAKE"
    GENERATOR_SPECIFIC_ARGUMENTS="-- -j $JOBS"
fi

# build dummyKeychain first
DUMMYKEYCHAIN_SRC_DIR=/data/src/tests/dummyKeychain
DUMMYKEYCHAIN_BUILD_DIR=/data/build/dummyKeychain
rm -rf $DUMMYKEYCHAIN_BUILD_DIR
mkdir $DUMMYKEYCHAIN_BUILD_DIR
cd $DUMMYKEYCHAIN_BUILD_DIR
cmake $GENERATOR_VAR \
      -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR \
      -DCMAKE_BUILD_TYPE=$BUILDTYPE \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DENABLE_CLANG_FORMATTER=$CLANGFORMATTER \
      $ADDITIONAL_CMAKE_ARGS \
      $DUMMYKEYCHAIN_SRC_DIR

time cmake --build . --target all $GENERATOR_SPECIFIC_ARGUMENTS


# build selected test(s)
rm -rf /data/build/tests
mkdir /data/build/tests
cd /data/build/tests

cmake $GENERATOR_VAR \
      -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR \
      -DENABLE_CLANG_FORMATTER=$CLANGFORMATTER \
      -DJOYNR_SERVER=localhost:8080 \
      -DCMAKE_BUILD_TYPE=$BUILDTYPE \
      -DCMAKE_INSTALL_PREFIX=/usr \
      $ADDITIONAL_CMAKE_ARGS \
      ${SRC_FOLDER}

time cmake --build . --target all $GENERATOR_SPECIFIC_ARGUMENTS

if [ "all" == "${SELECTED_TEST}" ]
then
   log "Archiving tests bin folder for ALL"
   tar czf joynr-all-tests.tar.gz bin
else
    log "Archiving tests bin folder for $SELECTED_TEST"
    tar czf joynr-$SELECTED_TEST.tar.gz bin
fi

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Overall test build time: $DIFF seconds"
log "### end cpp-build-tests.sh ###"
