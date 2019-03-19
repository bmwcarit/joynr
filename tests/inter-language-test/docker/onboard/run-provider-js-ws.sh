#!/bin/bash

function usage() {
    echo "Usage: run-provider-js-ws.sh"
    echo "    [-d <domain>]"
    echo "    [-h <cc_host>] [-p <cc_port>] [-P <cc_protocol>]"
}

ILT_NODE_HOME=/data/ilt-node-app
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

# Provide proper config settings
CONFIG_FILE=$ILT_NODE_HOME/node_modules/test-base/provisioning_common.js
sed -i "s/protocol : \".*\"/protocol : \"$CC_PROTOCOL\"/" $CONFIG_FILE
sed -i "s/host : \".*\"/host : \"$CC_HOST\"/" $CONFIG_FILE
sed -i "s/port : .*,/port : $CC_PORT,/" $CONFIG_FILE

FILE_SUFFIX=$DOMAIN

cd $ILT_NODE_HOME
npm run-script startprovider --inter-language-test:domain=$DOMAIN > $ILT_RESULTS_DIR/provider-js-$FILE_SUFFIX.log 2>&1 &
