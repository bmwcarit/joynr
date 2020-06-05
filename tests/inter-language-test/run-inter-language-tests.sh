#!/bin/bash
#set -x
#set -u

JOYNR_SOURCE_DIR=""
ILT_BUILD_DIR=""
ILT_RESULTS_DIR=""
CC_LANGUAGE=""
BACKEND_SERVICES=""
while getopts "b:c:r:s:B:" OPTIONS;
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
		B)
			BACKEND_SERVICES=$OPTARG
			;;
		\?)
			echo "Illegal option found."
			echo "Synopsis: run-inter-language-tests.sh [-b <ilt-build-dir>] [-c <cluster-controller-language (CPP|JAVA)>] [-r <ilt-results-dir>] [-s <joynr-source-dir>] [-B <backend-services> (MQTT|HTTP)"
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
	ILT_RESULTS_DIR=$ILT_DIR/ilt-results-$(date "+%Y-%m-%d-%H:%M:%S")
fi

if [ -z "$BACKEND_SERVICES" ]
then
	# use default (MQTT/JEE) Discovery and Access Control
	BACKEND_SERVICES=MQTT
elif [ "$BACKEND_SERVICES" != "MQTT" ] && [ "$BACKEND_SERVICES" != "HTTP" ]
then
	log 'Invalid value for backend services: $BACKEND_SERVICES.'
	exit 1
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

	if [ ! -f "$ILT_DIR/target/discovery-jee.war" ]
	then
		log 'discovery-jee.war not found in $ILT_DIR/target/discovery-jee.war'
		exit 1
	fi

	if [ ! -f "$ILT_DIR/target/bounceproxy.war" ]
	then
		log 'bounceproxy.war not found in $ILT_DIR/target/bounceproxy.war'
		exit 1
	fi

	if [ ! -f "$ILT_DIR/target/accesscontrol-jee.war" ]
	then
		log 'accesscontrol-jee.war not found in $ILT_DIR/target/accesscontrol-jee.war'
		exit 1
	fi
}

function start_payara {
	DISCOVERY_WAR_FILE=$ILT_DIR/target/discovery-jee.war
	ACCESS_CONTROL_WAR_FILE=$ILT_DIR/target/accesscontrol-jee.war

    echo "Starting payara"

	asadmin start-database
	asadmin start-domain

	asadmin deploy --force=true $DISCOVERY_WAR_FILE
	asadmin deploy --force=true $ACCESS_CONTROL_WAR_FILE

    echo "payara started"
}

function stop_payara {
    echo "stopping payara"
	for app in `asadmin list-applications | egrep '(discovery|access)' | cut -d" " -f1`;
	do
		echo "undeploy $app";
		asadmin undeploy --droptables=true $app;
	done

	asadmin stop-domain
	asadmin stop-database
}

function start_jetty {
	mvn $SPECIAL_MAVEN_OPTIONS jetty:run-war --quiet > $ILT_RESULTS_DIR/jetty-$1.log 2>&1 &
	JETTY_PID=$!
	echo "Starting Jetty with PID $JETTY_PID"
	# wait until server is up and running or 60 seconds (= 30 * 2) timeout is exceeded
	started=
	count=0
	while [ "$started" != "200" -a "$count" -lt "30" ]
	do
		sleep 2
		started=`curl -o /dev/null --silent --head --write-out '%{http_code}\n' http://localhost:8080/bounceproxy/time/`
		let count+=1
	done
	if [ "$started" != "200" ]
	then
		log "Starting Jetty FAILED"
		# startup failed
		stopall
	fi
	echo "Jetty successfully started."
}

function stop_jetty {
	if [ -n "$JETTY_PID" ]
	then
		cd $ILT_DIR
		mvn $SPECIAL_MAVEN_OPTIONS jetty:stop --quiet
		wait $JETTY_PID
		echo "Stopped Jetty with PID $JETTY_PID"
		JETTY_PID=""
	fi
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

	if [ "$BACKEND_SERVICES" = "HTTP" ]
	then
		start_jetty
	else
		start_payara
	fi
	sleep 10
}

function stop_services {
	log '# stopping services'

	if [ "$BACKEND_SERVICES" = "HTTP" ]
	then
		stop_jetty
	else
		stop_payara
	fi

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
		if [ "$BACKEND_SERVICES" = "HTTP" ]
		then
			mvn exec:java -Dexec.mainClass="io.joynr.runtime.ClusterController" -Dexec.args="-t http" -Djoynr.messaging.gcd.url="http://localhost:8080/discovery/channels/discoverydirectory_channelid/" > $ILT_RESULTS_DIR/clustercontroller-java-$1.log 2>&1 &
		else
			mvn exec:java -Dexec.mainClass="io.joynr.runtime.ClusterController" -Dexec.args="-t mqtt" > $ILT_RESULTS_DIR/clustercontroller-java-$1.log 2>&1 &
		fi
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
		if [ "$BACKEND_SERVICES" = "HTTP" ]
		then
			./cluster-controller resources/cc.http.messaging.settings > $ILT_RESULTS_DIR/clustercontroller-cpp-$1.log 2>&1 &
		else
			./cluster-controller resources/cc.mqtt.messaging.settings > $ILT_RESULTS_DIR/clustercontroller-cpp-$1.log 2>&1 &
		fi
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
	if [ "$BACKEND_SERVICES" = "HTTP" ]
	then
		mvn $SPECIAL_MAVEN_OPTIONS exec:java -Dexec.mainClass="io.joynr.test.interlanguage.IltProviderApplication" -Dexec.args="$DOMAIN http" -Djoynr.messaging.gcd.url="http://localhost:8080/discovery/channels/discoverydirectory_channelid/" > $ILT_RESULTS_DIR/provider-java-cc.log 2>&1 &
	else
		mvn $SPECIAL_MAVEN_OPTIONS exec:java -Dexec.mainClass="io.joynr.test.interlanguage.IltProviderApplication" -Dexec.args="$DOMAIN mqtt" > $ILT_RESULTS_DIR/provider-java-cc.log 2>&1 &
	fi
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

function start_cpp_provider {
	log 'Starting C++ provider.'
	PROVIDER_DIR=$ILT_BUILD_DIR/provider_bin
	rm -fr $PROVIDER_DIR
	cp -a $ILT_BUILD_DIR/bin $PROVIDER_DIR
	cd $PROVIDER_DIR
	[[ $? == "0" ]] && echo "cd $PROVIDER_DIR OK"
	./ilt-provider-ws $DOMAIN > $ILT_RESULTS_DIR/provider-cpp.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started C++ provider with PID $PROVIDER_PID in directory $PROVIDER_DIR"
	# Allow some time for startup
	sleep 10
}

function start_javascript_provider {
	log 'Starting Javascript provider.'
	cd $ILT_DIR
	nohup npm run-script startprovider --interlanguageTest:domain=$DOMAIN > $ILT_RESULTS_DIR/provider-javascript.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started Javascript provider with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function start_javascript_provider_bundle {
	log 'Starting Javascript provider bundle.'
	cd $ILT_DIR
	nohup npm run startproviderbundle --interlanguageTest:domain=$DOMAIN > $ILT_RESULTS_DIR/provider-javascript-bundle.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started Javascript provider with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function start_java_consumer_cc {
	log 'Starting Java consumer CC (with in process clustercontroller).'
	cd $ILT_DIR
	rm -f java-consumer.persistence_file
	mkdir $ILT_RESULTS_DIR/consumer-java-cc-$1
	rm -fr $ILT_DIR/target/surefire-reports
	if [ "$BACKEND_SERVICES" = "HTTP" ]
	then
		mvn $SPECIAL_MAVEN_OPTIONS surefire:test -Dtransport=http -Djoynr.messaging.gcd.url="http://localhost:8080/discovery/channels/discoverydirectory_channelid/" -DskipTests=false >> $ILT_RESULTS_DIR/consumer-java-cc-$1.log 2>&1
	else
		mvn $SPECIAL_MAVEN_OPTIONS surefire:test -Dtransport=mqtt -DskipTests=false >> $ILT_RESULTS_DIR/consumer-java-cc-$1.log 2>&1
	fi
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

function start_cpp_consumer {
	log 'Starting C++ consumer.'
	CONSUMER_DIR=$ILT_BUILD_DIR/consumer-bin
	cd $ILT_BUILD_DIR
	rm -fr $CONSUMER_DIR
	cp -a $ILT_BUILD_DIR/bin $CONSUMER_DIR
	cd $CONSUMER_DIR
	[[ $? == "0" ]] && echo "cd $CONSUMER_DIR OK"
	#./ilt-consumer-cc $DOMAIN >> $ILT_RESULTS_DIR/consumer-cpp-$1.log 2>&1
	./ilt-consumer-ws $DOMAIN --gtest_color=yes --gtest_output="xml:$ILT_RESULTS_DIR/consumer-cpp-$1.junit.xml" >> $ILT_RESULTS_DIR/consumer-cpp-$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
	log 'C++ consumer FAILED.'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
	log 'C++ consumer successfully completed.'
	fi

	log 'Starting C++ consumer.'
	./ilt-consumer-proxy-provider-interface-mismatch-ws $DOMAIN --gtest_color=yes --gtest_output="xml:$ILT_RESULTS_DIR/consumer-cpp-proxy-provider-mismatch$1.junit.xml" >> $ILT_RESULTS_DIR/consumer-cpp-proxy-provider-mismatch-$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
	log 'C++ consumer proxy-provider-mismatch FAILED.'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
	log 'C++ consumer proxy-provider-mismatch successfully completed.'
	fi
}

function start_javascript_consumer {
	log 'Starting Javascript consumer.'
	cd $ILT_DIR
	rm -fr localStorageStorage
	npm run-script startjasmine --interlanguageTest:domain=$DOMAIN > $ILT_RESULTS_DIR/consumer-javascript-$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
		log 'Javascript consumer FAILED.'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
		log 'Javascript consumer successfully completed.'
	fi
}

function start_consumers {
	start_java_consumer_cc $1
	start_java_consumer_ws $1
	start_cpp_consumer $1
	start_javascript_consumer $1
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
stop_services

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
stop_services

# run checks with C++ provider
clean_up
log 'RUN CHECKS WITH C++ PROVIDER.'
PROVIDER="provider-cpp"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_cpp_provider
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services

# run checks with Javascript provider
clean_up
log 'RUN CHECKS WITH JAVASCRIPT PROVIDER.'
PROVIDER="provider-javascript"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_javascript_provider
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services

# run checks with Javascript provider
clean_up
log 'RUN CHECKS WITH JAVASCRIPT BUNDLE PROVIDER.'
PROVIDER="provider-javascript-bundle"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_javascript_provider_bundle
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services

# check for failed tests
if [ "$FAILED_TESTS" -gt 0 ]
then
	test_failed
	exit $FAILED_TESTS
fi

# If this point is reached, the tests have been successfull.
log 'TEST SUCCEEDED :)'
exit $SUCCESS
