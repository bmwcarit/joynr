# Building joynr JavaScript

## Prerequisites
The joynr JavaScript repository has to be cloned.

## Building joynr JavaScript
Joynr JavaScript can be built by executing the following command:
```bash
sudo docker run --rm --sig-proxy -e DEV_UID="$(id -u)"
    -v <FULL_PATH_TO_JOYNR_JAVASCRIPT_REPOSITORY>:/data/src
    -v <FULL_PATH_TO_MAVEN_DIRECTORY>:/home/joynr/.m2
    joynr-javascript
    /data/src/docker/joynr-javascript/scripts/build/javascript-clean-build
```



This will start the docker container **joynr-javascript** and execute the script
**docker/joynr-javascript/scripts/build/javascript-clean-build.sh** in the joynr repository.
This script builds joynr JavaScript.

The joynr artifacts necessary for building joynr JavaScript are taken from the local Maven
repository **&lt;FULL_PATH_TO_HOST_MAVEN_DIRECTORY&gt;** mounted at **/home/joynr/.m2** in the
docker container. Like for building joynr Java, the path to the joynr repository
**&lt;FULL_PATH_TO_JOYNR_JAVASCRIPT_REPOSITORY&gt;** has to be provided to be accessible from the
docker container at **/data/src**.
