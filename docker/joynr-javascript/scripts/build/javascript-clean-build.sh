#!/bin/bash

set -e
(
  cd /data/src/javascript
  mvn clean install \
  -Djslint.failOnError=true \
  -Dskip.copy-notice-file=true \
  -Dskip.unpack-license-info=true
)
