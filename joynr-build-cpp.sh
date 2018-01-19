#!/bin/bash

if hash nproc 2>/dev/null; then
    JOBS=$(nproc)
else
    JOBS=1
fi

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

function usage
{
    echo "usage: joynr-build-cpp.sh [--jobs X]"
    echo "default jobs is $JOBS"
}

while [ "$1" != "" ]; do
    case $1 in
        --jobs )                 shift
                                 JOBS=$1
                                 ;;
        * )                      usage
                                 exit 1
    esac
    shift
done

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

cmake -DUSE_PLATFORM_SPDLOG=OFF \
      -DUSE_PLATFORM_WEBSOCKETPP=OFF \
      -DUSE_PLATFORM_MOSQUITTO=OFF \
      -DUSE_PLATFORM_MUESLI=OFF \
      -DUSE_PLATFORM_SMRF=OFF \
      -DENABLE_GCOV=OFF \
      -DENABLE_DOXYGEN=OFF \
      -DENABLE_CLANG_FORMATTER=OFF \
      -DJOYNR_ENABLE_ACCESS_CONTROL:BOOL=OFF \
      -DBUILD_TESTS=OFF \
      -DJOYNR_SERVER_HOST=localhost \
      -DJOYNR_SERVER_HTTP_PORT=8080 \
      -DJOYNR_SERVER_MQTT_PORT=1883 \
      -DCMAKE_BUILD_TYPE=Debug ../cpp


log "BUILD C++ JOYNR"
make -j $JOBS

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ build time: $DIFF seconds"
