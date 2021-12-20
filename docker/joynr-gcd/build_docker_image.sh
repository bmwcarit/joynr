#!/bin/bash

echo "### start build_docker_image.sh for joynr-gcd ###"

set -e -x

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "ERROR: The docker command seems to be unavailable."
	exit 1
fi

if [ -d target ]; then
	rm -Rf target
fi
mkdir target

function copy_build_artifact {
    if [ ! -f $1 ]; then
        echo "ERROR: Missing $1 build artifact. Can't proceed."
        echo "ERROR: You have to have built the project with 'mvn package' first."
        exit 1
    fi
    cp $1 $2
}

PATH_PREFIX=../../java/backend-services/capabilities-directory/target/deploy
GCD_JAR=$PATH_PREFIX/capabilities-directory-jar-with-dependencies.jar
LOGGER_CONFIG=$PATH_PREFIX/log4j*.properties

copy_build_artifact $GCD_JAR target/gcd.jar
copy_build_artifact $LOGGER_CONFIG target/

docker build $@ -t joynr-gcd:latest .
docker image prune -f
rm -rf target

echo "### end build_docker_image.sh for joynr-gcd ###"
