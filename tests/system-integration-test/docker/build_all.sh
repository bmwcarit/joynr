#!/bin/bash

# change to the directory where this script resides
# in order to make sure relative path specs work
BASEDIR=$(dirname "$0")
cd $BASEDIR

function usage() {
	echo "Usage: build_all.sh [-h] [-o]"
	echo "       -o omit building the onboard image"
	echo "       -h display help"
}

BUILD_ONBOARD_IMAGE=1
while getopts "oh" OPTIONS;
do
	case $OPTIONS in
		o)
			BUILD_ONBOARD_IMAGE=0
			echo "Onboard image will be skipped."
			;;
		h)
			usage
			exit 1
			;;
		\?)
			usage
			exit 1
			;;
	esac
done

BASE_IMAGES="sit-jee-app sit-jee-stateless-consumer sit-controller"
if [ "$BUILD_ONBOARD_IMAGE" -eq 0 ]
then
	IMAGES=$BASE_IMAGES
else
	# build onboard image first because it builds joynr
	# which is required by the other build scripts
	IMAGES="onboard $BASE_IMAGES"
fi

# build the other images
for BUILD_DIR in $IMAGES
do
	echo "About to build $BUILD_DIR"
	pushd $BUILD_DIR
	./build_docker_image.sh
	popd
done

# build joynr backend images
cd ../../../docker/ && ./build_backend.sh
cd -
