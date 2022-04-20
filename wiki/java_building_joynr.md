# Building joynr Java and common components

Joynr can be built using a [docker](http://www.docker.com) container. The necessary docker images are
defined in the docker files in the subfolder **docker** of the **joynr repository**. These docker
images provide a virtualized build environment which includes all necessary dependencies (e.g. JDK,
Maven, CMake and GCC for C++, etc.) for building joynr.

**Joynr Java** also includes **common joynr components** like the **joynr Code Generators** which are
also necessary for [building joynr C++](cpp_building_joynr.md). By building joynr Java, the following
components (listed by subfolder) are built and installed into the local Maven repository:

* `tools`
  * build resources needed during build
  * dependency libraries needed during build not available in Maven Central
  * code generators and a corresponding Maven Plugin
* `basemodel`
  * Franca files describing communication interfaces to infrastructure services
* `java`
  * joynr Java API
  * generated Java source code (needed to access infrastructure services and tests)
  * infrastructure services (discovery directory)
* `cpp`
  * generated C++ source code (needed to access infrastructure services and tests)
* `examples`
  * Radio app example (including code generation for Java and C++)


## Prerequisites

### Cloning the joynr source repository
The joynr source repository (https://github.com/bmwcarit/joynr) which contains the Java, JavaScript
and C++ sources has to be cloned, and the correct branch checked out.


### Building docker images for joynr Java
For the installation of docker, please refer to the docker documentation at the
[docker website](http://docs.docker.com).

In order to use docker for building joynr, the joynr docker images have to be built first by executing

```bash
$ cd <JOYNR>/docker
<JOYNR>/docker$ ./joynr-docker build
```
in the joynr repository. These images can then be executed as docker containers to build joynr.


## Building joynr Java
After the joynr docker images have been created, joynr Java can be built by executing the following
command:

```bash
$ docker run --rm --sig-proxy -e DEV_UID="$(id -u)"  \
    -v <FULL_PATH_TO_JOYNR_SOURCES>:/data/src \
    -v <FULL_PATH_TO_MAVEN_DIRECTORY>:/home/joynr/.m2 \
    joynr-base \
    /data/src/docker/joynr-base/scripts/test/java-clean-build skipTests
```

This will start the docker container **joynr-base** and execute the script
**docker/joynr-base/scripts/test/java-clean-build**, which builds joynr Java and the common
components. The resulting artifacts are stored in the local Maven repository. 
The Maven directory **&lt;FULL_PATH_TO_HOST_MAVEN_DIRECTORY&gt;** which contains the
local Maven repository is mounted at **/home/joynr/.m2** in the docker container. In the same way,
the path to the joynr repository **&lt;FULL_PATH_TO_JOYNR_SOURCES&gt;** has to be provided to be
accessible from the docker container at **/data/src**.

```-e DEV_UID="$(id -u)"``` makes sure that artifacts created during the build are owned by the user
of the host system afterwards, allowing temporary build artifacts to be deleted by a continous
integration server, for instance.

Make sure the local Maven directory

* contains the settings in the files ```settings.xml``` and ```security-settings.xml```
* does not change the storage location for artifacts (```localRepository``` inside
  ```settings.xml```)
* contains only path settings (if any) which are reachable from inside the docker container
* contains no invalid encrypted password entries in ```settings.xml```

since otherwise the build might break.
