#!/bin/bash

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

stop_cluster_controller
exit 0
