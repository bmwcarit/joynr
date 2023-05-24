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

function wait_for_payara() {
  for i in {1..60}
  do
    if curl -s --head --request GET http://localhost:4848 | grep "200 OK" > /dev/null; then
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

asadmin --interactive=false --user admin --passwordFile=/opt/payara/passwordFile deploy /sit-jee-app.war
wait $PID
