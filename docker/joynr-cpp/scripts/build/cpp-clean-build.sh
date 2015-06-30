#!/bin/bash

source /data/scripts/global.sh

function usage
{
    echo "usage: cpp-clean-build.sh [--dbus ON|OFF --gcov ON|OFF]"
    echo "default dbus is OFF, gcov is OFF"
}

DBUS='OFF'
GCOV='OFF'

while [ "$1" != "" ]; do
    case $1 in
        --dbus )                shift
                                DBUS=$1
                                ;;
        --gcov )                shift
                                GCOV=$1
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

log "CPP CLEAN BUILD DBUS: $DBUS GCOV: $GCOV"

START=$(date +%s)


log "ENVIRONMENT"
env

log "CLEAN BUILD DIRECTORY"
rm -rf ~/.cmake/packages
rm -rf /data/build/joynr
mkdir /data/build/joynr

cd /data/build/joynr

log "RUN CMAKE"

# fail on first error
set -e
cmake -DUSE_DBUS_COMMONAPI_COMMUNICATION=$DBUS \
      -DENABLE_GCOV=$GCOV \
      -DPYTHON_EXECUTABLE=/usr/bin/python \
      -DJOYNR_SERVER=localhost:8080 \
      -DCMAKE_BUILD_TYPE=Debug /data/src/cpp

if [ "$GCOV" == "ON" ] ; then
    echo "run coverage build"
    make -j20 UnitCoverageTarget
fi

log "BUILD C++ JOYNR"
make -j 20

END=$(date +%s)
DIFF=$(( $END - $START ))
log "C++ build time: $DIFF seconds"
