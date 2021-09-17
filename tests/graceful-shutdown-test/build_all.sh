#!/bin/bash
set -Eeuxo pipefail

# build joynr backend images
cd ../../docker/ && ./build_backend.sh
cd -

# build the other images
for BUILD_DIR in graceful-shutdown-test-consumer graceful-shutdown-test-provider graceful-shutdown-test-second-level-provider
do
    pushd $BUILD_DIR
    ./build_docker_image.sh
    popd
done
