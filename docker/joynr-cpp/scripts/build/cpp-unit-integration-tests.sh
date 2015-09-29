#!/bin/bash

START=$(date +%s)

source /data/src/docker/joynr-base/scripts/global.sh

log "ENVIRONMENT"
env

set -e

cd /data/build/joynr/bin
rm -f *.junit.xml

./g_UnitTests --gtest_shuffle --gtest_output="xml:g_UnitTests.junit.xml"

./g_IntegrationTests --gtest_shuffle --gtest_output="xml:g_IntegrationTests.junit.xml"

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ unit and integration tests time: $DIFF seconds"
