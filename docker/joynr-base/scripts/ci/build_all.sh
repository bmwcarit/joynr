#!/bin/bash

# fail on first error
# exit immediately if a command exits with a non-zero status
set -e

(
  echo '####################################################'
  echo '# building joynr java, basemodel and tools'
  echo '####################################################'
  cd /data/src
  mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle \
  -DskipTests=true \
  -Denforcer.skip=true \
  -Dmaven.compile.fork=true
)

(
  echo '####################################################'
  echo '# building joynr JavaScript API'
  echo '####################################################'
  cd /data/src/javascript
  mvn clean install \
  -Dskip.copy-notice-file=true \
  -Dskip.unpack-license-info=true \
  -DskipTests=true

  echo '####################################################'
  echo '# building joynr npm generator'
  echo '####################################################'
  cd ../tools/generator/joynr-generator-npm
  mvn clean install
)

./cpp-clean-build.sh --buildtests OFF --enableclangformatter OFF
./cpp-build-tests.sh inter-language-test

