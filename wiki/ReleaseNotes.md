#joynr 0.16.0

##API relevant changes
* **[JS]** Naming change for subscription qos parameters. Added suffix "Ms" to all
  timing related settings expected in the qos constructor. For backward compatibiliy reasons,
  the old way of specifying parameters is still possible, but will be removed by 01/01/2017.
* **[JS]** API change of subscription qos. It is now possible to specify the validity (relative
  from current time) instead of an absolute expiry date. In addition, added the function
  subscriptionQos.clearExpiryDate() and subscriptionQos.clearAlertAfterInterval() to
  invalidate a previous specified expiry date or the alert after interval. Added
  missing default values and lower/upper limits for some of the qos parameters.

##Other changes
None.

#joynr 0.15.1

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[C++]** Fix segmentation fault in cluster-controller when a libjoynr disconnects.
* **[C++]** Define proper import targets for Mosquitto in the joynr package configuration.
* **[Java]** Use correct MQTT topics to fix incompatibilities with joynr C++.
* **[Java]** Improved stability in websocket implementation.

#joynr 0.15.0

##Notes
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

##API relevant changes
* **[C++]** Removed the RequestStatus object returned by joynr::Future::getStatus().
  Instead, an enum named "StatusCode::Enum" is returned.
* **[C++]** joynr code now requires C++14

##Other changes
* **[JS]** Updated the versions of joynr dependencies log4js (0.6.29), requirejs (2.1.22),
  bluebird (3.1.1) and promise (7.1.1). No API impact.
* **[JS]** The several joynr runtimes (e.g. WebSocketLibjoynrRuntime or InProcessRuntime)
  now bring their own default values for joynr internal settings. Thus, joynr
  applications no longer need to provide this information via the provisioning
  object when loading the library.

#joynr 0.14.3

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[C++]** Removed absolute paths from export targets for the install tree.
* **[C++]** Fix segmentation fault in cluster-controller checkServerTime function.
* **[C++]** Added /etc/joynr to settings search path. This is a workaround for builds with
  incorrect CMAKE_INSTALL_PREFIX.

#joynr 0.14.2

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[C++]** Fix dependency resolution in the CMake package config file for joynr.

#joynr 0.14.1

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[JS]** Fixed bug in generated proxies with broadcast subscription requests
  having no filters.

#joynr 0.14.0

##Notes
* **[Java,JS,C++]** Franca `ByteBuffer` is supported.
* **[Java,JS,C++]** Franca `typedef` is supported. For Java and JS, typedefs
  are ignored and the target datatypes are used instead.
* **[C++]** libjoynr does not depend on Qt anymore.
* **[C++]** libjoynr uses libwebsockets of the libwebsockets project (http://libwebsockets.org)
  to communicate with the cluster-controller. Due to an incompatibility with Mac OS X,
  the C++-Websocket-Runtime currently does not work on Mac OS X.

##API relevant changes
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

##Other changes
* **[C++]** The logging syntax is changed to the following format:
  `JOYNR_LOG_DEBUG(logger, "this {}: {}", "is", "a message");`
* **[C++]** Fixed bug in filters for broadcast having arrays as output parameters.
* **[JS]** Set version for node dependency module "ws" to 1.0.1.

#joynr 0.13.0

##Notes
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

##API relevant changes
* **[JS]** Async loading of libjoynr (libjoynr.load()) returns a Promise object instead
  expecting a callback function as input parameter. See the
  [JavaScript Tutorial](JavaScriptTutorial.md) for more details.
* **[Java,JS,C++]** Support Franca type Map
* **[JS]** Support Franca type Bytebuffer
* **[C++]** ApplicationException.getError<T>() now expects a template parameter T
  to get access to the real enum value
* **[Java]** It is no longer necessary to cast error enums retrieved from modelled
  application exceptions.

##Other changes
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

#joynr 0.12.3

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[C++]** Selective broadcasts of basic types generate compilable code.

#joynr 0.12.2

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[C++]** Generated enum throws exception in the getLiteral method in case of an
  unresolved value.

#joynr 0.12.1

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[C++]** Fixed bug during deserialization of joynr messages caused by
  incorrect meta type registration of nested structs.

#joynr 0.12.0

##Notes
* **[Java]** Uint types are not supported in Java: Unsigned values are thus read as
  signed values, meaning for example that 255 is represented as -1 in a Java Byte. The
  Java application is responsible for converting from signed to unsigned values as
  required. Note that this is only an issue if values exceed the largest possible
  values that can be represented by the signed Java values.
* **[Java]** The previously mentioned issue with handling of "number" types and enums in Lists
  has now been repaired.

##API relevant changes
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

##Other changes
* **[C++]** Fixed bug causing a consumer to crash when subscribing to attributes of type
  enumeration
* **[JS]** Support of methods with multiple output parameters
* **[Java,C++]** Fixed bug with arrays as return parameter types of methods and
  broadcasts and as attribute types of subscriptions
* **[Tooling]** The joynr generator ignores invalid Franca models, and outputs a list of errors to
  the console.

#joynr 0.11.1

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[JS]** Minimum minInterval for subscriptions is 0ms
* **[JS]** The PublicationManager checks if the delay
  between two subsequent broadcasts is below the minInterval of the
  subscription. If yes, the broadcast is not communicated to the
  subscribing entity.
* **[JS]** Allow to load generated datatypes prior to invoking joynr.load
  in the node environment
* **[JS]** Smaller bug fixes in PublicationManager

#joynr 0.11.0

##Notes
* **[Java]** Uint types are not supported in Java: Unsigned values are thus read as
  signed values, meaning for example that 255 is represented as -1 in a Java Byte. The
  Java application is responsible for converting from signed to unsigned values as
  required. Note that this is only an issue if values exceed the largest possible
  values that can be represented by the signed java values.

##Known issues
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

##API relevant changes
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

##Other changes
* **[C++]** Replaced QSharedPointer with std::shared_ptr
* **[C++]** Replaced QDatetime with std counterpart "chrono"
* **[C++]** Replaced log4qt with spdlog
* **[C++]** Fixed bug which prevented the onError callback of async method calls to be called in
  case of unexpected behavior (e.g. timeouts)
* **[Java,JS,C++]** Fixed bug which caused joynr message loss due to wrong time interpreation in
  case of very high expiry dates.
* **[Java,JS]** Enriched the radio example with exception handling

#joynr 0.10.2

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[JS]** Reworked the handling of enums defined in Franca models.
  This resolves issues when using enums as input/output parameter of
  methods in JavaScript.

#joynr 0.10.1

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[Java]** Correct exception handling when messages are not routable
* **[JS]** Integrate JavaScript markdown in general documentation
* **[JS]** Fix bug in documentation regarding the Maven group ID of the joynr
  generators

#joynr 0.10.0

joynr JavaScript is now also officially open source. JavaScript can be run in Chrome or node.js.
Have a look in the [JavaScript Tutorial](JavaScriptTutorial.js) to get started with joynr
JavaScript, and try out the radio app examples to see it all in action.

##Known issues
* **[Java]** Handling of “number” types and enums in Lists is not implemented correctly. Accessing
  these values individually can result in ClassCastExceptions being thrown.
* **[Java]** Uint types not handled correctly: Unsigned values from C++ are read as signed values
  in Java. Workaround: the Java application must convert from signed to unsigned values itself.
  Note that this is only an issue if values exceed the largest possible values that can be
  represented by the signed java values.

##API relevant changes
* **[Java, C++, JS]** In order to fix compatibility in all supported languages with types using
  type collections, the generators now use the spelling of Franca element names as-is for packages,
  type collections, interfaces, etc., meaning that they no longer perform upper/lower case
  conversions on Franca element names. Models that contain elements with identical spelling other
  than case may cause unexpected behavior depending on which operating system is used. Files in
  Windows will be overwritten, for example, while files in Linux will co-exist.
* **[Java, C++, JS]** Franca's error enums are currently supported in Java, but not yet complete in
  JavaScript or C++. We recommend not using FIDLs with Error Enums until 0.11 is released.

##Other changes
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

#joynr 0.9.4

This is a minor bug fix release.

##API relevant changes
* **[Java, C++, JS]** Use spelling of Franca element names (packages, type collections,
  interfaces, ...) as defined in the model (.fidl files) in generated code. I.e. perform
  no upper/lower case conversions on Franca element names.

##Other changes
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

#joynr 0.9.3

This is a minor bug fix release. It includes a preview version of the **joynr JavaScript** language
binding. Have a look in the [JavaScript Tutorial](JavaScriptTutorial.js) to get started with joynr
JavaScript.

##API relevant changes
* **[Java, C++, JS]** Using American English in radio.fidl (renaming favourite into favorite).

##Other changes
None.

#joynr 0.9.2

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[C++]** Problems with receiving messages in libjoynr via WebSockets have been resolved.
* **[Java, C++]** Default domain for backend services is now "io.joynr".

#joynr 0.9.1

This is a minor bug fix release.

##API relevant changes
None.

##Other changes
* **[Android]** callback onProxyCreationError is now called correctly when an error occurs creating
  a proxy. onProxyCreation is no longer called with null.
* **[Java]** problems with multiple calls to register and deregister the same provider have been
  resolved.
* logging settings in the examples have been reduced to focus on the sent and received messages.

#joynr 0.9.0

##API relevant changes
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

#joynr 0.8.0

##API relevant changes
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

##Other changes
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

#joynr 0.7.0
##API relevant changes
* **[Java]** SubscriptionListener is now called AttributeSubscriptionListener, and
  unregisterSubscription renamed unregisterAttributeSubcription (change required to differentiate
  from broadcasts)
* **[Java]** The hostPath property can now be set as joynr.servlet.hostPath.
* **[Java, C++]** SSL support for C++ and Java

##Other changes
* **[C++]** libjoynr and cluster-controller now communicate over a single DBus interface
* **[C++]** introduce MessageRouter on libjoynr and cluster-controller side to resolve next hop for
  messages
* **[C++]** remove EndpointAddress term and use simply Address
* **[Java]** use URL rewriting to implement load balancing on bounce proxy cluster
* **[Java]** enable bounce proxy controller to run in clustered mode
* **[Java]** refactor bounce proxy modules:
  * use Guice injection to configure servlets
  * use RESTful service adapters for messaging related components

#joynr 0.6.0

##API relevant changes
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

##Other changes
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
