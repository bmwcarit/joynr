#!/bin/bash
#set -x
#set -u

JOYNR_SOURCE_DIR=""
ILT_BUILD_DIR=""
ILT_RESULTS_DIR=""
CC_LANGUAGE=""
while getopts "b:c:r:s:" OPTIONS;
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

# source global.sh
if [ -f /data/scripts/global.sh ]
then
	source /data/scripts/global.sh
else
	source $JOYNR_SOURCE_DIR/docker/joynr-base/scripts/global.sh
fi

if [ -z "$CC_LANGUAGE" ]
then
	# use default C++ cluster-controller
	CC_LANGUAGE=CPP
elif [ "$CC_LANGUAGE" != "CPP" ] && [ "$CC_LANGUAGE" != "JAVA" ]
then
	log 'Invalid value for cluster-controller language: $CC_LANGUAGE.'
	exit 1
fi

ILT_DIR=$JOYNR_SOURCE_DIR/tests/inter-language-test

if [ -z "$ILT_BUILD_DIR" ]
then
	ILT_BUILD_DIR=$ILT_DIR/build
fi

if [ -z "$ILT_RESULTS_DIR" ]
then
	ILT_RESULTS_DIR=$ILT_DIR/ilt-results-$(date "+%Y_%m_%d_%H_%M_%S")
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

	if [ ! -f "$ILT_DIR/target/discovery-jee.war" ]
	then
		log 'discovery-jee.war not found in $ILT_DIR/target/discovery-jee.war'
		exit 1
	fi

}

PAYARA_LOG=$ILT_RESULTS_DIR/payara.log
function prepare_payara {
	asadmin --user admin start-domain
	asadmin --user admin set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.rotationLimitInBytes=512000000
	asadmin --user admin set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.file="${PAYARA_LOG}"
	asadmin --user admin stop-domain
}

function start_payara {
	DISCOVERY_WAR_FILE=$ILT_DIR/target/discovery-jee.war

    echo "Starting payara"

	# When starting the database there is no need to add the option:
	# --jvmoptions="-Dderby.storage.useDefaultFilePermissions=true"
	# It is already added in inter-language-test/docker/joynr-backend-jee/start-me-up.sh
	# Otherwise CI reports an error:
	# Repeats not allowed for option: jvmoptions
	asadmin --user admin start-database
	asadmin --user admin start-domain

	asadmin --user admin deploy --force=true $DISCOVERY_WAR_FILE

	SUCCESS=$?
	if [ "$SUCCESS" != 0 ]
	then
		log 'Payara deployment FAILED.'
		PAYARA_FAILED=1
	else
		echo "payara started"
	fi
}

function stop_payara {
    echo "stopping payara"
	for app in `asadmin list-applications | egrep '(discovery)' | cut -d" " -f1`;
	do
		echo "undeploy $app";
		asadmin --user admin undeploy --droptables=true $app;
	done

	asadmin --user admin stop-domain
	asadmin --user admin stop-database

	# save payara log for a particular provider
	mv $PAYARA_LOG $ILT_RESULTS_DIR/payara-$1.log
}

function start_services {
	cd $ILT_DIR
	rm -f joynr.properties
	rm -f joynr_participantIds.properties
	log '# starting services'

	echo "Starting mosquitto"
	mosquitto -c /etc/mosquitto/mosquitto.conf > $ILT_RESULTS_DIR/mosquitto-$1.log 2>&1 &
	MOSQUITTO_PID=$!
	echo "Mosquitto started with PID $MOSQUITTO_PID"
	sleep 2

	start_payara
}

function stop_services {
	log '# stopping services'

	stop_payara $1

	if [ -n "$MOSQUITTO_PID" ]
	then
		echo "Stopping mosquitto with PID $MOSQUITTO_PID"
		disown $MOSQUITTO_PID
		killProcessHierarchy $MOSQUITTO_PID
		wait $MOSQUITTO_PID
		MOSQUITTO_PID=""
	fi
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

function start_consumers {
	start_java_consumer_cc $1
	start_java_consumer_ws $1
	start_cpp_consumer_ws $1
	start_cpp_consumer_uds $1
	start_javascript_consumer_ws $1
	start_javascript_consumer_uds $1
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

# prepare payara
prepare_payara

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

# check for failed tests
if [ "$FAILED_TESTS" -gt 0 ]
then
	test_failed
	exit $FAILED_TESTS
fi

# If this point is reached, the tests have been successfull.
log 'TEST SUCCEEDED :)'
exit $SUCCESS
