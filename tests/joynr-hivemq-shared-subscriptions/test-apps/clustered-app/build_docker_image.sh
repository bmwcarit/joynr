#!/bin/bash

docker build $@ -t shared-subs-test-clustered-app:latest .

for image in `docker images | grep '<none' | awk '{print $3}'`
do
    docker rmi -f $image
done
