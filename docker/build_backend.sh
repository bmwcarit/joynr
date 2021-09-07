#!/bin/bash

for BUILD_DIR in joynr-gcd-db joynr-gcd
do
	pushd $BUILD_DIR
	./build_docker_image.sh $@
	popd
done
