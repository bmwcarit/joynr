#!/bin/bash

echo "### start build_docker_image.sh for sit-controller ###"

set -e

if [ ! -d ../../sit-controller/target ] || [ ! -f ../../sit-controller/target/sit-controller.war ]; then
	echo "ERROR: You have to have built the project with 'mvn package' first."
	exit -1
fi

cp ../../sit-controller/target/sit-controller.war .

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "ERROR: The docker command seems to be unavailable."
	exit -1
fi

docker build -t sit-controller:latest .
rm sit-controller.war

echo "### end build_docker_image.sh for sit-controller ###"
