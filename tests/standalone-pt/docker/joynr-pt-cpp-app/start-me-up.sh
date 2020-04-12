#!/bin/bash

set -xe

GBIDS='joynrdefaultgbid'
DOMAIN='pt-domain.jee.provider'
SYNCMODE='async'
CALLS='10000'
MAX_INFLIGHT_CALLS='100'
CONTAINERID=''

function usage
{   
    echo "usage: performance-consumer-app-cc
            [--gbids <default: joynrdefaultgbid>]
            [--domain <default: pt-domain.jee.provider>]
            [--syncMode sync|async <default: async>]
            [--calls <default: 10000>]
            [--maxInflightCalls <default: 100>]
            [--containerId <anyId> <default: \"\">]"
}

while [ "$1" != "" ]; do
    case $1 in
        --gbids )               shift
                                GBIDS=$1
                                ;;
        --domain )              shift
                                DOMAIN=$1
                                ;;
        --syncMode )            shift
                                SYNCMODE=$1
                                ;;
        --calls )               shift
                                CALLS=$1
                                ;;
        --maxInflightCalls )    shift
                                MAX_INFLIGHT_CALLS=$1
                                ;;
        --containerId )         shift
                                CONTAINERID=$1
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

echo "GBIDS: $GBIDS"
echo "DOMAIN: $DOMAIN"
echo "SYNCMODE: $SYNCMODE"
echo "CALLS: $CALLS"
echo "MAX_INFLIGHT_CALLS: $MAX_INFLIGHT_CALLS"
echo "CONTAINERID: $(hostname)"

printf "\n\n >>>  STARTING cpp consumer test app <<<\n\n"

# give the broker and backend some time to startup
sleep 25

num=1
while true
do
    performance-consumer-app-cc \
    --domain $DOMAIN \
    --gbids $GBIDS \
    --syncMode $SYNCMODE \
    --calls $CALLS \
    --maxInflightCalls $MAX_INFLIGHT_CALLS \
    --repetition $num \
    --containerId $(hostname)
    wait=$!
    ((++num))
done
