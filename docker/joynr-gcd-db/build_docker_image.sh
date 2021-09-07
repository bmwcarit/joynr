#!/bin/bash

echo "### start build_docker_image.sh for joynr-gcd-db ###"

set -e -x

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "ERROR: The docker command seems to be unavailable."
	exit 1
fi

DB_INIT_FILE=../joynr-base/scripts/init.sql

if [ ! -f $DB_INIT_FILE ]; then
    echo "ERROR: Missing $DB_INIT_FILE. Can't proceed."
    exit 1
fi

cp $DB_INIT_FILE init.sql

docker build $@ -t joynr-gcd-db:latest .
docker image prune -f
rm init.sql

echo "### end build_docker_image.sh for joynr-gcd-db ###"
