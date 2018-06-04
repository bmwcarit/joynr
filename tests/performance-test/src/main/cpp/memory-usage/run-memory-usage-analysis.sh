#!/bin/bash
###
# #%L
# %%
# Copyright (C) 2018 BMW Car IT GmbH
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
# The CPP memory usage tests (CPP_MEMORY_SYNC and CPP_MEMORY_ASYNC) assumes that valgrind is installed
####################

# Shell script parameters
JOYNR_SOURCE_DIR=""
JOYNR_BUILD_DIR=""
PERFORMANCETESTS_BUILD_DIR=""
PERFORMANCETESTS_RESULTS_DIR=""
TESTCASE=""
VALGRIND_COMMAND_PARMS="--leak-check=full --show-leak-kinds=all"
VALIDITY=7200000 # 7200000ms = 2h
PERIOD=100 # 100ms

### Constants ###
DOMAINNAME="memory_usage_test_domain"

# If a test case has to transmit a string, the length will be determined by this constant
INPUTDATA_STRINGLENGTH=10

MQTT_BROKER_URI="tcp://localhost:1883"

# Process IDs for processes which must be terminated later
MOSQUITTO_PID=""
CLUSTER_CONTROLLER_PID=""
PROVIDER_PID=""
MEMORY_USAGE_TEST_PID=""

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

    cd $PERFORMANCETESTS_RESULTS_DIR
    valgrind $VALGRIND_COMMAND_PARMS $JOYNR_BUILD_DIR/bin/cluster-controller 1>$CC_STDOUT 2>$CC_STDERR &
    CLUSTER_CONTROLLER_PID=$!

    # Wait long enough in order to allow the cluster controller finish its start procedure
    sleep 5
}

function startCppPerformanceTestProvider {
    echo '### Starting C++ performance test provider '$TESTCASE' ###'

    PROVIDER_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/provider_${TESTCASE}_stdout.txt
    PROVIDER_STDERR=$PERFORMANCETESTS_RESULTS_DIR/provider_${TESTCASE}_stderr.txt

    cd $PERFORMANCETESTS_RESULTS_DIR
    valgrind $VALGRIND_COMMAND_PARMS $PERFORMANCETESTS_BUILD_DIR/bin/performance-provider-app -d $DOMAINNAME 1>$PROVIDER_STDOUT 2>$PROVIDER_STDERR &
    PROVIDER_PID=$!

    # Wait long enough in order to allow the provider to finish the registration procedure
    sleep 5
}

function startMemoryUsageTest {
    echo '### Starting memory usage analysis test '$TESTCASE' ###'

    TEST_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/test_memory_${TESTCASE}_stdout.txt
    TEST_STDERR=$PERFORMANCETESTS_RESULTS_DIR/test_memory_${TESTCASE}_stderr.txt

    cd $PERFORMANCETESTS_RESULTS_DIR
    valgrind $VALGRIND_COMMAND_PARMS $PERFORMANCETESTS_BUILD_DIR/bin/memory-usage-consumer-app $DOMAINNAME $TESTCASE $VALIDITY $INPUTDATA_STRINGLENGTH 1>$TEST_STDOUT 2>$TEST_STDERR &
    MEMORY_USAGE_TEST_PID=$!
    wait $MEMORY_USAGE_TEST_PID
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
-p <performance-build-dir>
-r <performance-results-dir>
-s <joynr-source-dir> -y <joynr-build-dir>
-t <CPP_MEMORY_SYNC|CPP_MEMORY_ASYNC>
-v <validity-ms-for-cpp-memory>

This tool creates several files in the folder specified with -r. They are:
<cc/provider/test_memory>_CPP_MEMORY_<SYNC/ASYNC>_<stderr/stdout>.txt: \
The output of the cluster controller / provider / consumer.
mosquitto-<stdout/stderr>.txt: Output of the mosquitto broker. \
If these files do not exist, make sure that mosquitto is installed, not running and can be executed manually. \
"
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

echo "Performed startup checks. Running tests..."

export JOYNR_LOG_LEVEL=TRACE
cp -a ${PERFORMANCETESTS_BUILD_DIR}/bin/resources ${PERFORMANCETESTS_RESULTS_DIR}
startMosquitto
startCppClusterController
startCppPerformanceTestProvider
startMemoryUsageTest

echo "Test finished. Shutting down provider and cluster controller."
stopAnyProvider
stopCppClusterController
stopMosquitto

rm -fr resources
