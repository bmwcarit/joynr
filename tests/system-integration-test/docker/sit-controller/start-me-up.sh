#!/bin/bash

# Give the JEE Discovery Directory a chance to start ...
sleep 30

function wait_for_endpoint {
	retry_count=0
	max_retries=60
	until curl --noproxy localhost -f -s $2 || ((retry_count++ > max_retries))
	do
		echo "$1 ping not started yet ..."
		sleep 2
	done
	if (( retry_count > max_retries )); then
		echo "SIT RESULT error: $1 failed to start in time."
        asadmin stop-domain
		exit 1
	fi
    echo " $1 started successfully."
	return 0
}

function call_consumer {
	# We'll assume it takes AT LEAST 15 sec to start up Payara
	sleep 15
	wait_for_endpoint "SIT controller" "http://localhost:8080/sit-controller/ping" && \
	printf "\n\n >>>  STARTING SIT CONTROLLER  <<<\n\n" && \
	SIT_RESULT=$(curl --noproxy localhost -f -s http://localhost:8080/sit-controller/test)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ]
    then
        echo "SIT RESULT error: failed to start SIT controller"
    fi
    if [ -z "$SIT_RESULT" ]
    then
        echo "SIT RESULT error: response from SIT controller is empty"
    fi
    echo "$SIT_RESULT"
    asadmin --user admin --passwordFile=/opt/payara/passwordFile stop-domain
    exit $EXIT_CODE
}

asadmin --user admin --passwordFile=/opt/payara/passwordFile --interactive=false start-domain --debug --verbose &
PID=$!
sleep 40
asadmin --interactive=false --user admin --passwordFile=/opt/payara/passwordFile deploy /sit-controller.war
call_consumer
wait $PID
