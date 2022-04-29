#!/bin/bash

source /data/src/docker/joynr-base-alpine/scripts/ci/global.sh
source /data/src/docker/joynr-base-alpine/scripts/ci/start-and-stop-gcd-service.sh

log "ENVIRONMENT"
env

SUCCESS=0

echo '####################################################'
echo '# start services'
echo '# This script assumes mvn was run.'
echo '####################################################'

/data/src/docker/joynr-base-alpine/scripts/ci/start-db.sh

log "start mosquitto"
mosquitto -c /data/src/docker/joynr-base-alpine/mosquitto.conf &
MOSQUITTO_PID=$!
log "Mosquitto started with PID $MOSQUITTO_PID"

# wait a while to allow mosquitto server to initialize
sleep 5

function stopservices
{
    stopGcd

    log "stop mosquitto"
    kill -TERM $MOSQUITTO_PID
    wait $MOSQUITTO_PID
    /data/src/docker/joynr-base-alpine/scripts/ci/stop-db.sh
}

startGcd
SUCCESS=$?
if [ "$SUCCESS" != "0" ]; then
    echo '########################################################'
    echo '# Start GCD failed with exit code:' $SUCCESS
    echo '########################################################'

    stopservices
    exit $SUCCESS
fi

# wait a while to allow backend service to startup and connect to mosquitto
sleep 5

echo '####################################################'
echo '# run C++ system integration test'
echo '####################################################'
(
    cd /data/build/joynr/bin
    ./g_SystemIntegrationTests --gtest_shuffle --gtest_color=yes --gtest_output="xml:g_SystemIntegrationTests.junit.xml"
    CHECK=$?
    if [ "$CHECK" != "0" ]; then
        echo '########################################################'
        echo '# C++ System Integration Test failed with exit code:' $CHECK
        echo '########################################################'
    fi
    exit $CHECK
)
SUCCESS=$?

echo '####################################################'
echo '# stop services'
echo '####################################################'

stopservices

exit $SUCCESS
