# Docker based multi version inter-language-test

## Legend

The following abbreviations are used:

`<JOYNR>` refers to the root of the joynr repository clone
`<ILT_DIR>` refers to `<JOYNR>/tests/inter-language-test`

## Purpose
Supports testing of consumers, providers and cluster controllers of different
joynr versions against each other.

## Layout
### Docker images
The test environment uses 4 docker containers:
- `mqttbroker` container providing transport between different cluster controllers; using HiveMQ MQTT broker
- `joynrbackend` container providing Joynr GCD backend via Postgresql db and Java implementation
- `ilt-onboard-apps` container providing full joynr runtime environment, scripts and C++, Java, JS implementations (where applicable) of
 - cluster controller
 - provider
 - consumer

The `joynrbackend` and `ilt-onboard-apps` docker images are tagged so that the test can select from different joynr versions.

### Build scripts

Build scripts are provided to create the needed inter-language-test specific docker image,
see `<ILT_DIR>/docker/onboard/build_docker_image.sh`.

The general joynr docker images are a prerequisite for this, they have to be built first
by executing

```bash
$ cd <JOYNR>/docker
$ ./joynr-docker build --proxy_host <proxy-ip-address> --proxy_port <proxy-port>
```

in the joynr repository. The proxy parameters are optional and can be omitted.
It has been successfully tested with a local `cntlm` proxy where the company
proxy contacted by cntlm requires a user and password credential which can be
stored in encrypted form within the cntlm configuration, so there is no need to
expose this information to docker.

The joynr gcd backend need to be build first.
```bash
cd ../../../docker/ && ./build_backend.sh
cd -
```

The inter-language-test specific docker image can then be built as follows:

```bash
$ cd <JOYNR>
$ mkdir -p build
$ chmod 777 build
$ cd <ILT_DIR>/docker/onboard
$ ./build_docker_image.sh --docker-run-flags "-e DEV_UID=$(id -u)" <other params>
```

### Run scripts

A parameterized script `<ILT_DIR>/docker/run-docker-inter-language-tests.sh` is
provided to run the docker concerto using docker compose and execute the required
steps within each container instance.

```
Synopsis: run-docker-inter-language-tests.sh <options>
    -a <old_version> (mandatory: tag of docker image)
    -b <new_version> (mandatory: tag of docker image)
    [-c <backend_version>] (default: <new_version>)
    [-l <consumer languages] (default: "cpp java js")
    [-L <provider languages] (default: "cpp java js")
    [-n <consumer node] (default: ilt-onboard-apps-2)
    [-N <provider node] (default: ilt-onboard-apps-1)
    [-h|-H <consumer|provider> cc host (default: ilt-onboard-apps-1|ilt-onboard-apps-1]
    [-p|-P <consumer|provider> cc port (default: 4242|4242]
    [-w|-W <consumer|provider> cc ws protocol (default: ws|ws]
```

The actual default values are printed when the command is invoked without parameters.

### Docker repository

It is recommended to create `joynrbackend` and `ilt-onboard-apps` docker images upon
release of a new joynr version. These images should then be tagged with the joynr
version and pushed to some central docker repository for later reference.

### Standard test scenario

#### Prerequisites

The test expects the aforementioned tagged joynr docker images to be present in local
docker repository.

The images can either be created from scratch using the provided scripts which must be
run inside a joynr repository clone checked out to the intended version, or being pulled
from a central docker repository into the local repository provided someone created and
uploaded them to that central repository at an earlier time.

Example:

```
$ docker images
...
ilt-onboard-apps   1.9.0  ...
ilt-onboard-apps   1.9.1  ...
hivemq/hivemq-ce   latest ...
...
```

#### Testing an older joynr version against a newer joynr version

- The `mqttbroker` HiveMQ docker image is started
- The `joynrbackend` container instance is started using the image related to the new joynr version (since a new backend is expected to be backward compatible)
- A first `ilt-onboard-apps` container instance is invoked related to the new joynr version
- A second `ilt-onboard-apps` container instance is invoked related to the old joynr version
- A Cluster controller is started within each `ilt-onboard-apps` container
- Providers are started within the first `ilt-onboard-apps` container related to the new joynr version. To avoid conflicts and allow some parallel processing, a different provider domain is used for each programming language
- Consumers are started within the second `ilt-onboard-apps` container related to the old joynr version
- The consumers can be configured
 - [1] to contact their own cluster controller via Websocket or
 - [2] to contact the cluster controller of the first `ilt-onboard-apps` container instance related to the new joynr version
- In case [1] the cluster controller of both `ilt-onboard-apps` container instances communicate over MQTT with each other and with the JDS backend
- In case [2] only the cluster controller of the first `ilt-onboard-apps` container communicates over MQTT with the JDS backend, when required
- Consumer tests will be executed discovering provider and building required proxies and running test method calls and subscriptions
- Test logs and exit codes are written to files on mounted volumes for later examination
- After the test has been completed (i.e. the configured consumsers have been run against configured providers) the docker container instances are stopped and removed again
- The test exit codes are evaluated

Example:

```
mkdir -p $HOME/docker/build/ilt-onboard-apps-1
mkdir -p $HOME/docker/build/ilt-onboard-apps-2
chmod 777 $HOME/docker/build/ilt-onboard-apps-1
chmod 777 $HOME/docker/build/ilt-onboard-apps-2
cd <ILT_DIR>/docker
./run-docker-inter-language-tests.sh -a 1.9.0 -b 1.9.1
```

#### Hints

While a container instance is running, it is possible to login interactively into it using.

Example:
```
$ docker exec -e COLUMNS="`tput cols`" -e LINES="`tput lines`" -it ilt-onboard-apps-1 /bin/bash
```

The logfiles created in `$HOME/docker/build/ilt-onboard-apps-X` have the following name format:

- `cc.log` - Logfile of the cluster controller
- `provider-<language>-<domain>.log` - Logfile of the provider implemented in `language` serving `domain`
- `consumer-<language>-<domain>.log` - Logfile of the consumer implemented in `language` using domain `domain`

Example for expected results:

```
$ ./run-docker-inter-language-tests.sh -a 1.9.0 -b 1.9.1
JOYNR_VERSION_OLD=1.9.0
JOYNR_VERSION_NEW=1.9.1
Creating joynrbackend       ... done
Creating mqttbroker         ... done
Creating ilt-onboard-apps-2 ... done
Creating ilt-onboard-apps-1 ... done
Starting cluster controller on ilt-onboard-apps-1
Starting cpp provider on ilt-onboard-apps-1
Testing cpp consumer on ilt-onboard-apps-2 against cpp provider on ilt-onboard-apps-1 using domain ilt-cpp-domain via consumer CC on ws://ilt-onboard-apps-1:4242
Testing java consumer on ilt-onboard-apps-2 against cpp provider on ilt-onboard-apps-1 using domain ilt-cpp-domain via consumer CC on ws://ilt-onboard-apps-1:4242
Testing js consumer on ilt-onboard-apps-2 against cpp provider on ilt-onboard-apps-1 using domain ilt-cpp-domain via consumer CC on ws://ilt-onboard-apps-1:4242
Starting cluster controller on ilt-onboard-apps-1
Starting java provider on ilt-onboard-apps-1
Testing cpp consumer on ilt-onboard-apps-2 against java provider on ilt-onboard-apps-1 using domain ilt-java-domain via consumer CC on ws://ilt-onboard-apps-1:4242
Testing java consumer on ilt-onboard-apps-2 against java provider on ilt-onboard-apps-1 using domain ilt-java-domain via consumer CC on ws://ilt-onboard-apps-1:4242
Testing js consumer on ilt-onboard-apps-2 against java provider on ilt-onboard-apps-1 using domain ilt-java-domain via consumer CC on ws://ilt-onboard-apps-1:4242
Stopping ilt-onboard-apps-2 ... done
Stopping ilt-onboard-apps-1 ... done
Stopping joynrbackend       ... done
Stopping mqttbroker         ... done
Going to remove ilt-onboard-apps-2, ilt-onboard-apps-1, joynrbackend, mqttbroker
Removing ilt-onboard-apps-2 ... done
Removing ilt-onboard-apps-1 ... done
Removing joynrbackend       ... done
Removing mqttbroker         ... done
RESULT: Test successfully completed.

$ cd $HOME/docker/build/ilt-onboard-apps-1
$ ls -1
cc.log
provider-cpp-ilt-cpp-domain.log
provider-java-ws-ilt-java-domain.log
provider-js-ilt-js-domain.log
$ cd $HOME/docker/build/ilt-onboard-apps-2
$ ls -1
consumer-cpp-ilt-cpp-domain.exitcode
consumer-cpp-ilt-cpp-domain.junit.xml
consumer-cpp-ilt-cpp-domain.log
consumer-cpp-ilt-java-domain.exitcode
consumer-cpp-ilt-java-domain.junit.xml
consumer-cpp-ilt-java-domain.log
consumer-cpp-ilt-js-domain.exitcode
consumer-cpp-ilt-js-domain.junit.xml
consumer-cpp-ilt-js-domain.log
consumer-java-ws-ilt-cpp-domain.exitcode
consumer-java-ws-ilt-cpp-domain.log
consumer-java-ws-ilt-java-domain.exitcode
consumer-java-ws-ilt-java-domain.log
consumer-java-ws-ilt-js-domain.exitcode
consumer-java-ws-ilt-js-domain.log
consumer-js-ilt-cpp-domain.exitcode
consumer-js-ilt-cpp-domain.log
consumer-js-ilt-java-domain.exitcode
consumer-js-ilt-java-domain.log
consumer-js-ilt-js-domain.exitcode
consumer-js-ilt-js-domain.log
$ cat *.exitcode
0
0
0
0
0
0
0
0
0
```
