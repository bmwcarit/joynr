#!/bin/bash
###
# #%L
# %%
# Copyright (C) 2021 BMW Car IT GmbH
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
# fail on first error
set -Eeuo pipefail

function performPerformanceMeasurementTest {
    TESTCASE_PARAM=$1
    NUMREQUESTS_PARAM=$2
    MAXINFLIGHT_PARAM=$3
    NUMPROXIES_PARAM=$4
    FILENAME_PARAM=$5
    ITERATIONS_PARAM=$6

    CONSUMERCLASS="io.joynr.performancemeasurement.PerformanceMeasurementApplication"
    CONSUMERARGS="-t $TESTCASE_PARAM -r $NUMREQUESTS_PARAM -m $MAXINFLIGHT_PARAM  -p $NUMPROXIES_PARAM -i $ITERATIONS_PARAM -f $FILENAME_PARAM"

    echo "Launching performance measurement test instance..."
    mvn exec:java -Dexec.mainClass="$CONSUMERCLASS" \
        -Dexec.args="$CONSUMERARGS" & CUR_PID=$!
           
    echo "Waiting until test performance finished ..."
    wait $CUR_PID
}

NUM_REQUESTS=100000
NUM_MAXINFLIGHT=1000
NUM_PROXIES=1000
FILENAME="PerformanceMeasurementTest.csv"
NUM_ITERATIONS=5

function echoUsage {
    echo "Usage: run-performance-tests.sh [<args>]"
    echo "  test parameters:"
    echo "   -r <number of requests to perform> (optional, defaults to $NUM_REQUESTS)"
    echo "   -m <number of max inflight requests to run in parallel> (optional, defaults to $NUM_MAXINFLIGHT"
    echo "   -p <number of proxies to create> (optional, defaults to $NUM_PROXIES)"
    echo "   -f <name of an output csv-file> (optional, defaults to $FILENAME)"
    echo "   -i <number of iterations> (optional, defaults to $NUM_ITERATIONS)"
    echo "      Number of internal reruns of the test."

}

while getopts "r:m:p:f:i:h" OPTIONS;
do
    case $OPTIONS in
# test paramters
        r)
            NUM_REQUESTS=$OPTARG
            ;;
        m)
            NUM_MAXINFLIGHT=$OPTARG
            ;;
        p)
            NUM_PROXIES=$OPTARG
            ;;
        f)
            FILENAME=$OPTARG
            ;;
        i)
            NUM_ITERATIONS=$OPTARG
            ;;
# usage
        h)
            echoUsage
            exit 0;;
        \?)
            echoUsage
            exit 1
            ;;
    esac
done

echo "Start performance measurement test..."
for testcase in 'REQUESTS_ONLY' 'REQUESTS_WITH_PROXY'; do
    echo "Testcase: $testcase"
    performPerformanceMeasurementTest $testcase $NUM_REQUESTS $NUM_MAXINFLIGHT $NUM_PROXIES $FILENAME $NUM_ITERATIONS
    sleep 5
done
