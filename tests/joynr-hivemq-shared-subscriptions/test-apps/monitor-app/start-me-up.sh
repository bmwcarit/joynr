#!/bin/bash

echo "Check that joynrbackend is running ..."

while [ -z "$(curl -v http://joynrbackend:8080/health 2>&1 | grep '200 OK')" ]; do
    echo "Waiting for joynr backend to become available ..."
    sleep 5
done

echo "Starting application ..."

java -jar /app.jar --postbootcommandfile /post-boot.txt
