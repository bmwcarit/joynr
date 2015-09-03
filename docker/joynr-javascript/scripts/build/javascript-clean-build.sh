#!/bin/bash

source /data/scripts/global.sh

# fail on first error
set -e

log "INSTALL JOYNR BASE MODEL, TOOLS AND INFRASTRUCTURE SERVICES"
cd /data/src
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests

log "building joynr JavaScript API"
cd /data/src/javascript
mvn clean install \
    -Djslint.failOnError=true \
    -Dskip.copy-notice-file=true \
    -Dskip.unpack-license-info=true
