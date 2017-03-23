# Joynr Java Settings

The following describes which settings can be made in Java, either by:

1. properties file
2. OS environment settings
3. Java System settings
4. programmatically

All defaults are set in defaultMessaging.properties and defaultServletMessaging.properties. The
properties that must be overriden for a normal deployment (assuming you are not just testing on
localhost) are marked below as REQUIRED.

## ConfigurableMessagingSettings

### `PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID`
The channel ID of the global capabilities directory (backend). To be able to connect to the global
capabilities directory a disovery entry is created in the local capabilities directory as well as an
appropriate routing table entry.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.messaging.capabilitiesdirectorychannelid`
* **Default value**: `discoverydirectory_channelid`

### `PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID`
The participant ID of the global capabilities directory (backend). To be able to connect to the
global capabilities directory a disovery entry is created in the local capabilities directory as
well as an appropriate routing table entry.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.messaging.capabilitiesdirectoryparticipantid`
* **Default value**: `capabilitiesdirectory_participantid`

### `PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN`
The domain of the discovery services (backend). To be able to connect to the global discovery
directories (capability directory, channel url directory, access controller) a disovery entry is
created in the local capabilities directory.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.messaging.discoverydirectoriesdomain`
* **Default value**: `io.joynr`

### `PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID`
The channel ID of the global domain access controller (backend). To be able to connect to the global
domain access controller a disovery entry is created in the local capabilities directory as well as
an appropriate routing table entry.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.messaging.domainaccesscontrollerchannelid`
* **Default value**: `domainaccesscontroller_channelid`

### `PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID`
The participant ID of the global domain access controller (backend). To be able to connect to the
global domain access controller a disovery entry is created in the local capabilities directory as
well as an appropriate routing table entry.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.messaging.domainaccesscontrollerparticipantid`
* **Default value**: `domainaccesscontroller_participantid`

### `PROPERTY_HOSTS_FILENAME`
File used by the HTTP messaging stub to map URLs. It uses the Java properties file format. The key
of an entry identifies a hostname that must be replaced in a URL. The value of an entry consists of
up to four compoments separated by a colon:

1. replacement host name,
2. replacement port,
3. path search term and
4. replacement path.

```
hostname-to-replace=host-replacement:port-replacement:path-search-term:path-replacement
```

This entry will apply to all URLs with hostname `hostname-to-replace`. The original hostname and
port are replaced by `host-replacement` and `port-replacement`, respectively. Furthermore, the
original path is searched for `path-search-term` and the first occurance is relaced by
`path-replacement`.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.messaging.hostsFileName`
* **Default value**: `hosts.properties`

### `PROPERTY_MAX_MESSAGE_SIZE`
The maximum length of a text message the WebSocket transport is able to send/receive.

* **OPTIONAL**
* **Type**: int
* **User property**: `joynr.messaging.maxmessagesize`
* **Default value**: `4000000`

### `PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS`
The number of threads used by the message router to send joynr messages.

* **OPTIONAL**
* **Type**: int
* **User property**: `joynr.messaging.maximumparallelsends`
* **Default value**: `20`

### `PROPERTY_MESSAGING_MAXIMUM_TTL_MS`
The maximum allowed time-to-live (TTL) of joynr messages. The TTL used in a joynr message is set on
the proxy builder using the messaging QoS object. These TTLs are only accepted up to the maximum
value defined by this property.

* **OPTIONAL**
* **Type**: long
* **User property**: `joynr.messaging.maxttlms`
* **Default value**: `2592000000` (30 days)

### `PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE`
If the file based participant ID storage (`PropertiesFileParticipantIdStorage`) is used, participant
IDs of registered providers are persisted to this file and reused for further registrations to the
same interface and domain combination.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.discovery.participantids_persistence_file`
* **Default value**: `joynr_participantIds.properties`

### `PROPERTY_SEND_MSG_RETRY_INTERVAL_MS`
The message router sends joynr messages through different messaging middlewares (WebSockets, HTTP,
MQTT, ...) using middleware-specific messaging stubs. On transmission errors the message router
initiates a retransmission. If the messaging stub does not provide information on when to retry
message transmission, the message router will use the send message retry interval defined by this
property to delay the message transmission and start a new transmission attempt. Multiple
unsuccessful retransmittion attempts will add an additional exponential backoff to delay message
transmission.

* **OPTIONAL**
* **Type**: long
* **User property**: `joynr.messaging.sendmsgretryintervalms`
* **Default value**: `3000`

### PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS

The cluster controller sends a freshness update message to the global discovery directory every
PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS milliseconds. The global discovery directory
updates the ```lastSeenDateMs``` of all capabilities registered via this cluster controller.

* **OPTIONAL**
* **Type**: long
* **User property**: `joynr.capabilities.freshnessupdateintervalms`
* **Default value**: `3600000`

##MessagingPropertyKeys

### `PROPERTY_BOUNCE_PROXY_URL`
The root URL of the BounceProxy backend service when using HTTP messaging. The cluster controller
uses this service to create a receive channel (message queue). Messages are posted to the receive
channel in the backend. The cluster controller polls the channel to download the incoming messages.

* **REQUIRED if using the AtmosphereMessagingModule**
* **Type**: String
* **User property**: `joynr.messaging.bounceproxyurl`
* **Default value**: `http://localhost:8080/bounceproxy/`

### `PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT`
Select primary global transport middleware which will be used to register providers. The provider
will be reachable via the selected global transport middleware.

* **REQUIRED if using more than one global transport**
* **Type**: String
* **User property**: `joynr.messaging.primaryglobaltransport`
* **Default value**: NOT SET

### `CAPABILITYDIRECTORYURL`
The URL of the receive channel (incoming message queue) of the global capabilities directory backend
service. To connect to the global capabilities directory the cluster controller creates an
appropriate entry in the local capabilities directory.  
If the capabilities directory is using MQTT as its primary transport, then the URL you set here
is that of the MQTT broker configured for the capabilities directory. E.g.
`tcp://mqttbroker:1883`.  
See also the static capabilities provisioning documentation below.

* **OPTIONAL** (see static capabilities provisioning)
* **Type**: String
* **User property**: `joynr.messaging.capabilitiesdirectoryurl`
* **Default value**: `http://localhost:8080/discovery/channels/discoverydirectory_channelid/`

### `DOMAINACCESSCONTROLLERURL`
The URL of the receive channel (incoming message queue) of the global domain access
controller service. To connect to the global domain access controller directory
the cluster controller creates an appropriate entry in the local capabilities directory.
See also the static capabilities provisioning documentation below.

* **OPTIONAL** (see static capabilities provisioning)
* **Type**: String
* **User property**: `joynr.messaging.domainaccesscontrollerurl`
* **Default value**: `http://localhost:8080/discovery/channels/discoverydirectory_channelid/`

### `PROPERTY_SERVLET_HOST_PATH`
If a joynr application is deployed into a servlet on an application server, the servlet host path is
used to register provider with the global capabilities and channel URL directories. Hence, this must
be a public host that is directly addressable from all joynr endpoints.

* **REQUIRED if using the JEE integration**
* **Type**: String
* **User property**: `joynr.servlet.hostpath`
* **Default value**: `http://localhost:8080`

### `PROPERTY_SERVLET_SHUTDOWN_TIMEOUT`
During joynr shutdown, providers must be removed from the global capabilities directory.
Since messages could not be received during servlet shutdown anymore, the joynr message receiver
switches to long polling to perform the directory cleanup. This timeout (in milliseconds) is
used to switch to long polling.

* **OPTIONAL**
* **Type**: int
* **User property**: `joynr.servlet.shutdown.timeout`
* **Default value**: `10000`

### `PROPERTY_SERVLET_SKIP_LONGPOLL_DEREGISTRATION`
If set to true, the joynr message receiver will not switch to long polling for deregistration (cf.
[`PROPERTY_SERVLET_SHUTDOWN_TIMEOUT`](#property_servlet_shutdown_timeout)).


* **OPTIONAL**
* **Type**: boolean
* **User property**: `joynr.servlet.skiplongpollderegistration`
* **Default value**: `false`

## MqttModule

### `PROPERTY_KEY_MQTT_BROKER_URI`
The URI of the MQTT broker backend service the cluster controller connects to.

* **REQUIRED if using the MQTTModule**
* **Type**: String
* **User property**: `joynr.messaging.mqtt.brokeruri`
* **Default value**:

### `PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS`
If an error occurs on the MQTT connection, joynr will wait for this time (in milliseconds) before
trying to connect again.

* **OPTIONAL**
* **Type**: int
* **User property**: `joynr.messaging.mqtt.reconnect.sleepms`
* **Default value**: `1000`

### `PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMER_SEC`
Sets the "keep alive" interval measured in seconds. If no message is transmitted during this period,
the client sends a ping message which is acknowledged by the server. This allows a client to detect
disconnects without using TCP/IP mechanisms. A value of 0 disables the "keep alive" mechanism.

* **OPTIONAL**
* **Type**: int
* **User property**: `joynr.messaging.mqtt.keepalivetimersec`
* **Default value**: `60`

### `PROPERTY_KEY_MQTT_CONNECTION_TIMEOUT_SEC`
Sets the connection timeout measured in seconds. This value states how long a client will wait until
a network connection to the server is established. A value of 0 means that a client will wait until
the network connection is established successfully or fails.

* **OPTIONAL**
* **Type**: int
* **User property**: `joynr.messaging.mqtt.connectiontimeoutsec`
* **Default value**: `30`

### `PROPERTY_KEY_MQTT_TIME_TO_WAIT_MS`
Sets the maximum time for an action to complete (measured in milliseconds) before the control is returned
to the application. A value of -1 means that no timeout is used for actions.

* **OPTIONAL**
* **Type**: int
* **User property**: `joynr.messaging.mqtt.timetowaitms`
* **Default value**: `-1`

### `PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS`

Use this key to activate shared subscription support by setting the property's value to true.
Shared subscriptions are a feature of HiveMQ which allow queue semantics to be used for
subscribers to MQTT topics. That is, only one subscriber receives a message, rather than all
subscribers. This feature can be used to load balance incoming messages on MQTT. This feature
is useful if you want to run a cluster of JEE nodes while using only MQTT for communication
(an alternative is to use the HTTP bridge configuration).

* **OPTIONAL**
* **Type**: Boolean
* **User property**: `joynr.jeeintegration.enable.sharedsubscriptions`
* **Default value**: `false`

## SystemServicesSettings

### `PROPERTY_CC_DISCOVERY_PROVIDER_PARTICIPANT_ID`
The participant ID of the discovery provider on the cluster controller. On the one hand, the cluster
controller assigns this participant ID to the discovery provider, on the other hand, the libjoynr
connects the discovery proxy to this participant ID and creates an appropriate routing table entry.
Hence, this config must match between processes using a libjoynr to connect to a cluster controller
and the cluster controller process itself.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.messaging.discoveryproviderparticipantid`
* **Default value**: `CC.DiscoveryProvider.ParticipantId`

### `PROPERTY_CC_ROUTING_PROVIDER_PARTICIPANT_ID`
The participant ID of the routing provider on the cluster controller. On the one hand, the cluster
controller assigns this participant ID to the routing provider, on the other hand, the libjoynr
connects the routing proxy to this participant ID and creates an appropriate routing table entry.
Hence, this config must match between processes using a libjoynr to connect to a cluster controller
and the cluster controller process itself.

* **INTERNAL ONLY**
* **Type**: String
* **User property**: `joynr.messaging.routingproviderparticipantid`
* **Default value**: `CC.RoutingProvider.ParticipantId`

### `PROPERTY_SYSTEM_SERVICES_DOMAIN`
The domain used to register system serivces (i.e. discovery and routing provider) on the cluster
controller. On the one hand, the cluster controller registers system serivces on this domain, on the
other hand, the libjoynr connects the discovery and routing proxy to this domain and creates
appropriate discovery entries. Hence, this config must match between processes using a libjoynr to
connect to a cluster controller and the cluster controller process itself.

* **INTERNAL ONLY**
* **Type**: String
* **User property**: `joynr.messaging.systemservicesdomain`
* **Default value**: `io.joynr.system`

## WebsocketModule

### `PROPERTY_WEBSOCKET_MESSAGING_HOST`
The host running a cluster controller with activated web socket transport to connect to.

* **REQUIRED if using the WebsocketModule to connect a Java libjoynr to the Cluster Controller
  via WebSockets**
* **Type**: String
* **User property**: `joynr.messaging.cc.host`
* **Default value**:

### `PROPERTY_WEBSOCKET_MESSAGING_IDLE_TIMEOUT`
The maximum idle time out (in milliseconds) for web socket connections.

* **OPTIONAL**
* **Type**: long
* **User property**: `joynr.messaging.cc.idletimeout`
* **Default value**: `60000`

### `PROPERTY_WEBSOCKET_MESSAGING_PATH`
The path to the cluster controller's web socket transport to connect to.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.messaging.cc.path`
* **Default value**: empty string

### `PROPERTY_WEBSOCKET_MESSAGING_PORT`
The port of the cluster controller's web socket transport to connect to.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.messaging.cc.port`
* **Default value**:

### `PROPERTY_WEBSOCKET_MESSAGING_RECONNECT_DELAY`
If the connect to the cluster controller's web socket transport fails, the libjoynr perform a new
connect attempt after this delay.

* **OPTIONAL**
* **Type**: long
* **User property**: `joynr.messaging.cc.reconnectdelay`
* **Default value**: `1000`

### `PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL`
The protocol used for the web socket connection to the cluster controller's web socket transport.
Possible values are:

* **OPTIONAL**
* **Type**: Enumeration
 * `ws`
 * `wss`
* **User property**: `joynr.messaging.cc.protocol`
* **Default value**: `ws`


## <a name="ExpiredDiscoveryEntryCacheCleaner"></a>ExpiredDiscoveryEntryCacheCleaner

### `DISCOVERY_ENTRY_CACHE_CLEANUP_INTERVAL`
The time interval in minutes at which the capabilities cache will be searched for expired
discovery entries, and these will be expunged from the cache. Applies to both local and
global cached discovery entries.

* **OPTIONAL**
* **Type**: int
* **User property**: `joynr.cc.discovery.entry.cache.cleanup.interval`
* **Default value**: `60`


## JEE Integration

These properties are defined as constants in the
`io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys` class.

### `JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY`

Set this property to `true` if you want to use the HTTP Bridge functionality. In this
configuration incoming messages are communicated via HTTP and can then be load-balanced
accross a cluster via, e.g. nginx, and outgoing messages are communicated directly
via MQTT. If you activate this mode, then you must also provide an endpoint registry
(see next property).

* **OPTIONAL**
* **Type**: Boolean
* **User property**: `joynr.jeeintegration.enable.httpbridge`
* **Default value**: `false`

### `JEE_INTEGRATION_ENDPOINTREGISTRY_URI`

This property needs to point to the endpoint registration service's URL with which the
JEE Integration will register itself for its channel's topic.
E.g. `http://endpointregistry.mycompany.net:8080`.
See also `io.joynr.jeeintegration.httpbridge.HttpBridgeEndpointRegistryClient`.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.jeeintegration.endpointregistry.uri`
* **Default value**: n/a

## Static Capabilties Provisioning

### `PROPERTY_PROVISIONED_CAPABILITIES_FILE`

This property can be used to determine the name, URI or path of a file / resource which
can be read from either a remote URI, the local file system, or if not found there the
classpath and contains the capabilities to be statically provisioned for the runtime.

By default the global capabilities directory and global domain access control directory
are statically provisioned. But you are not limited to just provisioning those.

The content of the file is a JSON serialised array of GlobalDiscoveryEntry objects. The
default file is `provisioned_capabilities.json` and is read from the classpath from the
`libjoynr.jar`.

When specifying a URI as the source of the provisioning file, then ensure that it is
an absolute URI (contains the scheme, e.g. `http://` or `file:`) and that the resource
it points to is available for reading at startup time of the application.

The capabilities directory and domain access control directory have a special status, in
that the system requires exactly one entry for each to be provisioned. The system will
fail to start if either one is lacking or duplicate entries have been provisioned.  
If you want to change either one of those entries from the default, you don't have to
do so using the JSON format. You can override the entries from the JSON by using the
properties listed in the `ConfigurableMessagingSettings` section above.  
Generally you will simply specifiy one of `DISCOVERYDIRECTORYURL` and/or
`DOMAINACCESSCONTROLLERURL`, although it is also possible to override all other parts
of the entry if necessary. Specifying an incomplete entry by, e.g., setting the
participant ID to an empty value will result in the system failing to start.

* **OPTIONAL**
* **Type**: String
* **User property**: `joynr.capabilities.provisioned.file`
* **Default value**: `provisioned_capabilities.json`
