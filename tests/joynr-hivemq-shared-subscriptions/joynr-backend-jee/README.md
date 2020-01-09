# joynr backend Docker Image

This directory contains the necessary scripts to build a Docker image
containing a Payara runtime with the following two joynr
WAR components deployed:

* Discovery Service
* Access Control Manager

The services need a mqtt broker listening on port 1883 in a docker
container named mqttbroker (see ../docker-compose.yml).


## Building

In order to be able to successfully build the Docker image, you first have
to have built the joynr project, so that the required WAR files are
available in the `target` sub-directories of the various projects.

Use the `./build_docker_image.sh` script to build the Docker Image.
