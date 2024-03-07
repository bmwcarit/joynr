#!/bin/bash

echo "### start build_docker_image.sh for java-11-with-curl ###"

set -e -x

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "ERROR: The docker command seems to be unavailable."
	exit 1
fi

docker build $@ -t java-11-with-curl:latest .
docker image prune -f
rm -rf target

echo "### end build_docker_image.sh for java-11-with-curl ###"
