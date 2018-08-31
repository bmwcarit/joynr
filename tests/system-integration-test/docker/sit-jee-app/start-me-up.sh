#!/bin/bash

# Give the JEE Discovery Directory a chance to start ...
sleep 30

function wait_for_endpoint {
	retry_count=0
	max_retries=60
	until curl -f -s http://localhost:8080/sit-jee-app/consumer/ping || ((retry_count++ > max_retries))
	do
		echo "JEE Consumer ping not started yet ..."
		sleep 2
	done
	if (( retry_count > max_retries )); then
		echo "SIT RESULT: JEE Consumer failed to start in time."
		return -1
	fi
	return 0
}

function call_consumer {
	# We'll assume it takes AT LEAST 15 sec to start up Payara
	sleep 15
	wait_for_endpoint && \
	printf "\n\n >>>  STARTING JEE CONSUMER  <<<\n\n" && \
	curl http://localhost:8080/sit-jee-app/consumer
}

call_consumer &

asadmin start-domain --debug --verbose
