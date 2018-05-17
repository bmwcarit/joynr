#!/bin/bash

if [ -f /data/src/docker/joynr-base/scripts/testbase.sh ]
then
    source /data/src/docker/joynr-base/scripts/testbase.sh
else
    echo "testbase.sh script not found in /data/src/docker/joynr-base/scripts/ - aborting"
    exit 1
fi

parse_arguments $@
folder_prechecks

function start_java_provider {
    echo '####################################################'
    echo '# starting Java provider'
    echo '####################################################'
    cd $JOYNR_SOURCE_DIR/tests/robustness-test
    # leave any persistence files instact, since this is a restart
    mvn $SPECIAL_MAVEN_OPTIONS exec:java -Dexec.mainClass="io.joynr.test.robustness.RobustnessProviderApplication" -Dexec.args="$DOMAIN mqtt" > $TEST_RESULTS_DIR/provider_java_$TIMESTAMP.log 2>&1 &
    PROVIDER_PID=$!
    echo "Started Java provider with PID $PROVIDER_PID"
    # Allow some time for startup
    sleep 10
}

function start_cpp_provider {
    echo '####################################################'
    echo '# starting C++ provider'
    echo '####################################################'
    PROVIDER_DIR=$TEST_BUILD_DIR/provider_bin
    # leave any persistence files instact, since this is a restart
    cd $PROVIDER_DIR
    ./robustness-tests-provider-ws $DOMAIN > $TEST_RESULTS_DIR/provider_cpp_$TIMESTAMP.log 2>&1 &
    PROVIDER_PID=$!
    echo "Started C++ provider with PID $PROVIDER_PID in directory $PROVIDER_DIR"
    # Allow some time for startup
    sleep 5
}

function start_javascript_provider {
    echo '####################################################'
    echo '# starting Javascript provider'
    echo '####################################################'
    cd $JOYNR_SOURCE_DIR/tests/robustness-test
    nohup npm run-script startprovider --robustnessTest:domain=$DOMAIN > $TEST_RESULTS_DIR/provider_javascript_$TIMESTAMP.log 2>&1 &
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
TIMESTAMP=$(date "+%Y-%m-%d-%H:%M:%S")

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
