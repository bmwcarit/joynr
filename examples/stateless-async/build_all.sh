#!/bin/bash
set -Eeuxo pipefail

# build joynr backend images
cd ../../docker/ && ./build_backend.sh
cd -

# build the other images
for BUILD_DIR in stateless-async-car-sim stateless-async-consumer stateless-async-jee-car-sim stateless-async-jee-consumer
do
    pushd $BUILD_DIR
    ./build_docker_image.sh
    popd
done
