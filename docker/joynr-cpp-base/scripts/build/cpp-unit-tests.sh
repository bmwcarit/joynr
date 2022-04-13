#!/bin/bash


source /data/src/docker/joynr-base/scripts/ci/global.sh

log "ENVIRONMENT"
env

mosquitto -c /data/src/docker/joynr-base/mosquitto.conf &
MOSQUITTO_PID=$!

# wait a while to allow mosquitto server to initialize
sleep 5

cd /data/build/joynr/bin
rm -f *.junit.xml

START=$(date +%s)

./g_UnitTests --gtest_shuffle --gtest_output="xml:g_UnitTests.junit.xml"
SUCCESS=$?
if [ "$SUCCESS" != "0" ]; then
    echo '########################################################'
    echo '# C++ Unit Tests failed with exit code:' $SUCCESS
    echo '########################################################'
fi

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ unit integration tests time: $DIFF seconds"

kill -TERM $MOSQUITTO_PID
wait $MOSQUITTO_PID

exit $SUCCESS
