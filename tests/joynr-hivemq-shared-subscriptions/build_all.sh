#!/bin/bash
set -Eeuxo pipefail

# build joynr backend images
cd ../../docker/ && ./build_backend.sh
cd -

# build the other images
for BUILD_DIR in test-apps/backpressure-clustered-provider test-apps/backpressure-monitor-app test-apps/clustered-app test-apps/monitor-app test-driver-container
do
	pushd $BUILD_DIR
	./build_docker_image.sh $@
	popd
done
