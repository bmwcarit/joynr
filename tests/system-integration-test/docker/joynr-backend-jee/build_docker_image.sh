#!/bin/bash

if [ -d target ]; then
	rm -Rf target
fi
mkdir target

function copy_war {
	if [ ! -f $1 ]; then
		echo "Missing $1 build artifact. Can't proceed."
		exit -1
	fi
	cp $1 $2
}

DISCOVERY_WAR_FILE=../../../../java/backend-services/discovery-directory-jee/target/discovery-directory-jee*.war
ACCESS_CTRL_WAR_FILE=../../../../java/backend-services/domain-access-controller-jee/target/domain-access-controller-jee*.war

copy_war $DISCOVERY_WAR_FILE target/discovery-directory-jee.war
copy_war $ACCESS_CTRL_WAR_FILE target/domain-access-controller-jee.war

if [ -z "$(docker version 2>/dev/null)" ]; then
	echo "The docker command seems to be unavailable."
	exit -1
fi

docker build -t joynr-backend-jee:latest .
docker images --filter "dangling=true" -q | xargs docker rmi -f 2>/dev/null
rm -Rf target
