#!/bin/bash

# build onboard image first because it builds joynr
# which is required by the other build scripts
#for BUILD_DIR in onboard sit-jee-app sit-jee-stateless-consumer sit-controller

# build joynr backend images
cd ../../../docker/ && ./build_backend.sh
cd -

# build the other images
for BUILD_DIR in sit-jee-app sit-jee-stateless-consumer sit-controller
do
	pushd $BUILD_DIR
	./build_docker_image.sh
	popd
done
