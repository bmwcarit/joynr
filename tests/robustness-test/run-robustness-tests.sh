#!/bin/bash
#set -x

if [ -f /data/src/docker/joynr-base/scripts/testbase.sh ]
then
    source /data/src/docker/joynr-base/scripts/testbase.sh
else
    echo "testbase.sh script not found in /data/src/docker/joynr-base/scripts/ - aborting"
    exit 1
fi

parse_arguments $@
folder_prechecks

CPP_TESTS="ON"
JAVA_TESTS="OFF"
JS_TESTS="OFF"
DOMAIN="joynr-robustness-test-domain"

# in case of interrupts, forcibly kill background stuff
function stopall {
    stop_cpp_providers
    stop_java_providers
    stop_javascript_providers
    stop_cluster_controller
    stop_services
    exit 1
}

log "ENVIRONMENT"
env

function prechecks {

    if [ ! -f "$TEST_BUILD_DIR/bin/robustness-tests-provider-ws" ]
    then
        echo 'C++ provider not built'
        exit 1
    fi
}

function start_services {
    cd $JOYNR_SOURCE_DIR/tests/robustness-test
    rm -f joynr.properties
    rm -f joynr_participantIds.properties

    start_mosquitto
    start_payara
    sleep 5
}

function stop_services {
    stop_payara
    stop_mosquitto
}

function start_cluster_controller {
    $JOYNR_SOURCE_DIR/tests/robustness-test/start-clustercontroller.sh
}

function start_java_provider {
	echo '####################################################'
	echo '# starting Java provider'
	echo '####################################################'
	cd $JOYNR_SOURCE_DIR/tests/robustness-test
	rm -f java-provider.persistence_file
	rm -f java-consumer.persistence_file
	mvn $SPECIAL_MAVEN_OPTIONS exec:java -Dexec.mainClass="io.joynr.test.robustness.RobustnessProviderApplication" -Dexec.args="$DOMAIN mqtt" > $TEST_RESULTS_DIR/provider_java_$(date "+%Y-%m-%d-%H:%M:%S").log 2>&1 &
	PROVIDER_PID=$!
	disown $PROVIDER_PID
	echo "Started Java provider with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function start_cpp_provider {
	echo '####################################################'
	echo '# starting C++ provider'
	echo '####################################################'
	PROVIDER_DIR=$TEST_BUILD_DIR/provider_bin
	rm -fr $PROVIDER_DIR
	cp -a $TEST_BUILD_DIR/bin $PROVIDER_DIR
	cd $PROVIDER_DIR
	./robustness-tests-provider-ws $DOMAIN > $TEST_RESULTS_DIR/provider_cpp_$(date "+%Y-%m-%d-%H:%M:%S").log 2>&1 &
	PROVIDER_PID=$!
	disown $PROVIDER_PID
	echo "Started C++ provider with PID $PROVIDER_PID in directory $PROVIDER_DIR"
	# Allow some time for startup
	sleep 5
}

function start_javascript_provider {
	echo '####################################################'
	echo '# starting Javascript provider'
	echo '####################################################'
	cd $JOYNR_SOURCE_DIR/tests/robustness-test
	nohup npm run-script startprovider --robustness-test:domain=$DOMAIN > $TEST_RESULTS_DIR/provider_javascript_$(date "+%Y-%m-%d-%H:%M:%S").log 2>&1 &
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
	mvn $SPECIAL_MAVEN_OPTIONS exec:java -Dexec.mainClass="io.joynr.test.robustness.RobustnessConsumerApplication" -Dexec.args="$DOMAIN mqtt" >> $TEST_RESULTS_DIR/consumer_java_$1_$(date "+%Y-%m-%d-%H:%M:%S").log 2>&1
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
	cd $TEST_BUILD_DIR/bin
	./robustness-tests-ws $DOMAIN >> $TEST_RESULTS_DIR/consumer_cpp_$1_$(date "+%Y-%m-%d-%H:%M:%S").log 2>&1
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
	npm run-script startjasmine --robustness-test:domain=$DOMAIN --robustness-test:testcase=$TESTCASE --robustness-test:cmdPath=$JOYNR_SOURCE_DIR/tests/robustness-test > $TEST_RESULTS_DIR/consumer_javascript_$1_$(date "+%Y-%m-%d-%H:%M:%S").log 2>&1
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
rm -fr $TEST_RESULTS_DIR
mkdir -p $TEST_RESULTS_DIR
cd $JOYNR_SOURCE_DIR/tests/robustness-test
rm -fr node_modules
rm -fr localStorageStorage
rm -fr reports
rm -f npm-debug.log
rm -f joynr_participantIds.properties

export JOYNR_LOG_LEVEL=TRACE

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
	stop_java_providers
	stop_cluster_controller
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
        stop_cpp_providers
	stop_cluster_controller
	stop_services
fi

if [ "$JS_TESTS" == "ON" ]
then
	echo '####################################################'
	echo '####################################################'
	echo '# RUN CHECKS WITH JAVASCRIPT'
	echo '####################################################'
	echo '####################################################'

	# prepare JavaScript
	npm run preinstall
	npm install
	npm install jasmine-node

	TESTCASE="js_tests"
	start_services $TESTCASE
	start_cluster_controller $TESTCASE
	start_javascript_provider
	start_javascript_consumer $TESTCASE
	stop_javascript_providers
	stop_cluster_controller
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
	stop_cpp_providers
	stop_cluster_controller
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
