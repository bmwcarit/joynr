# Building the System Integration Test JEE stateless consumer application Image

This directory contains the docker image definition and build scripts for creating a Docker image
with the JEE stateless consumer application for the system integration stateless async test.

Simply execute the `./build_docker_image.sh` script from within this directory.

## Pre-requisites

In order for the build script to work you have to:

* Be executing from within this directory
* Have built the `tests/system-integration-test/sit-jee-stateless-consumer` project
* Have a working installation of docker

## What is built

The Docker image which is built is based on the publicly available Payara full-stack image and
installs the WAR file built by the `../../sit-jee-stateless-consumer/` project.

It also sets the log levels of joynr messaging, dispatch and the jeeintegration to `FINEST` so that
you can see what is going on during the test. This means, that this image is NOT suitable to run
performance tests without first changing the log levels back to a higher value (e.g. `ERROR`).

Also, the `./start-me-up.sh` script, which is the entry point for the Docker image, will start up a
forked process to allow the consumer endpoint to be triggered, so that the JEE application will
attempt to call the Node.js provider of the test service.

