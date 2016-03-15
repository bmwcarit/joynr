#!/bin/bash
###
# #%L
# %%
# Copyright (C) 2016 BMW Car IT GmbH
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
# The JavaScript test assumes that 'npm install' was executed within the
# /test/performance directory
####################

# Shell script parameters
JOYNR_SOURCE_DIR=""
JOYNR_BUILD_DIR=""
PERFORMANCETESTS_BUILD_DIR=""
PERFORMANCETESTS_RESULTS_DIR=""
TESTCASE=""

### Constants ###
DOMAINNAME="performance_test_domain"

# If a test case uses a java consumer, some warmup runs are required in order
# to force the java runtime to perform all JIT optimizations
JAVA_WARMUPS=50

# For test cases with a single consumer, this constant stores the number of messages which
# will be transmitted during the test
SINGLECONSUMER_RUNS=10000

# For test cases with several consumers, this constant stores how many consumer instances will
# be created
MULTICONSUMER_NUMINSTANCES=5

# For test cases with several consumers, this constant stores how many messages a single
# consumer transmits
MULTICONSUMER_RUNS=2000

# If a test case has to transmit a string, the length will be determined by this constant
INPUTDATA_STRINGLENGTH=10

# If a test case has to transmit a byte array, the length will be determined by this constant
INPUTDATA_BYTEARRAYSIZE=100

# Process IDs for processes which must be terminated later
JETTY_PID=""
CLUSTER_CONTROLLER_PID=""
PROVIDER_PID=""

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

function startClusterController {
    echo '### Starting cluster controller ###'

    CC_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/cc_stdout.txt
    CC_STDERR=$PERFORMANCETESTS_RESULTS_DIR/cc_stderr.txt
    CC_CONFIG_FILE=$PERFORMANCETESTS_BUILD_DIR/bin/resources/cc-default-messaging.settings

    cd $JOYNR_BUILD_DIR/bin/
    ./cluster-controller $CC_CONFIG_FILE 1>$CC_STDOUT 2>$CC_STDERR & CLUSTER_CONTROLLER_PID=$!

    # Wait long enough in order to allow the cluster controller finish its start procedure
    sleep 5

    echo "Cluster controller started"
}

function startPerformanceTestProvider {
    echo '### Starting performance test provider ###'

    PROVIDER_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/provider_stdout.txt
    PROVIDER_STDERR=$PERFORMANCETESTS_RESULTS_DIR/provider_stderr.txt

    cd $PERFORMANCETESTS_BUILD_DIR/bin/
    ./performance-provider-app $DOMAINNAME 1>$PROVIDER_STDOUT 2>$PROVIDER_STDERR & PROVIDER_PID=$!

    # Wait long enough in order to allow the provider to finish the registration procedure
    sleep 5

    # Workaround: We need to start the provider twice. Otherwise, the registration process
    # will not succeed.
    kill $PROVIDER_PID
    wait $PROVIDER_PID
    ./performance-provider-app $DOMAINNAME 1>$PROVIDER_STDOUT 2>$PROVIDER_STDERR & PROVIDER_PID=$!
    sleep 5

    echo "Performance test provider started"
}

function performSingleJavaConsumerTest {
    MODE_PARAM=$1
    TESTCASE_PARAM=$2
    STDOUT_PARAM=$3
    REPORTFILE_PARAM=$4

    CONSUMERCLASS="joynr.tests.performance.PerformanceTestConsumerApplication"
    CONSUMERARGS="-d $DOMAINNAME -w $JAVA_WARMUPS -r $SINGLECONSUMER_RUNS \
                  -s $MODE_PARAM -t $TESTCASE_PARAM -bs $INPUTDATA_BYTEARRAYSIZE \
                  -sl $INPUTDATA_STRINGLENGTH"

    mvn exec:java -o -Dexec.mainClass="$CONSUMERCLASS" \
       -Dexec.args="$CONSUMERARGS" 1>>$STDOUT_PARAM 2>>$REPORTFILE_PARAM
}

function performMultiJavaConsumerTest {
    MODE_PARAM=$1
    TESTCASE_PARAM=$2
    STDOUT_PARAM=$3
    REPORTFILE_PARAM=$4

    CONSUMERCLASS="joynr.tests.performance.PerformanceTestConsumerApplication"
    CONSUMERARGS="-d $DOMAINNAME -w $JAVA_WARMUPS -r $MULTICONSUMER_RUNS \
                  -s $MODE_PARAM -t $TESTCASE_PARAM -bs $INPUTDATA_BYTEARRAYSIZE \
                  -sl $INPUTDATA_STRINGLENGTH"

    TEST_PIDS=()
    for (( i=0; i < $MULTICONSUMER_NUMINSTANCES; ++i ))
    do
        echo "Launching consumer $i ..."
        mvn exec:java -o -Dexec.mainClass="$CONSUMERCLASS" \
           -Dexec.args="$CONSUMERARGS" 1>>$STDOUT_PARAM 2>>$REPORTFILE_PARAM & CUR_PID=$!
        TEST_PIDS+=$CUR_PID
        TEST_PIDS+=" "
    done

    echo "Waiting until consumers finished ..."
    wait $TEST_PIDS
}

function performJsConsumerTest {
    STDOUT_PARAM=$1
    REPORTFILE_PARAM=$2

    npm run-script --performance-test:runs=$SINGLECONSUMER_RUNS \
                   --performance-test:domain=$DOMAINNAME \
                   --performance-test:stringlength=$INPUTDATA_STRINGLENGTH \
                   --performance-test:bytearraylength=$INPUTDATA_BYTEARRAYSIZE \
                     jsconsumertest 1>>$STDOUT_PARAM 2>>$REPORTFILE_PARAM
}

function startPerformanceTest {
    echo "### Starting performance tests ###"

    REPORTFILE=$PERFORMANCETESTS_RESULTS_DIR/performancetest_result.txt
    STDOUT=$PERFORMANCETESTS_RESULTS_DIR/consumer_stdout.txt

    cd $JOYNR_SOURCE_DIR/tests/performance
    rm -f $STDOUT
    rm -f $REPORTFILE

    if [ "$TESTCASE" == "JAVA_ASYNC" ] || [ "$TESTCASE" == "ALL" ]
    then
        echo "Testcase: JAVA ASYNC STRING" | tee -a $REPORTFILE
        performSingleJavaConsumerTest "ASYNC" "SEND_STRING" $STDOUT $REPORTFILE

        echo "Testcase: JAVA ASYNC STRUCT" | tee -a $REPORTFILE
        performSingleJavaConsumerTest "ASYNC" "SEND_STRUCT" $STDOUT $REPORTFILE

        echo "Testcase: JAVA ASYNC BYTEARRAY" | tee -a $REPORTFILE
        performSingleJavaConsumerTest "ASYNC" "SEND_BYTEARRAY" $STDOUT $REPORTFILE
    fi

    if [ "$TESTCASE" == "JAVA_SYNC" ] || [ "$TESTCASE" == "ALL" ]
    then
        echo "Testcase: JAVA SYNC STRING" | tee -a $REPORTFILE
        performSingleJavaConsumerTest "SYNC" "SEND_STRING" $STDOUT $REPORTFILE

        echo "Testcase: JAVA SYNC STRUCT" | tee -a $REPORTFILE
        performSingleJavaConsumerTest "SYNC" "SEND_STRUCT" $STDOUT $REPORTFILE

        echo "Testcase: JAVA SYNC BYTEARRAY" | tee -a $REPORTFILE
        performSingleJavaConsumerTest "SYNC" "SEND_BYTEARRAY" $STDOUT $REPORTFILE
    fi

    if [ "$TESTCASE" == "JAVA_MULTICONSUMER" ] || [ "$TESTCASE" == "ALL" ]
    then
        echo "Testcase: JAVA ASYNC STRING / MULTIPLE CONSUMERS" | tee -a $REPORTFILE
        performMultiJavaConsumerTest "ASYNC" "SEND_STRING" $STDOUT $REPORTFILE

        echo "Testcase: JAVA ASYNC STRUCT / MULTIPLE CONSUMERS" | tee -a $REPORTFILE
        performMultiJavaConsumerTest "ASYNC" "SEND_STRUCT" $STDOUT $REPORTFILE

        echo "Testcase: JAVA ASYNC BYTEARRAY / MULTIPLE CONSUMERS" | tee -a $REPORTFILE
        performMultiJavaConsumerTest "ASYNC" "SEND_BYTEARRAY" $STDOUT $REPORTFILE
    fi

    if [ "$TESTCASE" == "JS_ASYNC" ] || [ "$TESTCASE" == "ALL" ]
    then
        echo "Testcase: JS ASYNC" | tee -a $REPORTFILE
        performJsConsumerTest $STDOUT $REPORTFILE
    fi

    echo "### Performance tests finished ####"
}

function stopAllProcesses {
    echo "### Stopping all processes ###"

    echo "Killing provider"
    kill $PROVIDER_PID
    wait $PROVIDER_PID
    PROVIDER_PID=""

    echo "Killing CC"
    kill $CLUSTER_CONTROLLER_PID
    wait $CLUSTER_CONTROLLER_PID
    CLUSTER_CONTROLLER_PID=""

    echo "Stopping jetty"
    cd $JOYNR_SOURCE_DIR/cpp/tests/
    mvn jetty:stop --quiet
    wait $JETTY_PID
    JETTY_PID=""
}

function echoUsage {
    echo "Usage: run-performance-tests.sh -y <joynr-build-dir> -p <performance-build-dir> \
-s <joynr-source-dir> -r <performance-results-dir> \
-t <JAVA_SYNC|JAVA_ASYNC|JAVA_MULTICONSUMER|JS_ASYNC|ALL>"
}

function checkDirExists {
    if [ -z "$1" ] || [ ! -d "$1" ]
    then
        echo "Directory \"$1\" does not exist"
        echoUsage
        exit 1
    fi
}

while getopts "y:p:s:r:t:" OPTIONS;
do
    case $OPTIONS in
        p)
            PERFORMANCETESTS_BUILD_DIR=${OPTARG%/}
            ;;
        y)
            JOYNR_BUILD_DIR=${OPTARG%/}
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
        \?)
            echoUsage
            exit 1
            ;;
    esac
done

if [ "$TESTCASE" != "JAVA_SYNC" ] && [ "$TESTCASE" != "JAVA_ASYNC" ] && \
   [ "$TESTCASE" != "JAVA_MULTICONSUMER" ] && [ "$TESTCASE" != "ALL" ] && \
   [ "$TESTCASE" != "JS_ASYNC" ]
then
    echo "\"$TESTCASE\" is not a valid testcase"
    echo "-t option can be either JAVA_SYNC, JAVA_ASYNC, JAVA_MULTICONSUMER, JS_ASYNC OR ALL"
    echoUsage
    exit 1
fi

checkDirExists $JOYNR_SOURCE_DIR
checkDirExists $JOYNR_BUILD_DIR
checkDirExists $PERFORMANCETESTS_BUILD_DIR
checkDirExists $PERFORMANCETESTS_RESULTS_DIR

startJetty
startClusterController
startPerformanceTestProvider
startPerformanceTest
stopAllProcesses
