#!/bin/bash

echo "### start build_docker_image.sh for joynr-faked-backend-jee-1 ###"

EXTRA_OPTIONS="--no-cache"

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

# here is the fake one
DISCOVERY_WAR_FILE=../../backend-services-standalone-pt/discovery-directory-pt-jee/target/discovery-directory-pt-jee-shared-db*.war

copy_war $DISCOVERY_WAR_FILE target/discovery-directory-pt-jee-shared-db.war

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "ERROR: The docker command seems to be unavailable."
	exit 1
fi

docker build $EXTRA_OPTIONS -t joynr-faked-backend-jee-1:latest .
#docker image prune
rm -rf target

echo "### end build_docker_image.sh for joynr-faked-backend-jee-1 ###"
