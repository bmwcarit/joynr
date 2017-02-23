#!/bin/bash

source /data/scripts/global.sh

log "ENVIRONMENT"
env

SUCCESS=0

echo '####################################################'
echo '# start services'
echo '####################################################'

(
    cd /data/src/java
    mvn install -DskipTests -o

    DISCOVERY_DIRECTORY_WAR_FILE=$(find /data/src/java/backend-services/discovery-directory-jee/target -iregex ".*discovery-directory-jee-.*war")

    /data/src/docker/joynr-base/scripts/start-payara.sh -w $DISCOVERY_DIRECTORY_WAR_FILE
)

mosquitto -c /etc/mosquitto/mosquitto.conf &
MOSQUITTO_PID=$!

echo '####################################################'
echo '# run system integration test'
echo '####################################################'
(
    cd /data/build/joynr/bin
    ./g_SystemIntegrationTests --gtest_shuffle --gtest_color=yes --gtest_output="xml:g_SystemIntegrationTests.junit.xml"
    CHECK=$?
    if [ "$CHECK" != "0" ]; then
        echo '########################################################'
        echo '# System Integration Test failed with exit code:' $CHECK
        echo '########################################################'
    fi
    exit $CHECK
)
SUCCESS=$?

echo '####################################################'
echo '# stop services'
echo '####################################################'
kill -TERM $MOSQUITTO_PID
wait $MOSQUITTO_PID

/data/src/docker/joynr-base/scripts/stop-payara.sh

exit $SUCCESS
