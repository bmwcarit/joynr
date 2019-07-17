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

mvn -B -Dorg.slf4j.simpleLogger.log.org.apache.maven.cli.transfer.Slf4jMavenTransferListener=warn \
-q dependency:get -Dartifact=android:android:9_r6

if [ $? -eq 0 ]
then
	echo 'Android SDK already installed in the local maven repository. Skip installation...'
	exit 0
fi

(
	log "Android SDK not yet installed in the local maven repository. Start installation..."

	log "CLONE SDK DEPLOYER"
	rm -rf maven-android-sdk-deployer
	git clone --depth 1 https://github.com/simpligility/maven-android-sdk-deployer
)
(
	log "INSTALL PIE SDK TO MAVEN REPOSITORY"
	cd maven-android-sdk-deployer
	mvn -B -Dorg.slf4j.simpleLogger.log.org.apache.maven.cli.transfer.Slf4jMavenTransferListener=warn \
	install -P 9.0

)
(
	log "CLEANUP"
	rm -rf maven-android-sdk-deployer
)

log "DONE"

