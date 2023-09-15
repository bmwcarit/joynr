#!/bin/sh

while [ -z "$(curl --noproxy provider -v http://provider:8080/control/ping 2>&1 | grep '200 OK')" ]; do
    echo "Waiting for provider to become available ..."
    sleep 5
done

echo "Starting application ..."
java -cp /app.jar io.joynr.tests.gracefulshutdown.Bootstrap &
CONSUMER_PID=$!
echo "Waiting until scenario finishes"
sleep 60

echo "Stopping consumer ..."
kill -9 ${CONSUMER_PID}
echo "FINISHED"
