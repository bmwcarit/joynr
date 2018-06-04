#!/bin/bash

###
# #%L
# %%
# Copyright (C) 2011 - 2018 BMW Car IT GmbH
# %%
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# #L%
###
#set -x

# PARAMETERS
#################################
JOYNR_SOURCE_DIR=""
ROBUSTNESS_ENV_BUILD_DIR=""
ROBUSTNESS_ENV_RESULTS_DIR=""
JOYNR_LOG_LEVEL=""

US='_'  # underscore
CLUSTER_CONTROLLER_APP="cluster-controller"
PROVIDER_APP="provider-app"
CONSUMER_APP="consumer-app"

APP_EXIT_MESSAGE="exiting"
SUCCESS=0
FAILED_TESTS=0
CONSUMER_PIDS=()
NUM_CONSUMERS=0

DOMAIN="joynr-robustness-test-env-domain"
PROV_NUM_OF_RUNTIMES=2
PROV_NUM_OF_PROVIDERS=2
PROV_THREAD_DELAY_MS=200

CONS_NUM_OF_RUNTIMES=2
CONS_NUM_OF_PROXYBUILDERS=2
CONS_NUM_OF_PROXIES=1
CONS_TEST_CASE=1
CONS_THREAD_DELAY_MS=100
CONS_NUM_OF_TEST_CYCLES=2
CONS_TEST_CYCLE_TIME=3

while getopts "b:l:s:r:" OPTIONS;
do
        case $OPTIONS in
                b)
                        ROBUSTNESS_ENV_BUILD_DIR=$OPTARG
                        ;;
                l)
                        JOYNR_LOG_LEVEL=$OPTARG
                        ;;
                r)
                        ROBUSTNESS_ENV_RESULTS_DIR=$OPTARG
                        ;;
                s)
                        JOYNR_SOURCE_DIR=$OPTARG
                        if [ ! -d "$JOYNR_SOURCE_DIR" ]
                        then
                                echo "Directory $JOYNR_SOURCE_DIR does not exist!"
                                exit 1
                        else
                                echo "JOYNR_SOURCE_DIR=$OPTARG"
                        fi
                        ;;
                \?)
                        echo "Illegal option found."
                        echo "Synopsis: run-prov-cons-robustness-tests.sh [-b <robustness-test-env-build-dir>] [-r <robustness-test-env-results-dir>] [-s <joynr-source-dir>]"
                        exit 1
                        ;;
        esac
done

if [ -z "$JOYNR_SOURCE_DIR" ]
then
        # assume this script is started inside a git repo subdirectory,
        JOYNR_SOURCE_DIR=`git rev-parse --show-toplevel`
fi

if [ -z "$ROBUSTNESS_ENV_BUILD_DIR" ]
then
        ROBUSTNESS_ENV_BUILD_DIR=$JOYNR_SOURCE_DIR/tests/robustness-test-env/build
fi

# if CI environment, source global settings
if [ -f /data/scripts/global.sh ]
then
        source /data/scripts/global.sh
fi

if [ -z "$ROBUSTNESS_ENV_RESULTS_DIR" ]
then
        ROBUSTNESS_ENV_RESULTS_DIR=$JOYNR_SOURCE_DIR/tests/robustness-test-env/robustness-test-env-results-$(date "+%Y-%m-%d-%H:%M:%S")
fi

if [ -z "$JOYNR_LOG_LEVEL" ]
then
        JOYNR_LOG_LEVEL=TRACE
fi

# process ids for background stuff
MOSQUITTO_PID=""

# log files
CLUSTER_CONTROLLER_LOG=$ROBUSTNESS_ENV_RESULTS_DIR/clustercontroller.log
PROVIDER_LOG=$ROBUSTNESS_ENV_RESULTS_DIR/$PROVIDER_APP.log

# in case of interrupts, forcibly kill background processes
function stopall {
        stop_all_consumers
        stop_provider
        stop_cluster_controller
        stop_services
        log 'TEST FAILED'
        exit 1
}

trap stopall INT

function prechecks {
        if [ ! -f "$ROBUSTNESS_ENV_BUILD_DIR/bin/cluster-controller" ]
        then
                echo 'cluster controller not found'
                exit 1
        fi

        if [ ! -f "$ROBUSTNESS_ENV_BUILD_DIR/bin/provider-app" ]
        then
                echo 'provider-app not found'
                exit 1
        fi

        if [ ! -f "$ROBUSTNESS_ENV_BUILD_DIR/bin/consumer-app" ]
        then
                echo 'consumer-app not found'
                exit 1
        fi
}

function start_services {
        echo "Starting mosquitto"
        mosquitto > $ROBUSTNESS_ENV_RESULTS_DIR/mosquitto.log 2>&1 &
        MOSQUITTO_PID=$!
        echo "Mosquitto started with PID $MOSQUITTO_PID"
}

function stop_services {
        if [ -n "$MOSQUITTO_PID" ]
        then
            log "stopping mosquitto with PID $MOSQUITTO_PID"
            kill -9 $MOSQUITTO_PID
            MOSQUITTO_PID=""
        fi
        log "Mosquitto stopped"
}

function start_cluster_controller {
        if [ ! -d $ROBUSTNESS_ENV_BUILD_DIR -o ! -d $ROBUSTNESS_ENV_BUILD_DIR/bin ]
        then
                echo "C++ build directory or build/bin directory does not exist!"
                stopall
        fi
        log 'starting cluster controller'
        cd $ROBUSTNESS_ENV_BUILD_DIR/bin
        ./$CLUSTER_CONTROLLER_APP > $CLUSTER_CONTROLLER_LOG 2>&1 &
        CLUSTER_CONTROLLER_PID=$!
        log "Started cluster controller with PID $CLUSTER_CONTROLLER_PID"
}

function stop_cluster_controller {
        log 'stopping cluster controller'

        PID=`pgrep -f cluster-controller`
        if [ -z "$PID" ]
        then
                echo "No cluster-controller found."
        else
                echo "Found cluster-controller with pid $PID, about to stop it"
                kill -15 $PID
                while (test -d /proc/$PID)
                do
                        echo "PID $PID still alive. Waiting ..."
                        sleep 1
                done
                echo "PID $PID exited."
        fi
}

function start_provider {
        local DOMAIN_PREFIX=$1
        local NUM_RUN=$2
        local NUM_PROV=$3
        log 'starting provider-app with '$NUM_RUN' runtimes and '$NUM_PROV' providers on each runtime'
        cd $ROBUSTNESS_ENV_BUILD_DIR/bin
        ./$PROVIDER_APP -d$DOMAIN_PREFIX -r$NUM_RUN -p$NUM_PROV -t$PROV_THREAD_DELAY_MS > $PROVIDER_LOG 2>&1 &
        PROVIDER_PID=$!
        disown $PROVIDER_PID
        log "Started C++ provider with PID $PROVIDER_PID in directory $ROBUSTNESS_ENV_BUILD_DIR/bin"
        # Allow some time for startup
        sleep 5
}

function stop_provider {
        PID=`pgrep -f provider-app`
        if [ -z "$PID" ]
        then
                echo "No provider-app found."
                exit 1
        fi
        echo "Found provider-app with pid $PID, about to stop it"
        kill -15 $PID
        while (test -d /proc/$PID)
        do
                echo "PID $PID still alive. Waiting ..."
                sleep 1
        done
        echo "PID $PID exited."
}

function start_consumer {
        FULL_DOMAIN=$1
        log "starting consumer-app with domain $FULL_DOMAIN, $CONS_NUM_OF_TEST_CYCLES test cycles at $CONS_TEST_CYCLE_TIME seconds each under test case $CONS_TEST_CASE"
        CONSUMER_LOG=$ROBUSTNESS_ENV_RESULTS_DIR/$CONSUMER_APP$US$FULL_DOMAIN.log
        cd $ROBUSTNESS_ENV_BUILD_DIR/bin
        ./$CONSUMER_APP \
                -d $FULL_DOMAIN \
                -r $CONS_NUM_OF_RUNTIMES \
                -b $CONS_NUM_OF_PROXYBUILDERS \
                -p $CONS_NUM_OF_PROXIES \
                -c $CONS_TEST_CASE \
                --testcycles $CONS_NUM_OF_TEST_CYCLES \
                --testcycletime $CONS_TEST_CYCLE_TIME \
                --tdelay $CONS_THREAD_DELAY_MS \
                >> $CONSUMER_LOG 2>&1 &
        PID=$!
        log "Started C++ consumer app with PID $PID"
        CONSUMER_PIDS[$NUM_CONSUMERS]=$PID
        NUM_CONSUMERS=$NUM_CONSUMERS+1
}

function stop_all_consumers {
        log 'killing all consumers'
        for counter in "${!CONSUMER_PIDS[@]}"
        do
            pid=${CONSUMER_PIDS[$counter]}
            if [ -n "$pid" ]
            then
                log "stopping consumer with PID $pid"
                kill -9 $pid
                while (test -d /proc/$pid)
                do
                    echo "PID $pid still alive. Waiting ..."
                    sleep 1
                done
                log "consumer stopped"
                unset CONSUMER_PIDS[$counter]
            fi
        done
}

function check_all_processes {
        if ! pgrep -f $CLUSTER_CONTROLLER_APP > /dev/null
        then
                echo "$CLUSTER_CONTROLLER_APP not running"
                FAILED_TESTS=$((FAILED_TESTS + 1))
                stopall
        fi

        if ! pgrep -f $PROVIDER_APP > /dev/null
        then
                echo "$PROVIDER_APP not running"
                FAILED_TESTS=$((FAILED_TESTS + 1))
                stopall
        fi

        for pid in "${CONSUMER_PIDS[@]}"
        do
                if [ ! -d /proc/$pid ];
                then
                        wait $pid
                        Status=$?
                        if [ "$Status" -gt 0 ]
                        then
                                echo "Consumer with PID ${pid}; has been stopped - exit code: $Status"
                                FAILED_TESTS=$((FAILED_TESTS + 1))
                                stopall
                        fi
                fi
        done
}

function log {
        STRING=$1
        if [ -f $CLUSTER_CONTROLLER_LOG ]
        then
            echo "$STRING" >> $CLUSTER_CONTROLLER_LOG
        fi
        if [ -f $PROVIDER_LOG ]
        then
            echo "$STRING" >> $PROVIDER_LOG
        fi
        echo '####################################################'
        echo $STRING
        echo '####################################################'
}

function check_report {
        for f in $(find $SESS_RESULTS_DIR -name "$CONSUMER_APP*.log" -o -name "$PROVIDER_APP*.log");
        do
                if ! grep -q $APP_EXIT_MESSAGE $f;
                then
                        echo "$f : Could not find exit message."
                        FAILED_TESTS=$((FAILED_TESTS + 1))
                fi
        done
}

# TESTS

# check that the environment has been setup correctly
prechecks

# clean up
rm -fr $ROBUSTNESS_ENV_RESULTS_DIR
mkdir -p $ROBUSTNESS_ENV_RESULTS_DIR
cd $JOYNR_SOURCE_DIR/tests/robustness-test-env

#
# TEST 1
#################################

#INIT
    start_services
    start_cluster_controller
    start_provider $DOMAIN $PROV_NUM_OF_RUNTIMES $PROV_NUM_OF_PROVIDERS

    log '# starting consumer-apps with '$CONS_NUM_OF_RUNTIMES' runtimes and '$CONS_NUM_OF_PROXYBUILDERS' proxy builders on each runtime'
    # The number of consumers is based on the number of runtimes AND providers of the PROVIDER-APP!!!
    for run in `seq 0 $((PROV_NUM_OF_RUNTIMES - 1))`;
    do
        for prov in `seq 0 $((PROV_NUM_OF_PROVIDERS - 1))`;
        do
            start_consumer $DOMAIN$US$run$US$prov
        done
    done
    log "TEST 1 started"

#RUN
    # wait until all instances of consumer-app have finished their test cases
    while pgrep -f $CONSUMER_APP > /dev/null
    do
        sleep 1
        NUM_CONSUMERS=`pgrep -cf $CONSUMER_APP`
        echo "$NUM_CONSUMERS consumer-apps are still running"
        check_all_processes
    done

#END
    stop_provider
    stop_cluster_controller
    stop_services
    log "TEST 1 finished"

    check_report

# check for failed tests
if [ "$FAILED_TESTS" -gt 0 ]
then
        log 'TEST FAILED'
        exit $FAILED_TESTS
fi

# If this point is reached, the tests have been successfull.
log 'TEST SUCCEEDED'
echo
echo "Results stored under: $ROBUSTNESS_ENV_RESULTS_DIR/"
echo
exit $SUCCESS
