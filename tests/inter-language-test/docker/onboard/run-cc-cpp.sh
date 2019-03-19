#!/bin/bash

set -x
CC_CONFIG=/data/onboard-cc-messaging.settings
CPP_HOME=/data/ilt-cpp-app
export LD_LIBRARY_PATH=$CPP_HOME:$LD_LIBRARY_PATH
export JOYNR_LOG_LEVEL=TRACE
cd $CPP_HOME
./bin/cluster-controller $CC_CONFIG > /data/build/cc.log 2>&1 &
