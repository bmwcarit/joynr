#!/bin/bash

echo "Check that joynrbackend is running ..."

while [ -z "$(echo "\n" | curl -v telnet://joynrbackend:9998 2>&1 | grep 'OK')" ]; do
    echo "Waiting for joynr backend to become available ..."
    sleep 5
done

echo "Starting application ..."

java -jar /app.jar --postbootcommandfile /post-boot.txt
