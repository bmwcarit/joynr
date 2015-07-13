#!/bin/bash

START=$(date +%s)

# fail on first error
set -e

source /data/scripts/global.sh
log "INSTALL BASE MODEL"
cd /data/src/basemodel
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests

log "INSTALL FRANCA AND BASE GENERATOR"
cd /data/src/tools/generator
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests

log "INSTALL GENERATOR AND GENERATE CPP SOURCES, ALSO FOR USE OF LATER BUILD STEPS"
cd /data/src/cpp
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests

END=$(date +%s)
DIFF=$(( $END - $START ))
log "c++ generate sources took $DIFF seconds"
