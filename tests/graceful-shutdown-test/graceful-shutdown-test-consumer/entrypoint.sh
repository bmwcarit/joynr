#!/bin/sh

while [ -z "$(curl -v http://provider:8080/control/ping 2>&1 | grep '200 OK')" ]; do
    echo "Waiting for provider to become available ..."
    sleep 5
done

echo "Starting application ..."

java -cp /app.jar io.joynr.tests.gracefulshutdown.Bootstrap &
CONSUMER_PID=$!

sleep 20
curl http://provider:8080/control/prepareForShutdown
sleep 20
FAILURE_REPORT="$(curl http://secondlevel:8080/control/failureReport)"
echo "Second level failure report: ${FAILURE_REPORT}"

echo "Stopping consumer ..."
kill -9 ${CONSUMER_PID}
echo "FINISHED"
