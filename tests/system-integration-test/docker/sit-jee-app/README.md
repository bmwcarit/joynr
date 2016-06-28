# Building the System Integration Test JEE Application Image

This directory contains the docker image definition and build
scripts for creating a Docker Image with the JEE Application
for the system integration test.

Simply execute the `./build_docker_image.sh` script from within
this directory.

## Pre-requisites

In order for the build script to work you have to:

* Be executing from within this directory
* Have built the tests/system-integration-test/sit-jee-app project
* Have a working installation of docker

## What is built

The Docker Image which is built is based on the publicly
available Payara full-stack image and installs the WAR file
built by the `../../sit-jee-app/` project.

It also sets the log levels of joynr messaging, dispatch and
the jeeintegration to `FINEST` so that you can see what is
going on during the test. This means, that this image is
NOT suitable to run performance tests without first changing
the log levels back to a higher value (e.g. `ERROR`).

Also, the `./start-me-up.sh` script, which is the entry point
for the Docker Image, will start up a forked process which
waits until the application is up and running, and will then
trigger the consumer endpoint, so that the JEE application
will attempt to call the Node.js, C++ and its own JEE
provider of the test service.
