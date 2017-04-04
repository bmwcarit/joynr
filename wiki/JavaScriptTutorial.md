This tutorial will guide you through a simple joynr radio application, explaining three essential
joynr concepts:

* A simple radio **communication interface**
* A **consumer** interested in knowing about radio information
* A **provider**, that provides the radio information

# Prerequisites
If you haven't built joynr yet, please do so first:

* [Building joynr Java](java_building_joynr.md).

This will install the necessary dependencies to your local Maven repository and generate the radio
application source files. In particular, the
[Franca IDL](https://code.google.com/a/eclipselabs.org/p/franca/) dependencies that are currently
not available from [Maven Central Repository](http://search.maven.org/) are installed. Since Franca
is needed for joynr code generation, we ship Franca dependencies together with the joynr source code
in the `<JOYNR>/tools/generator/dependency-libs/` directory.

## Building joynr JavaScript

The joynr JS binding is build using Maven:

```bash
<JOYNR>/javascript$ mvn clean install -DskipTests
```

# Exploring the JavaScript demo

The JavasciptRadioApp demo is located in `<JOYNR>/examples/radio-js`. We refer to this location as
`RADIO_HOME`.

There is also a Node.js version of the radio demo application located in `<JOYNR>/examples/radio-node`.

Have a look into the [Radio Tutorial](Tutorial.md) to get an understading of the communication
interface and the basic joynr concepts.

The generated source code from the Radio model is located in `<RADIO_HOME>/src/main/generated`.

## Providers

Have a look into [\<RADIO_HOME\>/src/main/webapp/js/provider.js]
(/examples/radio-js/src/main/webapp/js/provider.js) for provider implementation and
registration details.

## Consumers

Have a look into [\<RADIO_HOME\>/src/main/webapp/js/consumer.js]
(/examples/radio-js/src/main/webapp/js/consumer.js) for consumer implementation details.

## In Action

### Prerequisite
You need to have Maven installed. Joynr is tested with Maven 3.3.3, but more recent versions should
also work here.

For both, consumer and provider, the backend (Bounceproxy and Discovery) has to be started first.

### Starting the Backend
Run a MQTT broker (e.g. [Mosquitto](http://mosquitto.org)) listening on port 1883 and deploy
discovery-directory-jee and domain-access-controller-jee to a Java EE application server
(e.g. Payara):
```
asadmin deploy <RADIO_HOME>/target/discovery-jee.war
asadmin deploy <RADIO_HOME>/target/accesscontrol-jee.war
```

See [JEE Developer Guide](wiki/jee.md) or [Radio App Tutorial](wiki/Tutorial.md) for the
configuration of Payara.

### Deploy provider and consumer on a Jetty server
The following Maven command will start a [Jetty Server](http://eclipse.org/jetty/) on
`localhost:8080` and automatically deploy provider and consumer webapp:

```bash
<RADIO_HOME>$ mvn jetty:run-war
```

### Running the Provider and Consumer

To run the provider and consumer open the following URLs in your browser (tested with Chrome and
Firefox):

* Provider: http://localhost:8080/provider.html
* Consumer: http://localhost:8080/consumer.html

## Provisioning
joynr provides four different joynr runtimes:

* WebSocket libjoynr runtime: communicates
  via WebSocket with a preexisting cluster controller
* Inprocess runtime: provides its own cluster
  controller
* Intertab cluster controller runtime: runtime expected
  to run in the browser. Allows to be
  accessed by other browser tabs for uplink
  communication with the joynr backend infrastructure
* Intertab libjoynr runtime: runtime expected to run in
  the browser. Communicates via intertab
  communication with a preexisting cluster controller
  in the browser

Ensuring a proper startup of the several runtimes, the following objects must be provided to the constructor
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
            providerVersion: <provider version>,
            scope: <ProviderScope.GLOBAL|ProviderScope.LOCAL>,
            priority: <priority>,
            supportsOnChangeSubscriptions: <true|false>
        },
        participantId: <participantId>
    },
    ...
];

var loggingValue = {
    ttl: <ttl>, //default value: 172800000, two days in milliseconds
    configuration: {...} /*
                       * log4j2-style JSON config, but as JavaScript object
                       * See https://logging.apache.org/log4j/2.x/manual/configuration.html#JSON
                       * for more information
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

};

var websocketLibJoynrProvisioning = {
    capabilities: capabilitiesValue, //optional
    logging: loggingValue, //optional
    internalMessagingQos: internalMessagingQosValue, //optional
    messaging: messagingValue, //optional
    persistency: persistencyValue, //optional
    ccAddress: <ccAddress>, /*
                             * mandatory input: the address, how the cluster controller
                             * can be reached. For the WebSocketLibjoynrRuntime, the
                             * ccAddress expected to be of the following structure:
                             * {
                             *     protocol: <protocol>, //default value is "ws"
                             *     port: <port>,
                             *     host: <host>,
                             *     path: <path> //default value is ""
                             */
    websocket: { // optional
        // default value is 1000
        reconnectSleepTimeMs : <time in milliseconds between websocket reconnect attempts>
    }
};

var interTabLibjoynrProvisioning = {
    capabilities: capabilitiesValue, //optional
    logging: loggingValue, //optional
    internalMessagingQos: internalMessagingQosValue, //optional
    messaging: messagingValue, //optional
    persistency: persistencyValue, //optional
    window : <window object>,
    windowId : windowId,
    parentWindow : <parent windows>, // e.g. window.opener || window.top
    parentOrigin : <paranet origin> // e.g. location.origin || (window.location.protocol+'//'+window.location.host)
};

var inProcessProvisioning = {
    capabilities: capabilitiesValue, //optional
    logging: loggingValue, //optional
    internalMessagingQos: internalMessagingQosValue, //optional
    messaging: messagingValue, //optional
    persistency: persistencyValue, //optional
    brokerUrl: <url of the mqtt broker>, // e.g. tcp://127.0.0.1:9001
    bounceProxyBaseUrl: <base url to the bounce proxy>, // e.g. http://127.0.0.1:8080
    bounceProxyUrl: <url to bounce proxy>, // e.g. http://127.0.0.1:8080/bounceproxy/
    channelId: <channelId to be used>, // optional
    channelUrls: { ... }, // optional provisioned mapping of channelId to url
    channelQos: { // optional
        messageProcessors: <# of message processors>, //default value is 4
        resendDelay_ms: <resend delay>, //default value is 1000
    },
    // Interval in milliseconds at which the clustercontroller will send a freshness
    // update message to the global discovery directory
    capabilitiesFreshnessUpdateIntervalMs : <capabilitiesFreshnessUpdateIntervalMs> // optional, default value is 3600000 (1 hour)
};

var interTabClusterControllerProvisioning = {
    capabilities: capabilitiesValue, //optional
    logging: loggingValue, //optional
    internalMessagingQos: internalMessagingQosValue, //optional
    messaging: messagingValue, //optional
    persistency: persistencyValue, //optional
    brokerUrl: <url of the mqtt broker>, // e.g. tcp://127.0.0.1:9001
    bounceProxyBaseUrl: <base url to the bounce proxy>, // e.g. http://127.0.0.1:8080
    bounceProxyUrl: <url to bounce proxy>, // e.g. http://127.0.0.1:8080/bounceproxy/
    channelId: <channelId to be used>, // optional
    channelUrls: { ... }, // optional provisioned mapping of channelId to url
    channelQos: { // optional
        messageProcessors: <# of message processors>, //default value is 4
        resendDelay_ms: <resend delay>, //default value is 1000
    }
    parentWindow : <parent windows>, // e.g. window.opener || window.top
    parentOrigin : <paranet origin>, // e.g. location.origin || (window.location.protocol+'//'+window.location.host)
    window : <window object>,
    // Interval in milliseconds at which the clustercontroller will send a freshness
    // update message to the global discovery directory
    capabilitiesFreshnessUpdateIntervalMs : <capabilitiesFreshnessUpdateIntervalMs> // optional, default value is 3600000 (1 hour)
};
```
