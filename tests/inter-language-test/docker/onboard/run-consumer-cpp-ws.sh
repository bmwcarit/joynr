#!/bin/bash

function usage() {
    echo "Usage: run-consumer-cpp-ws.sh"
    echo "    [-d <domain>]"
    echo "    [-h <cc_host>] [-p <cc_port>] [-P <cc_protocol>]"
}

CPP_HOME=/data/ilt-cpp-app
export LD_LIBRARY_PATH=$CPP_HOME:$LD_LIBRARY_PATH
ILT_RESULTS_DIR=/data/build

# default settings
DOMAIN=joynr-inter-language-test-domain
CC_HOST=localhost
CC_PORT=4242
CC_PROTOCOL=ws

# evaluate parameters
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

echo "DOMAIN=$DOMAIN"
echo "cluster-controller-messaging-url=$CC_PROTOCOL://$CC_HOST:$CC_PORT"

# Provide proper config settings
CONFIG_FILE=$CPP_HOME/bin/resources/ilt-consumer.settings
cat > $CONFIG_FILE << EOF
[websocket]
cluster-controller-messaging-url=$CC_PROTOCOL://$CC_HOST:$CC_PORT
reconnect-sleep-time-ms=100
tls-encryption=false

[messaging]
persistence-file=cpp-ilt-consumer.persistence_file
EOF

FILE_SUFFIX=$DOMAIN

cd $CPP_HOME
./bin/ilt-consumer-ws $DOMAIN --gtest_color=yes --gtest_output="xml:$ILT_RESULTS_DIR/consumer-cpp-$FILE_SUFFIX.junit.xml" >> $ILT_RESULTS_DIR/consumer-cpp-$FILE_SUFFIX.log 2>&1
EXIT_CODE=$?
echo $EXIT_CODE > $ILT_RESULTS_DIR/consumer-cpp-$FILE_SUFFIX.exitcode
