#!/bin/bash

START=$(date +%s)

source /data/scripts/global.sh
log "INSTALL BASE MODEL"
cd /data/src/basemodel
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests

log "GENERATE CPP SOURCES"
cd /data/src/cpp
mvn clean generate-sources -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests

END=$(date +%s)
DIFF=$(( $END - $START ))
log "c++ generate sources took $DIFF seconds"
