#!/bin/bash

function usage() {
    echo "run-provider-java-ws.sh [-h <cc_host>] [-p <cc_port>] [-P <cc_protocol>]"
}

ILT_RESULTS_DIR=/data/build
CC_HOST="localhost"
CC_PORT=4242
CC_PROTOCOL="ws"
DOMAIN="joynr-inter-language-test-domain"
while getopts "d:h:p:P:" OPTIONS;
do
    case $OPTIONS in
        d)
            DOMAIN=$OPTARG
            ;;
        h)
            CC_HOST=$OPTARG
            ;;
        p)
            CC_PORT=$OPTARG
            ;;
        P)
            CC_PROTOCOL=$OPTARG
            ;;
        \?)
            usage
            exit 1
            ;;
    esac
done

echo "CC_HOST=$CC_HOST"
echo "CC_PORT=$CC_PORT"
echo "CC_PROTOCOL=$CC_PROTOCOL"
echo "DOMAIN=$DOMAIN"

# expect to find e.g.
# inter-language-test-1.8.0-SNAPSHOT-jar-with-dependencies.jar
# in /data/ilt-java-app directory
#
# it is required to provide a src/main/resources/ilt-provider-test.settings
# relatively to the directory where the app is started
cd /data/ilt-java-app

# create config file
mkdir -p src/main/resources
cat > src/main/resources/ilt-provider-test.settings << EOF
provider.domain=$DOMAIN
#transport=http:mqtt
#transport=websocket
joynr.messaging.cc.host=$CC_HOST
joynr.messaging.cc.port=$CC_PORT
joynr.messaging.cc.protocol=$CC_PROTOCOL
joynr.messaging.cc.path=
joynr.messaging.sendMsgRetryIntervalMs=10
joynr.discovery.requestTimeout=200
joynr.arbitration.minimumRetryDelay=200
EOF

FILE_SUFFIX=$DOMAIN

# run test
java -cp inter-language-test-*-test-jar-with-dependencies.jar -Dtransport=websocket io.joynr.test.interlanguage.IltProviderApplication $DOMAIN websocket > $ILT_RESULTS_DIR/provider-java-ws-$FILE_SUFFIX.log 2>&1 &

# Format of potential resource file entries
# in ilt-provider-test.settings:
# 
# #inter-language-test.provider.domain=joynr-inter-language-test-domain
# provider.domain=joynr-inter-language-test-domain
# #transport=http:mqtt
# #transport=websocket
# joynr.messaging.cc.host=localhost
# joynr.messaging.cc.port=4242
# joynr.messaging.cc.protocol=ws
# joynr.messaging.cc.path=
# joynr.messaging.sendMsgRetryIntervalMs=10
# joynr.discovery.requestTimeout=200
# joynr.arbitration.minimumRetryDelay=200
