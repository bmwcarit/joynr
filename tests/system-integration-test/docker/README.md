# System Integration Tests - Docker

In order to run the system integration tests, you need to have built
or have available the various Docker Images which are used by
the `docker-compose.yml` script contained in this directory (see section
[Building](#building) below.


## Test scenario

![SIT Test Scenario Overview](docs/OverviewSIT.png)

## Overview

### The backend environment containers

The docker concerto simulates a multi-backend environment with
2 backends where one is using the global backend identifier (GBID)
`joynrdefaultgbid` and the other one is using GBID `othergbid`.

This includes 2 MQTT Brokers (one for each backend) with
container names `mqttbroker-1` and `mqttbroker-2`.

The clustercontrollers (standalone for vehicle simulation in the
`onboard` container or embedded in case of JEE containers) which
communicate with the backends over the MQTT brokers have to be
configured for all backends required by the applications.

This means a clustercontroller can be configured for either only a
single or both backends simultaneously by configuring the appropriate
brokerUris and GBID settings / properties.

Each backend has its own GCD (Global Capabilities Directory)
also known as JDS (Joynr discovery service), where each
instance knows about its own GBID and connects to the broker
associated with this GBID. It also knows about the other valid
GBIDs used in the environment in order to be able to detect
invalid ones used by an application when trying to register a
provider via add request or build a proxy via lookup request.

The GCD instances are implemented within the containers
`joynr-gcd-1` (for `joynrdefaultgbid`) and `joynr-gcd-2` (for `othergbid`)
respectively. The `joynr-gcd-1` connects to `mqttbroker-1`
and the `joynr-gcd-2` connects to `mqttbroker-2`.

Both GCD instances share a common database wherein the registered
providers are stored. This is implemented using a PostgreSQL
database in container `joynr-gcd-db`.

### The onboard container

The `sit-onboard-apps` container includes a standalone C++ cluster-controller
which is configured and connected to both mqttbrokers and GCDs.

It also includes C++, pure Java, and node provider and consumer
applications that can be run for testing. Those applications then
connect to the standalone cluster controller via websocket while
the clustercontroller connects to the MQTT brokers to communicate
with the backend (GCDs and/or JEE applications).

The sources of the test apps can be found in `sit-cpp-app`,
`sit-java-app` and `sit-node-app` which are subdirectories of
`tests/system-integration-test` in the root of the joynr repository.

The included `run-onboard-sit.sh` script is automatically invoked
on startup of the container instance and and runs the following activity:

Registration of providers for C++, Java, Node and Node with TLS
on single or multiple backends, i.e. for `joynrdefaultgbid`,
`othergbid` or both at the same time. The providers register on
a domain that is specific for the test case, taking into account
the used backends, programming language, TLS etc.

This way the correct provider can be easily looked up when
building a proxy inside a consumer application running within the
`onboard` or within an external container (JEE).

Further registrations are attempted using an invalid GBID; these are
of course expected to fail.

After the providers are registered several consumer tests are invoked for
consumer applications of all languages building proxies and performing method calls
against providers of all languages and with any combination of single or
multiple backends using the specific domain names for this purpose.

Furthermore some consumer tests are run using invalid domains which are of course
expected to fail since no providers have been registered for those domains.

### The JEE app containers

There are 3 JEE app containers named `sit-jee-app-1`, `sit-jee-app-2`
and `sit-jee-app-3` configured for `joynrdefaultgbid`, `othergbid` or
both.

Each app container contains a test provider application which registers
at the configured backends and with the configured domain.
This provider application is e.g. contacted by consumer test
applications which are invoked within the `onboard` container.

Furthermore the app container contains a sit-controller provider which
is used to remote control the instance by a consumer application residing
in the `sit-controller` container. This way consumer proxies can be
built within the container which execute method call tests.

### The sit-controller container

The `sit-controller` container has a consumer application that is
able to remote control consumer tests executed in `sit-jee-app-X`
containers as well as in `sit-jee-stateless-consumer-X` containers
by contacting their `sit-controller provider`. It also collects
the results of such consumer tests.

Furthermore it contains a REST application which listens on REST
requests sent in from outside by shell script via curl calls.

The REST requests then trigger calls via the consumer application
mentioned above.

### The sit-jee-stateless-consumer containers

The `sit-jee-stateless-consumer-1` and `sit-jee-stateless-consumer-2`
are used to test the stateless-async scenario.

Stateless-async means that the reply to a request created to implement
a proxy method call can be handled by multiple application instances
(usually located in different containers) and does not need to be
returned to and processed by the exact same instance where the
original request originated. However, in any case the reply will be
handled by exactly one instance.

Each container runs a sit-controller provider which can by used to
run tests via invocation of its `triggerTests` method.

The invocation of the 'triggerTests' can either be caused by a remote
proxy or via REST call handled by the SitStatelessAsyncConsumerRestEndpoint
inside the container.

Here the consumer in the `sit-controller` container calls
the `triggerTests` of the `sit-jee-stateless-consumer-1` container.

If triggered, the tests build a proxy for the interface
SystemIntegrationTest and invoke `add` method calls on it.
The provider associated with the proxy resides within the onboard
container and should send a reply for reach request received.

The replies are then distributed by the MQTT broker to the consumer
apps running in `sit-jee-stateless-consumer-1` and
`sit-jee-stateless-consumer-2` on a shared subscrption base,
meaning that each reply is sent to exactly one of the apps.
If a sufficient number of calls is made, replies should be received
on both instances.

## Building

The easiest way to build all necessary images is to execute the

`./build_all.sh`

script in this directory.

Alternatively, look inside the script to see what it does, and
execute any or all of the steps manually if you want more control
over what's built.

There are several sub-directories, which contain build scripts
for creating the images. See their individual README.md's for
instructions on how to build them.

## Running the scenario

Once you have built all the necessary Docker Images, you
can start up the scenario by executing the following script:

`run_sit.sh`

Alternatively, you can also start up the scenario manually by
executing the following commands:

`docker-compose up -d`

Check for progress of the various containers by running:

`docker-compose logs`

In order to shutdown and remove the containers, issue the
following commands:

`docker-compose stop`

and

`docker-compose rm -f`
