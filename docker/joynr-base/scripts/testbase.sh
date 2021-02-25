#!/bin/bash

# if CI environment, source global settings
if [ -f /data/scripts/global.sh ]
then
    source /data/scripts/global.sh
fi

# define variables
SUCCESS=0
FAILED_TESTS=0
JOYNR_SOURCE_DIR=""
TEST_BUILD_DIR=""
TEST_RESULTS_DIR=""
MOSQUITTO_PID=""

trap stopall INT

parse_arguments () {
    log "parsing arguments for test script: $0"

    while getopts "b:l:s:r:" OPTIONS;
    do
        case $OPTIONS in
            b)
                TEST_BUILD_DIR=$OPTARG
                log "TEST_BUILD_DIR has been set to $TEST_BUILD_DIR"
                ;;
            l)
                JOYNR_LOG_LEVEL=$OPTARG
                log "JOYNR_LOG_LEVEL has been set to $JOYNR_LOG_LEVEL"
                ;;
            r)
                TEST_RESULTS_DIR=$OPTARG
                log "TEST_RESULTS_DIR has been set to $TEST_RESULTS_DIR"
                ;;
            s)
                JOYNR_SOURCE_DIR=$OPTARG
                if [ ! -d "$JOYNR_SOURCE_DIR" ]
                then
                    log "Directory $JOYNR_SOURCE_DIR does not exist!"
                    exit 1
                else
                    log "JOYNR_SOURCE_DIR has been set to $JOYNR_SOURCE_DIR"
                fi
                ;;
            \?)
                echo "Illegal option found."
                echo "Synopsis: $0 [-b <test-build-dir>] [-l <loglevel>] [-r <test-results-dir>] [-s <joynr-source-dir>]"
                exit 1
            ;;
        esac
    done
}

folder_prechecks () {
    if [ -z "$JOYNR_SOURCE_DIR" ]
    then
        JOYNR_SOURCE_DIR="/data/src"
        log "JOYNR_SOURCE_DIR has been set to $JOYNR_SOURCE_DIR"
    fi
    if [ -z "$TEST_BUILD_DIR" ]
    then
        TEST_BUILD_DIR="/data/build/tests"
        log "TEST_BUILD_DIR has been set to $TEST_BUILD_DIR"
    fi

    if [ -z "$TEST_RESULTS_DIR" ]
    then
        TEST_RESULTS_DIR=$JOYNR_SOURCE_DIR/tests/results
        log "TEST_RESULTS_DIR has been set to $TEST_RESULTS_DIR"
    fi

    if [ ! -f "$TEST_BUILD_DIR/bin/cluster-controller" ]
    then
        log 'C++ environment not built'
        exit 1
    fi

}

kill_and_wait () {
    kill $1 2> /dev/null && wait $1 2> /dev/null
}

start_payara () {

    cd $JOYNR_SOURCE_DIR/java
    JOYNR_VERSION=$(mvn -q -Dexec.executable='echo' -Dexec.args='${project.version}' --non-recursive org.codehaus.mojo:exec-maven-plugin:exec)
    MVN_REPO=${REPODIR:=/home/$(whoami)/.m2/repository}

    DISCOVERY_DIRECTORY_WAR_FILE="$MVN_REPO/io/joynr/java/backend-services/discovery-directory-jee/$JOYNR_VERSION/discovery-directory-jee-$JOYNR_VERSION.war"

    if [ ! -f $DISCOVERY_DIRECTORY_WAR_FILE ]; then
      echo DISCOVERY_DIRECTORY_WAR_FILE=$DISCOVERY_DIRECTORY_WAR_FILE
      log "Cannot run tests: path DISCOVERY_DIRECTORY_WAR_FILE does not exist.\nMVN_REPO=$MVN_REPO"
      exit 1
    fi

    log "starting payara with DISCOVERY_DIRECTORY_WAR_FILE=$DISCOVERY_DIRECTORY_WAR_FILE"

    $JOYNR_SOURCE_DIR/docker/joynr-base/scripts/start-payara.sh -w $DISCOVERY_DIRECTORY_WAR_FILE
}

stop_payara () {

    log "starting payara with DISCOVERY_DIRECTORY_WAR_FILE=$DISCOVERY_DIRECTORY_WAR_FILE"
    $JOYNR_SOURCE_DIR/docker/joynr-base/scripts/stop-payara.sh
}

start_mosquitto () {

    log "starting Mosquitto"
    mosquitto -v > $TEST_RESULTS_DIR/mosquitto_$(date "+%Y-%m-%d-%H:%M:%S").log 2>&1 &
    MOSQUITTO_PID=$!
    log "Mosquitto started with PID $MOSQUITTO_PID"
}

stop_mosquitto () {

    if [ -n "$MOSQUITTO_PID" ]
    then
        log "stopping mosquitto with PID $MOSQUITTO_PID"
        kill_and_wait $MOSQUITTO_PID
    fi
    log "Mosquitto stopped"
}

stop_cluster_controller () {

    echo '####################################################'
    echo '# killing C++ clustercontroller'
    echo '####################################################'
    PID=`pgrep -f cluster-controller`
    if [ -z "$PID" ]
    then
        echo "No cluster-controller found."
        return
    fi
    echo "Found cluster-controller with pid $PID, about to kill it"
    kill_and_wait $PID
    echo "cluster-controller $PID exited."
}

stop_providers() {
    for PID in $PIDS
    do
        echo "Found provider with pid $PID, about to kill it"
        kill_and_wait $PID
        echo "provider $PID exited."
    done
}

stop_cpp_providers () {

    echo '####################################################'
    echo '# stopping all cpp providers'
    echo '####################################################'
    PIDS=`pgrep -f provider`
    if [ -z "$PIDS" ]
    then
        echo "No provider found."
        return
    fi
    stop_providers $PIDS
}

stop_java_providers () {

    echo '####################################################'
    echo '# stopping all java providers'
    echo '####################################################'
    PIDS=`pgrep -f ProviderApplication`
    if [ -z "$PIDS" ]
    then
        echo "No provider found."
        return
    fi
    stop_providers $PIDS
}

stop_javascript_providers () {

    echo '####################################################'
    echo '# stopping all javascript providers'
    echo '####################################################'
    PIDS=`pgrep -f provider.js`
    if [ -z "$PIDS" ]
    then
        echo "No provider found."
        return
    fi
    stop_providers $PIDS
}

print_test_failed () {
    echo '####################################################'
    echo '# TEST FAILED'
    echo '####################################################'
}

