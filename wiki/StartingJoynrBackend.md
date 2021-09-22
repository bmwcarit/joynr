# Joynr backend

This tutorial describes how to build and use the dockerized Joynr backend solution.

The applictions or tests which need to interact with the joynr backend (register, lookup,
unregister, etc) have to do the following steps in order to build and run the GCD.

### Build the Joynr Global Capabilities Directory (GCD)
The `GCD` needs to be built through joynr repository. Please open a terminal and do the following:

```bash

$ git clone git@cc-github.bmwgroup.net:joynr/joynr.git

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
cd ~/joynr/docker
./build_backend.sh
```
This will build the mentioned docker images on your local machine. To check whether the images have
been successfully built, please run the command:

```bash
$ docker images
joynr-gcd                                   latest         7aae6a4d4615   47 hours ago    260MB
joynr-gcd-db                                latest         471aa4729317   47 hours ago    192MB

```

### Run the MQTT broker and database services in `docker-compose` environment
As mentioned above, to run apps/tests which interact with the joynr backend we need to run an MQTT
broker along with a database. We have prepared a docker-compose file `joynr/docker/joynr-backend.yml`
which contains the required services. To start these services, please do the following:

```bash
cd ~/joynr/docker
docker-compose -f joynr-backend.yml up -d
```

This will create a network `docker_default` and start the services. The output looks as follows:

```bash
~/joynr/docker $ docker-compose -f joynr-backend.yml up -d
Creating network "docker_default" with the default driver
Creating joynr-gcd-db ... done
Creating mqttbroker-1 ... done
Creating joynr-gcd-1  ... done
~/joynr/docker $
```

To see the services which are running, please run: `docker ps -a`.

> **Note:** Please note that no other service is allowed to use the required ports, especially the
            MQTT port 1883, and the DB port 5432 on the machine.

At this point, the required services for any application or test which needs to interact with joynr
backend are available.

The MQTT broker is running and listening to the default port `1883`, the database server is running
and listening to the default port `5432`. The joynr gcd is running and connected to the database
server and the MQTT broker. External test apps can now reach the GCD through the MQTT broker from
outside the docker concerto.

### Run the apps/tests
After preparing and running the required services, please run your applications and tests. Once you
are finished, you can stop the services by stopping the `docker-compose` as follows:

```bash
docker-compose -f joynr-backend.yml stop
docker-compose -f joynr-backend.yml rm -f
```
