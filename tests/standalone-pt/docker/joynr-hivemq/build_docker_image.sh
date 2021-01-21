#!/bin/bash
set -Eeuxo pipefail

echo "### start build_docker_image.sh for joynr-hivemq ###"

EXTRA_OPTIONS="--no-cache"

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "ERROR: The docker command seems to be unavailable."
	exit 1
fi

docker build $EXTRA_OPTIONS -t joynr-hivemq:latest .

echo "### end build_docker_image.sh for joynr-hivemq ###"
