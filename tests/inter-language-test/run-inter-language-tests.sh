#!/bin/bash
#set -x

JOYNR_SOURCE_DIR=""
ILT_BUILD_DIR=""
ILT_RESULTS_DIR=""
CC_LANGUAGE=""
while getopts "b:c:s:r:" OPTIONS;
do
	case $OPTIONS in
		c)
			CC_LANGUAGE=$OPTARG
			;;
		b)
			ILT_BUILD_DIR=$OPTARG
			;;
		r)
			ILT_RESULTS_DIR=$OPTARG
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
			echo "Synopsis: run-inter-language-tests.sh [-b <joynr-build-dir>] [-c <cluster-controller-language (CPP|JAVA)>] [-r <ilt-results-dir>] [-s <joynr-source-dir>]"
			exit 1
			;;
	esac
done

if [ -z "$CC_LANGUAGE" ]
then
	# use default C++ cluster-controller
	CC_LANGUAGE=CPP
elif [ "$CC_LANGUAGE" != "CPP" ] && [ "$CC_LANGUAGE" != "JAVA" ]
then
	echo 'invalid value for cluster-controller language: $CC_LANGUAGE'
	exit 1
fi

if [ -z "$JOYNR_SOURCE_DIR" ]
then
	# assume this script is started inside a git repo subdirectory,
	JOYNR_SOURCE_DIR=`git rev-parse --show-toplevel`
fi

ILT_DIR=$JOYNR_SOURCE_DIR/tests/inter-language-test

if [ -z "$ILT_BUILD_DIR" ]
then
	ILT_BUILD_DIR=$ILT_DIR/build
fi

# if CI environment, source global settings
if [ -f /data/scripts/global.sh ]
then
	source /data/scripts/global.sh
fi

if [ -z "$ILT_RESULTS_DIR" ]
then
	ILT_RESULTS_DIR=$ILT_DIR/ilt-results-$(date "+%Y-%m-%d-%H:%M:%S")
fi

# process ids for background stuff
JETTY_PID=""
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

#log "ENVIRONMENT"
#env

SUCCESS=0
FAILED_TESTS=0
DOMAIN="joynr-inter-language-test-domain"

function prechecks {
	if [ ! -f "$ILT_BUILD_DIR/bin/cluster-controller" ]
	then
		echo 'C++ environment not built'
		exit 1
	fi

	if [ ! -f "$ILT_BUILD_DIR/bin/ilt-consumer-cc" ]
	then
		echo 'C++ environment not built'
		exit 1
	fi

	if [ ! -f "$ILT_BUILD_DIR/bin/ilt-provider-cc" ]
	then
		echo 'C++ environment not built'
		exit 1
	fi

	if [ ! -f "$ILT_DIR/target/discovery.war" ]
	then
		echo 'Java environment not built'
		exit 1
	fi

	if [ ! -f "$ILT_DIR/target/bounceproxy.war" ]
	then
		echo 'Java environment not built'
		exit 1
	fi

	if [ ! -f "$ILT_DIR/target/accesscontrol.war" ]
	then
		echo 'Java environment not built'
		exit 1
	fi
}

function start_services {
	cd $ILT_DIR
	rm -f joynr.properties
	rm -f joynr_participantIds.properties
	echo '####################################################'
	echo '# starting services'
	echo '####################################################'
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
		echo "Starting Jetty FAILED"
		# startup failed
		stopall
	fi
	echo "Jetty successfully started."
	sleep 5
}

function stop_services {
	if [ -n "$JETTY_PID" ]
	then
		cd $ILT_DIR
		echo '####################################################'
		echo '# stopping services'
		echo '####################################################'
		mvn $SPECIAL_MAVEN_OPTIONS jetty:stop --quiet
		wait $JETTY_PID
		echo "Stopped Jetty with PID $JETTY_PID"
		JETTY_PID=""
	fi
}

function start_cluster_controller {
	if [ "$CC_LANGUAGE" = "JAVA" ]
	then
		echo '####################################################'
		echo '# starting JAVA clustercontroller'
		echo '####################################################'
		CLUSTER_CONTROLLER_DIR=$JOYNR_SOURCE_DIR/java/core/clustercontroller-standalone
		cd $CLUSTER_CONTROLLER_DIR
		mvn exec:java -Dexec.mainClass="io.joynr.runtime.ClusterController" -Dexec.args="http::mqtt" > $ILT_RESULTS_DIR/clustercontroller-java-$1.log 2>&1 &
	else
		echo '####################################################'
		echo '# starting C++ clustercontroller'
		echo '####################################################'
		if [ ! -d $ILT_BUILD_DIR -o ! -d $ILT_BUILD_DIR/bin ]
		then
			echo "C++ build directory or build/bin directory does not exist!"
			stopall
		fi
		CLUSTER_CONTROLLER_DIR=$ILT_BUILD_DIR/cluster-controller-bin
		cd $ILT_BUILD_DIR
		rm -fr $CLUSTER_CONTROLLER_DIR
		cp -a $ILT_BUILD_DIR/bin $CLUSTER_CONTROLLER_DIR
		cd $CLUSTER_CONTROLLER_DIR
		./cluster-controller > $ILT_RESULTS_DIR/clustercontroller-cpp-$1.log 2>&1 &
	fi
	CLUSTER_CONTROLLER_PID=$!
	echo "Started external cluster controller with PID $CLUSTER_CONTROLLER_PID in directory $CLUSTER_CONTROLLER_DIR"
	# Allow some time for startup
	sleep 5
}

function stop_cluster_controller {
	if [ -n "$CLUSTER_CONTROLLER_PID" ]
	then
		echo '####################################################'
		echo '# stopping clustercontroller'
		echo '####################################################'
		disown $CLUSTER_CONTROLLER_PID
		kill -9 $CLUSTER_CONTROLLER_PID
		wait $CLUSTER_CONTROLLER_PID
		echo "Stopped external cluster controller with PID $CLUSTER_CONTROLLER_PID"
		CLUSTER_CONTROLLER_PID=""
	fi
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
	cd $ILT_DIR
	rm -f java-provider.persistence_file
	rm -f java-consumer.persistence_file
	mvn $SPECIAL_MAVEN_OPTIONS exec:java -Dexec.mainClass="io.joynr.test.interlanguage.IltProviderApplication" -Dexec.args="$DOMAIN http:mqtt" -Djoynr.messaging.primaryglobaltransport=mqtt > $ILT_RESULTS_DIR/provider-java.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started Java provider with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function stop_provider {
	if [ -n "$PROVIDER_PID" ]
	then
		echo '####################################################'
		echo '# stopping provider'
		echo '####################################################'
		cd $ILT_DIR
		disown $PROVIDER_PID
		kill -9 $PROVIDER_PID
		wait $PROVIDER_PID
		echo "Stopped provider with PID $PROVIDER_PID"
		PROVIDER_PID=""
	fi
}

function start_cpp_provider {
	echo '####################################################'
	echo '# starting C++ provider'
	echo '####################################################'
	PROVIDER_DIR=$ILT_BUILD_DIR/provider_bin
	rm -fr $PROVIDER_DIR
	cp -a $ILT_BUILD_DIR/bin $PROVIDER_DIR
	cd $PROVIDER_DIR
	./ilt-provider-cc $DOMAIN > $ILT_RESULTS_DIR/provider-cpp.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started C++ provider with PID $PROVIDER_PID in directory $PROVIDER_DIR"
	# Allow some time for startup
	sleep 10
}

function start_javascript_provider {
	echo '####################################################'
	echo '# starting Javascript provider'
	echo '####################################################'
	cd $ILT_DIR
	nohup npm run-script startprovider --interlanguageTest:domain=$DOMAIN > $ILT_RESULTS_DIR/provider-javascript.log 2>&1 &
	PROVIDER_PID=$!
	echo "Started Javascript provider with PID $PROVIDER_PID"
	# Allow some time for startup
	sleep 10
}

function start_java_consumer {
	echo '####################################################'
	echo '# starting Java consumer'
	echo '####################################################'
	cd $ILT_DIR
	rm -f java-consumer.persistence_file
	mkdir $ILT_RESULTS_DIR/consumer-java-$1
	rm -fr $ILT_DIR/target/surefire-reports
	mvn $SPECIAL_MAVEN_OPTIONS surefire:test -DskipTests=false >> $ILT_RESULTS_DIR/consumer-java-$1.log 2>&1
	SUCCESS=$?
	cp -a $ILT_DIR/target/surefire-reports $ILT_RESULTS_DIR/consumer-java-$1
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
	cd $ILT_BUILD_DIR/bin
	#./ilt-consumer-cc $DOMAIN >> $ILT_RESULTS_DIR/consumer-cpp-$1.log 2>&1
	./ilt-consumer-ws $DOMAIN >> $ILT_RESULTS_DIR/consumer-cpp-$1.log 2>&1
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
	cd $ILT_DIR
	rm -fr localStorageStorage
	#npm install
	#npm install jasmine-node
	npm run-script startjasmine --interlanguageTest:domain=$DOMAIN > $ILT_RESULTS_DIR/consumer-javascript-$1.log 2>&1
	SUCCESS=$?

	if [ "$SUCCESS" != 0 ]
	then
		echo '####################################################'
		echo '# Javascript consumer FAILED'
		echo '####################################################'
		echo "STATUS = $SUCCESS"
		#test_failed
		let FAILED_TESTS+=1
		#stopall
	else
		echo '####################################################'
		echo '# Javascript consumer successfully completed'
		echo '####################################################'
	fi
}

function start_consumers {
	start_java_consumer $1
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
rm -fr localStorageStorage
rm -fr reports
rm -f npm-debug.log
rm -f joynr_participantIds.properties

# prepare JavaScript
npm install
npm install jasmine-node

# run the checks
#
# the start of consumers includes start/stop of seperate cluster
# controller executables, if required (e.g. for Javascript).
#
# Note that the services (jetty) need to be stopped once the
# provider is changed, since otherwise the discovery service
# continues to return data for the already stopped provider
# since it currently assumes that it will be restarted.

# run checks with Java provider
echo '####################################################'
echo '####################################################'
echo '# RUN CHECKS WITH JAVA PROVIDER'
echo '####################################################'
echo '####################################################'
PROVIDER="provider-java"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_java_provider
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services

# run checks with C++ provider
echo '####################################################'
echo '####################################################'
echo '# RUN CHECKS WITH C++ PROVIDER'
echo '####################################################'
echo '####################################################'
PROVIDER="provider-cpp"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_cpp_provider
start_consumers $PROVIDER
stop_provider
stop_cluster_controller
stop_services

# run checks with Javascript provider
echo '####################################################'
echo '####################################################'
echo '# RUN CHECKS WITH JAVASCRIPT PROVIDER'
echo '####################################################'
echo '####################################################'
PROVIDER="provider-javascript"
start_services $PROVIDER
start_cluster_controller $PROVIDER
start_javascript_provider
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
echo '####################################################'
echo '# TEST SUCCEEDED'
echo '####################################################'
exit $SUCCESS
