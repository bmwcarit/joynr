#!/bin/sh

while [ -z "$(curl --noproxy secondlevel -v http://secondlevel:8080/control/ping 2>&1 | grep '200 OK')" ]; do
    echo "Waiting for second level provider to become available ..."
    sleep 5
done

echo "Starting application ..."

java -jar /app.jar --postbootcommandfile /post-boot.txt
