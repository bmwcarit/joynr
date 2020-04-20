#!/bin/bash
set -Eeuxo pipefail

echo "### start build_docker_image.sh for joynr-backend-jee-db ###"

EXTRA_OPTIONS="--no-cache"

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

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "ERROR: The docker command seems to be unavailable."
	exit 1
fi

docker build $EXTRA_OPTIONS -t joynr-backend-jee-db:latest .
#docker image prune
rm -rf target

echo "### end build_docker_image.sh for joynr-backend-jee-db ###"
