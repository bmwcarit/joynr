# joynr 0.27.0

## Other changes
* **[C++]** The cluster controller can be configured in such a way that access control checks are performed
  only with locally provisioned or cached data. The configuration is done by using the
  `access-control/use-ldas-only` property in the cluster controller settings.

# joynr 0.26.0

## API relevant changes
* **[JavaScript]** It is now possible to register a provider with a preconfigured
  participantId. The application is responsible to ensure that the participantId is
  unique, globally or locally depending on the provider's scope. See the JSDoc for more
  information.
* **[Java, JEE]** RawMessagingPreprocessor now accepts a byte array as an input parameter and returns
  a byte array instead of a string.
* **[Java]** JoynrMessageProcessor uses the new SMRF message types: MutableMessage for outgoing messages
  and ImmutableMessage for incoming messages.

## Other changes
* **[Java,C++,JS]** HTTP communication is not supported at the moment
* **[JS]** Browser based environments, e.g. radio-js, are not supported at the moment
* **[JS]** Direct MQTT based communication is not supported at the moment.
  Please use the WebSocketLibjoynrRuntime to connect to external cluster controller handling
  the MQTT connection to the backend.
* **[JS]** Introduced mandatory dependency to the 'wscpp' module (https://github.com/bmwcarit/wscpp)
  for the node environment.
* **[Java]** Global Discovery and Global Domain Access Controller via Jetty using HTTP based
  communication are no longer supported, please use the JEE implementations based
  on MQTT communication instead
* **[Java]** Updated to use of Jackson 2.8.8 in order to improve compatibility with Payara.

## Backward compatibility
This version of joynr is NOT compatible with previous versions due to internal changes:
* **[cluster-controller]** cluster-controller expects libjoynr to signal globally visible providers
* **[messaging layer]** Switched to SMRF messaging format.

# joynr 0.25.3

## API relevant changes

## Other changes
* **[C++]** setting "discovery-entry-expiry-interval-ms" can now store values up to 2^63-1

# joynr 0.25.2

## API relevant changes

## Other changes
* **[C++]** libCommon has been moved to libJoynr. This fixes issues with static linking with libjoynr.
* **[JEE]** Shutdown MQTT client when undeploying WebApp.

# joynr 0.25.1

## API relevant changes

## Other changes
* **[C++]** Fixed a race condition in DelayedScheduler potentially leading to an assertion.
* **[JEE]** Fixed Mqtt reconnect bug by updating the mqtt-client

# joynr 0.25.0

## API relevant changes
* **[JEE]** Applications can inject a RawMessagingPreprocessor to modify or inspect messages arriving
  via MQTT
* **[JAVA/JEE]** JoynrMessageProcessor.process was divided into a processOutgoing and processIncoming
  method. The processOutgoing method is called right after a message was created that will be sent
  to another Joynr instance. The processIncoming method is called for messages which were received
  from another Joynr instance.

## Other changes
* **[C++]** Added POSIX signal handling which can control starting/stopping external communication
in cluster-controller process. It can also trigger termination of the cluster-controller process.
See [Joynr C++ configuration reference](CppConfigurationReference.md) for more information.

# joynr 0.24.1

## API relevant changes

## Other changes
* **[Java]** Fixed a bug where enumeration parameters in fire and forget method calls
  were improperly deserialized on provider side, leading to an exception.

# joynr 0.24.0

## API relevant changes

* **[All]** Added 'encrypt' to MessagingQos (incl. additional constructors, getter/setter),
  existing MessagingQos APIs remain working
* **[C++]** Providers can be (un)registered asynchronously through `(un)registerProviderAsync`
* **[All]** The 'GlobalDomainAccessController' interface has been split up into 3 interfaces:
  'GlobalDomainRoleController' (contains the 'role' based APIs),
  'GlobalDomainAccessController' (contains the read-only getter & broadcast APIs for
  master / mediator / owner access and registration control entries) and
  'GlobalDomainAccessControlListEditor' (contains the modification related APIs for
  master / mediator / owner access and registration control entries)
  See `basemodel/src/main/franca/joynr/*.fidl` for details.
* **[Java]** Moved and changed property `JeeIntegrationPropertyKeys.
  JEE_ENABLE_SHARED_SUBSCRIPTIONS="joynr.jeeintegration.enable.sharedsubscriptions"` to `MqttModule.
  PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS="joynr.messaging.mqtt.enable.sharedsubscriptions"`
* **[Java]** MQTT shared subscriptions are not restricted to JEE any longer
* **[All]** Added 'compress' to MessagingQos (available with Java/C++ solely via
  getter/setter, in JS also via constructor), existing MessagingQos APIs remain working
* **[All]** Multiple global transports cannot be used in parallel any longer, either Mqtt
  or Http has to be used
* **[All]** Mqtt / Jee joynr backend services are used by default now (set in default settings).
  * To use the http backend in **Java**, set DISCOVERYDIRECTORYURL and DOMAINACCESSCONTROLLERURL
    as explained in the [Java Configuration Reference](JavaSettings.md).
  * To use the http backend in **C++**, change the cluster controller's messaging settings:
    set "broker-url" to the bounceproxy's url (e.g. "http://localhost:8080/bounceproxy/"),
    set "capabilities-directory-url" to the capabilities directory's channel url (e.g.
    "http://localhost:8080/discovery/channels/discoverydirectory_channelid/"),
    set "capabilities-directory-channelid" to "discoverydirectory_channelid" (default is the
    serialized Mqtt address of the global discovery directory)
  * To use the http backend in **JS**, start a C++/Java cluster controller configured to use
    the http backend (see above)
* **[C++]** Removed messaging setting bounceproxy-url since it is not possible to use http
  (bounceproxy) in parallel with mqtt (broker)

## Other changes
* **[C++]** Access control can be activated in the cluster-controller. Default: OFF.
  Refer to [cluster controller settings](ClusterControllerSettings.md) for more info.
* **[Java]** Added properties (`PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMER_SEC`,
  `PROPERTY_KEY_MQTT_CONNECTION_TIMEOUT_SEC` and `PROPERTY_KEY_MQTT_TIME_TO_WAIT_MS`) to
  configure the MQTT connection. See [JavaSettings](JavaSettings.md) for more information.
* **[C++]** Moved to muesli 0.3.1 for serialization of boost::multi_index containers
* **[Java]** Allow to set prefixes of Mqtt topics, see [JavaSettings](JavaSettings.md).
* **[C++]** Allow to set prefixes of Mqtt topics in cluster-controller settings:
  `mqtt-multicast-topic-prefix` and `mqtt-unicast-topic-prefix`
* **[Java]** Added property (`PROPERTY_ACCESSCONTROL_ENABLE`) to enable access control checks.
  See [JavaSettings](JavaSettings.md) for more information.

# joynr 0.23.2

## API relevant changes
none

## Other changes
* **[JEE]** Fixed issue that caused joynr not to start correctly with debug logging enabled
* **[Java]** Do not send customHeaders as their own json object
* **[Java]** Made MQTT reconnect behavior more robust

# joynr 0.23.1

## API relevant changes
* **[C++]** createRuntimeAsync returns runtime directly instead of via a callback;
  the runtime must not be used until the onSuccess callback is called
* **[C++]** ProxyBuilder::buildAsync will not block if arbitration is not possible

## Other changes
* **[C++, Java, JS]** Fix bugs in code generation for typedef.

# joynr 0.23.0

## API relevant changes

* **[JEE]** Providers are no longer deregistered automatically when the application is shutdown.
* **[C++]** Proxy builder returns a std::unique_ptr to the created proxy instead of a raw pointer.
* **[C++]** Joynr runtime returns a std::unique_ptr to a created proxy builder instead of a raw pointer.
* **[C++]** Created joynr runtime is returned as a std::unique_ptr.
* **[All]** Introduce MulticastSubscriptionQos for non selective broadcasts.
* **[All]** Removed deprecated time related APIs from `SubscriptionQos`, `PeriodicSubscriptionQos`,
  `OnChangeSubscriptionQos`, `OnChangeWithKeepAliveSubscriptionQos`,
  `HeartbeatSubscriptionInformation`, `DiscoveryQos`
* **[JS]** Removed deprecated `capabilities` member from runtimes
* **[JS]** Removed deprecated `registerCapability`, `unregisterCapability` methods from
  `CapabilitiesRegistrar`
* **[C++, Java]** Removed deprecated `providerQos` attribute from provider and related
  `registerProvider` API (without `providerQos` parameter) from `JoynrRuntime`
* **[Java]** Removed deprecated `CAPABILITYDIRECTORYURL` from provisioning
* **[All]** Removed deprecated `outputHeaderPath` member from AbstractJoynGeneratorMojo
* **[C++]** createRuntimeAsync error callback exception parameter is now a const reference.
* **[C++]** Removed method `setCached()` from ProxyBuilder
* **[C++]** Removed protected member `cache` from ProxyBase, ProxyFactory

## Other changes
* **[C++]** fix lifetime issue in JoynrMessagingConnector

# joynr 0.22.4

## API relevant changes
None.

## Other changes

* **[C++, JS, Java]** Apply configurable Time To Live (TTL) Uplift to each outgoing message and to
  the expiry date of subscriptions

# joynr 0.22.3

## API relevant changes
None.

## Other changes

* **[C++]** fix MQTT connection to broker blocked after first message was sent
* **[JS]** fix typing issues with maps of structs
* **[JS]** fix receiving too many multicast publications when provider and proxy are in same
  libjoynr
* **[C++]** Bugfix: Provider and consumers do not crash after reconnect to cluster-controller

# joynr 0.22.2

## Other changes

* **[C++]** Bugfix: MQTT sender blocks message router thread in case of connection to broker not
  established.

# joynr 0.22.1

## Other changes

* **[JS]** Bugfix: For non-selective broadcast subscriptions the listeners could be called too
  often if multiple matching listeners were found.

# joynr 0.21.4
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Fix bug in generated data types if base and derived classes have different
  package names.

# joynr 0.22.0

## API relevant changes
* **[Java]** constant PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT has been moved to
  io.joynr.messaging.MessagingPropertyKeys
* **[C++]** During a provider call a call context can be queried which provides the creator user id
  field from the joynr message. Please delete the broadcastsubscriptionrequest-persistence-file and
  subscriptionrequest-persistence-file because the file format changed.
* **[C++]** Introduced async proxy creation. ProxyBuilder now provides a buildAsync method which
  accepts a success and an error callback as parameters.
* **[C++]** Introduced async joynr runtime creation. See JoynrRuntime::createRuntimeAsync for more
  information.
* **[C++]** joynr can now be built with a static and a dynamic log level. The corresponding cmake
  properties are called JOYNR_MAX_LOG_LEVEL and JOYNR_DEFAULT_RUNTIME_LOG_LEVEL. In order to change
  the dynamic log level at runtime a environment variable, which is called "JOYNR_LOG_LEVEL", must
  be exported before any joynr component starts. The runtime log levels are called "TRACE", "DEBUG",
  "INFO", "WARNING", "ERROR" and "FATAL".
* Non-selective broadcasts work only with MQTT until further notice.
  HTTP is currently not supported.
* Non-selective broadcasts support partitions to control broadcast delivery to subscribers.
  * **[C++]** On provider side the fire broadcast method has now an optional partitions argument;
    see [C++ documentation for firing a broadcast](cplusplus.md#firing-a-broadcast). On consumer
    side the subscribe method has now an optional partitions argument; see [C++ documentation for
    subscribing to a broadcast](cplusplus.md#subscribing-to-a-%28non-selective%29-broadcast). The
    subscription ID parameter of the subscribe method for updating an existing subscription moved
    from the last to the first position in the argument list. In addition, it also has now an
    optional partitions argument; see [C++ documentation for updating an existing subscription]
    (cplusplus.md#updating-a-%28non-selective%29-broadcast-subscription).
  * **[Java]** On provider side the fire broadcast method has now an optional varargs argument to
    provide partitions; see [Java documentation for firing a broadcast](java.md#firing-a-broadcast).
    On consumer side the subscribe method has now an optional varargs argument to provide
    partitions; see [Java documentation for subscribing to a broadcast]
    (java.md#subscribing-to-a-%28non-selective%29-broadcast). The subscription ID parameter of the
    subscribe method for updating an existing subscription moved from the last to the first position
    in the argument list. In addition, it has now an optional varargs argument to provide
    partitions; see [Java documentation for updating an existing subscription]
    (java.md#updating-a-%28non-selective%29-broadcast-subscription).
  * **[JS]** On provider side the fire broadcast method has now an optional partitions argument; see
    [JavaScript documentation for firing a broadcast](javascript.md#sending-a-broadcast). On
    consumer side the subscribe method has now an optional partitions entry in the subscription
    settings object; see [JavaScript documentation for subscribing to a broadcast]
    (javascript.md#subscribing-to-a-%28non-selective%29-broadcast). The subscription settings object
    of the subscribe method for updating an existing subscription has also an optional partitions
    entry; see [JavaScript documentation for updating an existing subscription]
    (javascript.md#updating-a-%28non-selective%29-broadcast-subscription).

## Other changes
* **[JS]** Introduced mqtt messaging layer, allowing javascript runtimes including
  cluster controller functionality to connect to a mqtt broker.
* On top of MQTT messaging, joynr uses now a multicast approach to send non-selective broadcast
  publications instead of sending an unicast message to each subscriber. See the [Multicast Concept
  Documentation](../docs/multicast.md) for more details. This change breaks the compatibility on the
  messaging layer to joynr version 0.21.x.

# joynr 0.21.3
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[JS]** Fix bug which prevents successful restore of persisted broadcast subscription
  requests

# joynr 0.21.2
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Fix cluster controller crash if many persisted discovery entries are present

# joynr 0.21.1
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Catch websocket exception if connection is not valid anymore. This caused
  the cluster-controller to crash.
* **[C++]** Fixed installation path of system integration tests.

# joynr 0.21.0

## API relevant changes
* **[JEE]** Ability to specify individual domains for providers via new
  `@ProviderDomain` annotation. See
  [JEE Documentation / Customising the registration domain](jee.md#provider_domain).
* **[Java, JS, C++]** Introduce LastSeen arbitration strategy and set it as default arbitration.
* **[JEE]** Ability to publish multicast messages by injecting the
  subscription publisher. See [JEE Documentation / Publishing Multicasts](jee.md#publishing_multicasts).

## Other changes
* **[Java, C++]** The local capabilities directory will periodically be checked for
  expired discovery entries, and any which have expired will be purged from the
  caches.
  In Java, the interval at which the entries are checked can be configured using
  the `joynr.cc.discovery.entry.cache.cleanup.interval` property (See also the
  [Java Configuration Guide](JavaSettings.md#ExpiredDiscoveryEntryCacheCleaner)).
  In C++ the interval can be configured using the
  `messaging/purge-expired-discovery-entries-interval-ms` key in the messaging
  settings.
* **[C++]** Build variable `USE_PLATFORM_GTEST_GMOCK` now defaults to ON so that
  it is consistent with the other `USE_PLATFORM_*` variables.
* **[C++]** Reduced the number of threads which are used by a cluster controller instance
* **[C++]** The dependency to Qt is now fully removed.

# joynr 0.20.4
This is a minor bug fix release.

## Other changes
* **[C++]** Fixed an issue which caused a high CPU load when a client disconnected from a
  cluster controller.

# joynr 0.20.3
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[JS]** Fix bug which resulted in improper shutdown of joynr.

# joynr 0.20.2
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[JS]** Fixed bug which caused exception when loading persisted
  subscriptions during startup.

# joynr 0.20.1
This is a minor bug fix release.

## API relevant changes
* **[Java]** The BroadcastSubscriptionListener is now able to get informed about succeeded
  subscription requests. For this purpose, it implements a callback having
  the following signature: public void onSubscribed(String subscriptionId).
  In case of failure the onError callback can be invoked with a SubscriptionException.

## Other changes
* **[Java]** the MQTT client now performs a manual re-connect and re-subscribe if the
  connection is lost, because the Paho auto reconnect and persistent subscriptions
  are buggy in the version we're using.
* moved to muesli 0.1.2 to get its bugfix

# joynr 0.20.0

## API relevant changes
* **[JS]** The SubscriptionListener is now able to get informed about succeeded
  subscription requests. For this purpose, he can implement a callback having
  the following signature: void onSubscribed(subscriptionId). In case of
  failure the onError callback can be invoked with a SubscriptionException.
* **[JS]** The consumer is able to synchronize to subscription requests.
  The promise returned by <Interface>Proxy.subscribeTo<Attribute|Broadcast> is
  resolved, once the subscription request has been successfully delivered to the
  interface provider. In case of failure, it can be rejected with a
  SubscriptionException.
* **[Java]** The AttributeSubscriptionAdapter is now able to get informed about succeeded
  subscription requests. For this purpose, it implements a callback having
  the following signature: public void onSubscribed(String subscriptionId).
  In case of failure the onError callback can be invoked with a SubscriptionException.
* **[Java]** The consumer is able to synchronize to subscription requests.
  The subscribeTo<BroadcastName> and subscribeTo<AttributeName> methods
  now return a Future that is resolved once the subscription request has been
  successfully delivered to the interface provider. The get() method of the
  Future returns the subscriptionId on successful execution or can throw
  a SubscriptionException in case of failure.
* **[C++]** The ISubscriptionListener interface is now able to get informed about succeeded
  subscription requests. For this purpose, it can implement a callback having
  the following signature: virtual void onSubscribed(const std::string& subscriptionId).
  In case of failure the onError callback can be invoked with a SubscriptionException.
* **[C++]** The consumer is able to synchronize to subscription requests.
  The subscribeTo<BroadcastName> and subscribeTo<AttributeName> methods
  now return a Future that is resolved once the subscription request has been
  successfully delivered to the interface provider. The get() method of the
  Future returns the subscriptionId on successful execution or can throw
  a SubscriptionException in case of failure.
* **[Java]** Static capabilities provisioning can now be specified as a URI.
  See the [Java Configuration Guide](JavaSettings.md) for details.
* **[Java]** the domain access controller now has it's own property with which one can set its
  URI rather than it using the discovery directory URI. See the documentation to
  `DOMAINACCESSCONTROLLERURL` in the [Java Configuration Guide](JavaSettings.md) for details.
* **[Java]** when specifying the discovery directory or domain access controller URIs via
  configuration properties, it is now __not__ necessary to specify the participant IDs as well.
* **[JS]** Optional expiryDateMs (mills since epoch) can be passed to registerProvider. Default
  value is one day from now.
* **[JEE]** Added ability to specifiy message processors which can be used to, e.g., add custom
  headers to outgoing joynr messages. See the [JEE Documentation](jee.md) for details.
* **[Java]** the container classes for multi-out return values are now marked with an interface:
  `MultiReturnValuesContainer`.
* **[C++]** the QoS parameter has to be passed as std::shared_ptr to the `subscribeTo...` methods
* **[C++]** Joynr runtime object can be created with a settings object as well as with a path
  to a settings file.

## Other changes
* **[JEE]** a JEE version of the discovery service was added which can be deployed to EE
  containers like, e.g., Payara.
* **[JEE]** corrected configuration of Radio App JEE and System Integration Tests sit-jee-app
  to match the new capabilities provisioning and some other minor fixes.
* **[Java, JS, C++, JEE]** Ability to specify effort to be expent on ensuring delivery of
  messages. When set to `best effort` and using MQTT as transport, this results in a QoS 0
  MQTT message being sent (fire-and-forget). See `MessagingQosEffort` classes in each language.
* **[C++]** muesli is now used as serializer; it can be found at https://github.com/bmwcarit/muesli

# joynr 0.19.5
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Fix multi-threading issue in LocalCapabilitiesDirectory.

# joynr 0.19.4
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Correctly load persisted routing table in the LibJoynrRuntime.

# joynr 0.19.3
This is a minor bug fix release.

## API relevant changes
* **[C++]** Add new API to create joynr runtime with settings object.

## Other changes
* **[JS]** Support attributes starting with capital letters.

# joynr 0.19.2
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Do not crash joynr runtime if writing persistency files fails.

# joynr 0.19.1
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Fix issue in the generated JoynrTargets-release.cmake in relation with boost::thread

# joynr 0.19.0

## API relevant changes
* **[Java]** Added ability to pass a callback to the proxyBuilder.build() method to be notified on
  completion (or failure) of the discovery process.

## Other changes
* **[C++, Java, JS]** Enriched the system integration tests to have test from c++/node apps towards
  java jee apps
* **[C++]** Removed option `USE_PLATFORM_DEPENDENCIES` from CMake. By default all dependencies are
  resolved from system installation paths. However, joynr offers options
  (`USE_PLATFORM_<DEPENDENCY>=OFF`) to turn system resolution off. In this case, joynr downloads
  and builds individual dependencies during the joynr build using CMake's ExternalProject_Add
  mechanism.
* **[JS]** The unit-, integration-, system-integration- and intertab-tests are now using the
  [Jasmine](http://jasmine.github.io) 2.4.1 test framework.
  [Karma](https://karma-runner.github.io) is now used as test runner.
* **[Java]** The way in which the global capabilities and domain access control directories are
  provisioned has changed. See `StaticCapabilitiesProvisioning` as well as its entry in the
  [Java Settings documentation](JavaSettings.md) for details.
* **[JEE]** You can now inject the calling principal in providers in order to see who performed
  the call currently being executed.
* **[JEE]** Support for HiveMQ shared subscriptions, which enables clustering using only
  MQTT for communication.

# joynr 0.18.5
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[JEE]** Fixed bug with multi-out return values not being translated
  between container classes and multi-valued deferred instances in the
  `ProviderWrapper`.

# joynr 0.18.4
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Fixed high cpu load which occurs when the system time is changed
* **[C++]** Fixed persistency of local capability entries
* **[C++]** Stability fixes for proxy arbitration
* **[JS]** Added reconnect after connection loss for websockets
* **[JS]** Support to clear local storage when loading joynr library

# joynr 0.18.2
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[JS]** Fixed bug when using joynr with node version >= 6

# joynr 0.18.1
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[JS]** Include README in joynr node package

# joynr 0.18.0

## API relevant changes
* **[C++, Java, JS]** The communication protocol between local directories on the cluster controller
  and global directories in the backend changed. Please make sure that clients and backend use
  compatible versions.
* **[C++, Java, JS]** Support for fire and forget methods. Methods modelled with
  the franca keyword "fireAndForget" are now supported in the intended way, i.e. no
  reply is expected and allowed for the calling provider.
* **[Java]** Support for multi-addressed proxies. This way, a single proxy can communicate with
  multiple providers at the same time. The consumer can share a number of domains with the proxy
  builder, and depending on the arbitration strategy, multiple providers are connected with the
  returning proxy. In this case, the communication with the proxy is limited to fire and forget
  methods and subscriptions (attributes and broadcasts).
* **[JEE]** MQTT is now used for incoming and outgoing messages by default. The HTTP Bridge
  functionality is still available, but must be explicitely activated by setting the
  `joynr.jeeintegration.enable.httpbridge` property to `true`.
  See
  `io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys.JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY`
  for details.

## Other changes
* **[Tools]** Refactored joynr generator framework to simplify the maintenance,
   revised its required dependencies.

# joynr 0.17.2
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[JS]** Updated dependency for atmoshpere.js to version 2.3.2. This ensures that
  joynr has no native dependencies in its npm package.

# joynr 0.17.1
This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* Updated disclaimers, added README for npm

# joynr 0.17.0

## API relevant changes
* **[JEE]** Backend JEE applications are now supported natively with new joynr annotations
  @ServiceProvider and @ServiceLocator, allowing applications to focus solely on business logic.
  See [the JEE documentation](JEE.md) for more information.
* **[C++, Java, JS]** Added suffix "Ms" to timing related discoveryQos parameters:
  _discoveryTimeoutMs_, _cacheMaxAgeMs_, and _retryIntervalMs_. The original getters and setters
  are now deprecated and will be removed by the end of 2016.
* **[C++, Java, JS]** Provider and proxy interfaces as well as generated types (structs, enums and
  maps) contain version constants (`MAJOR_VERSION` and `MINOR_VERSION`) that reflect the version set
  in the Franca interface or type collection. Setters for provider version have been removed
  from the API of the ProviderQos.
* **[Java]** Restructured the class hierarchy of the generated providers. The application provider
  now implements an interface free of joynr-internal details. <Interface>AbstractProvider has been
  kept to maintain backwards compatibility, but implementations derived directly from
  <Interace>Provider must change to the new API. Please have a look at the class diagram
  in docs/diagrams for further details about the restructured class hierarchy.
* **[C++, Java, JS]** The communication protocol between local directories on the cluster controller
  and global directories in the backend changed. Please make sure that clients and backend use
  the same versions.
* **[Java]** Renamed setting _joynr.messaging.capabilitiesdirectoryurl_ to
  _joynr.messaging.discoverydirectoryurl_. The older setting will continue to work until the end of
  2016.
* **[JS, C++, Java]** The provider version can no longer be set programmatically in ProviderQos.
  Instead the value as modeled in Franca is generated into the provider interface.
* **[C++, Java, JS]** Support for empty broadcast. Broadcast with no output parameter is now
  supported in all three languages.

## Other changes
* **[C++]** The content of the message router and the local capabilities directory is now persisted
  by default and automatically loaded at cluster-controller startup. Entries are being saved (in
  JSON format) respectively to _MessageRouter.persist_ and to _LocalCapabilitiesDirectory.persist_.
* **[C++, Java, JS]** The backend service ChannelUrlDirectory has been eliminated. Addressing is
  now saved in the Discovery Directory.
* **[JS]** Small fixes in the jsdoc of generated proxies and providers.

# joynr 0.16.0

## API relevant changes
* **[JS, C++, Java]** Unified subscription QoS API among all programming languages.
 * Add suffix "Ms" to timing related subscription QoS parameters such as
   _expiryDateMs_, _publicationTtlMs_, _periodMs_, _alertAfterIntervalMs_, _minIntervalMs_ and
   _maxIntervalMs_. Getters, setters and constants are renamed accordingly.
 * Subscription QoS allows to specify the validity (relative from current time) instead of
   an absolute expiry date. The clearExpiryDate() function removes a previously set expiry date.
 * The clearAlertAfterInterval function removes a previously set alert after interval.
 * Add suffix "_MS" to timing related subscription QoS constants (default, min and max values).
 * Add missing default values and min/max limits for the QoS parameters.
 * The old interface is deprecated but still available for backward compatibility reasons and might
   be removed by end of 2016.
* **[C++, Java]** Provider QoS are passed in at provider registration on the joynr runtime. Storing
  the provider QoS in the provider object itself is deprecated and will be removed by the end of
  2016.
* **[JS]** "joynr.capabilities.registerCapabilitiy" is deprecated. Use
  "joynr.registration.registerProvider" instead. "registerCapability" is deprecated and will be
  removed by the end of 2016.
* **[JS]** registerProvider does not take an auth token. When renaming registerCapability to
  registerProvider, make sure also to delete the authToken parameter.
* **[C++, Java, JS]** The maximum messaging TTL is now configurable via messaging settings and
  enforced. The default value is set to 30 days.
 * C++: default-messaging.settings

   ```
   [messaging]
   # The maximum allowed TTL value for joynr messages.
   # 2592000000 = 30 days in milliseconds
   max-ttl-ms=2592000000
   ```
 * Java: defaultMessaging.properties

   ```
   joynr.messaging.maxTtlMs=2592000000
   ```
 * JS: defaultMessagingSettings.js

   ```
   // 30 days
   MAX_MESSAGING_TTL_MS : 2592000000
   ```
* **[C++]** libjoynr uses websocketpp (https://github.com/zaphoyd/websocketpp) to communicate with
  the cluster-controller.
* **[C++]** Use `CMAKE_CXX_STANDARD` to specify the C++ standard. This feature was introduced by
  CMake 3.1. See [\<RADIO_HOME\>/CMakeLists.txt](/examples/radio-app/CMakeLists.txt) on how to use
  it.

## Other changes
* **[C++, Java]** Fix bug in code generation for typedef.
* **[C++]** CMake integration of the joynr generator now available. See
  [\<RADIO_HOME\>/CMakeLists.txt](/examples/radio-app/CMakeLists.txt) on how to use it.

# joynr 0.15.1

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Fix segmentation fault in cluster-controller when a libjoynr disconnects.
* **[C++]** Define proper import targets for Mosquitto in the joynr package configuration.
* **[Java]** Use correct MQTT topics to fix incompatibilities with joynr C++.
* **[Java]** Improved stability in websocket implementation.

# joynr 0.15.0

## Notes
* **[Java,C++]** Java and C++ cluster controllers are now able to communciate to an MQTT broker as
  a replacement, or in addition to, the original bounceproxy. Java uses the Eclipse Paho client,
  while C++ uses mosquitto as an MQTT client.
* **[C++]** There is a new build and runtime dependency for the clustercontroller to mosquitto 1.4.7
* **[Java]** Handling of different transport middlewares has been refactored to be much more
  extensible. Using Guice Multibinders, it is now possible for external projects to add transport
  middleware implementations and inject these into the runtime. See the ```
joynr-mqtt-client``` project for an example of how this can be done.
* **[C++]** libjoynr uses libwebsockets of the libwebsockets project (http://libwebsockets.org)
  to communicate with the cluster-controller. Due to an incompatibility with Mac OS X,
  the C++-Websocket-Runtime currently does not work on Mac OS X.

## API relevant changes
* **[C++]** Removed the RequestStatus object returned by joynr::Future::getStatus().
  Instead, an enum named "StatusCode::Enum" is returned.
* **[C++]** joynr code now requires C++14

## Other changes
* **[JS]** Updated the versions of joynr dependencies log4js (0.6.29), requirejs (2.1.22),
  bluebird (3.1.1) and promise (7.1.1). No API impact.
* **[JS]** The several joynr runtimes (e.g. WebSocketLibjoynrRuntime or InProcessRuntime)
  now bring their own default values for joynr internal settings. Thus, joynr
  applications no longer need to provide this information via the provisioning
  object when loading the library.

# joynr 0.14.3

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Removed absolute paths from export targets for the install tree.
* **[C++]** Fix segmentation fault in cluster-controller checkServerTime function.
* **[C++]** Added /etc/joynr to settings search path. This is a workaround for builds with
  incorrect CMAKE_INSTALL_PREFIX.

# joynr 0.14.2

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Fix dependency resolution in the CMake package config file for joynr.

# joynr 0.14.1

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[JS]** Fixed bug in generated proxies with broadcast subscription requests
  having no filters.

# joynr 0.14.0

## Notes
* **[Java,JS,C++]** Franca `ByteBuffer` is supported.
* **[Java,JS,C++]** Franca `typedef` is supported. For Java and JS, typedefs
  are ignored and the target datatypes are used instead.
* **[C++]** libjoynr does not depend on Qt anymore.
* **[C++]** libjoynr uses libwebsockets of the libwebsockets project (http://libwebsockets.org)
  to communicate with the cluster-controller. Due to an incompatibility with Mac OS X,
  the C++-Websocket-Runtime currently does not work on Mac OS X.

## API relevant changes
* **[C++]** The minimum required version of `gcc` is 4.9.
* **[C++]** The CMake variables when linking against libjoynr have been renamed :
  * `Joynr_LIB_COMMON_*` contains only generic stuff needed to build generated code.
  * `Joynr_LIB_INPROCESS_*` contains stuff needed to build in-process including cluster controller.
* **[C++]** The `onError` callback for async method calls is changed:
  * The error callback has been renamed to `onRuntimeError`.
    Its signature expects a `JoynrRuntimeException`.
  * If the method has an error modeled in Franca, a separate `onApplicationError` callback is
     generated. The signature of this callback expects the generated error `enum` .
* **[Java]** Modify async proxy API for error callbacks. If an error enum is defined
  for methods in Franca, onFailure callback is split into two methods, one for
  modeled Franca errors (called ApplicationExceptison) and one for joynr runtime
  exceptions.

## Other changes
* **[C++]** The logging syntax is changed to the following format:
  `JOYNR_LOG_DEBUG(logger, "this {}: {}", "is", "a message");`
* **[C++]** Fixed bug in filters for broadcast having arrays as output parameters.
* **[JS]** Set version for node dependency module "ws" to 1.0.1.

# joynr 0.13.0

## Notes
* **[Java]** Uint types are not supported in Java: Unsigned values are thus read as
  signed values, meaning for example that 255 is represented as -1 in a Java Byte. The
  Java application is responsible for converting from signed to unsigned values as
  required. Note that this is only an issue if values exceed the largest possible
  values that can be represented by the signed Java values.
* **[C++]** Removing QT dependencies from libjoynr stack is almost done. Final cleanup
  is performed in upcoming releases.
* **[Java,JS,C++]** The JSON serializer in all three languages escapes already escaped
  quotas in strings incorrectly.
* **[Java, Android]** The Android runtime now contains all necessary transitive dependencies in an
  uber jar. The total size has been reduced so that a minimal app with joynr capability is
  now ca. 2.5 MB large, and multi-dexing is no longer necessary.
* **[Java]** The stand-alone cluster controller in Java is in Beta, and is not yet stable.
  Reconnects from clients are not being handled correctly. It is configured statically to
  disallow backend communication, so all discovery / registration requests must be set to
  LOCAL_ONLY / LOCAL.

## API relevant changes
* **[JS]** Async loading of libjoynr (libjoynr.load()) returns a Promise object instead
  expecting a callback function as input parameter. See the
  [JavaScript Tutorial](JavaScriptTutorial.md) for more details.
* **[Java,JS,C++]** Support Franca type Map
* **[JS]** Support Franca type Bytebuffer
* **[C++]** ApplicationException.getError<T>() now expects a template parameter T
  to get access to the real enum value
* **[Java]** It is no longer necessary to cast error enums retrieved from modelled
  application exceptions.

## Other changes
* **[Android]** The Android runtime has been modified to use an external cluster
  controller using WebSockets, and no longer can communicate itself via HTTP.
* **[Java, Android]** The following configuration properties must now be set when configuring
  the joynr runtime:
  * WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST
  * WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT

  Optionally the following can also be set:

  * WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL
  * WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH
* **[Java]** Clear separation between libjoynr and cluster controller functionality.
  Java applications do not need to be deployed with their own cluster controller anymore,
  but can instead communicate with one provided by the environment.
* **[Java]** Libjoynr client is now able to communicate with a cluster controller
  via Websocket communication.
* **[Java]** Cluster controller supports Websocket communication
* **[C++]** Replaced QJson-based serializer with a custom implementation, thus increasing
  speed ca 3x.
* **[C++]** Replace Qt functionality and data types (QThreadPool,
  QSemaphore, QMutex, QThread, QHash, QSet, QMap, QList, ...) by custom or std
  implementations.

# joynr 0.12.3

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Selective broadcasts of basic types generate compilable code.

# joynr 0.12.2

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Generated enum throws exception in the getLiteral method in case of an
  unresolved value.

# joynr 0.12.1

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Fixed bug during deserialization of joynr messages caused by
  incorrect meta type registration of nested structs.

# joynr 0.12.0

## Notes
* **[Java]** Uint types are not supported in Java: Unsigned values are thus read as
  signed values, meaning for example that 255 is represented as -1 in a Java Byte. The
  Java application is responsible for converting from signed to unsigned values as
  required. Note that this is only an issue if values exceed the largest possible
  values that can be represented by the signed Java values.
* **[Java]** The previously mentioned issue with handling of "number" types and enums in Lists
  has now been repaired.

## API relevant changes
* **[Java]** Java datatype java.util.List has been replaced with Array in the joynr API.
* **[Java]** The onError callback of subscriptions now passes a JoynrRuntimeException as
  input parameter instead of a JoynrException, as application-level exceptions cannot be defined
  for subcription errors.
* **[Java]** The method "getReply" of Future object was renamed to "get".
* **[Java]** The Java Short datatype has been introduced for Franca types UInt16 and Int16, as is
  Java Float now used for the Franca type Float.
* **[C++]** Support of exceptions for methods/attributes. Exceptions at provider side are now
  communicated via joynr to the consumer, informing it about unexpected application-level and
  communication behavior. joynr providers are able to reject method calls by using error enum values
  as modelled in the Franca model.
* **[JS]** Method input/output parameters and broadcast parameters are now consistently
  passed as key-value pairs.
* **[Java,JS,C++]** Harmonized the handling of minimum interval for subscriptions with
  OnChangeSubscriptionQos. Set the MIN value to 0 ms.
* **[Java,JS,C++]** Harmonized the handling of subscription qos parameters for broadcast
  subscriptions. If two subsequent broadcasts occur within the minimum interval, the
  latter broadcast will not be sent to the subscribing entity.

## Other changes
* **[C++]** Fixed bug causing a consumer to crash when subscribing to attributes of type
  enumeration
* **[JS]** Support of methods with multiple output parameters
* **[Java,C++]** Fixed bug with arrays as return parameter types of methods and
  broadcasts and as attribute types of subscriptions
* **[Tooling]** The joynr generator ignores invalid Franca models, and outputs a list of errors to
  the console.

# joynr 0.11.1

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[JS]** Minimum minInterval for subscriptions is 0ms
* **[JS]** The PublicationManager checks if the delay
  between two subsequent broadcasts is below the minInterval of the
  subscription. If yes, the broadcast is not communicated to the
  subscribing entity.
* **[JS]** Allow to load generated datatypes prior to invoking joynr.load
  in the node environment
* **[JS]** Smaller bug fixes in PublicationManager

# joynr 0.11.0

## Notes
* **[Java]** Uint types are not supported in Java: Unsigned values are thus read as
  signed values, meaning for example that 255 is represented as -1 in a Java Byte. The
  Java application is responsible for converting from signed to unsigned values as
  required. Note that this is only an issue if values exceed the largest possible
  values that can be represented by the signed java values.

## Known issues
* **[Java]** Handling of "number" types and enums in Lists is not implemented
  correctly. Accessing these values individually can result in ClassCastExceptions
  being thrown.
* **[Java]** uint16 and int16 declarations in Franca are currently being represented
  as Integer in Java.Though this is not associated with any functional problem, in
  the future int16 types will be generated to Short.
* **[C++]** Missing support of exceptions for methods/attributes. While the
  exception handling is already implemented for Java + JS, required extensions for C++
  are currently under development and planned for the upcoming major release
  0.12.0 mid November 2015.

## API relevant changes
* **[Java]** The onError callback of subscriptions expects now a JoynrException as input parameter
  instead of an empty parameter list. In addition, exceptions received from subscription publication
  are now forwarded to the onError callback.
* **[Java,JS]** Support of exceptions for methods/attributes. Exceptions at provider side are now
  communicated via joynr to the consumer, informing him about unexpected behavior. joynr providers
  are able to reject method calls by using error enum values as associated with the method in the
  Franca model.
* **[JS]** The callback provided at broadcast subscription is now called with key value pairs for
  the broadcast parameters. Previously, the callback has been invoked with individual function
  arguments for each broadcast parameter.
* **]Java,JS,C++]** Harmonized the handling of expiry dates in SubscriptionQos

## Other changes
* **[C++]** Replaced QSharedPointer with std::shared_ptr
* **[C++]** Replaced QDatetime with std counterpart "chrono"
* **[C++]** Replaced log4qt with spdlog
* **[C++]** Fixed bug which prevented the onError callback of async method calls to be called in
  case of unexpected behavior (e.g. timeouts)
* **[Java,JS,C++]** Fixed bug which caused joynr message loss due to wrong time interpreation in
  case of very high expiry dates.
* **[Java,JS]** Enriched the radio example with exception handling

# joynr 0.10.2

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[JS]** Reworked the handling of enums defined in Franca models.
  This resolves issues when using enums as input/output parameter of
  methods in JavaScript.

# joynr 0.10.1

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[Java]** Correct exception handling when messages are not routable
* **[JS]** Integrate JavaScript markdown in general documentation
* **[JS]** Fix bug in documentation regarding the Maven group ID of the joynr
  generators

# joynr 0.10.0

joynr JavaScript is now also officially open source. JavaScript can be run in Chrome or node.js.
Have a look in the [JavaScript Tutorial](JavaScriptTutorial.js) to get started with joynr
JavaScript, and try out the radio app examples to see it all in action.

## Known issues
* **[Java]** Handling of “number” types and enums in Lists is not implemented correctly. Accessing
  these values individually can result in ClassCastExceptions being thrown.
* **[Java]** Uint types not handled correctly: Unsigned values from C++ are read as signed values
  in Java. Workaround: the Java application must convert from signed to unsigned values itself.
  Note that this is only an issue if values exceed the largest possible values that can be
  represented by the signed java values.

## API relevant changes
* **[Java, C++, JS]** In order to fix compatibility in all supported languages with types using
  type collections, the generators now use the spelling of Franca element names as-is for packages,
  type collections, interfaces, etc., meaning that they no longer perform upper/lower case
  conversions on Franca element names. Models that contain elements with identical spelling other
  than case may cause unexpected behavior depending on which operating system is used. Files in
  Windows will be overwritten, for example, while files in Linux will co-exist.
* **[Java, C++, JS]** Franca's error enums are currently supported in Java, but not yet complete in
  JavaScript or C++. We recommend not using FIDLs with Error Enums until 0.11 is released.

## Other changes
* **[Java]** Logging can now be focused on message flow. Set log4j.rootLogger=error and then use a
  single logger to view messages: log4j.logger.io.joynr.messaging.routing.MessageRouterImpl=info
  shows only the flow, =debug shows the body as well.
* **[C++]** Now using Qt 5.5
* **[JS]** Fix radio example made for node, to be compatible with the radio example
  in C++, Java and the browser-based JavaScript application.
* **[Tooling]** Minor fixes in build scripts.
* **[Tooling]** Move java-generator, cpp-generator and js-generator into the tools folder.
  All generator modules have the Maven group ID "io.joynr.tools.generator".
* **[Tooling]** The joynr-generator-standalone supports JavaScript code generation
  language.
* **[Tooling, JS]** The joynr JavaScript build is part of the profile "javascript" of the
  root joynr Maven POM.

# joynr 0.9.4

This is a minor bug fix release.

## API relevant changes
* **[Java, C++, JS]** Use spelling of Franca element names (packages, type collections,
  interfaces, ...) as defined in the model (.fidl files) in generated code. I.e. perform
  no upper/lower case conversions on Franca element names.

## Other changes
* **[C++]** Param datatypes in a joynr request message includes type collection names
* **[JS]** Fix radio example made for node, to be compatible with the radio example
  in C++, Java and the browser-based JavaScript application.
* **[Tooling]** Minor fixes in build scripts.
* **[Tooling]** Move java-generator, cpp-generator and js-generator into the tools folder.
  All generator modules have the Maven group ID "io.joynr.tools.generator".
* **[Tooling]** The joynr-generator-standalone supports JavaScript code generation
  language.
* **[Tooling, JS]** The joynr JavaScript build is part of the profile "javascript" of the
  root joynr Maven POM.

# joynr 0.9.3

This is a minor bug fix release. It includes a preview version of the **joynr JavaScript** language
binding. Have a look in the [JavaScript Tutorial](JavaScriptTutorial.js) to get started with joynr
JavaScript.

## API relevant changes
* **[Java, C++, JS]** Using American English in radio.fidl (renaming favourite into favorite).

## Other changes
None.

# joynr 0.9.2

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[C++]** Problems with receiving messages in libjoynr via WebSockets have been resolved.
* **[Java, C++]** Default domain for backend services is now "io.joynr".

# joynr 0.9.1

This is a minor bug fix release.

## API relevant changes
None.

## Other changes
* **[Android]** callback onProxyCreationError is now called correctly when an error occurs creating
  a proxy. onProxyCreation is no longer called with null.
* **[Java]** problems with multiple calls to register and deregister the same provider have been
  resolved.
* logging settings in the examples have been reduced to focus on the sent and received messages.

# joynr 0.9.0

## API relevant changes
* **[Java, C++]** The provider class hierarchy has been simplified. A class diagram is at
  docs/diagram/ClassDiagram-JavaProvider.png. To implement a provider from scratch, extend
  <Interface>AbstractProvider. To implement a provider based on the default implementation extend
  Default<Interface>Provider.
* **[C++]** Qt-related datatypes have been removed from the API, both in generated classes and in
  runtime classes used for proxy creation, provider registration etc. Std types are now used
  instead.
* **[C++]** Future no longer accepts a callback as well; in order to synchronously retrieve values
  from the future, call Future::getValues.
* **[C++]** getProxyBuilder() has been renamed to createProxyBuilder()
* **[C++]** ProxyBuilder::RuntimeQos has been renamed to MessagingQos (as in Java)
* **[C++]** setProxyQos() has been removed from the ProxyBuilder. Messaging timeouts are set using
  the MessagingQos, while qos attributes related to discovery are set in setDiscoveryQos()
* **[C++]** The async API of proxies for method calls and attribute setters/getters allows
  to provide onSuccess and onError callback functions. OnSuccess is invoked by the joynr runtime
  in case of a successful call, onError in all other cases (e.g. joynr internal errors like
  timeouts).
* **[C++]** The sync API of proxies for method calls and attribute setters/getters now always
  provides a RequestStatus object as return value. This object informs the caller upon successful or
  erroneous execution of the respective call.
* **[Java]** Access control has been activated, meaning that all Java-based providers will not be
  accessible unless the request message passes an access control check. As development of access
  control is ongoing (there is not yet official support for entering access control information in
  the global access control directory), currently providers can be made accessible by using a
  statically-injected access control property. The MyRadioProviderApplication class in
  examples/radio-app provides an example of how this can be done.
* **[Java, C++]** registerCapability has been renamed to registerProvider and no longer takes an
  "auth token", which was a placeholder that is no longer needed.
* **[Java, C++]** Providers may now only be implemented using the asynchronous interface. The
  sychronous provider API has been removed. Providers return by calling onSuccess callback function.
* **[Java, C++]** Franca's multiple output parameters are now supported.
* **[Build]** Added Dockerfiles for building Java and C++ builds, with included scripts. These
  scripts are also used by the joynr project itself in its own CI (Jenkins-based) environment.
* **[Java]** Capability Directory entries on the global directory are now persisted using JPA.

# joynr 0.8.0

## API relevant changes
* **[Java, C++]** Support of broadcast: it is now possible to subscribe to broadcasts on proxy side.
  Providers are able to fire broadcast events, which are then forwarded to subscribed proxies. See
  the [Broadcast Tutorial](Broadcast-Tutorial.md) for more information.
* **[Java, C++]** Support to stop/update an existing subscription: the creation of a new
  subscription returns a unique subscription ID. Supplying this id to the proxy API allows to stop
  or update an existing subscription.
* **[Java, C++]** Generate proxy API according to modifier flags in Franca model: only generate
  setters/getters/subscribeTo methods on proxy side, if respective flags are defined in the Franca
  model (e.g. readOnly implies no setters)
* **[Java, C++]** Names defined in Franca are taken 1:1 into code: the joynr generator framework
  reuses the upper and lower case as defined in the Franca model where possible
* **[Java]** Add copy constructor to complex types of Franca model: for each complex data structure
  in the Franca model, a copy constructor is created in the respective Java class
* **[Java, C++]** Rename subscription listener methods
  * onReceive: Gets called on every received publication
  * onError: Gets called on every error that is detected on the subscription

## Other changes
* **[Tooling]** Enable cleanup capability of joynr generator framework: it is now possible to
  trigger the joynr generator with the "clean" goal, meaning that previously generated files are
  deleted
* **[Tooling]** Create standalone joynr generator: joynr provides now a standalone joynr generator,
  which can be used independent of maven as build environment
* **[Tooling]** The joynr generator framework migrates to xtend 2.7.2
* **[Tooling]** Update Java version from 1.6 to 1.7
* **[Java, C++]** Added ability to radio-app example to apply geocast broadcast filters: the example
  shows how broadcasts can be used to implement a geocast
* **[C++]** Update to CommonAPI version 2.1.4
* **[C++]** C++ cluster controller offers WebSocket messaging interface: the C++ cluster controller
  provides now a WebSocket API to be accessed by joynr applications. The C++ libjoynr version
  supports WebSocket communication with the cluster controller
* **[C++]** Implement message queue: in case the destination address of the joynr message cannot be
  resolved, the message router is now able to queue messages for later delivery
* **[Android]**	Now supporting platform version 19.
* **[Android]**	AsyncTask from the Android SDK went from being executed in parallel in API 10, to
  sequential handling in later Android versions. Since there is no clean way to support the old
  and new semantics without wrapping the class, we are now bumping up support API 19. Prior versions
  are no longer supported.

# joynr 0.7.0
## API relevant changes
* **[Java]** SubscriptionListener is now called AttributeSubscriptionListener, and
  unregisterSubscription renamed unregisterAttributeSubcription (change required to differentiate
  from broadcasts)
* **[Java]** The hostPath property can now be set as joynr.servlet.hostPath.
* **[Java, C++]** SSL support for C++ and Java

## Other changes
* **[C++]** libjoynr and cluster-controller now communicate over a single DBus interface
* **[C++]** introduce MessageRouter on libjoynr and cluster-controller side to resolve next hop for
  messages
* **[C++]** remove EndpointAddress term and use simply Address
* **[Java]** use URL rewriting to implement load balancing on bounce proxy cluster
* **[Java]** enable bounce proxy controller to run in clustered mode
* **[Java]** refactor bounce proxy modules:
  * use Guice injection to configure servlets
  * use RESTful service adapters for messaging related components

# joynr 0.6.0

## API relevant changes
* **[Java]** exceptions: removed checked exceptions from ProxyBuilder
* **[Java]** Check for correct usage of SubscriptionQos
* **[Java]** ChannelUrlDirectoryImpl correctly implements unregisterChannelUrl
* **[Java]** Changes to GlobalCapabilitiesDirectory API causes this version to be incompatible with
  older server installations.
* **[C++]** joynr is now compatible with Qt 5.2
* **[C++]** read default messaging settings from file
  Default messaging settings are now read from file "resources/default-messaging.settings". When
  using the find_package command to resolve joynr, it will set the JOYNR_RESOURCES_DIR variable.
  You can copy the default resources to your bin dir using cmake's file command:

  ```bash
  file(
      COPY ${JOYNR_RESOURCES_DIR}
      DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  )
  ```

## Other changes
* **[C++]** cleaning up joynr libraries used in cmake find_package
* **[Java]** refactored messaging project structure for scalability-related components
* **[Java]** definition of RESTful service adapters for messaging related components
* **[Java]** implementation of lifecycle management and performance monitoring for controlled bounce
  proxy
* **[Java]** scalability extensions for channel setup at bounce proxy
* **[Java]** scalability extensions for messaging in non-exceptional situations
* **[Java]** backend: improved shutdown responsiveness
* **[Java]** discovery directory servlet: mvn commands to start for local testing
* **[Java]** logging: preparations to allow logging to logstash (distributed logging)
* **[Java]** binary archives (WAR format) of backend components are available on Maven Central
* **[Java]** Enable backend module "MessagingService" to work with joynr messages of unknown content
  type
* **[Java]** Joynr provides rudimentary embedded database capabilities
* **[Tooling]** Augment features of joynr C++ code generator
* **[Tooling]** Write common util for all generation templates to resolve names of methods, types,
  interaces, arguments, …
