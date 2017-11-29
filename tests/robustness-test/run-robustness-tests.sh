#!/bin/bash
#set -x

JOYNR_SOURCE_DIR=""
ROBUSTNESS_BUILD_DIR=""
ROBUSTNESS_RESULTS_DIR=""
CPP_TESTS="ON"
JAVA_TESTS="OFF"
JS_TESTS="ON"

while getopts "b:s:r:" OPTIONS;
do
	case $OPTIONS in
		b)
			ROBUSTNESS_BUILD_DIR=$OPTARG
			;;
		r)
			ROBUSTNESS_RESULTS_DIR=$OPTARG
			;;
		s)
			JOYNR_SOURCE_DIR=$OPTARG
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
			echo "Synopsis: run-robustness-tests.sh [-b <joynr-build-dir>] [-r <robustness-results-dir>] [-s <joynr-source-dir>]"
			exit 1
			;;
	esac
done

if [ -z "$JOYNR_SOURCE_DIR" ]
then
	# assume this script is started inside a git repo subdirectory,
	JOYNR_SOURCE_DIR=`git rev-parse --show-toplevel`
fi

if [ -z "$ROBUSTNESS_BUILD_DIR" ]
then
	ROBUSTNESS_BUILD_DIR=$JOYNR_SOURCE_DIR/tests/robustness-test/build
fi

# if CI environment, source global settings
if [ -f /data/scripts/global.sh ]
then
	source /data/scripts/global.sh
fi

if [ -z "$ROBUSTNESS_RESULTS_DIR" ]
then
	ROBUSTNESS_RESULTS_DIR=$JOYNR_SOURCE_DIR/tests/robustness-test/robustness-results-$(date "+%Y-%m-%d-%H:%M:%S")
fi

# Expand the PATH in order to let the test consumers find
# the tools to kill / restart cluster-controller or provider
export PATH=$PATH:$JOYNR_SOURCE_DIR/tests/robustness-test

# process ids for background stuff
MOSQUITTO_PID=""

# in case of interrupts, forcibly kill background stuff
function stopall {
	stop_any_provider
	stop_any_cluster_controller
	stop_services
	exit 1
}

trap stopall INT

#log "ENVIRONMENT"
#env

SUCCESS=0
FAILED_TESTS=0
DOMAIN="joynr-robustness-test-domain"

function prechecks {
	if [ ! -f "$ROBUSTNESS_BUILD_DIR/bin/cluster-controller" ]
	then
		echo 'C++ environment not built'
		exit 1
	fi

	if [ ! -f "$ROBUSTNESS_BUILD_DIR/bin/robustness-tests-provider-ws" ]
	then
		echo 'C++ environment not built'
		exit 1
	fi

	if [ ! -f "$JOYNR_SOURCE_DIR/tests/robustness-test/target/discovery.war" ]
	then
		echo 'Java environment not built'
		exit 1
	fi

	if [ ! -f "$JOYNR_SOURCE_DIR/tests/robustness-test/target/bounceproxy.war" ]
	then
		echo 'Java environment not built'
		exit 1
	fi

	if [ ! -f "$JOYNR_SOURCE_DIR/tests/robustness-test/target/accesscontrol.war" ]
	then
		echo 'Java environment not built'
		exit 1
	fi
}

function start_payara {
	DISCOVERY_WAR_FILE=$JOYNR_SOURCE_DIR/tests/robustness-test/target/discovery-jee.war
	ACCESS_CONTROL_WAR_FILE=$JOYNR_SOURCE_DIR/tests/robustness-test/target/accesscontrol-jee.war

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

function start_services {
	cd $JOYNR_SOURCE_DIR/tests/robustness-test
	rm -f joynr.properties
	rm -f joynr_participantIds.properties
	echo '####################################################'
	echo '# starting services'
	echo '####################################################'

	echo "Starting mosquitto"
	mosquitto -c /etc/mosquitto/mosquitto.conf > $ROBUSTNESS_RESULTS_DIR/mosquitto-$1.log 2>&1 &
	MOSQUITTO_PID=$!
	echo "Mosquitto started with PID $MOSQUITTO_PID"

	start_payara

	sleep 5
}

function stop_services {
	echo '####################################################'
	echo '# stopping services'
	echo '####################################################'

	stop_payara

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
	echo '####################################################'
	echo '# starting C++ clustercontroller'
	echo '####################################################'
	if [ ! -d $ROBUSTNESS_BUILD_DIR -o ! -d $ROBUSTNESS_BUILD_DIR/bin ]
	then
		echo "C++ build directory or build/bin directory does not exist!"
		stopall
	fi
	CLUSTER_CONTROLLER_DIR=$ROBUSTNESS_BUILD_DIR/cluster_controller_bin
	cd $ROBUSTNESS_BUILD_DIR
	rm -fr $CLUSTER_CONTROLLER_DIR
	cp -a $ROBUSTNESS_BUILD_DIR/bin $CLUSTER_CONTROLLER_DIR
	cd $CLUSTER_CONTROLLER_DIR
	./cluster-controller $CLUSTER_CONTROLLER_DIR/resources/cc.messaging.settings > $ROBUSTNESS_RESULTS_DIR/clustercontroller_$1.log 2>&1 &
	CLUSTER_CONTROLLER_PID=$!
	disown $CLUSTER_CONTROLLER_PID
	echo "Started external cluster controller with PID $CLUSTER_CONTROLLER_PID in directory $CLUSTER_CONTROLLER_DIR"
	# Allow some time for startup
	sleep 5
}

function stop_any_cluster_controller {
	echo '####################################################'
	echo '# stopping C++ clustercontroller'
	echo '####################################################'
	$JOYNR_SOURCE_DIR/tests/robustness-test/kill-clustercontroller.sh
}

function test_failed {
	echo '####################################################'
	echo '# TEST FAILED'
	echo '####################################################'
}

function start_java_provider {
	echo '####################################################'
	echo '# starting Java provider'
	echo '####################################################'
	cd $JOYNR_SOURCE_DIR/tests/robustness-test
	rm -f java-provider.persistence_file
	rm -f java-consumer.persistence_file
	mvn $SPECIAL_MAVEN_OPTIONS exec:java -Dexec.mainClass="io.joynr.test.robustness.RobustnessProviderApplication" -Dexec.args="$DOMAIN mqtt" > $ROBUSTNESS_RESULTS_DIR/provider_java.log 2>&1 &
	PROVIDER_PID=$!
	disown $PROVIDER_PID
	echo "Started Java provider with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function stop_any_provider {
	echo '####################################################'
	echo '# stopping any provider'
	echo '####################################################'
	# javascript provider
	PIDS=`pgrep -f provider.js`
	if [ -z "$PIDS" ]
	then
		# search java provider
		PIDS=`pgrep -f RobustnessProviderApplication`
		if [ -z "$PIDS" ]
		then
			# search C++ provider
			PIDS=`pgrep -f robustness-tests-provider-ws`
			if [ -z "$PIDS" ]
			then
				echo "No provider found."
				return
			fi
		fi
	fi
	echo "Found provider with pid $PIDS, about to kill it"
	for PID in $PIDS
	do
		kill -9 $PID
		while (test -d /proc/$PID)
		do
			echo "PID $PID still alive. Waiting ..."
			sleep 1
		done
		echo "PID $PID exited."
	done
}

function start_cpp_provider {
	echo '####################################################'
	echo '# starting C++ provider'
	echo '####################################################'
	PROVIDER_DIR=$ROBUSTNESS_BUILD_DIR/provider_bin
	rm -fr $PROVIDER_DIR
	cp -a $ROBUSTNESS_BUILD_DIR/bin $PROVIDER_DIR
	cd $PROVIDER_DIR
	./robustness-tests-provider-ws $DOMAIN > $ROBUSTNESS_RESULTS_DIR/provider_cpp.log 2>&1 &
	PROVIDER_PID=$!
	disown $PROVIDER_PID
	echo "Started C++ provider with PID $PROVIDER_PID in directory $PROVIDER_DIR"
	# Allow some time for startup
	sleep 10
}

function start_javascript_provider {
	echo '####################################################'
	echo '# starting Javascript provider'
	echo '####################################################'
	cd $JOYNR_SOURCE_DIR/tests/robustness-test
	nohup npm run-script startprovider --robustness-test:domain=$DOMAIN > $ROBUSTNESS_RESULTS_DIR/provider_javascript.log 2>&1 &
	PROVIDER_PID=$!
	disown $PROVIDER_PID
	echo "Started Javascript provider with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function start_java_consumer {
	echo '####################################################'
	echo '# starting Java consumer'
	echo '####################################################'
	cd $JOYNR_SOURCE_DIR/tests/robustness-test
	rm -f java-consumer.persistence_file
	mvn $SPECIAL_MAVEN_OPTIONS exec:java -Dexec.mainClass="io.joynr.test.robustness.RobustnessConsumerApplication" -Dexec.args="$DOMAIN mqtt" >> $ROBUSTNESS_RESULTS_DIR/consumer_java_$1.log 2>&1
	SUCCESS=$?
	if [ "$SUCCESS" != 0 ]
	then
		echo '####################################################'
		echo '# Java consumer FAILED'
		echo '####################################################'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
		echo '####################################################'
		echo '# Java consumer successfully completed'
		echo '####################################################'
	fi
}

function start_cpp_consumer {
	echo '####################################################'
	echo '# starting C++ consumer'
	echo '####################################################'
	cd $ROBUSTNESS_BUILD_DIR/bin
	./robustness-tests-ws $DOMAIN >> $ROBUSTNESS_RESULTS_DIR/consumer_cpp_$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
		echo '####################################################'
		echo '# C++ consumer FAILED'
		echo '####################################################'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
		echo '####################################################'
		echo 'C++ consumer successfully completed'
		echo '####################################################'
	fi
}

function start_javascript_consumer {
	echo '####################################################'
	echo '# starting Javascript consumer'
	echo '####################################################'
	cd $JOYNR_SOURCE_DIR/tests/robustness-test
	rm -fr localStorageStorage
	npm run-script startjasmine --robustness-test:domain=$DOMAIN --robustness-test:testcase=$TESTCASE --robustness-test:cmdPath=$JOYNR_SOURCE_DIR/tests/robustness-test > $ROBUSTNESS_RESULTS_DIR/consumer_javascript_$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
		echo '####################################################'
		echo '# Javascript consumer FAILED'
		echo '####################################################'
		echo "STATUS = $SUCCESS"
		let FAILED_TESTS+=1
	else
		echo '####################################################'
		echo '# Javascript consumer successfully completed'
		echo '####################################################'
	fi
}

# TESTS

# check that the environment has been setup correctly
prechecks

# clean up
rm -fr $ROBUSTNESS_RESULTS_DIR
mkdir -p $ROBUSTNESS_RESULTS_DIR
cd $JOYNR_SOURCE_DIR/tests/robustness-test
rm -fr node_modules
rm -fr localStorageStorage
rm -fr reports
rm -f npm-debug.log
rm -f joynr_participantIds.properties

# prepare JavaScript
npm run preinstall
npm install
npm install jasmine-node

# run the checks
#
# Note that the joynr backend services need to be stopped once the
# provider is changed, since otherwise the discovery service
# continues to return data for the already stopped provider
# since it currently assumes that it will be restarted.

if [ "$JAVA_TESTS" == "ON" ]
then
	echo '####################################################'
	echo '####################################################'
	echo '# RUN CHECKS WITH JAVA'
	echo '####################################################'
	echo '####################################################'
	TESTCASE="java_tests"
	start_services $TESTCASE
	start_cluster_controller $TESTCASE
	start_java_provider
	start_java_consumer $TESTCASE
	stop_any_provider
	stop_any_cluster_controller
	stop_services
fi

if [ "$CPP_TESTS" == "ON" ]
then
	echo '####################################################'
	echo '####################################################'
	echo '# RUN CHECKS WITH C++'
	echo '####################################################'
	echo '####################################################'
	TESTCASE="cpp_tests"
	start_services $TESTCASE
	start_cluster_controller $TESTCASE
	start_cpp_provider
	start_cpp_consumer $TESTCASE
	stop_any_provider
	stop_any_cluster_controller
	stop_services
fi

if [ "$JS_TESTS" == "ON" ]
then
	echo '####################################################'
	echo '####################################################'
	echo '# RUN CHECKS WITH JAVASCRIPT'
	echo '####################################################'
	echo '####################################################'
	TESTCASE="js_tests"
	start_services $TESTCASE
	start_cluster_controller $TESTCASE
	start_javascript_provider
	start_javascript_consumer $TESTCASE
	stop_any_provider
	stop_any_cluster_controller
	echo '####################################################'
	echo '####################################################'
	echo '# RUN CHECKS WITH JAVASCRIPT and C++ PROVIDER'
	echo '####################################################'
	echo '####################################################'
	TESTCASE="js_cpp_tests"
	start_services $TESTCASE
	start_cluster_controller $TESTCASE
	start_cpp_provider
	start_javascript_consumer $TESTCASE
	stop_any_provider
	stop_any_cluster_controller
	stop_services
fi

# check for failed tests
if [ "$FAILED_TESTS" -gt 0 ]
then
	test_failed
	exit $FAILED_TESTS
fi

# If this point is reached, the tests have been successfull.
echo '####################################################'
echo '# TEST SUCCEEDED'
echo '####################################################'
exit $SUCCESS
