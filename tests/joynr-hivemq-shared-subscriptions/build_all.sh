#!/bin/bash
set -Eeuxo pipefail

# build joynr backend images and java-11-with-curl
cwd="$(pwd)"
cd ../../docker/ && ./build_backend.sh
cd java-11-with-curl && ./build_docker_image.sh
cd "$cwd"

# build the other images
for BUILD_DIR in test-apps/backpressure-clustered-provider test-apps/backpressure-monitor-app test-apps/clustered-app test-apps/monitor-app test-driver-container
do
	pushd $BUILD_DIR
	./build_docker_image.sh $@
	popd
done
