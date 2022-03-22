#!/bin/bash


source /data/src/docker/joynr-ilt-gcc/scripts/ci/global.sh

log "ENVIRONMENT"
env

mosquitto -c /data/src/docker/joynr-ilt-gcc/mosquitto.conf &
MOSQUITTO_PID=$!

# wait a while to allow mosquitto server to initialize
sleep 5

cd /data/build/joynr/bin
rm -f *.junit.xml

START=$(date +%s)

cd ..
ctest

SUCCESS=$?
if [ "$SUCCESS" != "0" ]; then
    echo '########################################################'
    echo '# C++ Unit Tests failed with exit code:' $SUCCESS
    echo '########################################################'
fi

cd -

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ unit integration tests time: $DIFF seconds"

kill -TERM $MOSQUITTO_PID
wait $MOSQUITTO_PID

exit $SUCCESS
