#!/bin/bash

# enable logging, if required
#set -x

# Default values (also used in usage() function)
DEFAULT_JOYNR_CONSUMER_LANGUAGES="cpp java js"
DEFAULT_JOYNR_PROVIDER_LANGUAGES="cpp java js"
DEFAULT_JOYNR_PROVIDER_NODE="ilt-onboard-apps-1"
DEFAULT_JOYNR_CONSUMER_NODE="ilt-onboard-apps-2"
DEFAULT_JOYNR_PROVIDER_WS_HOST=$DEFAULT_JOYNR_PROVIDER_NODE
DEFAULT_JOYNR_PROVIDER_WS_PORT=4242
DEFAULT_JOYNR_PROVIDER_WS_PROTOCOL="ws"
DEFAULT_JOYNR_CONSUMER_WS_HOST=$DEFAULT_JOYNR_PROVIDER_NODE
DEFAULT_JOYNR_CONSUMER_WS_PORT=4242
DEFAULT_JOYNR_CONSUMER_WS_PROTOCOL="ws"

# Current values (derived from defaults, may be overridden by parameters)
JOYNR_VERSION_OLD=""
JOYNR_VERSION_NEW=""
JOYNR_CONSUMER_LANGUAGES=$DEFAULT_JOYNR_CONSUMER_LANGUAGES
JOYNR_PROVIDER_LANGUAGES=$DEFAULT_JOYNR_PROVIDER_LANGUAGES
JOYNR_PROVIDER_NODE=$DEFAULT_JOYNR_PROVIDER_NODE
JOYNR_CONSUMER_NODE=$DEFAULT_JOYNR_CONSUMER_NODE
JOYNR_PROVIDER_WS_HOST=$DEFAULT_JOYNR_PROVIDER_WS_HOST
JOYNR_PROVIDER_WS_PORT=$DEFAULT_JOYNR_PROVIDER_WS_PORT
JOYNR_PROVIDER_WS_PROTOCOL=$DEFAULT_JOYNR_PROVIDER_WS_PROTOCOL
JOYNR_CONSUMER_WS_HOST=$DEFAULT_JOYNR_CONSUMER_WS_HOST
JOYNR_CONSUMER_WS_PORT=$DEFAULT_JOYNR_CONSUMER_WS_PORT
JOYNR_CONSUMER_WS_PROTOCOL=$DEFAULT_JOYNR_CONSUMER_WS_PROTOCOL

function usage() {
    echo "Synopsis: run-docker-inter-language-tests.sh <options>"
    echo "    -a <old_version> (mandatory: tag of docker image)"
    echo "    -b <new_version> (mandatory: tag of docker image)"
    echo "    [-c <backend_version>] (default: <new_version>)"
    echo "    [-l <consumer languages] (default: \"$DEFAULT_JOYNR_CONSUMER_LANGUAGES\")"
    echo "    [-L <provider languages] (default: \"$DEFAULT_JOYNR_PROVIDER_LANGUAGES\")"
    echo "    [-n <consumer node] (default: $DEFAULT_JOYNR_CONSUMER_NODE)"
    echo "    [-N <provider node] (default: $DEFAULT_JOYNR_PROVIDER_NODE)"
    echo "    [-h|-H <consumer|provider> cc host (default: ${DEFAULT_JOYNR_CONSUMER_WS_HOST}|${DEFAULT_JOYNR_PROVIDER_WS_HOST}]"
    echo "    [-p|-P <consumer|provider> cc port (default: ${DEFAULT_JOYNR_CONSUMER_WS_PORT}|${DEFAULT_JOYNR_PROVIDER_WS_PORT}]"
    echo "    [-w|-W <consumer|provider> cc ws protocol (default: ${DEFAULT_JOYNR_CONSUMER_WS_PROTOCOL}|${DEFAULT_JOYNR_PROVIDER_WS_PROTOCOL}]"
}

# check working directory
if [ ! -f docker-compose.yml ]
then
    echo "ERROR: docker-compose.yml file not found."
    echo "ERROR: Command invoked within invalid directory"
    usage
    exit 1
fi

while getopts "a:b:c:h:H:l:L:n:N:p:P:w:W:" OPTIONS;
do
    case $OPTIONS in
        a)
            JOYNR_VERSION_OLD=$OPTARG
            ;;
        b)
            JOYNR_VERSION_NEW=$OPTARG
            ;;
        c)
            JOYNR_BACKEND_IMAGE_VERSION=$OPTARG
            ;;
        h)
            JOYNR_CONSUMER_WS_HOST=$OPTARG
            ;;
        H)
            JOYNR_PROVIDER_WS_HOST=$OPTARG
            ;;
        l)
            JOYNR_CONSUMER_LANGUAGES=$OPTARG
            ;;
        L)
            JOYNR_CONSUMER_LANGUAGES=$OPTARG
            ;;
        n)
            JOYNR_CONSUMER_NODE=$OPTARG
            ;;
        N)
            JOYNR_PROVIDER_NODE=$OPTARG
            ;;
        p)
            JOYNR_CONSUMER_WS_PORT=$OPTARG
            ;;
        P)
            JOYNR_PROVIDER_WS_PORT=$OPTARG
            ;;
        w)
            JOYNR_CONSUMER_WS_PROTOCOL=$OPTARG
            ;;
        W)
            JOYNR_PROVIDER_WS_PROTOCOL=$OPTARG
            ;;
        \?)
            echo "Illegal option found."
            usage
            exit 1
            ;;
    esac
done

# sanity checks
if [ -z "$JOYNR_VERSION_OLD" ]
then
    echo "ERROR: old joynr version not specified"
    usage
    exit 1
fi
if [ -z "$JOYNR_VERSION_NEW" ]
then
    echo "ERROR: new joynr version not specified"
    usage
    exit 1
fi
if [ -z "$JOYNR_BACKEND_IMAGE_VERSION" ]
then
    JOYNR_BACKEND_IMAGE_VERSION=$JOYNR_VERSION_NEW
fi

echo "JOYNR_VERSION_OLD=$JOYNR_VERSION_OLD"
echo "JOYNR_VERSION_NEW=$JOYNR_VERSION_NEW"
echo "JOYNR_BACKEND_IMAGE_VERSION=$JOYNR_BACKEND_IMAGE_VERSION"

# derive image versions used by docker compose
#
# Container 1 uses new version
# while Container 2 uses the old version
export HIVEMQ_IMAGE_VERSION=$JOYNR_VERSION_NEW
export ILT_ONBOARD_APPS_IMAGE_VERSION_1=$JOYNR_VERSION_NEW
export ILT_ONBOARD_APPS_IMAGE_VERSION_2=$JOYNR_VERSION_OLD
export JOYNR_BACKEND_IMAGE_VERSION

# start containers in background mode
docker compose up -d
if [ "$?" -ne 0 ]
then
    echo "ERROR: docker compose up -d failed."
    docker compose stop
    exit 1
fi

# all containers started
# wait a while for them to boot up
sleep 30

CONSUMER_COUNT=0
for PROV in $JOYNR_PROVIDER_LANGUAGES
do
    # start up services in background on $JOYNR_PROVIDER_NODE image
    # tbd: support JOYNR_PROVIDER_WS_HOST, JOYNR_PROVIDER_WS_PORT, JOYNR_PROVIDER_WS_PROTOCOL
    # via parameter for CC and provider
    echo "Starting cluster controller on $JOYNR_PROVIDER_NODE"
    docker exec -it $JOYNR_PROVIDER_NODE sh -c 'cd /data; nohup ./run-cc-cpp.sh > /dev/null 2>&1'

    # starting cluster controller on consumer node is only required if
    # there is a separate consumer node (i.e. not all apps are running on the provider node)
    # and consumers are not configured to contact cluster controller on provider side directly
    if [ "$JOYNR_PROVIDER_NODE" != "$JOYNR_CONSUMER_NODE" -a "$JOYNR_PROVIDER_WS_HOST" != "$JOYNR_CONSUMER_WS_HOST" ]
    then
        echo "Starting cluster controller on $JOYNR_CONSUMER_NODE"
        docker exec -it $JOYNR_CONSUMER_NODE sh -c 'cd /data; nohup ./run-cc-cpp.sh > /dev/null 2>&1'
    fi

    DOMAIN="ilt-${PROV}-domain"
    echo "Starting $PROV provider on $JOYNR_PROVIDER_NODE"
    docker exec -it $JOYNR_PROVIDER_NODE sh -c "cd /data; nohup ./run-provider-${PROV}-ws.sh -d $DOMAIN > /dev/null 2>&1"

    for CONS in $JOYNR_CONSUMER_LANGUAGES
    do
        echo "Testing $CONS consumer on $JOYNR_CONSUMER_NODE against $PROV provider on $JOYNR_PROVIDER_NODE using domain $DOMAIN via consumer CC on ${JOYNR_CONSUMER_WS_PROTOCOL}://${JOYNR_CONSUMER_WS_HOST}:${JOYNR_CONSUMER_WS_PORT}"
        docker exec -it $JOYNR_CONSUMER_NODE sh -c "cd /data; nohup ./run-consumer-${CONS}-ws.sh -d $DOMAIN -h $JOYNR_CONSUMER_WS_HOST -P $JOYNR_CONSUMER_WS_PROTOCOL -p $JOYNR_CONSUMER_WS_PORT > /dev/null 2>&1"
        let "CONSUMER_COUNT++"
    done

    echo "Terminating test apps / CC on $JOYNR_CONSUMER_NODE"
    docker exec -it $JOYNR_CONSUMER_NODE sh -c 'cd /data; nohup ./terminate-test.sh > /dev/null 2>&1'

    if [ "$JOYNR_PROVIDER_NODE" != "$JOYNR_CONSUMER_NODE" ]
    then
        echo "Terminating test apps / CC on $JOYNR_PROVIDER_NODE"
        docker exec -it $JOYNR_PROVIDER_NODE sh -c 'cd /data; nohup ./terminate-test.sh > /dev/null 2>&1'
    fi
done

sleep 5

# stop containers
docker compose stop

# remove containers
docker compose rm -f

# evaluate results
NUMBER_OF_FILES=`ls -1 $HOME/docker/build/${JOYNR_CONSUMER_NODE}/*.exitcode 2> /dev/null | wc -l`
if [ $NUMBER_OF_FILES -ne $CONSUMER_COUNT ]
then
    echo "RESULT: Test failed, found only ${NUMBER_OF_FILES} of ${CONSUMER_COUNT} exitcode files."
    exit 1
fi

cat $HOME/docker/build/${JOYNR_CONSUMER_NODE}/*.exitcode | grep -q -v '^0$'
if [ "$?" -ne 1 ]
then
    echo "RESULT: Test failed, found files with exitcode != 0"
    grep -v '^0$' $HOME/docker/build/${JOYNR_CONSUMER_NODE}/*.exitcode
    exit 1
fi

echo "RESULT: Test successfully completed."
exit 0
