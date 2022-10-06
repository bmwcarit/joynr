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

cd ..
ctest --verbose

SUCCESS=$?
if [ "$SUCCESS" != "0" ]; then
    echo '########################################################'
    echo '# C++ Tests failed with exit code:' $SUCCESS
    echo '########################################################'
fi

cd -

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ unit integration tests time: $DIFF seconds"

kill -TERM $MOSQUITTO_PID
wait $MOSQUITTO_PID

exit $SUCCESS
