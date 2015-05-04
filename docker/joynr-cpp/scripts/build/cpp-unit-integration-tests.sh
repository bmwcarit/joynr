#!/bin/bash

START=$(date +%s)

source /data/scripts/global.sh

log "ENVIRONMENT"
env

SUCCESS=0

cd /data/build/joynr/bin
rm -f *.junit.xml

./g_UnitTests --gtest_shuffle --gtest_output="xml:g_UnitTests.junit.xml"
SUCCESS=$(( ($? == 0 && SUCCESS == 0) ? 0 : 1 ))

./g_IntegrationTests --gtest_shuffle --gtest_output="xml:g_IntegrationTests.junit.xml"
SUCCESS=$(( ($? == 0 && SUCCESS == 0) ? 0 : 1 ))

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ unit and integration tests time: $DIFF seconds"

exit $SUCCESS