#!/bin/bash
#set -x
#set -u

function configure_single_broker
{
    echo "Configuring for single broker"
    export GCD_TEST_CHANNELID="gcd_test"
    export GCD_TEST_BROKERURIS="tcp://localhost:1883"
    export GCD_TEST_GBIDS="joynrdefaultgbid"
    export GCD_TEST_MQTT_CONNECTION_TIMEOUTS_SEC="60"
    export GCD_TEST_MQTT_KEEP_ALIVE_TIMERS_SEC="60"
    export GCD_MQTT_DISABLE_HOSTNAME_VERIFICATION="false"
    export GCD_MQTT_RECEIVE_MAXIMUM="65535"
}

function configure_single_broker_with_tls
{
    echo "Configuring for single broker with TLS"

    configure_single_broker

    export GCD_TEST_BROKERURIS="ssl://localhost:8883"

    export GCD_TEST_MQTT_KEYSTORE_PATH="$REPO/docker/joynr-mqttbroker/var/certs/clientkeystore.jks"
    export GCD_TEST_MQTT_KEYSTORE_TYPE="jks"
    export GCD_TEST_MQTT_KEYSTORE_PWD="password"

    export GCD_TEST_MQTT_TRUSTSTORE_PATH="$REPO/docker/joynr-mqttbroker/var/certs/catruststore.jks"
    export GCD_TEST_MQTT_TRUSTSTORE_TYPE="jks"
    export GCD_TEST_MQTT_TRUSTSTORE_PWD="password"
}

function configure_multiple_broker
{
    echo "Configuring for multiple broker"
    export GCD_TEST_CHANNELID="gcd_test"
    export GCD_TEST_BROKERURIS="tcp://localhost:1883,tcp://localhost:1884"
    export GCD_TEST_GBIDS="gbid1,gbid2"
    export GCD_TEST_MQTT_CONNECTION_TIMEOUTS_SEC="60,60"
    export GCD_TEST_MQTT_KEEP_ALIVE_TIMERS_SEC="60,60"
    export GCD_MQTT_DISABLE_HOSTNAME_VERIFICATION="false"
    export GCD_MQTT_RECEIVE_MAXIMUM="65535"
}

function usage
{
    echo "Synopsis: run-gcd-e2e-tests.sh [-s <joynr-source-dir>] [-1|-2]"
    echo "-1: single broker"
    echo "-2: multiple broker"
}

JOYNR_SOURCE_DIR=""
SINGLE_BROKER=0
MULTIPLE_BROKER=0
while getopts "12s:" OPTIONS;
do
	case $OPTIONS in
		1)
			SINGLE_BROKER=1
			;;
		2)
			MULTIPLE_BROKER=1
			;;
		s)
			JOYNR_SOURCE_DIR=`realpath $OPTARG`
			if [ ! -d "$JOYNR_SOURCE_DIR" ]
			then
				echo "Directory $JOYNR_SOURCE_DIR does not exist!"
				exit 1
			else
				echo "JOYNR_SOURCE_DIR=$OPTARG"
			fi
			;;
		\?)
			echo "Illegal option found."
            usage
			exit 1
			;;
	esac
done

if [ $SINGLE_BROKER -eq 1 ] && [ $MULTIPLE_BROKER -eq 1 ]
then
	echo "Single and multiple broker options are mutually exclusive."
    usage
	exit 1
fi
if [ $SINGLE_BROKER -eq 0 ] && [ $MULTIPLE_BROKER -eq 0 ]
then
	echo "Please select either single or multiple broker option."
    usage
	exit 1
fi

# remove all aliases to get correct return codes
unalias -a

if [ -z "$JOYNR_SOURCE_DIR" ]
then
	# assume this script is started inside a git repo subdirectory,
	JOYNR_SOURCE_DIR=`git rev-parse --show-toplevel`
	if [ $? -ne 0 ]
	then
		echo "This script must be called from within the repository."
		exit 1
	fi
fi

echo "### Stop and remove any potentially still running joynr-backend containers"
cd $JOYNR_SOURCE_DIR/docker
docker-compose -f joynr-backend.yml stop
docker-compose -f joynr-backend.yml rm -f
docker-compose -f joynr-multiple-backend.yml stop
docker-compose -f joynr-multiple-backend.yml rm -f

echo "### Start required joynr-backend"
if [ $SINGLE_BROKER -eq 1 ]
then
	echo "Configuring for single broker"
	cd $JOYNR_SOURCE_DIR/docker
	docker-compose -f joynr-backend.yml up -d
	configure_single_broker
fi
if [ $MULTIPLE_BROKER -eq 1 ]
then
	echo "Configuring for multiple broker"
	cd $JOYNR_SOURCE_DIR/docker
	docker-compose -f joynr-multiple-backend.yml up -d
	configure_multiple_broker
fi

echo "### Starting GCD test"

cd $JOYNR_SOURCE_DIR/tests/gcd-e2e-test
mvn install
mvn $SPECIAL_MAVEN_OPTIONS surefire:test -Dtransport=mqtt -DskipTests=false 2>&1 | tee log
EXITCODE=$?

echo "### Stop and remove joynr-backend containers"
if [ $SINGLE_BROKER -eq 1 ]
then
    cd $JOYNR_SOURCE_DIR/docker
    docker-compose -f joynr-backend.yml stop
    docker-compose -f joynr-backend.yml rm -f
fi
if [ $MULTIPLE_BROKER -eq 1 ]
then
    cd $JOYNR_SOURCE_DIR/docker
    docker-compose -f joynr-multiple-backend.yml stop
    docker-compose -f joynr-multiple-backend.yml rm -f
fi

echo "### Test completed with exit code $EXITCODE"
exit $EXITCODE
