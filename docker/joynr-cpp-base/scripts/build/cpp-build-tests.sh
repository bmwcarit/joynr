#!/bin/bash

# fail on first error
set -e

source /data/src/docker/joynr-base/scripts/global.sh

START=$(date +%s)
CLANGFORMATTER='ON'
BUILDTYPE='Debug'
ARCHIVEBINARIES='OFF'
ADDITIONAL_CMAKE_ARGS=''
RUN_MAVEN='OFF'

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
    echo "usage: cpp-build-tests.sh all$joined_tests [--jobs X --clangformatter ON|OFF \
    --buildtype Debug|Release --archivebinaries ON|OFF --additionalcmakeargs <args> --run-maven ON|OFF]"
    echo "default: jobs is $JOBS, clangformatter is $CLANGFORMATTER, buildtype is \
    $BUILDTYPE, archivebinaries is $ARCHIVEBINARIES and additionalcmakeargs is $ADDITIONAL_CMAKE_ARGS"
}

SELECTED_TEST=$1
shift

while [ "$1" != "" ]; do
    case $1 in
        --jobs )                shift
                                JOBS=$1
                                ;;
        --clangformatter )      shift
                                CLANGFORMATTER=$1
                                ;;
        --buildtype )           shift
                                BUILDTYPE=$1
                                ;;
        --archivebinaries )     shift
                                ARCHIVEBINARIES=$1
                                ;;
        --additionalcmakeargs ) shift
                                ADDITIONAL_CMAKE_ARGS=$1
                                ;;
        --run-maven )           shift
                                RUN_MAVEN=$1
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

# build dummyKeychain first
DUMMYKEYCHAIN_SRC_DIR=/data/src/tests/dummyKeychain
DUMMYKEYCHAIN_BUILD_DIR=/data/build/dummyKeychain
rm -rf $DUMMYKEYCHAIN_BUILD_DIR
mkdir $DUMMYKEYCHAIN_BUILD_DIR
cd $DUMMYKEYCHAIN_BUILD_DIR
cmake -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR \
      -DCMAKE_BUILD_TYPE=$BUILDTYPE \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DENABLE_CLANG_FORMATTER=$CLANGFORMATTER \
      $DUMMYKEYCHAIN_SRC_DIR
make -j ${JOBS}

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
echo "ADDITIONAL_CMAKE_ARGS is $ADDITIONAL_CMAKE_ARGS"

if [ ${RUN_MAVEN} == "ON" ]
then
    cd /data/src/
    mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests \
    --projects \
    io.joynr.tools.generator:dependency-libs,\
    io.joynr.tools.generator:generator-framework,\
    io.joynr.tools.generator:joynr-generator-maven-plugin,\
    io.joynr.tools.generator:java-generator,\
    io.joynr.tools.generator:js-generator,\
    io.joynr.tools.generator:cpp-generator\
    ${MAVEN_PROJECT}
fi

rm -rf /data/build/tests
mkdir /data/build/tests
cd /data/build/tests

cmake -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR \
      -DENABLE_CLANG_FORMATTER=$CLANGFORMATTER \
      -DJOYNR_SERVER=localhost:8080 \
      -DCMAKE_BUILD_TYPE=$BUILDTYPE \
      -DCMAKE_INSTALL_PREFIX=/usr \
      $ADDITIONAL_CMAKE_ARGS \
      ${SRC_FOLDER}

time make -j $JOBS

if [ "ON" == "${ARCHIVEBINARIES}" ]
then
	if [ "all" == "${SELECTED_TEST}" ]
	then
 	   tar czf joynr-all-tests.tar.gz bin
	else
	    tar czf joynr-$SELECTED_TEST.tar.gz bin
	fi
fi

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Test build time: $DIFF seconds"
