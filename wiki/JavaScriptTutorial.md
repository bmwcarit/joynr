This tutorial will guide you through a simple node joynr radio application, explaining three essential
joynr concepts:

* A simple radio **communication interface**
* A **consumer** interested in knowing about radio information
* A **provider**, that provides the radio information

# Prerequisites
If you haven't built joynr yet, please do so first:

* [Building joynr Java](java_building_joynr.md).

This will install the necessary dependencies to your local Maven repository and generate the radio
application source files.

## Building joynr JavaScript

The joynr JS binding is build using Maven:

```bash
<JOYNR>/javascript$ mvn clean install
```

# Exploring the JavaScript demo

The node JavasciptRadioApp demo is located in `<JOYNR>/examples/radio-node`. We refer to this
location as `RADIO_HOME`.

Have a look into the [Radio Tutorial](Tutorial.md) to get an understading of the communication
interface and the basic joynr concepts.

The generated source code from the Radio model is located in `<RADIO_HOME>/src/main/generated`.

## Providers

Have a look into
[\<RADIO_HOME\>/src/main/js/radioProvider.js](/examples/radio-node/src/main/js/radioProvider.js)
and
[\<RADIO_HOME\>/src/main/js/MyRadioProvider.js](/examples/radio-node/src/main/js/MyRadioProvider.js)
for provider implementation and registration details.

## Consumers

Have a look into
[\<RADIO_HOME\>/src/main/js/radioConsumer.js](/examples/radio-node/src/main/js/radioConsumer.js)
for consumer implementation details.

## In Action

### Prerequisite

You need to have Maven installed.

For both, consumer and provider, the backend (MQTT broker, Global Discovery Service)
and a local standalone cluster-controller need to be started first.

### Starting the Backend

Please refer to the
[starting joynr backend instructions](../wiki/infrastructure.md)

### Starting the Cluster controller

In order to be able to start a standalone C++ cluster controller,
the C++ environment must have been built previously.

Start the standalone cluster controller as follows:

```bash
cd <JOYNR>/cpp/build/joynr/bin
./cluster_controller
```

### Prepare the node runtime environment

The node environment has to be prepared once prior to starting
applications.

```bash
<RADIO_HOME>$ mvn install
<RADIO_HOME>$ npm install
```

### Running the Provider and Consumer

Run the provider application in a terminal

```bash
<RADIO_HOME>$ npm run startprovider
```

Run the consumer in another terminal

```bash
<RADIO_HOME>$ npm run startconsumer
```

## Provisioning

The provisioning is partially done in the files
[\<RADIO_HOME\>/package.json](/examples/radio-node/package.json) and
[\<RADIO_HOME\>/src/main/js/provisioning_common.js](/examples/radio-node/src/main/js/provisioning_common.ts).

This example uses the UDS libjoynr runtime which communicates via unix domain socket with a preexisting
cluster controller.  
For different runtimes (e.g. WebSocket runtime) and further options, see [radio-node README](/examples/radio-node/README).

See [JavaScript Configuration Reference](JavaScriptSettings.md) for details on the possible
provisioning settings.

