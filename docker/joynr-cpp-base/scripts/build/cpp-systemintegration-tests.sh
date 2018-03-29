#!/bin/bash

source /data/scripts/global.sh

log "ENVIRONMENT"
env

SUCCESS=0

echo '####################################################'
echo '# start services'
echo '# This script assumes mvn was run.'
echo '####################################################'

mosquitto -c /etc/mosquitto/mosquitto.conf &
MOSQUITTO_PID=$!

# wait a while to allow mosquitto server to initialize
sleep 5

(
    cd /data/src/java
    JOYNR_VERSION=$(mvn -q -Dexec.executable='echo' -Dexec.args='${project.version}' --non-recursive org.codehaus.mojo:exec-maven-plugin:exec)
    MVN_REPO=${REPODIR:=/home/$(whoami)/.m2/repository}

    ACCESS_CTRL_WAR_FILE="$MVN_REPO/io/joynr/java/backend-services/domain-access-controller-jee/$JOYNR_VERSION/domain-access-controller-jee-$JOYNR_VERSION.war"
    DISCOVERY_DIRECTORY_WAR_FILE="$MVN_REPO/io/joynr/java/backend-services/discovery-directory-jee/$JOYNR_VERSION/discovery-directory-jee-$JOYNR_VERSION.war"

    if [ ! -f $ACCESS_CTRL_WAR_FILE ] || [ ! -f $DISCOVERY_DIRECTORY_WAR_FILE ]; then
      echo ACCESS_CTRL_WAR_FILE=$ACCESS_CTRL_WAR_FILE
      echo DISCOVERY_DIRECTORY_WAR_FILE=$DISCOVERY_DIRECTORY_WAR_FILE
      log "Cannot run tests: paths to ACCESS_CTRL_WAR_FILE and DISCOVERY_DIRECTORY_WAR_FILE do not exist.\nMVN_REPO=$MVN_REPO"
      exit 1
    fi

    /data/src/docker/joynr-base/scripts/start-payara.sh -w $DISCOVERY_DIRECTORY_WAR_FILE,$ACCESS_CTRL_WAR_FILE
)

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

/data/src/docker/joynr-base/scripts/stop-payara.sh
kill -TERM $MOSQUITTO_PID
wait $MOSQUITTO_PID

exit $SUCCESS
