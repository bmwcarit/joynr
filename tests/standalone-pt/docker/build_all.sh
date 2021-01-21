#!/bin/bash
set -Eeuxo pipefail

# build onboard image first because it builds joynr
# which is required by the other build scripts

for BUILD_DIR in joynr-hivemq joynr-consumer joynr-infra joynr-infra-db joynr-jee-provider 
do
    pushd $BUILD_DIR
    ./build_docker_image.sh
    popd
done
