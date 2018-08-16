#!/bin/bash
###
# #%L
# %%
# Copyright (C) 2016 - 2017 BMW Car IT GmbH
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
JETTY_PATH=""
JOYNR_BIN_DIR=""
PERFORMANCETESTS_BIN_DIR=""
PERFORMANCETESTS_SOURCE_DIR=""
PERFORMANCETESTS_RESULTS_DIR=""
TESTCASE=""
USE_MAVEN=ON # Indicates whether java applications shall be started with maven or as standalone apps
MOSQUITTO_CONF=""
USE_NPM=ON # Indicates whether npm will be used to launch javascript applications.

### Constants ###
DOMAINNAME="performance_test_domain"

# If a test case uses a java consumer, some warmup runs are required in order
# to force the java runtime to perform all JIT optimizations
JAVA_WARMUPS=50

# For test cases with a single consumer, this constant stores the number of messages which
# will be transmitted during the test
SINGLECONSUMER_RUNS=5000

# For test cases with several consumers, this constant stores how many consumer instances will
# be created
MULTICONSUMER_NUMINSTANCES=5

# For test cases with several consumers, this constant stores how many messages a single
# consumer transmits
MULTICONSUMER_RUNS=200

# If a test case has to transmit a string, the length will be determined by this constant
INPUTDATA_STRINGLENGTH=100

# If a test case has to transmit a byte array, the length will be determined by this constant
INPUTDATA_BYTEARRAYSIZE=100

MQTT_BROKER_URI="tcp://localhost:1883"

# Process IDs for processes which must be terminated later
JETTY_PID=""
MOSQUITTO_PID=""
CLUSTER_CONTROLLER_PID=""

# Stores the PID of the launched provider. Will be empty if the provider is deployed to a payara
# server as a JEE app. In this case PROVIDER_JEE_APP_NAME can be used to identify the provider.
PROVIDER_PID=""

# If a provider is deployed to a payara server, this variable will store the application
# name. If this variable is set, PROVIDER_PID will store an empty string.
PROVIDER_JEE_APP_NAME=""

# arguments which are passed to the C++ cluster-controller
ADDITIONAL_CC_ARGS=""

SKIPBYTEARRAYSIZETIMESK=false

# Select backend service protocol
BACKEND_SERVICES="MQTT"

function getCpuTime {
    PID=$1
    UTIME=$(cat /proc/$PID/stat | cut -d" " -f 14)
    STIME=$(cat /proc/$PID/stat | cut -d" " -f 15)
    SUM_CPU_TIME=$(echo $UTIME $STIME | awk '{ printf "%f", $1 + $2 }')
    echo $SUM_CPU_TIME
}

function minus {
    echo $1 $2 | awk '{ printf "%f", $1 - $2 }'
}

function startMeasureCpuUsage {
    START_TIME=$(cat /proc/uptime | cut -d" " -f 1)

    #get cluster-controller & provider initial cputime
    if [ -n "$JETTY_PID" ]
    then
        JETTY_CPU_TIME_1=$(getCpuTime $JETTY_PID)
    fi
    if [ -n "$MOSQUITTO_PID" ]
    then
        MOSQUITTO_CPU_TIME_1=$(getCpuTime $MOSQUITTO_PID)
    fi
    CC_CPU_TIME_1=$(getCpuTime $CLUSTER_CONTROLLER_PID)
    PROVIDER_CPU_TIME_1=$(getCpuTime $PROVIDER_PID)
}

function stopMeasureCpuUsage {
    REPORTFILE_PARAM=$1

    echo "----- overall statistics -----" | tee -a $REPORTFILE_PARAM
    echo "startTime:         $START_TIME" | tee -a $REPORTFILE_PARAM
    if [ -n "$JETTY_PID" ]
    then
        JETTY_CPU_TIME_2=$(getCpuTime $JETTY_PID)
        JETTY_CPU_TIME=$(minus $JETTY_CPU_TIME_2 $JETTY_CPU_TIME_1)
    else
        JETTY_CPU_TIME=0
    fi
    if [ -n "$MOSQUITTO_PID" ]
    then
        MOSQUITTO_CPU_TIME_2=$(getCpuTime $MOSQUITTO_PID)
        MOSQUITTO_CPU_TIME=$(minus $MOSQUITTO_CPU_TIME_2 $MOSQUITTO_CPU_TIME_1)
    else
        MOSQUITTO_CPU_TIME=0
    fi
    CC_CPU_TIME_2=$(getCpuTime $CLUSTER_CONTROLLER_PID)
    PROVIDER_CPU_TIME_2=$(getCpuTime $PROVIDER_PID)

    END_TIME=$(cat /proc/uptime | cut -d" " -f 1)
    echo "endTime:           $END_TIME" | tee -a $REPORTFILE_PARAM
    WALL_CLOCK_DURATION=$(echo $END_TIME $START_TIME | awk '{ printf "%f", 1000*($1 - $2) }')
    echo "wallClockDuration: $WALL_CLOCK_DURATION" | tee -a $REPORTFILE_PARAM

    # cluster-controller & provider net cputime
    CC_CPU_TIME=$(minus $CC_CPU_TIME_2 $CC_CPU_TIME_1)
    PROVIDER_CPU_TIME=$(minus $PROVIDER_CPU_TIME_2 $PROVIDER_CPU_TIME_1)

    #get child cputime
    #here we need to access different values than for "non-waited-for" processes
    TEST_UTIME=$(cat /proc/$$/stat | cut -d" " -f 16)
    TEST_STIME=$(cat /proc/$$/stat | cut -d" " -f 17)

    #sum up all cputime values
    TEST_CPU_TIME=$(echo -e "$TEST_UTIME\n$TEST_STIME" | awk '{sum+=$1};END{print sum}')
    TOTAL_CPU_TIME=$(echo -e "$CC_CPU_TIME\n$PROVIDER_CPU_TIME\n$JETTY_CPU_TIME\n$MOSQUITTO_CPU_TIME\n$TEST_UTIME\n$TEST_STIME" | awk '{sum+=$1};END{print sum}')
    echo "ccCpuTime:         $CC_CPU_TIME" | tee -a $REPORTFILE_PARAM
    echo "providerCpuTime:   $PROVIDER_CPU_TIME" | tee -a $REPORTFILE_PARAM
    echo "jettyCpuTime:      $JETTY_CPU_TIME" | tee -a $REPORTFILE_PARAM
    echo "mosquittoCpuTime:  $MOSQUITTO_CPU_TIME" | tee -a $REPORTFILE_PARAM
    echo "testCpuTime:       $TEST_CPU_TIME" | tee -a $REPORTFILE_PARAM
    echo "totalCpuTime:      $TOTAL_CPU_TIME" | tee -a $REPORTFILE_PARAM
    CLOCK_TICK_DURATION=10 # milliseconds; we can get number of clock ticks per second from "getconf CLK_TCK", by default this is 100
    CPU_PERCENT=$(echo $TOTAL_CPU_TIME $CLOCK_TICK_DURATION $WALL_CLOCK_DURATION | awk '{ printf "%f", 100*($1 * $2 / $3) }')
    echo "cpuPercent:        $CPU_PERCENT" | tee -a $REPORTFILE_PARAM
}

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

    cd $JETTY_PATH

    if [ "$USE_MAVEN" != "ON" ]
    then
        java -jar start.jar 1>$JETTY_STDOUT 2>$JETTY_STDERR & JETTY_PID=$!
    else
        mvn jetty:run-war --quiet 1>$JETTY_STDOUT 2>$JETTY_STDERR & JETTY_PID=$!
    fi

    waitUntilJettyStarted
}

function startMosquitto {
    echo '### Starting mosquitto ###'

    MOSQUITTO_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/mosquitto_stdout.txt
    MOSQUITTO_STDERR=$PERFORMANCETESTS_RESULTS_DIR/mosquitto_stderr.txt

    if [ "$MOSQUITTO_CONF" != "" ] && [ -f $MOSQUITTO_CONF ]
    then
        mosquitto -c $MOSQUITTO_CONF 1>$MOSQUITTO_STDOUT 2>$MOSQUITTO_STDERR & MOSQUITTO_PID=$!
    else
        echo "WARNING: No mosquitto.conf provided"
        mosquitto 1>$MOSQUITTO_STDOUT 2>$MOSQUITTO_STDERR & MOSQUITTO_PID=$!
    fi

    sleep 2

    echo 'Mosquitto started'
}

function startCppClusterController {
    echo '### Starting cluster controller ###'

    CC_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/cc_stdout.txt
    CC_STDERR=$PERFORMANCETESTS_RESULTS_DIR/cc_stderr.txt

    cd $JOYNR_BIN_DIR

    # ensure previously created persistence files are gone
    rm -Rf *.persist joynr.settings

    ./cluster-controller $ADDITIONAL_CC_ARGS 1>$CC_STDOUT 2>$CC_STDERR & CLUSTER_CONTROLLER_PID=$!
    CLUSTER_CONTROLLER_CPU_TIME_1=$(getCpuTime $CLUSTER_CONTROLLER_PID)

    # Wait long enough in order to allow the cluster controller finish its start procedure
    sleep 5

    echo "Cluster controller started"
}

function startCppPerformanceTestProvider {
    echo '### Starting c++ performance test provider ###'

    PROVIDER_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/provider_stdout.txt
    PROVIDER_STDERR=$PERFORMANCETESTS_RESULTS_DIR/provider_stderr.txt

    cd $PERFORMANCETESTS_BIN_DIR
    ./performance-provider-app-ws --globalscope on --domain $DOMAINNAME 1>$PROVIDER_STDOUT 2>$PROVIDER_STDERR & PROVIDER_PID=$!
    PROVIDER_CPU_TIME_1=$(getCpuTime $PROVIDER_PID)

    # Wait long enough in order to allow the provider to finish the registration procedure
    sleep 5

    echo "C++ performance test provider started"
}

function startJavaPerformanceTestProvider {
    echo '### Starting java performance test provider (in process cc) ###'

    PROVIDER_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/provider_stdout.txt
    PROVIDER_STDERR=$PERFORMANCETESTS_RESULTS_DIR/provider_stderr.txt

    PROVIDERCLASS="io.joynr.performance.EchoProviderApplication"
    PROVIDERARGS="-d $DOMAINNAME -s GLOBAL -r IN_PROCESS_CC  -b MQTT -mbu $MQTT_BROKER_URI"

    cd $PERFORMANCETESTS_SOURCE_DIR

    if [ "$USE_MAVEN" != "ON" ]
    then
        java -jar target/performance-test-provider*.jar $PROVIDERARGS 1>$PROVIDER_STDOUT 2>$PROVIDER_STDERR & PROVIDER_PID=$!
    else
        mvn exec:java -o -Dexec.mainClass="$PROVIDERCLASS" -Dexec.args="$PROVIDERARGS" \
            1>$PROVIDER_STDOUT 2>$PROVIDER_STDERR & PROVIDER_PID=$!
    fi
    PROVIDER_CPU_TIME_1=$(getCpuTime $PROVIDER_PID)

    sleep 5

    echo "Performance test provider started"
}

function startJavaJeePerformanceTestProvider {
    echo '### Starting java JEE performance test provider (in process cc) ###'

    asadmin deploy --force=true $PERFORMANCETESTS_SOURCE_DIR/../performance-test-jee/performance-test-jee-provider/target/performance-test-jee-provider.war
    PROVIDER_JEE_APP_NAME="performance-test-jee-provider"

    echo "Performance test provider started"
}

function startJsPerformanceTestProvider {
    PROVIDER_STDOUT=$PERFORMANCETESTS_RESULTS_DIR/provider_stdout.txt
    PROVIDER_STDERR=$PERFORMANCETESTS_RESULTS_DIR/provider_stderr.txt

    if [ "$USE_NPM" == "ON" ]
    then
        npm run-script --performance-test:domain=$DOMAINNAME \
                         startprovider 1>>$PROVIDER_STDOUT 2>>$PROVIDER_STDERR & PROVIDER_PID=$!
    else
        # This call assumes that the required js dependencies are installed locally
        node src/main/js/provider.js $DOMAINNAME 1>>$PROVIDER_STDOUT 2>>$PROVIDER_STDERR & PROVIDER_PID=$!
    fi
}

function performJavaConsumerTest {
    MODE_PARAM=$1
    TESTCASE_PARAM=$2
    STDOUT_PARAM=$3
    REPORTFILE_PARAM=$4
    NUM_INSTANCES=$5
    NUM_RUNS=$6
    DISCOVERY_SCOPE=$7

    CONSUMERCLASS="io.joynr.performance.ConsumerApplication"
    CONSUMERARGS="-d $DOMAINNAME -w $JAVA_WARMUPS -r $NUM_RUNS \
                  -s $MODE_PARAM -t $TESTCASE_PARAM -bs $INPUTDATA_BYTEARRAYSIZE \
                  -sl $INPUTDATA_STRINGLENGTH -ds $DISCOVERY_SCOPE"

    cd $PERFORMANCETESTS_SOURCE_DIR

    TEST_PIDS=()
    for (( i=0; i < $NUM_INSTANCES; ++i ))
    do
        echo "Launching consumer $i ..."

        if [ "$USE_MAVEN" != "ON" ]
        then
            java -jar target/performance-test-consumer*.jar $CONSUMERARGS 1>>$STDOUT_PARAM 2>>$REPORTFILE_PARAM & CUR_PID=$!
        else
            mvn exec:java -o -Dexec.mainClass="$CONSUMERCLASS" \
            -Dexec.args="$CONSUMERARGS" 1>>$STDOUT_PARAM 2>>$REPORTFILE_PARAM & CUR_PID=$!
        fi

        TEST_PIDS+=$CUR_PID
        TEST_PIDS+=" "
    done

    echo "Waiting until consumers finished ..."
    wait $TEST_PIDS
}

function performCppSerializerTest {
    STDOUT_PARAM=$1
    REPORTFILE_PARAM=$2

    cd $PERFORMANCETESTS_BIN_DIR

    ./performance-serializer 1>>$STDOUT_PARAM 2>>$REPORTFILE_PARAM
}

function performCppConsumerTest {
    MODE_PARAM=$1
    TESTCASE_PARAM=$2
    STDOUT_PARAM=$3
    REPORTFILE_PARAM=$4
    NUM_INSTANCES=$5
    NUM_RUNS=$6

    CONSUMERARGS="-r $NUM_RUNS -t $TESTCASE_PARAM"

    cd $PERFORMANCETESTS_BIN_DIR
    if [ "$MODE_PARAM" == "SHORTCIRCUIT" ]
    then
        PERFORMCPPBINARY="performance-short-circuit"
    else
        CONSUMERARGS+=" -d $DOMAINNAME -s $MODE_PARAM -l $INPUTDATA_STRINGLENGTH \
                       -b $INPUTDATA_BYTEARRAYSIZE"
        PERFORMCPPBINARY="performance-consumer-app-ws"
    fi

    TEST_PIDS=()
    for (( i=0; i < $NUM_INSTANCES; ++i ))
    do
        echo "Launching consumer $i ..."
        ./$PERFORMCPPBINARY $CONSUMERARGS 1>>$STDOUT_PARAM 2>>$REPORTFILE_PARAM & CUR_PID=$!
        TEST_PIDS+=$CUR_PID
        TEST_PIDS+=" "
    done

    echo "Waiting until consumers finished ..."
    wait $TEST_PIDS
}

function performJsPerformanceTest {
    STDOUT_PARAM=$1
    REPORTFILE_PARAM=$2

    cd $PERFORMANCETESTS_SOURCE_DIR


    if [ "$USE_NPM" == "ON" ]
    then
        npm run-script startPerformance 1>>$STDOUT_PARAM 2>>$REPORTFILE_PARAM
    else
        # This call assumes that the required js dependencies are installed locally
        node src/main/js/runPerformanceTests.js 1>>$STDOUT_PARAM 2>>$REPORTFILE_PARAM
    fi
}

function stopJetty {
    echo "Stopping jetty"

    JETTY_CPU_TIME_2=$(getCpuTime $JETTY_PID)
    JETTY_CPU_TIME=$(minus $PROVIDER_CPU_TIME_2 $PROVIDER_CPU_TIME_1)
    if [ "$USE_MAVEN" != "ON" ]
    then
        kill $JETTY_PID
    else
        cd $JETTY_PATH
        mvn jetty:stop --quiet
    fi

    wait $JETTY_PID
    JETTY_PID=""
}

function stopMosquitto {
    echo "Stopping mosquitto"
    MOSQUITTO_CPU_TIME_2=$(getCpuTime $MOSQUITTO_PID)
    MOSQUITTO_CPU_TIME=$(minus $MOSQUITTO_CPU_TIME_2 $MOSQUITTO_CPU_TIME_1)
    kill $MOSQUITTO_PID
    wait $MOSQUITTO_PID
    MOSQUITTO_PID=""
}

function stopCppClusterController {
    echo "Killing C++ CC"
    CC_CPU_TIME_2=$(getCpuTime $CLUSTER_CONTROLLER_PID)
    CC_CPU_TIME=$(minus $CC_CPU_TIME_2 $CC_CPU_TIME_1)
    kill $CLUSTER_CONTROLLER_PID
    wait $CLUSTER_CONTROLLER_PID
    CLUSTER_CONTROLLER_PID=""
}

function stopAnyProvider {
    echo "Killing provider"
    if [ "$PROVIDER_PID" != "" ]
    then
        PROVIDER_CPU_TIME_2=$(getCpuTime $PROVIDER_PID)
        PROVIDER_CPU_TIME=$(minus $PROVIDER_CPU_TIME_2 $PROVIDER_CPU_TIME_1)
        echo "USE_MAVEN: $USE_MAVEN"
        if [ "$USE_MAVEN" != "ON" ]
        then
            echo "do not call pkill for provider id $PROVIDER_ID"
        else
            # pkill is required if maven is used to start a provider. Maven launches the
            # provider as a child process, which seems not to be killed automatically along
            # with the parent process
            pkill -P $PROVIDER_PID
        fi
        kill $PROVIDER_PID
        wait $PROVIDER_PID
        PROVIDER_PID=""
    fi

    if [ "$PROVIDER_JEE_APP_NAME" != "" ]
    then
        asadmin undeploy --droptables=true $PROVIDER_JEE_APP_NAME
        PROVIDER_JEE_APP_NAME=""
    fi
}

function startPayara {
    DISCOVERY_WAR_FILE=$PERFORMANCETESTS_SOURCE_DIR/target/discovery-jee.war
    ACCESS_CONTROL_WAR_FILE=$PERFORMANCETESTS_SOURCE_DIR/target/accesscontrol-jee.war

    echo "Starting payara"

    asadmin start-database
    asadmin start-domain

    asadmin deploy --force=true $DISCOVERY_WAR_FILE
    asadmin deploy --force=true $ACCESS_CONTROL_WAR_FILE

    echo "payara started"
}

function stopPayara {
    echo "stopping payara"
    for app in `asadmin list-applications | egrep '(discovery|access)' | cut -d" " -f1`;
    do
        echo "undeploy $app";
        asadmin undeploy --droptables=true $app;
    done

    asadmin stop-domain
    asadmin stop-database
}

function startServices {
    startMosquitto
    echo '# starting services'

    if [ "$BACKEND_SERVICES" = "HTTP" ]
    then
        startJetty
    else
        startPayara
    fi
    sleep 5
}

function stopServices {
    stopMosquitto
    echo '# stopping services'

    if [ "$BACKEND_SERVICES" = "HTTP" ]
    then
        stopJetty
    else
        stopPayara
    fi

    if [ -n "$MOSQUITTO_PID" ]
    then
        echo "Stopping mosquitto with PID $MOSQUITTO_PID"
        disown $MOSQUITTO_PID
        killProcessHierarchy $MOSQUITTO_PID
        wait $MOSQUITTO_PID
        MOSQUITTO_PID=""
    fi
}

function echoUsage {
    echo "Usage: run-performance-tests.sh -j <jetty-dir> -p <performance-bin-dir> \
-r <performance-results-dir> -s <performance-source-dir> \
-t <JAVA_SYNC|JAVA_ASYNC|JAVA_MULTICONSUMER|JS_CONSUMER|OAP_TO_BACKEND_MOSQ|\
CPP_SYNC|CPP_ASYNC|CPP_MULTICONSUMER|JEE_PROVIDER|ALL> -y <joynr-bin-dir>\
-B <backend-services (MQTT|HTTP)>\
[-c <number-of-consumers> -x <number-of-runs> -m <use maven ON|OFF> -z <mosquitto.conf> -n <use node ON|OFF>]"
}

function checkDirExists {
    if [ -z "$1" ] || [ ! -d "$1" ]
    then
        echo "Directory \"$1\" does not exist"
        echoUsage
        exit 1
    fi
}

function checkForJavaTestCase {
    case "$1" in
        JAVA_*)
            return 1;;
    esac
    return 0
}

while getopts "a:c:d:j:k:m:n:p:r:s:t:x:y:z:B:" OPTIONS;
do
    case $OPTIONS in
        a)
            ADDITIONAL_CC_ARGS=$OPTARG
            ;;
        c)
            MULTICONSUMER_NUMINSTANCES=$OPTARG
            ;;
        d)
            DOMAINNAME=${OPTARG%/}
            ;;
        j)
            JETTY_PATH=${OPTARG%/}
            ;;
        k)
            SKIPBYTEARRAYSIZETIMESK=$OPTARG
            ;;
        m)
            USE_MAVEN=$OPTARG
            ;;
        n)
            USE_NPM=$OPTARG
            ;;
        p)
            PERFORMANCETESTS_BIN_DIR=${OPTARG%/}
            ;;
        r)
            PERFORMANCETESTS_RESULTS_DIR=${OPTARG%/}
            ;;
        s)
            PERFORMANCETESTS_SOURCE_DIR=${OPTARG%/}
            ;;
        t)
            TESTCASE=$OPTARG
            ;;
        x)
            SINGLECONSUMER_RUNS=$OPTARG
            MULTICONSUMER_RUNS=$OPTARG
            ;;
        y)
            JOYNR_BIN_DIR=${OPTARG%/}
            ;;
        z)
            MOSQUITTO_CONF=$OPTARG
            ;;
        B)
            BACKEND_SERVICES=$OPTARG
            ;;
        \?)
            echoUsage
            exit 1
            ;;
    esac
done

if [ "$TESTCASE" != "JAVA_SYNC" ] && [ "$TESTCASE" != "JAVA_ASYNC" ] && \
   [ "$TESTCASE" != "JAVA_MULTICONSUMER" ] && \
   [ "$TESTCASE" != "JS_CONSUMER" ] && [ "$TESTCASE" != "OAP_TO_BACKEND_MOSQ" ] && \
   [ "$TESTCASE" != "CPP_SYNC" ] && [ "$TESTCASE" != "CPP_ASYNC" ] && \
   [ "$TESTCASE" != "CPP_MULTICONSUMER" ] && [ "$TESTCASE" != "CPP_SERIALIZER" ] && \
   [ "$TESTCASE" != "CPP_SHORTCIRCUIT" ] && [ "$TESTCASE" != "CPP_PROVIDER" ] && \
   [ "$TESTCASE" != "JEE_PROVIDER" ] && [ "$TESTCASE" != "JS_CONSUMER_CPP_PROVIDER" ] && \
   [ "$TESTCASE" != "CPP_CONSUMER_JS_PROVIDER" ]
then
    echo "\"$TESTCASE\" is not a valid testcase"
    echo "-t option can be either JAVA_SYNC, JAVA_ASYNC, JAVA_MULTICONSUMER, JS_CONSUMER, \
OAP_TO_BACKEND_MOSQ, CPP_SYNC, CPP_ASYNC, CPP_MULTICONSUMER, \
CPP_SERIALIZER, CPP_SHORTCIRCUIT, CPP_PROVIDER, JEE_PROVIDER, JS_CONSUMER_CPP_PROVIDER, CPP_CONSUMER_JS_PROVIDER"
    echoUsage
    exit 1
fi

checkDirExists $JOYNR_BIN_DIR
checkDirExists $PERFORMANCETESTS_BIN_DIR
checkDirExists $PERFORMANCETESTS_RESULTS_DIR
checkDirExists $PERFORMANCETESTS_SOURCE_DIR

REPORTFILE=$PERFORMANCETESTS_RESULTS_DIR/performancetest-result.txt
STDOUT=$PERFORMANCETESTS_RESULTS_DIR/consumer-stdout.txt

rm -f $STDOUT
rm -f $REPORTFILE

TESTCASES=('SEND_STRING' 'SEND_STRUCT' 'SEND_BYTEARRAY')

if [ ! $SKIPBYTEARRAYSIZETIMESK ]
then
    TESTCASES+=('SEND_BYTEARRAY_WITH_SIZE_TIMES_K')
fi

if [ -z "$BACKEND_SERVICES" ]
then
    # use default (MQTT/JEE) Discovery and Access Control
    BACKEND_SERVICES=MQTT
elif [ "$BACKEND_SERVICES" != "MQTT" ] && [ "$BACKEND_SERVICES" != "HTTP" ]
then
    echo 'Invalid value for backend services: $BACKEND_SERVICES.'
    exit 1
fi

if [ "$TESTCASE" != "OAP_TO_BACKEND_MOSQ" ] && [ "$TESTCASE" != "JEE_PROVIDER" ]
then
    checkForJavaTestCase $TESTCASE
    if [ "$?" -eq 1 ]
    then
        startServices
    fi
    startCppClusterController
    startMeasureCpuUsage

    echo "### Starting performance tests ###"

    for mode in 'ASYNC' 'SYNC'; do
        if [ "$TESTCASE" == "JAVA_$mode" ]
        then
            startCppPerformanceTestProvider
            for testcase in 'SEND_STRING' 'SEND_STRUCT' 'SEND_BYTEARRAY'; do
                echo "Testcase: JAVA $testcase" | tee -a $REPORTFILE
                performJavaConsumerTest $mode $testcase $STDOUT $REPORTFILE 1 $SINGLECONSUMER_RUNS "LOCAL_THEN_GLOBAL"
            done
        fi
    done

    if [ "$TESTCASE" == "JAVA_MULTICONSUMER" ]
    then
        startCppPerformanceTestProvider
        for testcase in 'SEND_STRING' 'SEND_STRUCT' 'SEND_BYTEARRAY'; do
            echo "Testcase: JAVA $testcase / MULTIPLE CONSUMERS" | tee -a $REPORTFILE
            performJavaConsumerTest "ASYNC" $testcase $STDOUT $REPORTFILE $MULTICONSUMER_NUMINSTANCES $MULTICONSUMER_RUNS "LOCAL_THEN_GLOBAL"
        done
    fi

    for mode in 'ASYNC' 'SYNC' 'SHORTCIRCUIT'; do
        if [ "$TESTCASE" == "CPP_$mode" ]
        then
            startCppPerformanceTestProvider
            for testcase in ${TESTCASES[@]}; do
                echo "Testcase: $TESTCASE::$testcase" | tee -a $REPORTFILE
                performCppConsumerTest $mode $testcase $STDOUT $REPORTFILE 1 $SINGLECONSUMER_RUNS
            done
        fi
    done

    if [ "$TESTCASE" == "CPP_SERIALIZER" ]
    then
        echo "Testcase: CPP_SERIALIZER" | tee -a $REPORTFILE
        performCppSerializerTest $STDOUT $REPORTFILE
    fi

    if [ "$TESTCASE" == "CPP_MULTICONSUMER" ]
    then
        startCppPerformanceTestProvider
        for testcase in 'SEND_STRING' 'SEND_STRUCT' 'SEND_BYTEARRAY'; do
            echo "Testcase: CPP $testcase / MULTIPLE CONSUMERS" | tee -a $REPORTFILE
            performCppConsumerTest "ASYNC" $testcase $STDOUT $REPORTFILE $MULTICONSUMER_NUMINSTANCES $MULTICONSUMER_RUNS
        done
    fi

    if [ "$TESTCASE" == "JS_CONSUMER" ]
    then
        echo "Testcase: JS_CONSUMER" | tee -a $REPORTFILE
        performJsPerformanceTest $STDOUT $REPORTFILE
    fi

    if [ "$TESTCASE" == "JS_CONSUMER_CPP_PROVIDER" ]
    then
        echo "Testcase: JS_CONSUMER_CPP_PROVIDER" | tee -a $REPORTFILE
        startCppPerformanceTestProvider
        performJsPerformanceTest $STDOUT $REPORTFILE
    fi

    if [ "$TESTCASE" == "CPP_CONSUMER_JS_PROVIDER" ]
    then
         startJsPerformanceTestProvider
         for testcase in ${TESTCASES[@]}; do
                echo "Testcase: $TESTCASE::$testcase" | tee -a $REPORTFILE
                performCppConsumerTest "ASYNC" $testcase $STDOUT $REPORTFILE 1 $SINGLECONSUMER_RUNS
         done
    fi

    if [ "$TESTCASE" == "CPP_PROVIDER" ]
    then
        echo "Testcase: CPP_PROVIDER for domain $DOMAINNAME" | tee -a $REPORTFILE
        startCppPerformanceTestProvider
        # this testcase is used to start a provider which is then accessed from an external consumer
        # in order to keep the provider running, we sleep for a long time here
        sleep 100000000
    fi

    stopMeasureCpuUsage $REPORTFILE
    stopAnyProvider
    stopCppClusterController
    checkForJavaTestCase $TESTCASE
    if [ "$?" -eq 1 ]
    then
        stopServices
    fi
fi

if [ "$TESTCASE" == "JEE_PROVIDER" ]
then
    startServices
    startJavaJeePerformanceTestProvider

    for mode in 'ASYNC' 'SYNC'; do
        for testcase in 'SEND_STRING' 'SEND_STRUCT' 'SEND_BYTEARRAY'; do
            echo "Testcase: JEE_PROVIDER $mode $testcase" | tee -a $REPORTFILE
            performJavaConsumerTest $mode $testcase $STDOUT $REPORTFILE 1 $SINGLECONSUMER_RUNS "GLOBAL_ONLY"
        done
    done

    stopAnyProvider
    stopServices
fi

if [ "$TESTCASE" == "OAP_TO_BACKEND_MOSQ" ]
then
    checkDirExists $JETTY_PATH
    startServices
    startCppClusterController
    startJavaPerformanceTestProvider
    startMeasureCpuUsage

    echo "### Starting performance tests ###"

    echo "Testcase: OAP_TO_BACKEND_MOSQ" | tee -a $REPORTFILE
    performJsPerformanceTest $STDOUT $REPORTFILE true OFF $SKIPBYTEARRAYSIZETIMESK

    stopMeasureCpuUsage $REPORTFILE
    stopAnyProvider
    stopCppClusterController
    stopServices
fi
