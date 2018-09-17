#!/bin/bash

echo "### start build_docker_image.sh for sit-jee-stateless-consumer ###"

set -e

if [ ! -f ../../sit-jee-stateless-consumer/target/sit-jee-stateless-consumer.war ]; then
	echo "ERROR: You have to have built the project with 'mvn package' first."
	exit -1
fi

cp ../../sit-jee-stateless-consumer/target/sit-jee-stateless-consumer.war .

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "ERROR: The docker command seems to be unavailable."
	exit -1
fi

docker build -t sit-jee-stateless-consumer:latest .
docker images --filter "dangling=true" -q | xargs docker rmi -f 2>/dev/null
rm sit-jee-stateless-consumer.war

echo "### end build_docker_image.sh for sit-jee-stateless-consumer ###"
