#!/bin/bash

source /data/scripts/global.sh

log "ENVIRONMENT"
env

SUCCESS=0

echo '####################################################'
echo '# start services'
echo '####################################################'
(
    cd /data/src/cpp/tests
    mvn jetty:run-war --quiet &
    # wait until server is up and running
    started=
    while [ "$started" != "200" ]
    do
        sleep 2s
        started=`curl -o /dev/null --silent --head --write-out '%{http_code}\n' http://localhost:8080/bounceproxy/time/`
    done
    sleep 5s
)

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
(
    cd /data/src/cpp/tests
    mvn jetty:stop --quiet
)

exit $SUCCESS
