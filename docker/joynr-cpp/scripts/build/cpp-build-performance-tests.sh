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

START=$(date +%s)
JOBS=$(nproc)

function usage
{
    echo "usage: cpp-build-performance-tests.sh [--jobs X]"
    echo "default jobs is $JOBS"
}

while [ "$1" != "" ]; do
    case $1 in
        --jobs )                shift
                                JOBS=$1
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

log "CPP BUILD PERFORMANCE TESTS JOBS: $JOBS"

log "ENVIRONMENT"
env

# fail on first error
set -e

# Performance tests currently (or in future) also require generating for Java and Javascript, so
# install the java- und javascripts generators
cd /data/src/
mvn clean install -P no-license-and-notice,no-java-formatter,no-checkstyle -DskipTests \
--projects \
io.joynr.tools.generator:generator-framework,\
io.joynr.tools.generator:joynr-generator-maven-plugin,\
io.joynr.tools.generator:java-generator,\
io.joynr.tools.generator:js-generator,\
io.joynr.tools.generator:cpp-generator,\
io.joynr:performance

rm -rf /data/build/performance-tests
mkdir /data/build/performance-tests
cd /data/build/performance-tests

cmake -DCMAKE_PREFIX_PATH=$JOYNR_INSTALL_DIR /data/src/tests/performance

time make -j $JOBS

END=$(date +%s)
DIFF=$(( $END - $START ))
log "Performance-test build time: $DIFF seconds"
