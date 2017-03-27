#!/bin/bash
###
# #%L
# %%
# Copyright (C) 2017 BMW Car IT GmbH
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

### PREREQUISITE ###
# The CPP memory usage tests (CPP_MEMORY_SYNC and CPP_MEMORY_ASYNC) assume that
# heaptrack is installed
# https://quickgit.kde.org/?p=heaptrack.git
# https://github.com/KDE/heaptrack
####################

# Shell script parameters
JOYNR_SOURCE_DIR=""
JOYNR_BUILD_DIR=""
PERFORMANCETESTS_BUILD_DIR=""
PERFORMANCETESTS_RESULTS_DIR=""
TESTCASE=""
VALIDITY=7200000 # 7200000ms = 2h
PERIOD=100 # 100ms

### Constants ###
DOMAINNAME="memory_usage_test_domain"

# If a test case has to transmit a string, the length will be determined by this constant
INPUTDATA_STRINGLENGTH=10

MQTT_BROKER_URI="tcp://localhost:1883"

# Process IDs for processes which must be terminated later
JETTY_PID=""
MOSQUITTO_PID=""
CLUSTER_CONTROLLER_PID=""
HEAPTRACK_CLUSTER_CONTROLLER_PID=""
PROVIDER_PID=""
HEAPTRACK_PROVIDER_PID=""
MEMORY_USAGE_TEST_PID=""

function waitUntilJettyStarted {
    started=0
    count=0
    while [ "$started" != "200" -a "$count" -lt "30" ]
    do
            sleep 2
            started=`curl -o /dev/null --silent --head --write-out '%{http_code}\n' \
            http://localhost:8080/bounceproxy/time/`
            let count+=1
    done
    if [ "$started" != "200" ]
    then
            # startup failed
            echo "ERROR: Failed to start jetty"
            exit
    fi
    echo "Jetty started."
    sleep 5
}

function startJetty {
    echo '### Starting jetty ###'

    JETTY_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/jetty_stdout.txt
    JETTY_STDERR=$PERFORMANCETESTS_RESULTS_DIR/jetty_stderr.txt

    cd $JOYNR_SOURCE_DIR/cpp/tests/
    mvn jetty:run-war --quiet 1>$JETTY_STDOUT 2>$JETTY_STDERR & JETTY_PID=$!

    waitUntilJettyStarted
}

function startMosquitto {
    echo '### Starting mosquitto ###'

    MOSQUITTO_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/mosquitto_stdout.txt
    MOSQUITTO_STDERR=$PERFORMANCETESTS_RESULTS_DIR/mosquitto_stderr.txt

    mosquitto -c /etc/mosquitto/mosquitto.conf 1>$MOSQUITTO_STDOUT 2>$MOSQUITTO_STDERR & MOSQUITTO_PID=$!

    sleep 2

    echo 'Mosquitto started'
}

function startCppClusterController {
    echo '### Starting C++ cluster controller '$TESTCASE' ###'

    CC_STDOUT=${PERFORMANCETESTS_RESULTS_DIR}/cc_${TESTCASE}_stdout.txt
    CC_STDERR=${PERFORMANCETESTS_RESULTS_DIR}/cc_${TESTCASE}_stderr.txt
    CC_CONFIG_FILE=${PERFORMANCETESTS_RESULTS_DIR}/resources/$1

    cd $PERFORMANCETESTS_RESULTS_DIR
    heaptrack $JOYNR_BUILD_DIR/bin/cluster-controller $CC_CONFIG_FILE 1>$CC_STDOUT 2>$CC_STDERR & HEAPTRACK_CLUSTER_CONTROLLER_PID=$!

    # Wait long enough in order to allow the cluster controller finish its start procedure
    sleep 5

    CLUSTER_CONTROLLER_PID=`ps -o pid,cmd | grep cluster-controller | grep -v heaptrack | cut -c1-7`

    echo "Memory usage analysis cluster controller started"
}

function startCppPerformanceTestProvider {
    echo '### Starting C++ performance test provider '$TESTCASE' ###'

    PROVIDER_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/provider_${TESTCASE}_stdout.txt
    PROVIDER_STDERR=$PERFORMANCETESTS_RESULTS_DIR/provider_${TESTCASE}_stderr.txt

    cd $PERFORMANCETESTS_RESULTS_DIR
    heaptrack $PERFORMANCETESTS_BUILD_DIR/bin/performance-provider-app $DOMAINNAME 1>$PROVIDER_STDOUT 2>$PROVIDER_STDERR & HEAPTRACK_PROVIDER_PID=$!

    # Wait long enough in order to allow the provider to finish the registration procedure
    sleep 5

    PROVIDER_PID=`ps -o pid,cmd | grep performance-provider-app | grep -v heaptrack | cut -c1-7`

    echo "Memory usage analysis provider started"
}

function startMemoryUsageTest {
    echo '### Starting memory usage analysis test '$TESTCASE' ###'

    TEST_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/test_memory_${TESTCASE}_stdout.txt
    TEST_STDERR=$PERFORMANCETESTS_RESULTS_DIR/test_memory_${TESTCASE}_stderr.txt

    cd $PERFORMANCETESTS_RESULTS_DIR
    heaptrack $PERFORMANCETESTS_BUILD_DIR/bin/memory-usage-consumer-app $DOMAINNAME $TESTCASE $VALIDITY $INPUTDATA_STRINGLENGTH 1>$TEST_STDOUT 2>$TEST_STDERR & MEMORY_USAGE_TEST_PID=$!
    wait $MEMORY_USAGE_TEST_PID
}

function stopJetty {
    echo "Stopping jetty"
    cd $JOYNR_SOURCE_DIR/cpp/tests/
    mvn jetty:stop --quiet
    wait $JETTY_PID
}

function stopMosquitto {
    echo "Stopping mosquitto"
    kill $MOSQUITTO_PID
    wait $MOSQUITTO_PID
}

function stopCppClusterController {
    echo "Killing C++ CC"
    kill $CLUSTER_CONTROLLER_PID
    wait $CLUSTER_CONTROLLER_PID
}

function stopAnyProvider {
    echo "Killing provider"
    # pkill is required if maven is used to start a provider. Maven launches the
    # provider as a child process, which seems not to be killed automatically along
    # with the parent process
    pkill -P $PROVIDER_PID
    kill $PROVIDER_PID
    wait $PROVIDER_PID
}

function echoUsage {
    echo "Usage: run-memory-usage-anaylsis.sh
-p <performance-build-dir> \
-r <performance-results-dir>
-s <joynr-source-dir> -y <joynr-build-dir> \
-t <CPP_MEMORY_SYNC|CPP_MEMORY_ASYNC> \
-v <validity-ms-for-cpp-memory>"
}

function checkDirExists {
    if [ -z "$1" ] || [ ! -d "$1" ]
    then
        echo "Directory \"$1\" does not exist"
        echoUsage
        exit 1
    fi
}

while getopts "p:r:s:t:v:y:" OPTIONS;
do
    case $OPTIONS in
        p)
            PERFORMANCETESTS_BUILD_DIR=${OPTARG%/}
            ;;
        r)
            PERFORMANCETESTS_RESULTS_DIR=${OPTARG%/}
            ;;
        s)
            JOYNR_SOURCE_DIR=${OPTARG%/}
            ;;
        t)
            TESTCASE=$OPTARG
            ;;
        v)
            VALIDITY=$OPTARG
            ;;
        y)
            JOYNR_BUILD_DIR=${OPTARG%/}
            ;;
        \?)
            echoUsage
            exit 1
            ;;
    esac
done

if [ "$TESTCASE" != "CPP_MEMORY_ASYNC" ] && [ "$TESTCASE" != "CPP_MEMORY_SYNC" ]
then
    echo "\"$TESTCASE\" is not a valid testcase"
    echo "-t option can be either CPP_MEMORY_SYNC OR CPP_MEMORY_ASYNC"
    echoUsage
    exit 1
fi

checkDirExists $JOYNR_SOURCE_DIR
checkDirExists $JOYNR_BUILD_DIR
checkDirExists $PERFORMANCETESTS_BUILD_DIR
checkDirExists $PERFORMANCETESTS_RESULTS_DIR

cp -a ${PERFORMANCETESTS_BUILD_DIR}/bin/resources ${PERFORMANCETESTS_RESULTS_DIR}
startJetty
startCppClusterController cc-default-messaging.settings
startCppPerformanceTestProvider
startMemoryUsageTest
stopAnyProvider
stopCppClusterController
stopJetty
###
# The heaptrack output can be visualized with massif-visualizer
# (shipped together with heaptrack) after it has been converted
# to valgrind-massif format with heaptrack_print -M
###
cd $PERFORMANCETESTS_RESULTS_DIR
rm -fr resources
cp heaptrack.cluster-controller.${HEAPTRACK_CLUSTER_CONTROLLER_PID}.gz heaptrack.cluster-controller.${HEAPTRACK_CLUSTER_CONTROLLER_PID}.org.gz
cp heaptrack.performance-provider-app.${HEAPTRACK_PROVIDER_PID}.gz heaptrack.performance-provider-app.${HEAPTRACK_PROVIDER_PID}.org.gz
cp heaptrack.memory-usage-consumer-app.${MEMORY_USAGE_TEST_PID}.gz heaptrack.memory-usage-consumer-app.${MEMORY_USAGE_TEST_PID}.org.gz
heaptrack_print heaptrack.cluster-controller.${HEAPTRACK_CLUSTER_CONTROLLER_PID}.gz --massif-threshold 1 --massif-detailed-freq 100 -M heaptrack.cluster-controller.${HEAPTRACK_CLUSTER_CONTROLLER_PID}.th1freq100.massif
heaptrack_print heaptrack.performance-provider-app.${HEAPTRACK_PROVIDER_PID}.gz --massif-threshold 1 --massif-detailed-freq 100 -M heaptrack.performance-provider-app.${HEAPTRACK_PROVIDER_PID}.th1freq100.massif
heaptrack_print heaptrack.memory-usage-consumer-app.${MEMORY_USAGE_TEST_PID}.gz --massif-threshold 1 --massif-detailed-freq 100 -M heaptrack.memory-usage-consumer-app.${MEMORY_USAGE_TEST_PID}.th1freq100.massif

