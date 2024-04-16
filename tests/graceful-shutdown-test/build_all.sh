#!/bin/bash
set -Eeuxo pipefail

# build joynr backend images and java-11-with-curl
cwd="$(pwd)"
cd ../../docker/ && ./build_backend.sh
cd java-11-with-curl && ./build_docker_image.sh
cd "$cwd"

# build the other images
for BUILD_DIR in graceful-shutdown-test-consumer graceful-shutdown-test-provider graceful-shutdown-test-second-level-provider
do
    pushd $BUILD_DIR
    ./build_docker_image.sh
    popd
done
