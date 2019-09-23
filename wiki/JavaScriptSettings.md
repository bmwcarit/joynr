# Joynr Javascript / TypeScript Settings (Provisioning)

Ensuring a proper startup of the JavaScript joynr runtime, some settings are required.

The following describes which settings can be changed in the WebSocketLibjoynrRuntime by providing
a settings object (`websocketLibJoynrProvisioning`) to the constructor of the runtime.
Most settings are optional.

```
var websocketLibJoynrProvisioning = {
    capabilities: capabilitiesValue, //optional
    discoveryQos: discoveryQosValue, //optional
    logging: loggingValue, //optional
    internalMessagingQos: internalMessagingQosValue, //optional
    messaging: messagingValue, //optional
    persistency: persistencyValue, //optional
    shutdownSettings: shutdownSettingsValue, //optional
    ccAddress: ccAddressValue, // MANDATORY
    websocket: { // optional
        // default value is 1000
        reconnectSleepTimeMs : <time in milliseconds between websocket reconnect attempts>
    }
};

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

var ccAddressValue = {
    // the address, how the cluster controller can be reached.
    protocol: <protocol>, //optional, default value is "ws"
    port: <port>, // MANDATORY
    host: <host>, // MANDATORY
    path: <path> //optional, default value is ""
};
```

