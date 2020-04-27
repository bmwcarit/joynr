#!/bin/bash

# Give the JEE Discovery Directory a chance to start ...

wait $PID

function call_consumer {
    # sleep a few seconds to give Payara some time to startup
    sleep 40
    printf "\n\n >>>  STARTING consumer of standalone performance test  <<<\n\n" && \
    PT_RESULT=$(curl -f -s http://localhost:8080/pt-jee-app/performance-tests/test)
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne 0 ]
    then
        echo "PT RESULT error: failed to start PT test"
    fi
    if [ -z "$PT_RESULT" ]
    then
        echo "PT RESULT error: response from PT test is empty"
    fi
    echo "$PT_RESULT"
    asadmin stop-domain
    exit $EXIT_CODE
}

asadmin --interactive=false start-domain --debug --verbose &
PID=$!
sleep 40
asadmin --interactive=false --user admin --passwordfile=/opt/payara41/pwdfile deploy /pt-jee-app.war
call_consumer
wait $PID

