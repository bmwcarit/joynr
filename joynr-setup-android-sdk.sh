#!/bin/bash

error () {
[ "$1" != "0" ] &&  exit 1 || :
}


log () {
echo ""
echo "========================================"
echo "= $1"
echo "========================================"
echo ""
}


# Check that ANDROID_HOME is set and exists
if [ -z "$ANDROID_HOME" ] || [ ! -d "$ANDROID_HOME" ]
then
	echo '$ANDROID_HOME should be set to the Android SDK directory'
	exit 1
fi 

# fail on first error
set -e

(
	log "CLONE SDK DEPLOYER"
	git clone --depth 1 https://github.com/simpligility/maven-android-sdk-deployer
)
(
	log "INSTALL LOLLIPOP SDK TO MAVEN REPOSITORY"
	cd maven-android-sdk-deployer
	mvn install -P 5.0
	cd -
)
(
	log "CLEANUP"
	rm -rf maven-android-sdk-deployer
)

log "DONE"

