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

# PARAMETERS
#################################
DOMAIN="joynr-robustness-test-domain"
PROVIDER_APP="robustness-tests-provider-cc"
CONSUMER_APP="robustness-tests-consumer-mqtt-cc"

# log file
APP_LOG=$TEST_RESULTS_DIR/App.log

# in case of interrupts, forcibly kill background processes
function stopall {
    deinit
    exit 1
}

trap stopall INT

log "ENVIRONMENT"
env

# FUNCTIONS
#################################

function prechecks {

    # Directories checking
    # check that the results directory exists
    if [ ! -d "$TEST_RESULTS_DIR" ]; then
        mkdir -p $TEST_RESULTS_DIR
        echo "Result directory created: $TEST_RESULTS_DIR"
    fi

    # provider-app
    if [ ! -f "$TEST_BUILD_DIR/bin/$PROVIDER_APP" ]; then
        echo "$PROVIDER_APP not found in $TEST_BUILD_DIR/bin - aborting test"
        exit 1
    fi
    # consumer-app
    if [ ! -f "$TEST_BUILD_DIR/bin/$CONSUMER_APP" ]; then
        echo "$CONSUMER_APP not found in $TEST_BUILD_DIR/bin - aborting test"
        exit 1
    fi
}

function deinit {

    stop_provider
    stop_consumer
    stop_services
}

function start_services {

    log "starting services"

    start_mosquitto
    start_payara

    log "services started"
    sleep 5
}

function stop_services {

    log "stopping services"

    stop_payara
    stop_mosquitto

    log "services stopped"
}

function startExternalCommunication {
    echo "# SIGUSR1 - startExternalCommunication"
    kill -SIGUSR1 $1
}

function stopExternalCommunication {
    echo "# SIGUSR2 - stopExternalCommunication"
    kill -SIGUSR2 $1
}

function start_provider {
    log "starting provider"

    cd $TEST_BUILD_DIR/bin
    ./$PROVIDER_APP $DOMAIN >> $APP_LOG 2>&1 &
    PROVIDER_PID=$!
    log "Provider started with PID $PROVIDER_PID"
    # Allow some time for startup
    sleep 5
}

function stop_provider {
    log "stopping provider"
    PID=`pgrep -f $PROVIDER_APP`
    if [ -z "$PID" ]
    then
        echo "No $PROVIDER_APP found."
    else
        echo "Found $PROVIDER_APP with pid $PID, about to stop it"
        kill -15 $PID
        while (test -d /proc/$PID)
        do
                echo "PID $PID still alive. Waiting ..."
                sleep 1
        done
        echo "PID $PID exited."
    fi
}

function start_consumer {
    log "starting consumer"

    cd $TEST_BUILD_DIR/bin
    ./$CONSUMER_APP $DOMAIN >> $APP_LOG 2>&1 &
    CONSUMER_PID=$!
    log "Consumer started with PID $CONSUMER_PID"
}

function stop_consumer {
    log "stopping consumer"
    PID=`pgrep -f $CONSUMER_APP`
    if [ -z "$PID" ]
    then
        echo "No $CONSUMER_APP found."
    else
        echo "Found $CONSUMER_APP with pid $PID, about to stop it"
        kill -15 $PID
        while (test -d /proc/$PID)
        do
                echo "PID $PID still alive. Waiting ..."
                sleep 1
        done
        echo "PID $PID exited."
    fi
}

function check_log_file {
    COUNT=`grep -c "RobustnessTestMqttConnectionReset First Pong" $APP_LOG`
    if [ "$COUNT" != $1 ]
    then
        log "TEST finished with errors - First ping message could not be sent!"
        let FAILED_TESTS+=1
    else
        COUNT=`grep -c "RobustnessTestMqttConnectionReset Third Pong" $APP_LOG`
        if [ "$COUNT" != $1 ]
        then
            log "TEST finished with errors - Third ping message could not be sent!"
            let FAILED_TESTS+=1
        else
            COUNT=`grep -c "OK ] RobustnessTestMqttConnectionReset" $APP_LOG`
            if [ "$COUNT" != $1 ]
            then
                log "TEST finished with errors"
                let FAILED_TESTS+=1
            fi
        fi
    fi
}

# INITIALISE
#################################
prechecks
PAUSE_DURATION=4

start_services
start_provider

# Scenario A
#################################
#INIT
start_consumer
log "Scenario A (killing the MQTT process) started"

sleep 1

#RUN
#start and stop the MQTT/Mosquitto service
stop_mosquitto
sleep 3
start_mosquitto
log "start services"
sleep $PAUSE_DURATION

#END
log "Scenario A finished"
sleep $PAUSE_DURATION

check_log_file 1
stop_consumer

# Scenario B
#################################
#INIT
start_consumer

log "Scenario B (killing the connection between CC and MQTT) started"

sleep 1

#RUN
#stop and start the communications between MQTT and cc by sending POSIX signals to the cc
log "stop External Communication"
stopExternalCommunication $CONSUMER_PID
sleep $PAUSE_DURATION
log "start External Communication"
startExternalCommunication $CONSUMER_PID
sleep $PAUSE_DURATION

#END
log "Scenario B finished"
sleep $PAUSE_DURATION

check_log_file 2
stop_consumer

# Scenario C (Mixed)
#################################
#INIT
start_consumer

log "Scenario C (killing the connection between CC and MQTT and the MQTT process) started"

sleep 1

#RUN
#stop the communication between MQTT broker and cc by sending POSIX signal to the cc
log "stop External Communication"
stopExternalCommunication $CONSUMER_PID
sleep 1

#stop the MQTT/Mosquitto service
stop_mosquitto
sleep 1

#restart the communication between MQTT broker and cc by sending POSIX signal to the cc
log "start External Communication"
startExternalCommunication $CONSUMER_PID
sleep 1

#restart the MQTT/Mosquitto service
start_mosquitto
log "start services"
sleep $PAUSE_DURATION

#END
log "Scenario C finished"
sleep $PAUSE_DURATION

check_log_file 3
stop_consumer

# FINISH
#################################
deinit

echo "Provider and consumer log written to: $APP_LOG"
echo
if [ "$FAILED_TESTS" -gt 0 ]
then
    log 'TEST FAILED'
    exit $FAILED_TESTS
fi

# If this point is reached, the tests have been successfull.
log 'TEST SUCCEEDED'
exit 0
