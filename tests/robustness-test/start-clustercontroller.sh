#!/bin/bash

JOYNR_SOURCE_DIR=""
ROBUSTNESS_BUILD_DIR=""
ROBUSTNESS_RESULTS_DIR=""

if [ -z "$JOYNR_SOURCE_DIR" ]
then
    # assume this script is started inside a git repo subdirectory,
    JOYNR_SOURCE_DIR=`git rev-parse --show-toplevel`
fi

if [ -z "$ROBUSTNESS_BUILD_DIR" ]
then
    ROBUSTNESS_BUILD_DIR=$JOYNR_SOURCE_DIR/tests/robustness/build
fi

# if CI environment, source global settings
if [ -f /data/scripts/global.sh ]
then
    source /data/scripts/global.sh
fi

if [ -z "$ROBUSTNESS_RESULTS_DIR" ]
then
    ROBUSTNESS_RESULTS_DIR=$JOYNR_SOURCE_DIR/tests/robustness/robustness-results-$(date "+%Y-%m-%d-%H:%M:%S")
fi
mkdir -p $ROBUSTNESS_RESULTS_DIR

function stopall {
    stop_cluster_controller
    exit 1
}

trap stopall INT

function start_cluster_controller {
    echo '####################################################'
    echo '# starting C++ clustercontroller'
    echo '####################################################'
    if [ ! -d $ROBUSTNESS_BUILD_DIR -o ! -d $ROBUSTNESS_BUILD_DIR/bin ]
    then
        echo "C++ build directory or build/bin directory does not exist!"
        stopall
    fi
    CLUSTER_CONTROLLER_DIR=$ROBUSTNESS_BUILD_DIR/cluster_controller_bin
    cd $ROBUSTNESS_BUILD_DIR
    # do not purge clustercontroller_dir if it already exists
    # in order to keep persistant storage on restart
    if [ ! -d $CLUSTER_CONTROLLER_DIR ]
    then
        cp -a $ROBUSTNESS_BUILD_DIR/bin $CLUSTER_CONTROLLER_DIR
    fi
    cd $CLUSTER_CONTROLLER_DIR
    ./cluster-controller $CLUSTER_CONTROLLER_DIR/resources/cc.messaging.settings > $ROBUSTNESS_RESULTS_DIR/clustercontroller_$1.log 2>&1 &
    CLUSTER_CONTROLLER_PID=$!
    disown $CLUSTER_CONTROLLER_PID
    echo "Started external cluster controller with PID $CLUSTER_CONTROLLER_PID in directory $CLUSTER_CONTROLLER_DIR"
    # Allow some time for startup
    sleep 5
}

function stop_cluster_controller {
    echo '####################################################'
    echo '# killing C++ clustercontroller'
    echo '####################################################'
    PID=`pgrep -f cluster-controller`
    if [ -z "$PID" ]
    then
        # no cluster-controller found
        echo "No cluster-controller found."
        exit 1
    fi
    echo "Found cluster-controller with pid $PID, about to kill it"
    kill -9 $PID
    while (test -d /proc/$PID)
    do
        echo "PID $PID still alive. Waiting ..."
        sleep 1
    done
    echo "PID $PID exited."
}

start_cluster_controller cpp
exit 0
