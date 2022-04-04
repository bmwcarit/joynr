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

source /data/src/docker/joynr-base-alpine/scripts/ci/global.sh

log "CPP RUN END TO END TEST"

CC_BINDIR=/data/build/tests/bin

function usage
{
    echo "usage: cpp-run-joynr-system-integration-test.sh [--cluster-controller-bin-dir <bin directory to cluster-controller>]"
}

while [ "$1" != "" ]; do
    case $1 in
        --cluster-controller-bin-dir )                shift
                                CC_BINDIR=$1
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

log "ENVIRONMENT"
env

log "CC_BINDIR" $CC_BINDIR
# add debugging output
set -x

TEST_BINDIR="/data/build/tests/bin"

cd $CC_BINDIR
./cluster-controller $TEST_BINDIR/resources/systemintegrationtest-clustercontroller.settings & CLUSTER_CONTROLLER_PID=$!

cd /data/build/tests/bin

./jsit-provider-ws -d testDomain -g joynrdefaultgbid & PROVIDER_PID=$!

# Run the test
./jsit-consumer-ws -d testDomain -g joynrdefaultgbid
TESTRESULT=$?

# Clean up
wait $PROVIDER_PID
kill $CLUSTER_CONTROLLER_PID

if [ $TESTRESULT == 0 ]
then
    echo "joynr system integration test succeeded"
else
    echo "ERROR joynr system integration test failed with error code $TESTRESULT"
fi

exit $TESTRESULT
