#!/bin/bash

echo "### start build_docker_image.sh for joynr-backend-jee ###"

set -e -x

if [ -d target ]; then
	rm -Rf target
fi
mkdir target

function copy_war {
	if [ ! -f $1 ]; then
		echo "ERROR: Missing $1 build artifact. Can't proceed."
		exit 1
	fi
	cp $1 $2
}

DISCOVERY_WAR_FILE=../../../../java/backend-services/discovery-directory-jee/target/discovery-directory-jee-[0-9]*.war

copy_war $DISCOVERY_WAR_FILE target/discovery-directory-jee.war

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "ERROR: The docker command seems to be unavailable."
	exit 1
fi

docker build -t joynr-backend-jee:latest .
docker image prune
rm -rf target

echo "### end build_docker_image.sh for joynr-backend-jee ###"
