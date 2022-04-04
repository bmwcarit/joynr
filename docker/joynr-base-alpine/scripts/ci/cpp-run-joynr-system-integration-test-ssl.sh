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

log "CPP RUN END TO END TEST (SSL VERSION)"

CC_BINDIR=/data/build/tests/bin

function usage
{
    echo "usage: cpp-run-joynr-system-integration-test-ssl.sh [--cluster-controller-bin-dir <bin directory to cluster-controller>]"
}

function createCACertAndPrivateKey() {
    openssl req -nodes -subj '/CN=ca' -keyout ca.key.pem -new -x509 -days 1024 -sha256 -extensions v3_ca -out ca.cert.pem
}

function createCertAndPrivateKey() {
    CN=$1
    KEYPEM_FILENAME=$2
    CERT_OUT_FILENAME=$3

    openssl req -nodes -keyout $KEYPEM_FILENAME -new -days 1024 -subj "/C=DE/ST=Bavaria/L=Munich/CN=$CN" -out $CN.csr.pem
    openssl x509 -req -days 1024 -CA ca.cert.pem -CAkey ca.key.pem -set_serial 01 -in $CN.csr.pem -out $CERT_OUT_FILENAME
}

while [ "$1" != "" ]; do
    case $1 in
        --cluster-controller-bin-dir )                shift
                                CC_BINDIR=${1%/}
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

log "ENVIRONMENT"
env

log "CC_BINDIR" $CC_BINDIR
# fail on first error
set -e

# add debugging output
set -x

log "Generate test TLS certificates"
SSLPATH="/data/build/tests/ssl-data"
mkdir -p $SSLPATH
cd $SSLPATH

createCACertAndPrivateKey
createCertAndPrivateKey "cluster-controller" "cc.key.pem" "cc.cert.pem"
createCertAndPrivateKey "sit-provider" "provider.key.pem" "provider.cert.pem"
createCertAndPrivateKey "sit-consumer" "consumer.key.pem" "consumer.cert.pem"

# Disable "fail on first error" in case the test itself fails
set +e

TEST_BINDIR="/data/build/tests/bin"

cd $CC_BINDIR
./cluster-controller $TEST_BINDIR/resources/systemintegrationtest-clustercontroller-ssl.settings & CLUSTER_CONTROLLER_PID=$!

cd $TEST_BINDIR

./jsit-provider-ws -d testDomain -g joynrdefaultgbid \
    --pathtosettings $TEST_BINDIR/resources/systemintegrationtest-provider-ssl.settings \
    --ssl-cert-pem $SSLPATH/provider.cert.pem --ssl-privatekey-pem $SSLPATH/provider.key.pem \
    --ssl-ca-cert-pem $SSLPATH/ca.cert.pem & PROVIDER_PID=$!

# Run the test
./jsit-consumer-ws -d testDomain -g joynrdefaultgbid \
    --pathtosettings $TEST_BINDIR/resources/systemintegrationtest-consumer-ssl.settings \
    --ssl-cert-pem $SSLPATH/consumer.cert.pem \
    --ssl-privatekey-pem $SSLPATH/consumer.key.pem \
    --ssl-ca-cert-pem $SSLPATH/ca.cert.pem

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
