#!/bin/bash

CLUSTER_CONTROLLER_PID=""
JOYNR_BIN_DIR='/data/build/joynr/bin'

function startCppClusterController {
    echo 'Starting cluster controller'
    cd $JOYNR_BIN_DIR

    # ensure previously created persistence files are gone
    rm -Rf *.persist joynr.settings

    ./cluster-controller 1>/dev/null 2>/dev/null &
    CLUSTER_CONTROLLER_PID=$!

    # Wait long enough in order to allow the cluster controller finish its start procedure
    sleep 5

    echo "Cluster controller started"
}

function stopCppClusterController {
    echo "Killing C++ cluster controller. PID:" $CLUSTER_CONTROLLER_PID
    kill $CLUSTER_CONTROLLER_PID
    wait $CLUSTER_CONTROLLER_PID
    CLUSTER_CONTROLLER_PID=""
}

startCppClusterController

cd /data/src/tests/performance-test
rm -rf node_modules package-lock.json
npm i
npm run startPerformance
sleep 3
npm run startMemory
sleep 3
npm run startBroadcast

stopCppClusterController