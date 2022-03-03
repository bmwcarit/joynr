#!/bin/bash

source /data/src/docker/joynr-base/scripts/ci/global.sh

log "ENVIRONMENT"
env

mosquitto -c /data/src/docker/joynr-base/mosquitto.conf &
MOSQUITTO_PID=$!

# wait a while to allow mosquitto server to initialize
sleep 5

START=$(date +%s)

SUCCESS=0
(
    cd /data/build/joynr/bin
    rm -f *.junit.xml

    echo '####################################################'
    echo '# run C++ unit tests'
    echo '####################################################'

    cd ..
    ctest

    CHECK=$?
    if [ "$CHECK" != "0" ]; then
        echo '########################################################'
        echo '# C++ Unit Tests failed with exit code:' $CHECK
        echo '########################################################'
        exit $CHECK
    fi

    cd -

    echo '####################################################'
    echo '# run C++ integration tests'
    echo '####################################################'

    ./g_IntegrationTests --gtest_shuffle --gtest_output="xml:g_IntegrationTests.junit.xml"
    CHECK=$?
    if [ "$CHECK" != "0" ]; then
        echo '########################################################'
        echo '# C++ Integration Tests failed with exit code:' $CHECK
        echo '########################################################'
        exit $CHECK
    fi
    exit $CHECK
)
SUCCESS=$?

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ unit and integration tests time: $DIFF seconds"

kill -TERM $MOSQUITTO_PID
wait $MOSQUITTO_PID

exit $SUCCESS
