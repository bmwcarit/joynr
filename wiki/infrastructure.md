# joynr infrastructure

In order for joynr participants to be able to communicate with
each other, a few infrastructure components are necessary for
them to be able to discover each other and correctly route
messages between the participants.

This tutorial describes how to build and use the dockerized joynr backend solution.

## Components

The following component is necessary in order to set up a joynr environment:

* Global Discovery service / Global Capabilities Directory (GCD)
   * A joynr based application with which the participants register their providers and query to
     discover other participants
   * MQTT Broker for communication with the GCD and participants connected to different Cluster
     Controllers.

## Prerequisite

### Build the Joynr Global Capabilities Directory (GCD)
The `GCD` needs to be built through joynr repository. Please open a terminal and do the following:

```bash

git clone https://github.com/bmwcarit/joynr.git

cd joynr

mvn clean install

```
The `GCD` app: `capabilities-directory-jar-with-dependencies.jar` is expected to be built and located
in: `joynr/java/backend-services/capabilities-directory/target/deploy/`

### Build the backend images
The backend application `capabilities-directory-jar-with-dependencies.jar` needs to interact with a
broker and a database.

For this purpose, we need to build two docker images `joynr-gcd-db` and `joynr-gcd` and run
`docker-compose` which runs the required components of the apps/tests.

There is a ready script in `joynr/docker` to build the mentioned docker images. Please execute it
as follows:

```bash
cd <joynr_repository>
cd docker
./build_backend.sh
```
This will build the mentioned docker images on your local machine. To check whether the images have
been successfully built, please run the command:

```bash
$ docker images
joynr-gcd                                   latest         7aae6a4d4615   47 hours ago    260MB
joynr-gcd-db                                latest         471aa4729317   47 hours ago    192MB

```

## Starting the Infrastructure Components with default configuration (single backend)

To run apps/tests which interact with the joynr backend we need to run an MQTT
broker along with a database. We have prepared a docker-compose file `joynr/docker/joynr-backend.yml`
which contains the required services.  
**Note**: no other service is allowed to use the required ports `1883` (MQTT broker) and `5432`
(capabilities database) on the machine.

When everything is appropriately prepared, the Java GCD, together with the required MQTT broker and
database, can be started from the joynr/docker directory via the following command:

```bash
cd <joynr_repository>
cd docker
docker-compose -f joynr-backend.yml up -d
```
This will create a network `docker_default` and start the services.

At this point, the required services for any application or test which needs to interact with joynr
backend are available.

The MQTT broker is running and listening to the default port `1883`, the database server is running
and listening to the default port `5432`. The joynr gcd is running and connected to the database
server and the MQTT broker. External test apps can now reach the GCD through the MQTT broker from
outside the docker concerto.

When the testing is finished, the orchestra should be cleaned up as follows:

```bash
docker-compose -f joynr-backend.yml stop
docker-compose -f joynr-backend.yml rm -f
```

For configuration options and more information about the local GlobalCapabilitiesDirectory, see the
appropriate [README](../java/backend-services/capabilities-directory/README.md).

You have to change the GBID settings of the GCD to match your test environment if you are not using
the default GBID `joynrdefaultgbid`: just adapt the environment configuration for `joynr-gcd-1` in
`docker/joynr-backend.yml`.

## Adaptions for a multiple backend test environment

When using multiple backends, each backend requires its own MQTT broker and its own instance of the
GCD with appropriate GBID configuration. Please check out [joynr multiple backends Guide](multiple-backends.md)
for more information about joynr with multiple backends.

We have also prepared a docker-compose file setting up multiple backends in `docker/joynr-multiple-backend.yml`.
This docker-concerto can be run and torn down just as described above. It will start two backend
containers that share one database, as well as two MQTT brokers representing the different backends.
The MQTT brokers are listening to ports 1883 and 1884 outside the docker network, so it has to be
made sure that those ports are not occupied.

```bash
docker-compose -f joynr-multiple-backend.yml up -d
# Tear down after testing
docker-compose -f joynr-multiple-backend.yml stop
docker-compose -f joynr-multiple-backend.yml rm -f
```
