#!/bin/bash

function wait_for_endpoint {
	retry_count=0
	max_retries=60
	until curl -f -s $1 || [ $retry_count -ge $max_retries ]
	do
		echo "$1 not available yet ..."
		retry_count=`expr $retry_count + 1`
		sleep 5
	done
	if [ $retry_count -gt $max_retries ]
	then
		echo "$1 not reachable in time."
		return -1
	fi
	return 0
}

function start_tests {
	# We'll assume it takes AT LEAST 15 sec to start up the other containers
	sleep 15
	RESULT_SST=""
	wait_for_endpoint "http://monitor-app:8080/monitor-app/test/0" && \
		wait_for_endpoint "http://clustered-app-node-1:8080/clustered-app/callstats" && \
		wait_for_endpoint "http://clustered-app-node-2:8080/clustered-app/callstats" && \
		printf "\n\n >>>  STARTING SHARED SUBSCRIPTION TEST  <<<\n\n" && \
		RESULT_SST="$(curl http://monitor-app:8080/monitor-app/test/100)"
	echo $RESULT_SST
	if [ -n "$(echo $RESULT_SST | grep '100 were successful')" ]; then
		echo "[SharedSubsTest] SUCCESS"
	else
		echo "[SharedSubsTest] FAILURE"
	fi

	RESULT_BACKPRESSURE=""
	wait_for_endpoint "http://backpressure-monitor-app:8080/backpressure-monitor-app/test/0" && \
		wait_for_endpoint "http://backpressure-provider-small:8080/backpressure-provider-small/callstats" && \
		wait_for_endpoint "http://backpressure-provider-large:8080/backpressure-provider-large/callstats" && \
		printf "\n\n >>>  STARTING BACKPRESSURE TEST  <<<\n\n" && \
		RESULT_BACKPRESSURE=="$(curl http://backpressure-monitor-app:8080/backpressure-monitor-app/test/500)"
	echo $RESULT_BACKPRESSURE
	if [ -n "$(echo $RESULT_BACKPRESSURE | grep 'SUCCESS')" ]; then
		echo "[BackpressureTest] SUCCESS"
	else
		echo "[BackpressureTest] FAILURE"
	fi
}

PID=$!
start_tests &
# Prevent script from exiting which would cause the container to terminate immediately
wait $PID
