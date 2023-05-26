#!/bin/bash

function wait_for_gcd {
    try_count=0
    max_retries=24
    while [ -z "$(echo '\n' | curl -v telnet://$1:9998 2>&1 | grep 'OK')" ]
    do
        echo "GCD $1 not started yet ..."
        try_count=$((try_count+1))
        if [ $try_count -gt $max_retries ]; then
            echo "GCD $1 failed to start in time."
            exit 1
        fi
        echo "try_count ${try_count}"
        sleep 5
    done
    echo "GCD $1 started successfully."
    return 0
}

function wait_for_endpoint {
	retry_count=0
	max_retries=60
	until curl --noproxy localhost -f -s $2 || ((retry_count++ > max_retries))
	do
		echo "$1 ping not started yet ..."
		sleep 5
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

function wait_for_payara() {
  for i in {1..60}
  do
    if asadmin --user admin --passwordFile=/opt/payara/passwordFile get-healthcheck-configuration | grep "executed successfully." > /dev/null; then
      echo "attempt #$i: Payara Server is up"
      return 0
    else
      echo "attempt #$i: Payara Server is down"
    fi
    sleep 5
  done
  echo "Payara Server failed to start in time (5 minutes)."
  exit 1
}

asadmin --user admin --passwordFile=/opt/payara/passwordFile --interactive=false start-domain --debug --verbose &
PID=$!

# Give Payara time to start
wait_for_payara

wait_for_gcd "joynr-gcd-1"
wait_for_gcd "joynr-gcd-2"

asadmin --interactive=false --user admin --passwordFile=/opt/payara/passwordFile deploy /sit-controller.war
call_consumer
wait $PID
