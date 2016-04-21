#!/bin/bash

function stop_provider {
    echo '####################################################'
    echo '# killing provider'
    echo '####################################################'
    # javascript provider
    PIDS=`pgrep -f provider.js`
    if [ -z "$PIDS" ]
    then
        # search java provider
        PIDS=`pgrep -f RobustnessProviderApplication`
        if [ -z "$PIDS" ]
        then
            # search C++ provider
            PIDS=`pgrep -f robustness-tests-provider-ws`
            if [ -z "$PIDS" ]
            then
                echo "No provider found."
                exit 1
            fi
        fi
    fi
    echo "Found provider with pid $PIDS, about to kill it"
    for PID in $PIDS
    do
        kill -9 $PID
        while (test -d /proc/$PID)
        do
            echo "PID $PID still alive. Waiting ..."
            sleep 1
        done
        echo "PID $PID exited."
    done
}

stop_provider

# success
exit 0
