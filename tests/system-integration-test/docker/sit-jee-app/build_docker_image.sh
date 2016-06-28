#!/bin/bash

if [ ! -d ../../sit-jee-app/target ] || [ ! -f ../../sit-jee-app/target/sit-jee-app.war ]; then
	echo "You have to have built the project with 'mvn package' first."
	exit -1
fi

cp ../../sit-jee-app/target/sit-jee-app.war .

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "The docker command seems to be unavailable."
	exit -1
fi

docker build -t sit-jee-app:latest .
docker images | grep '<none' | awk '{print $3}' | xargs docker rmi -f 2>/dev/null
rm sit-jee-app.war
