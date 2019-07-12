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

You need to have Maven installed. Joynr is tested with Maven 3.3.3, but more recent versions should
also work here.

For both, consumer and provider, the backend (MQTT broker, JEE based Discovery and AccessControl)
and a local standalone cluster-controller need to be started first.

### Starting the Backend

Run a MQTT broker (e.g. [Mosquitto](http://mosquitto.org)) listening on port 1883 and deploy
discovery-directory-jee and domain-access-controller-jee to a Java EE application server
(e.g. Payara):
```
asadmin deploy <RADIO_HOME>/target/discovery-jee.war
asadmin deploy <RADIO_HOME>/target/accesscontrol-jee.war
```

See [JEE Developer Guide](jee.md) or [Radio App Tutorial](Tutorial.md) for the
configuration of Payara.

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
<RADIO_HOME>$ npm install
```

### Running the Provider and Consumer

Run the provider application in a terminal

```bash
<RADIO_HOME>$ npm run-script startprovider
```

Run the consumer in another terminal

```bash
<RADIO_HOME>$ npm run-script startconsumer
```

## Provisioning

The provisioning is partially done in the files
[\<RADIO_HOME\>/package.json](/examples/radio-node/package.json) and
[\<RADIO_HOME\>/src/main/js/provisioning_common.js](/examples/radio-node/src/main/js/provisioning_common.js).

This example uses the WebSocket libjoynr runtime which communicates via WebSocket with a preexisting
cluster controller.

Ensuring a proper startup of the runtime, the following objects must be provided to the constructor
of the runtime.

```
var capabilitiesValue = [ // untyped list of provisioned capabilities
    {
        domain: <domain>,
        interfaceName: <fully/qualified/interface/name>,
        providerQos: {
            customParameters: [
                {
                    name: <name>,
                    value: <value>
                },
                ...
            ],
            scope: <ProviderScope.GLOBAL|ProviderScope.LOCAL>,
            priority: <priority>,
            supportsOnChangeSubscriptions: <true|false>
        },
        providerVersion: <provider version>,
        participantId: <participantId>
    },
    ...
];

var discoveryQosValue = {
    discoveryTimeoutMs: <number>
    discoveryRetryDelayMs: <number>
    discoveryExpiryIntervalMs: <number> // discoveryExpiryIntervalMs + Date.now() = expiryDateMs
};

var loggingValue = {
    configuration: {...} /*
                       * log4j2-style JSON config, but as JavaScript object
                       * See https://logging.apache.org/log4j/2.x/manual/configuration.html#JSON
                       * for more information.
                       * Since replacing log4javascript due to performance issues,
                       * not all configuration options are still supported.
                       * - only one appender is supported. Others will be ignored.
                       * - reduced complexity of supported patternLayouts.
                       */
};

var internalMessagingQosValue = { //messaging qos used for joynr internal communication
    ttl: <ttl> // round trip timeout ms for rpc requests, default value is 60000
};

var messagingValue = {
    maxQueueSizeInKBytes: <max queue size in KB bytes> // default value is 10000
};

var persistencyValue = {
    clearPersistency: <true|false>, // clear persistent data during startup. Default value is false
    location: /path/to/localStorage, // Optional. Only implemented for Node. Default is current dir
    quota: 10 * 1024 * 1024 // Optional. Max local storage quota, in MB. Defaults to 5 MB.
    routingTable: <true|false>, /* Optional. Default false. Persists RoutingTable entries and thus
                                 * allows the runtime to restart without help from the cc.
                                 */
    capabilities: <true|false>, /* Optional. Default true. Persists ParticipantIds of registered
                                 * providers and thus keeps them upon restart.
                                 */
    publications: <true|false>, /* Optional. Default true. Persists previously received
                                 * SubscriptionRequests and thus allows publications to resume
                                 * successfully upon restart.
                                 */
};

var shutdownSettingsValue = {
    clearSubscriptionsEnabled: <true|false>, // default true
    clearSubscriptionsTimeoutMs: <number> // default 1000
};

var websocketLibJoynrProvisioning = {
    capabilities: capabilitiesValue, //optional
    discoveryQos: discoveryQosValue, //optional
    logging: loggingValue, //optional
    internalMessagingQos: internalMessagingQosValue, //optional
    messaging: messagingValue, //optional
    persistency: persistencyValue, //optional
    shutdownSettings: shutdownSettingsValue, //optional
    ccAddress: <ccAddress>, /*
                             * mandatory input: the address, how the cluster controller
                             * can be reached. For the WebSocketLibjoynrRuntime, the
                             * ccAddress expected to be of the following structure:
                             * {
                             *     protocol: <protocol>, //default value is "ws"
                             *     port: <port>,
                             *     host: <host>,
                             *     path: <path> //default value is ""
                             * }
                             */
    websocket: { // optional
        // default value is 1000
        reconnectSleepTimeMs : <time in milliseconds between websocket reconnect attempts>
    }
};
```
