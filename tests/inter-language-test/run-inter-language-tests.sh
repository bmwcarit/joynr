#!/bin/bash
#set -x
#set -u

JOYNR_SOURCE_DIR=""
ILT_BUILD_DIR=""
ILT_RESULTS_DIR=""
CC_LANGUAGE=""
PAYARA_DIR=""
while getopts "b:c:r:s:p:" OPTIONS;
do
	case $OPTIONS in
		c)
			CC_LANGUAGE=$OPTARG
			;;
		b)
			ILT_BUILD_DIR=`realpath $OPTARG`
			;;
		r)
			ILT_RESULTS_DIR=`realpath $OPTARG`
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
		p)
			PAYARA_DIR=`realpath $OPTARG`
			;;
		\?)
			echo "Illegal option found."
			echo "Synopsis: run-inter-language-tests.sh [-b <ilt-build-dir>] [-c <cluster-controller-language (CPP|JAVA)>] [-r <ilt-results-dir>] [-s <joynr-source-dir>]"
			exit 1
			;;
	esac
done

# remove all aliases to get correct return codes
unalias -a

if [ -z "$JOYNR_SOURCE_DIR" ]
then
	# assume this script is started inside a git repo subdirectory,
	JOYNR_SOURCE_DIR=`git rev-parse --show-toplevel`
fi

if [ -z "$PAYARA_DIR" ]
then
	PAYARA_DIR="/opt/payara5"
fi

PAYARA_BIN_DIR="$PAYARA_DIR/glassfish/bin"

# source global.sh
source $JOYNR_SOURCE_DIR/docker/joynr-base/scripts/ci/global.sh

if [ -z "$CC_LANGUAGE" ]
then
	# use default C++ cluster-controller
	CC_LANGUAGE=CPP
elif [ "$CC_LANGUAGE" != "CPP" ] && [ "$CC_LANGUAGE" != "JAVA" ]
then
	log 'Invalid value for cluster-controller language: $CC_LANGUAGE.'
	exit 1
fi

ILT_DIR=$JOYNR_SOURCE_DIR/tests/inter-language-test/inter-language-test-base
GCD_PATH=$JOYNR_SOURCE_DIR/java/backend-services/capabilities-directory/target/deploy

if [ -z "$ILT_BUILD_DIR" ]
then
	ILT_BUILD_DIR=$ILT_DIR/build
fi

if [ -z "$ILT_RESULTS_DIR" ]
then
	ILT_RESULTS_DIR=$ILT_DIR/../ilt-results-$(date "+%Y_%m_%d_%H_%M_%S")
fi

# process ids for background stuff
JETTY_PID=""
MOSQUITTO_PID=""
CLUSTER_CONTROLLER_PID=""
PROVIDER_PID=""

# in case of interrupts, forcibly kill background stuff
function stopall {
	stop_provider
	stop_cluster_controller
	stop_services
	exit 1
}

trap stopall INT

SUCCESS=0
FAILED_TESTS=0
PAYARA_FAILED=0
DOMAIN="joynr-inter-language-test-domain"

function killProcessHierarchy {
	PID=$1
	type pstree > /dev/null 2>&1
	if [ "$?" -eq 0 ]
	then
		# find PIDs of hierarchy using pstree, excluding thread ids
		PIDS=`pstree -p $PID | perl -n -e '{ @pids = /[^\}]\((\d+)/g; foreach (@pids) { print $_ . " "; } ; }'`
		echo "Killing $PIDS"
		kill -9 $PIDS > /dev/null 2>&1
	else
		# kill any direct children, if any
		echo "Killing direct children of $PID"
		pkill -9 -P $PID > /dev/null 2>&1
		echo "Killing $PID"
		kill -9 $PID > /dev/null 2>&1
	fi
}

function prechecks {
	if [ ! -f "$ILT_BUILD_DIR/bin/cluster-controller" ]
	then
		log 'cluster-controller for ILT not found in $ILT_BUILD_DIR/bin/cluster-controller, trying fallback'
		if [ -f "$ILT_BUILD_DIR/../joynr/bin/cluster-controller" ]
		then
			ln -s "$ILT_BUILD_DIR/../joynr/bin/cluster-controller" "$ILT_BUILD_DIR/bin/cluster-controller"
		else
			echo "No CC found in $ILT_BUILD_DIR/../joynr/bin/cluster-controller"
			log 'cluster-controller could also not be found in fallback location'
			exit 1
		fi
	fi

	if [ ! -f "$ILT_BUILD_DIR/bin/ilt-consumer-ws" ]
	then
		log 'ilt-consumer-ws for ILT not found in $ILT_BUILD_DIR/bin/ilt-consumer-ws'
		exit 1
	fi

	if [ ! -f "$ILT_BUILD_DIR/bin/ilt-provider-cc" ]
	then
		log 'ilt-provider-cc for ILT not found in $ILT_BUILD_DIR/bin/ilt-provider-cc'
		exit 1
	fi

	if [ ! -f "$ILT_BUILD_DIR/bin/ilt-provider-uds" ]
	then
		log 'ilt-provider-uds for ILT not found in $ILT_BUILD_DIR/bin/ilt-provider-uds'
		exit 1
	fi

	if [ ! -f "$ILT_BUILD_DIR/bin/ilt-consumer-uds" ]
	then
		log 'ilt-consumer-uds for ILT not found in $ILT_BUILD_DIR/bin/ilt-consumer-uds'
		exit 1
	fi

	if [ ! -f "$GCD_PATH/capabilities-directory-jar-with-dependencies.jar" ]
	then
		log 'capabilities-directory-jar-with-dependencies.jar not found in $GCD_PATH/capabilities-directory-jar-with-dependencies.jar'
		exit 1
	fi

	if ! ls ./inter-language-test-jee-api/target/inter-language-test-jee-api-*.jar 1> /dev/null 2>&1
	then
		log 'inter-language-test-jee-api-*.jar not found in $ILT_DIR/../inter-language-test-jee-api/target/inter-language-test-jee-api-*.jar'
		exit 1
	fi

	if [ ! -f "$ILT_DIR/../inter-language-test-jee-consumer/target/inter-language-test-jee-consumer.war" ]
	then
		log 'inter-language-test-jee-consumer.war not found in $ILT_DIR/../inter-language-test-jee-consumer/target/inter-language-test-jee-consumer.war'
		exit 1
	fi

	if [ ! -f "$ILT_DIR/../inter-language-test-jee-provider/target/inter-language-test-jee-provider.war" ]
	then
		log 'inter-language-test-jee-consumer.war not found in $ILT_DIR/../inter-language-test-jee-provider/target/inter-language-test-jee-provider.war'
		exit 1
	fi
}

function wait_for_gcd {
	try_count=0
	max_retries=30
	while [ -z "$(echo '\n' | curl -v telnet://localhost:9998 2>&1 | grep 'OK')" ]
	do
		echo "GCD not started yet ..."
		try_count=$((try_count+1))
		if [ $try_count -gt $max_retries ]; then
			echo "GCD failed to start in time."
			kill -9 $GCD_PID
			return 1
		fi
		echo "try_count ${try_count}"
		sleep 2
	done
	echo "GCD started successfully."
	return 0
}


GCD_LOG=$ILT_RESULTS_DIR/gcd.log

function startGcd {
	log 'start GCD'
	java -Dlog4j2.configurationFile="file:${GCD_PATH}/log4j2.properties" -jar ${GCD_PATH}/capabilities-directory-jar-with-dependencies.jar 2>&1 > $GCD_LOG &
	GCD_PID=$!
	wait_for_gcd
	return $?
}

function stopGcd
{

	echo 'stop GCD'
	# The app will shutdown if a network connection is attempted on localhost:9999
	(
		set +e
		# curl returns error because the server closes the connection. We do not need the ret val.
		timeout 1 curl telnet://127.0.0.1:9999
		exit 0
	)
	wait $GCD_PID
	# save GCD log for a particular provider
	mv $GCD_LOG $ILT_RESULTS_DIR/gcd-$1.log
}

function stop_services
{
	log '# stop services'

	$PAYARA_BIN_DIR/asadmin stop-domain
	mv "`ls -dArt $PAYARA_DIR/glassfish/domains/domain1/logs/*.log* | tail -n 1`" $ILT_RESULTS_DIR/server-$1.log
	echo "Stopping Payara domain"

	stopGcd $1

	if [ -n "$MOSQUITTO_PID" ]
	then
		echo "Stopping mosquitto with PID $MOSQUITTO_PID"
		disown $MOSQUITTO_PID
		killProcessHierarchy $MOSQUITTO_PID
		wait $MOSQUITTO_PID
		MOSQUITTO_PID=""
	fi

	if [ -f /data/src/docker/joynr-base/scripts/ci/stop-db.sh ]; then
		/data/src/docker/joynr-base/scripts/ci/stop-db.sh
	fi
}

function start_services {
	cd $ILT_DIR
	rm -f joynr.properties
	rm -f joynr_participantIds.properties

	log '# start services'

	if [ -f /data/src/docker/joynr-base/scripts/ci/start-db.sh ]; then
		/data/src/docker/joynr-base/scripts/ci/start-db.sh
	fi

	echo "Starting mosquitto"
	mosquitto -c /etc/mosquitto/mosquitto.conf > $ILT_RESULTS_DIR/mosquitto-$1.log 2>&1 &
	MOSQUITTO_PID=$!
	echo "Mosquitto started with PID $MOSQUITTO_PID"
	sleep 2

	startGcd
	SUCCESS=$?
	if [ "$SUCCESS" != "0" ]; then
		echo '# Start GCD failed with exit code:' $SUCCESS

		stop_services
		exit $SUCCESS
	fi

	echo "Starting Payara domain"
	$PAYARA_BIN_DIR/asadmin start-domain
}

function start_cluster_controller {
	if [ "$CC_LANGUAGE" = "JAVA" ]
	then
		log '# starting JAVA clustercontroller'
		CLUSTER_CONTROLLER_DIR=$JOYNR_SOURCE_DIR/java/core/clustercontroller-standalone
		cd $CLUSTER_CONTROLLER_DIR
		mvn exec:java -Dexec.mainClass="io.joynr.runtime.ClusterController" -Dexec.args="-t mqtt" > $ILT_RESULTS_DIR/clustercontroller-java-$1.log 2>&1 &
	else
		log '# starting C++ clustercontroller'
		if [ ! -d $ILT_BUILD_DIR -o ! -d $ILT_BUILD_DIR/bin ]
		then
			log "C++ build directory or build/bin directory does not exist! Check ILT_BUILD_DIR."
			stopall
		fi
		CLUSTER_CONTROLLER_DIR=$ILT_BUILD_DIR/cluster-controller-bin
		cd $ILT_BUILD_DIR
		rm -fr $CLUSTER_CONTROLLER_DIR
		cp -a $ILT_BUILD_DIR/bin $CLUSTER_CONTROLLER_DIR
		cd $CLUSTER_CONTROLLER_DIR
		[[ $? == "0" ]] && echo "cd $CLUSTER_CONTROLLER_DIR OK"
		./cluster-controller resources/cc.mqtt.messaging.settings > $ILT_RESULTS_DIR/clustercontroller-cpp-$1.log 2>&1 &
	fi
	CLUSTER_CONTROLLER_PID=$!
	echo "Started external cluster controller with PID $CLUSTER_CONTROLLER_PID in directory $CLUSTER_CONTROLLER_DIR"
	# Allow some time for startup
	sleep 5
}

function stop_cluster_controller {
	if [ -n "$CLUSTER_CONTROLLER_PID" ]
	then
		log '# stopping clustercontroller'
		disown $CLUSTER_CONTROLLER_PID
		killProcessHierarchy $CLUSTER_CONTROLLER_PID
		wait $CLUSTER_CONTROLLER_PID
		echo "Stopped external cluster controller with PID $CLUSTER_CONTROLLER_PID"
		CLUSTER_CONTROLLER_PID=""
	fi
}

function test_failed {
	log 'TEST FAILED :('
}

function start_java_provider_cc {
	log 'Starting Java provider CC (with in process clustercontroller).'
	cd $ILT_DIR
	rm -f java-provider.persistence_file
	mvn $SPECIAL_MAVEN_OPTIONS exec:java -Dexec.mainClass="io.joynr.test.interlanguage.IltProviderApplication" -Dexec.args="$DOMAIN mqtt" > $ILT_RESULTS_DIR/provider-java-cc.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started Java provider cc with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function start_java_provider_ws {
	log 'Starting Java provider WS (with WS to standalone clustercontroller).'
	cd $ILT_DIR
	rm -f java-provider.persistence_file
	mvn $SPECIAL_MAVEN_OPTIONS exec:java -Dexec.mainClass="io.joynr.test.interlanguage.IltProviderApplication" -Dexec.args="$DOMAIN websocket" > $ILT_RESULTS_DIR/provider-java-ws.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started Java provider ws with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function stop_provider {
	if [ -n "$PROVIDER_PID" ]
	then
		log 'Stopping provider.'
		cd $ILT_DIR
		disown $PROVIDER_PID
		killProcessHierarchy $PROVIDER_PID
		wait $PROVIDER_PID
		echo "Stopped provider with PID $PROVIDER_PID"
		PROVIDER_PID=""
	fi
}

function stop_jee_provider {
	$PAYARA_BIN_DIR/asadmin undeploy inter-language-test-jee-provider >> $ILT_RESULTS_DIR/provider-jee.log 2>&1
}

function start_cpp_provider_ws {
	log 'Starting C++ provider WS (with WS to standalone cluster-controller).'
	PROVIDER_DIR=$ILT_BUILD_DIR/cpp-provider-ws-bin
	rm -fr $PROVIDER_DIR
	cp -a $ILT_BUILD_DIR/bin $PROVIDER_DIR
	cd $PROVIDER_DIR
	[[ $? == "0" ]] && echo "cd $PROVIDER_DIR OK"
	./ilt-provider-ws $DOMAIN > $ILT_RESULTS_DIR/provider-cpp-ws.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started C++ provider WS with PID $PROVIDER_PID in directory $PROVIDER_DIR"
	# Allow some time for startup
	sleep 10
}

function start_cpp_provider_uds {
	log 'Starting C++ provider UDS (with UDS to standalone cluster-controller).'
	PROVIDER_DIR=$ILT_BUILD_DIR/cpp-provider-uds-bin
	rm -fr $PROVIDER_DIR
	cp -a $ILT_BUILD_DIR/bin $PROVIDER_DIR
	cd $PROVIDER_DIR
	[[ $? == "0" ]] && echo "cd $PROVIDER_DIR OK"
	./ilt-provider-uds $DOMAIN > $ILT_RESULTS_DIR/provider-cpp-uds.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started C++ provider UDS with PID $PROVIDER_PID in directory $PROVIDER_DIR"
	# Allow some time for startup
	sleep 10
}


function start_javascript_provider_ws {
	log 'Starting Javascript provider WS (with WS to standalone cluster-controller).'
	RUNTIME="websocket"
	cd $ILT_DIR
	nohup npm run startprovider --inter-language-test:domain=$DOMAIN --inter-language-test:runtime=$RUNTIME > $ILT_RESULTS_DIR/provider-javascript-ws.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started Javascript provider WS with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function start_javascript_provider_uds {
	log 'Starting Javascript provider UDS (with UDS to standalone cluster-controller).'
	RUNTIME="uds"
	SOCKET_PATH=$ILT_BUILD_DIR/cluster-controller-bin/uds-libjoynr-ilt.sock
	cd $ILT_DIR
	nohup npm run startprovider --inter-language-test:domain=$DOMAIN --inter-language-test:runtime=$RUNTIME --inter-language-test:uds:path=$SOCKET_PATH > $ILT_RESULTS_DIR/provider-javascript-uds.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started Javascript provider UDS with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function start_javascript_provider_bundle_ws {
	log 'Starting Javascript provider bundle WS (with WS to standalone cluster-controller).'
	RUNTIME="websocket"
	cd $ILT_DIR
	nohup npm run startproviderbundle --inter-language-test:domain=$DOMAIN --inter-language-test:runtime=$RUNTIME > $ILT_RESULTS_DIR/provider-javascript-bundle_ws.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started Javascript provider bundle WS with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function start_javascript_provider_bundle_uds {
	log 'Starting Javascript provider bundle UDS (with UDS to standalone cluster-controller).'
	RUNTIME="uds"
	SOCKET_PATH=$ILT_BUILD_DIR/cluster-controller-bin/uds-libjoynr-ilt.sock
	cd $ILT_DIR
	nohup npm run startproviderbundle --inter-language-test:domain=$DOMAIN --inter-language-test:runtime=$RUNTIME --inter-language-test:uds:path=$SOCKET_PATH > $ILT_RESULTS_DIR/provider-javascript-bundle_uds.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started Javascript provider bundle UDS with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function start_jee_provider {
	log 'Starting JEE provider'
	cd $ILT_DIR

	$PAYARA_BIN_DIR/asadmin deploy --force $ILT_DIR/../inter-language-test-jee-provider/target/inter-language-test-jee-provider.war > $ILT_RESULTS_DIR/provider-jee.log 2>&1 &
	PROVIDER_PID=$!
	echo "Deployed JEE provider with PID $PROVIDER_PID"

	# Allow some time for startup
	sleep 10
}

function start_java_consumer_cc {
	if [ "$PAYARA_FAILED" != 0 ]
	then
		log 'SKIPPING Java consumer CC (with in process clustercontroller) because Payara deployment FAILED.'
		PAYARA_FAILED=0
		let FAILED_TESTS+=1
		return
	fi
	log 'Starting Java consumer CC (with in process clustercontroller).'
	cd $ILT_DIR
	rm -f java-consumer.persistence_file
	mkdir $ILT_RESULTS_DIR/consumer-java-cc-$1
	rm -fr $ILT_DIR/target/surefire-reports
	mvn $SPECIAL_MAVEN_OPTIONS surefire:test -Dtransport=mqtt -DskipTests=false >> $ILT_RESULTS_DIR/consumer-java-cc-$1.log 2>&1
	SUCCESS=$?
	cp -a $ILT_DIR/target/surefire-reports $ILT_RESULTS_DIR/consumer-java-cc-$1
	if [ "$SUCCESS" != 0 ]
	then
		log 'Java consumer CC FAILED.'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
		log 'Java consumer CC successfully completed.'
	fi
}

function start_java_consumer_ws {
	log 'Starting Java consumer WS (with WS to standalone clustercontroller).'
	cd $ILT_DIR
	rm -f java-consumer.persistence_file
	mkdir $ILT_RESULTS_DIR/consumer-java-ws-$1
	rm -fr $ILT_DIR/target/surefire-reports
	mvn $SPECIAL_MAVEN_OPTIONS surefire:test -Dtransport=websocket -DskipTests=false >> $ILT_RESULTS_DIR/consumer-java-ws-$1.log 2>&1
	SUCCESS=$?
	cp -a $ILT_DIR/target/surefire-reports $ILT_RESULTS_DIR/consumer-java-ws-$1
	if [ "$SUCCESS" != 0 ]
	then
		log 'Java consumer WS FAILED.'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
		log 'Java consumer WS successfully completed.'
	fi
}

function start_cpp_consumer_ws {
	log 'Starting C++ consumer WS (with WS to standalone cluster-controller).'
	CONSUMER_DIR=$ILT_BUILD_DIR/cpp-consumer-ws-bin
	cd $ILT_BUILD_DIR
	rm -fr $CONSUMER_DIR
	cp -a $ILT_BUILD_DIR/bin $CONSUMER_DIR
	cd $CONSUMER_DIR
	[[ $? == "0" ]] && echo "cd $CONSUMER_DIR OK"
	./ilt-consumer-ws $DOMAIN --gtest_color=yes --gtest_output="xml:$ILT_RESULTS_DIR/consumer-cpp-ws-$1.junit.xml" >> $ILT_RESULTS_DIR/consumer-cpp-ws-$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
	log 'C++ consumer WS FAILED.'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
	log 'C++ consumer WS successfully completed.'
	fi

	log 'Starting C++ consumer proxy-provider-mismatch-ws.'
	./ilt-consumer-proxy-provider-interface-mismatch-ws $DOMAIN --gtest_color=yes --gtest_output="xml:$ILT_RESULTS_DIR/consumer-cpp-proxy-provider-mismatch-ws-$1.junit.xml" >> $ILT_RESULTS_DIR/consumer-cpp-proxy-provider-mismatch-ws-$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
	log 'C++ consumer proxy-provider-mismatch-ws FAILED.'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
	log 'C++ consumer proxy-provider-mismatch-ws successfully completed.'
	fi
}

function start_cpp_consumer_uds {
	log 'Starting C++ consumer UDS (with UDS to standalone cluster-controller).'
	CONSUMER_DIR=$ILT_BUILD_DIR/cpp-consumer-uds-bin
	cd $ILT_BUILD_DIR
	rm -fr $CONSUMER_DIR
	cp -a $ILT_BUILD_DIR/bin $CONSUMER_DIR
	cd $CONSUMER_DIR
	[[ $? == "0" ]] && echo "cd $CONSUMER_DIR OK"
	./ilt-consumer-uds $DOMAIN --gtest_color=yes --gtest_output="xml:$ILT_RESULTS_DIR/consumer-cpp-uds-$1.junit.xml" >> $ILT_RESULTS_DIR/consumer-cpp-uds-$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
	log 'C++ consumer UDS FAILED.'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
	log 'C++ consumer UDS successfully completed.'
	fi

	log 'Starting C++ consumer proxy-provider-mismatch-uds.'
	./ilt-consumer-proxy-provider-interface-mismatch-uds $DOMAIN --gtest_color=yes --gtest_output="xml:$ILT_RESULTS_DIR/consumer-cpp-proxy-provider-mismatch-uds-$1.junit.xml" >> $ILT_RESULTS_DIR/consumer-cpp-proxy-provider-mismatch-uds-$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
	log 'C++ consumer proxy-provider-mismatch-uds FAILED.'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
	log 'C++ consumer proxy-provider-mismatch-uds successfully completed.'
	fi
}

function start_javascript_consumer_ws {
	log 'Starting Javascript consumer WS (with WS to standalone cluster-controller).'
	RUNTIME="websocket"
	cd $ILT_DIR
	rm -fr localStorageStorage
	npm run startjasmine --inter-language-test:domain=$DOMAIN --inter-language-test:runtime=$RUNTIME > $ILT_RESULTS_DIR/consumer-javascript-ws-$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
		log 'Javascript consumer WS FAILED.'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
		log 'Javascript consumer WS successfully completed.'
	fi
}

function start_javascript_consumer_uds {
	log 'Starting Javascript consumer UDS (with UDS to standalone cluster-controller).'
	RUNTIME="uds"
	SOCKET_PATH=$ILT_BUILD_DIR/cluster-controller-bin/uds-libjoynr-ilt.sock
	cd $ILT_DIR
	rm -fr localStorageStorage
	npm run startjasmine --inter-language-test:domain=$DOMAIN --inter-language-test:runtime=$RUNTIME --inter-language-test:uds:path=$SOCKET_PATH > $ILT_RESULTS_DIR/consumer-javascript-uds-$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
		log 'Javascript consumer UDS FAILED.'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
		log 'Javascript consumer UDS successfully completed.'
	fi
}

function start_jee_consumer {
	log 'Starting JEE consumer'
	cd $ILT_DIR
	SUCCESS=0

	$PAYARA_BIN_DIR/asadmin deploy --force $ILT_DIR/../inter-language-test-jee-consumer/target/inter-language-test-jee-consumer.war >> $ILT_RESULTS_DIR/consumer-jee-$1.log 2>&1
	SUCCESS=$?
	echo "Deployed JEE consumer"

	if [ "$SUCCESS" != 0 ]
	then
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
		RESPONSE=$(curl -k -v https://localhost:8181/inter-language-test-jee-consumer/start-tests 2>&1)
		echo "$RESPONSE" >> $ILT_RESULTS_DIR/consumer-jee-$1.log
		$PAYARA_BIN_DIR/asadmin undeploy inter-language-test-jee-consumer >> $ILT_RESULTS_DIR/consumer-jee.log 2>&1

		if [ -n "$(grep 'HTTP\/.* 200' <<< $RESPONSE)" ]
		then
			ERRORS=$(grep '(\b(errors)|\b(failures)).*\b[1-9][0-9]*' <<< $RESPONSE)
			if [ -z "$ERRORS" ]
			then
				log 'JEE consumer successfully completed.'
			else
				#ERRORS=$(grep 'status' <<< $RESPONSE | grep -v 'ok')
				echo "STATUS = 1"
				#test_failed
				let FAILED_TESTS+=1
				#stopall
			fi
		else
			echo "STATUS = 1"
			#test_failed
			let FAILED_TESTS+=1
			#stopall
		fi
	fi
}

function start_consumers {
	start_java_consumer_cc $1
	start_java_consumer_ws $1
	start_cpp_consumer_ws $1
	start_cpp_consumer_uds $1
	start_javascript_consumer_ws $1
	start_javascript_consumer_uds $1
	start_jee_consumer $1
}

# TESTS

# check that the environment has been setup correctly
prechecks

# clean up
rm -fr $ILT_RESULTS_DIR
mkdir -p $ILT_RESULTS_DIR
cd $ILT_DIR
rm -fr node_modules
rm -fr reports

function clean_up {
	cd $ILT_DIR
	rm -f derby.log npm-debug.log
	rm -fr localStorageStorage
	rm -rf target/discoverydb
}

# prepare JavaScript
npm run build

# run the checks
#
# the start of consumers includes start/stop of seperate cluster
# controller executables, if required (e.g. for Javascript).
#
# Note that the services (jetty) need to be stopped once the
# provider is changed, since otherwise the discovery service
# continues to return data for the already stopped provider
# since it currently assumes that it will be restarted.

# run checks with Java provider cc
clean_up
log 'RUN CHECKS WITH JAVA PROVIDER CC (with in process clustercontroller).'
PROVIDER="provider-java-cc"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_java_provider_cc
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services $PROVIDER

# run checks with Java provider ws
clean_up
log 'RUN CHECKS WITH JAVA PROVIDER WS (with WS to standalone clustercontroller).'
PROVIDER="provider-java-ws"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_java_provider_ws
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services $PROVIDER

# run checks with C++ provider
clean_up
log 'RUN CHECKS WITH C++ PROVIDER WS (with WS to standalone clustercontroller).'
PROVIDER="provider-cpp-ws"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_cpp_provider_ws
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services $PROVIDER

# run checks with C++ provider UDS
clean_up
log 'RUN CHECKS WITH C++ PROVIDER UDS (with UDS to standalone clustercontroller).'
PROVIDER="provider-cpp-uds"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_cpp_provider_uds
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services $PROVIDER

# run checks with Javascript provider WS
clean_up
log 'RUN CHECKS WITH JAVASCRIPT PROVIDER WS (with WS to standalone clustercontroller).'
PROVIDER="provider-javascript-ws"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_javascript_provider_ws
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services $PROVIDER

# run checks with Javascript provider UDS
clean_up
log 'RUN CHECKS WITH JAVASCRIPT PROVIDER UDS (with UDS to standalone clustercontroller).'
PROVIDER="provider-javascript-uds"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_javascript_provider_uds
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services $PROVIDER

# run checks with Javascript bundle provider WS
clean_up
log 'RUN CHECKS WITH JAVASCRIPT BUNDLE PROVIDER WS (with WS to standalone clustercontroller).'
PROVIDER="provider-javascript-bundle-ws"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_javascript_provider_bundle_ws
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services $PROVIDER

# run checks with Javascript bundle provider UDS
clean_up
log 'RUN CHECKS WITH JAVASCRIPT BUNDLE PROVIDER UDS (with UDS to standalone clustercontroller).'
PROVIDER="provider-javascript-bundle-uds"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_javascript_provider_bundle_uds
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services $PROVIDER

# run checks with JEE provider
clean_up
log 'RUN CHECKS WITH JEE PROVIDER'
PROVIDER="provider-jee"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_jee_provider
# start_consumers $PROVIDER is not called here because
# jee does not support other consumers' calls
start_jee_consumer $PROVIDER
stop_provider
stop_jee_provider
stop_cluster_controller
stop_services $PROVIDER
log 'JEE CHECKS FINISHED WITH SUCCESS'

# check for failed tests
if [ "$FAILED_TESTS" -gt 0 ]
then
	test_failed
	exit $FAILED_TESTS
fi

# If this point is reached, the tests have been successfull.
log 'TEST SUCCEEDED :)'
exit $SUCCESS
