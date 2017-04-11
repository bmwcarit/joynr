#!/bin/bash

JOYNR_SOURCE_DIR=""
ROBUSTNESS_BUILD_DIR=""
ROBUSTNESS_RESULTS_DIR=""

if [ -z "$JOYNR_SOURCE_DIR" ]
then
    # assume this script is started inside a git repo subdirectory,
    JOYNR_SOURCE_DIR=`git rev-parse --show-toplevel`
fi

if [ -z "$ROBUSTNESS_BUILD_DIR" ]
then
    ROBUSTNESS_BUILD_DIR=$JOYNR_SOURCE_DIR/tests/robustness-test/build
fi

# if CI environment, source global settings
if [ -f /data/scripts/global.sh ]
then
    source /data/scripts/global.sh
fi

if [ -z "$ROBUSTNESS_RESULTS_DIR" ]
then
    ROBUSTNESS_RESULTS_DIR=$JOYNR_SOURCE_DIR/tests/robustness-test/robustness-results-$(date "+%Y-%m-%d-%H:%M:%S")
fi
mkdir -p $ROBUSTNESS_RESULTS_DIR

function start_java_provider {
    echo '####################################################'
    echo '# starting Java provider'
    echo '####################################################'
    cd $JOYNR_SOURCE_DIR/tests/robustness-test
    # leave any persistence files instact, since this is a restart
    mvn $SPECIAL_MAVEN_OPTIONS exec:java -Dexec.mainClass="io.joynr.test.robustness.RobustnessProviderApplication" -Dexec.args="$DOMAIN mqtt" > $ROBUSTNESS_RESULTS_DIR/provider_java.log 2>&1 &
    PROVIDER_PID=$!
    echo "Started Java provider with PID $PROVIDER_PID"
    # Allow some time for startup
    sleep 10
}

function start_cpp_provider {
    echo '####################################################'
    echo '# starting C++ provider'
    echo '####################################################'
    PROVIDER_DIR=$ROBUSTNESS_BUILD_DIR/provider_bin
    # leave any persistence files instact, since this is a restart
    cd $PROVIDER_DIR
    ./robustness-tests-provider-ws $DOMAIN > $ROBUSTNESS_RESULTS_DIR/provider_cpp.log 2>&1 &
    PROVIDER_PID=$!
    echo "Started C++ provider with PID $PROVIDER_PID in directory $PROVIDER_DIR"
    # Allow some time for startup
    sleep 10
}

function start_javascript_provider {
    echo '####################################################'
    echo '# starting Javascript provider'
    echo '####################################################'
    cd $JOYNR_SOURCE_DIR/tests/robustness-test
    nohup npm run-script startprovider --robustnessTest:domain=$DOMAIN > $ROBUSTNESS_RESULTS_DIR/provider_javascript.log 2>&1 &
    PROVIDER_PID=$!
    echo "Started Javascript provider with PID $PROVIDER_PID"
    # Allow some time for startup
    sleep 10
}

# Main
if [ "$#" -ne 2 ]
then
    echo "Usage: $0 cpp|java|js domain"
    exit 1
fi
DOMAIN=$2

if [ "$1" == "cpp" ]
then
    start_cpp_provider
elif [ "$1" == "java" ]
then
    start_java_provider
elif [ "$1" == "js" ]
then
    start_javascript_provider
else
    echo "Usage: $0 cpp|java|js"
    exit 1
fi

# success
exit 0
