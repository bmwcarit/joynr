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

source /data/src/docker/joynr-base/scripts/global.sh

log "CPP RUN END TO END TEST"

log "ENVIRONMENT"
env

# fail on first error
set -e

cd /data/build/tests/bin

./cluster-controller & CLUSTER_CONTROLLER_PID=$!

./jsit-provider-ws testDomain & PROVIDER_PID=$!

# Run the test
./jsit-consumer-ws testDomain

if [ "$?" == "-1" ]
then
    echo "ERROR joynr system integration test failed with error code $?"
else
    echo "joynr system integration test succeeded"
fi

# Clean up
wait $PROVIDER_PID
kill $CLUSTER_CONTROLLER_PID
