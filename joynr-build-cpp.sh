#!/bin/bash

error () {
[ "$1" != "0" ] &&  exit 1 || :
}


log () {
echo ""
echo "========================================"
echo "= $1"
echo "========================================"
echo ""
}


START=$(date +%s)

# fail on first error
set -e

log "INSTALL REQUIRED MODULES AND GENERATE SOURCES"
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests

END=$(date +%s)
DIFF=$(( $END - $START ))
log "c++ generate sources took $DIFF seconds"

mkdir -p build
cd build

log "RUN CMAKE"

cmake -DUSE_DBUS_COMMONAPI_COMMUNICATION=$DBUS \
      -DENABLE_GCOV=$GCOV \
      -DENABLE_DOXYGEN=OFF \
      -DENABLE_CLANG_FORMATTER=OFF \
      -DPYTHON_EXECUTABLE=/usr/bin/python \
      -DJOYNR_SERVER=localhost:8080 \
      -DCMAKE_BUILD_TYPE=Debug ../cpp


log "BUILD C++ JOYNR"
make -j 20

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ build time: $DIFF seconds"
