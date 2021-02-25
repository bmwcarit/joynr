#!/bin/bash

source /data/scripts/global.sh

log "ENVIRONMENT"
env

SUCCESS=0

echo '####################################################'
echo '# start services'
echo '# This script assumes mvn was run.'
echo '####################################################'

log "start mosquitto"
mosquitto -c /data/src/docker/joynr-base/mosquitto.conf &
MOSQUITTO_PID=$!

# wait a while to allow mosquitto server to initialize
sleep 5

function stopmosquitto
{
    log "stop mosquitto"
    kill -TERM $MOSQUITTO_PID
    wait $MOSQUITTO_PID
}

(
    log "start payara"
    cd /data/src/java
    JOYNR_VERSION=$(mvn -q -Dexec.executable='echo' -Dexec.args='${project.version}' --non-recursive org.codehaus.mojo:exec-maven-plugin:exec)
    MVN_REPO=${REPODIR:=/home/$(whoami)/.m2/repository}

    DISCOVERY_DIRECTORY_WAR_FILE="$MVN_REPO/io/joynr/java/backend-services/discovery-directory-jee/$JOYNR_VERSION/discovery-directory-jee-$JOYNR_VERSION.war"

    echo DISCOVERY_DIRECTORY_WAR_FILE=$DISCOVERY_DIRECTORY_WAR_FILE
    if [ ! -f $DISCOVERY_DIRECTORY_WAR_FILE ]; then
      log "Cannot run tests: path DISCOVERY_DIRECTORY_WAR_FILE does not exist.\nMVN_REPO=$MVN_REPO"
      exit 1
    fi

    /data/src/docker/joynr-base/scripts/start-payara.sh -w $DISCOVERY_DIRECTORY_WAR_FILE
    SUCCESS=$?
    exit $SUCCESS
)
SUCCESS=$?
if [ "$SUCCESS" != "0" ]; then
    echo '########################################################'
    echo '# Start Payara failed with exit code:' $SUCCESS
    echo '########################################################'
    stopmosquitto
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

log "stop payara"
/data/src/docker/joynr-base/scripts/stop-payara.sh
stopmosquitto

exit $SUCCESS
