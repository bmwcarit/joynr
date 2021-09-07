#!/bin/bash

docker build $@ -t backpressure-test-monitor-app:latest .

for image in `docker images | grep '<none' | awk '{print $3}'`
do
    docker rmi -f $image
done
