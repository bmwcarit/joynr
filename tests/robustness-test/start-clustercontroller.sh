#!/bin/bash

if [ -f /data/src/docker/joynr-base/scripts/testbase.sh ]
then
    source /data/src/docker/joynr-base/scripts/testbase.sh
else
    echo "testbase.sh script not found in /data/src/docker/joynr-base/scripts/ - aborting"
    exit 1
fi

parse_arguments $@
folder_prechecks

function stopall {
    stop_cluster_controller
    exit 1
}

trap stopall INT

function start_cluster_controller {
    echo '####################################################'
    echo '# starting C++ clustercontroller'
    echo '####################################################'
    if [ ! -d $TEST_BUILD_DIR -o ! -d $TEST_BUILD_DIR/bin ]
    then
        echo "C++ build directory or build/bin directory does not exist!"
        stopall
    fi
    CLUSTER_CONTROLLER_DIR=$TEST_BUILD_DIR/cluster_controller_bin
    cd $TEST_BUILD_DIR
    # do not purge clustercontroller_dir if it already exists
    # in order to keep persistant storage on restart
    if [ ! -d $CLUSTER_CONTROLLER_DIR ]
    then
        cp -a $TEST_BUILD_DIR/bin $CLUSTER_CONTROLLER_DIR
    fi
    cd $CLUSTER_CONTROLLER_DIR
    ./cluster-controller $CLUSTER_CONTROLLER_DIR/resources/cc.messaging.settings > $TEST_RESULTS_DIR/clustercontroller_$1_$(date "+%Y-%m-%d-%H:%M:%S").log 2>&1 &
    CLUSTER_CONTROLLER_PID=$!
    disown $CLUSTER_CONTROLLER_PID
    echo "Started external cluster controller with PID $CLUSTER_CONTROLLER_PID in directory $CLUSTER_CONTROLLER_DIR"
    # Allow some time for startup
    sleep 2
}

start_cluster_controller cpp
exit 0
