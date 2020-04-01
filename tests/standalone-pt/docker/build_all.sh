#!/bin/bash

# build onboard image first because it builds joynr
# which is required by the other build scripts
#for BUILD_DIR in onboard joynr-backend-jee-1 joynr-backend-jee-2 joynr-pt-jee-app
#joynr-backend-jee-db

for BUILD_DIR in joynr-backend-jee-1 joynr-faked-backend-jee-1 joynr-pt-jee-app
do
    pushd $BUILD_DIR
    ./build_docker_image.sh
    popd
done
