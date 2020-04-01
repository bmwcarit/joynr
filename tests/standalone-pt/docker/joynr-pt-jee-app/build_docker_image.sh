#!/bin/bash

echo "### start build_docker_image.sh for joynr-pt-jee-app ###"

set -e -x

# build war file
current_work_dir=$(cd $(dirname $0) && pwd)
joynr_root=$(git rev-parse --show-toplevel)
cd $joynr_root/tests/standalone-pt
mvn clean install
cd $current_work_dir

if [ ! -d ../../pt-jee-app/target ] || [ ! -f ../../pt-jee-app/target/pt-jee-app.war ]; then
	echo "ERROR: You have to have built the project with 'mvn package' first."
	exit 1
fi

cp ../../pt-jee-app/target/pt-jee-app.war .

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "ERROR: The docker command seems to be unavailable."
	exit 1
fi

docker build -t joynr-pt-jee-app:latest .
#docker image prune
rm pt-jee-app.war

echo "### end build_docker_image.sh for joynr-pt-jee-app ###"
