# joynr backend Docker Image

This directory contains the necessary scripts to build a Docker Image
containing a Jetty runtime with the following three joynr
WAR components deployed:

* Discovery Service
* Access Control Manager
* Bounce Proxy

It exposes port 8080 via which the various services can then be reached.


## Building

In order to be able to successfully build the Docker Image, you first have
to have built the joynr project, so that the required WAR files are
available in the `target` sub-directories of the various projects.

Use the `./build_docker_image.sh` script to build the Docker Image.
