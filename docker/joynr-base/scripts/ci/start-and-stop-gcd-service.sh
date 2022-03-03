#!/bin/sh

function wait_for_gcd {
    try_count=0
    max_retries=30
    while [ -z "$(echo '\n' | curl -v telnet://localhost:9998 2>&1 | grep 'OK')" ]
    do
        echo "GCD not started yet ..."
        try_count=$((try_count+1))
        if [ $try_count -gt $max_retries ]; then
            echo "GCD failed to start in time."
            kill -9 $GCD_PID
            return 1
        fi
        echo "try_count ${try_count}"
        sleep 2
    done
    echo "GCD started successfully."
    return 0
}

JAR_PATH=/data/src/java/backend-services/capabilities-directory/target/deploy

function startGcd {
    # At the point the db should be started

    echo 'start GCD'
    if [ -d "/data/logs" ]
    then
        java -Dlog4j.configuration="file:${JAR_PATH}/log4j.properties" -jar ${JAR_PATH}/capabilities-directory-jar-with-dependencies.jar 2>&1 > /data/logs/gcd.log &
    else
        java -Dlog4j.configuration="file:${JAR_PATH}/log4j.properties" -jar ${JAR_PATH}/capabilities-directory-jar-with-dependencies.jar &
    fi

    GCD_PID=$!
    wait_for_gcd
    return $?
}

function stopGcd
{
    echo 'stop GCD'
    # The app will shutdown if a network connection is attempted on localhost:9999
    (
        set +e
        # curl returns error because the server closes the connection. We do not need the ret val.
        timeout 1 curl telnet://127.0.0.1:9999
        exit 0
    )
    wait $GCD_PID
}
