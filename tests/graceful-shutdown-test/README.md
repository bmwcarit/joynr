## Building

### Prerequisite

The joynr backend docker images `joynr-gcd-db` and `joynr-gcd` are required to run the tests.

You can build them by running `cd <joynr_repo>/docker/ && ./build_backend.sh`.

Common docker image `java-11-with-curl` is required to run the tests.

You can build it by running `cd <joynr_repo>/docker/ && ./build_docker_image.sh`.

### Test docker images

The easiest way to build all necessary images is to execute the

`./build_all.sh`

script in this directory.

## Running

To run the tests, change to this directory
(`${project_root}/docker/graceful-shutdown-test`) and
execute `run` script.
