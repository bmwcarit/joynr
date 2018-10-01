#!/bin/bash

# build onboard image first because it builds joynr
# which is required by the other build scripts
for BUILD_DIR in onboard joynr-backend-jee sit-jee-app
do
	pushd $BUILD_DIR
	./build_docker_image.sh
	popd
done
