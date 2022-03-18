# Release Notes
All relevant changes are documented in this file. You can find more information about
the versioning scheme [here](JoynrVersioning.md).

# joynr 1.19.9

## API-relevant Changes
* **[Generator]** Added new option `addVersionTo=comment` to the joynr code generator. It enables
  the evaluation of the `#noVersionGeneration` comment in Franca interface definition (.fidl)
  files.  
  Please make sure that all your Franca interfaces that shall be generated without version information
  have the #noVersionGeneration comment in their @description comment. The evaluation of the
  #noVersionGeneration comment will become the default soon. See the
  [Generator documentation](/wiki/generator.md) for more information about the generator versioning
  settings.  
  RECOMMENDATION: use `addVersionTo=comment` to make sure that everything works as expected.

## Other Changes
None.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** Fixed a bug in `LocalCapabilitiesDirectoryStore` that could lead to crash of the joynr
  cluster controller. An internal map was not properly protected from concurrent access.
* **[C++]** Persistence of participantIds (ParticipantIdStorage) is now assured by calling `fsync`
  on the file descriptor after writing.

# joynr 1.19.8

## API-relevant Changes
None.

## Other Changes
* **[TS]** Removed dependency to `lodash` from libjoynr-js.
* **[TS]** Updated `nanoid` dependency to version 3.3.1.
* **[TS]** Updated `mqtt` dependency to version 4.3.6.
* **[TS]** Updated `lodash` / `@types/lodash` dependencies (still used in `joynr-generator-npm` and
  tests) to versions 4.17.21 / 4.14.178.
* **[TS]** Updated `handlebars` dependency of `joynr-generator-npm` to version 4.7.7.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[Java,C++]** Fixed a bug where GuidedProxyBuilder failed to creata a proxy if interface minor
  number of provider was greater than interface minor number of proxy while major number was
  identical on both sides meaning the provider should have been considered to be compatible.
* **[Java]** Fixed a bug in 'GcdTaskSequencer` that could lead to a crash. No further global
  provider registration and provider unregistration is possible afterwards.
* **[Java]** Fixed a bug in `GcdTaskSequencer` that could lead to parallel task execution with
  unpredictable behaviour for global provider registration and unregistration.
* **[TS]** `joynr.load` now also accepts `UdsLibJoynrProvisioning` interface in addition to
  `WebSocketLibjoynrProvisioning` and `InProcessProvisioning` as provisioning parameter. See
  [JavaScript Configuration Reference](/wiki/JavaScriptSettings.md) for details about the different
  provisioning settings for the different runtimes (`UdsLibJoynrRuntime` and
  `WebSocketLibjoynrRuntime`).

# joynr 1.19.7

## API-relevant Changes
None.

## Other Changes
None.

## Configuration Property Changes
* **[Java]** The optional MqttModule property
  `PROPERTY_KEY_MQTT_DISABLE_HOSTNAME_VERIFICATION` has been added (default: false).
  See the [Java Configuration Reference](JavaSettings.md) for details about this property.

## Security Fixes
None.

## Bug Fixes
* **[Java]** Prevented repeated delivery of the same multicast publication:  
  In case of multiple subscriptions for the same multicast from the same runtime, the multicast was
  delivered multiple times to all subscribers in this runtime.
* **[Java]** Fixed unregistration for multicast subscriptions: unregistration was not implemented
  correctly which could cause error logs for non deliverable or expired messages as well as repeated
  delivery of the same multicast publication.

# joynr 1.19.6

## API-relevant Changes
* **[Java]** Removed superfluous class `StaticDomainAccessControlProvisioningModule`.

## Other Changes
* **[Docker]** Examples and tests now use the Docker image `hivemq/hivemq-ce:latest` instead of
  `hivemq/hivemq-ce:2021.2`.
* **[Generator, TS]** The generator now generates additional code to create a basic default provider
  implementation `<interface>ProviderImplementationCreator.ts` that can be used for testing.  
  NOTE: The generated code does not fully support interfaces with overloaded methods. It is not
  possible to define different return values for overloaded methods. All overloads of a method will
  return the same value. This is problematic if different return types are modelled in the Franca
  interface.
* **[C++]** Fixed some clang compiler warnings.
* **[Java]** Updated `org.eclipse.jetty.websocket` dependencies to version `11.0.7`.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
None.

# joynr 1.19.5

## API-relevant Changes
None.

## Other Changes
* **[C++]** Fixed the MQTT maximum message size check to correctly detect too large messages before
  trying to publish the message to the MQTT Broker. The whole MQTT packet is now taken into account
  instead of just the size of the payload.
* **[Java]** Updated Mockito dependency to `mockito-core` 4.2.0 and removed `mockito-all`.
* **[Java]** Updated Maven plugin `build-helper-maven-plugin` to version 3.2.0.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[Android]** Fixed log implementation to also log throwables if present as additional parameter.
* **[C++]** When ACL checks were enabled, it was possible that:
  * incoming request messages to the cluster controller caused unnecessary error logs
  * outgoing request messages were dropped if a provider discovery was required to perform the ACL
    check (this could happen only if a lot of providers had been discovered before and the required
    DiscoveryEntry had to be dropped from the cache because the size limit of 1000 entries had been
    exceeded)

  Discoveries for ACL checks now use a TTL of 60 seconds instead of invalid -1 seconds.

# joynr 1.19.4

## API-relevant Changes
None.

## Other Changes
None.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[Java]** Provider registration was not thread safe which could cause `NullPointerException` when
  multiple registrations were executed in parallel.
* **[C++]** Fixed a race condition that could cause unreachable providers when removal and
  registration of a provider overlapped in the cluster controller (the routing entry of the provider
  was erroneously removed by the callback of the asynchronous remove task in the cluster controller
  after the provider had been registered again).

# joynr 1.19.3

## API-relevant Changes
None.

## Other Changes
* **[C++]** Requests (joynr RPC calls) are now aborted with an exception if the corresponding
  request message is dropped because the queue limit is reached. A message is queued when the
  recipient or the required transport, e.g. MQTT, is currently not available. Joynr creates a
  reply message with an exception for each dropped request message.
* **[Java]** Updated org.jacoco:jacoco-maven-plugin to 0.8.7

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
None.

# joynr 1.19.2

## API-relevant Changes
None.

## Other Changes
* **[Java]** Updated net.sf.ehcache:ehcache to 2.10.9.2.
* **[Java]** Removed deprecated calls to `Method.isAccessible()`.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** Fixed memory leak in `Directory.h`.
* **[C++]** Fixed `CMakeLists.txt` of radio-app example.

# joynr 1.19.1

## API-relevant Changes
None.

## Other Changes
* **[Java]** In case of embedded clustercontroller runtime, reply messages
  now contain the same customheaders as were contained in the request message
  (i.e. derived from MQTT user properties and regular customHeaders for requests
  received via MQTT).In the Reply message the merged headers are provided as regular
  customHeaders which are then also additionally sent as MQTT user properties.

* **[Java]** Return empty GBID in results when lookup was called with empty GBID.
  In case of bad provisioning, the only known GBID is the empty GBID.
  On a successful lookup, if the received address contained a non-empty GBID,
  communication with the discovered provider would not be possible since the GBID
  is not known.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
None.

# joynr 1.19.0

## API-relevant Changes
None.

## Other Changes
None.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** Boost EPOLL deactivation does not work with Boost 1.72. Enabled again.
  Recommended to use Boost 1.72 or higher with joynr UDS runtime.
* **[Java]** Added missing Maven configuration to allow JavaDoc generation by executing
  `mvn javadoc:jar`.
* **[Java]** Fixed log statements in HivemqMqttClient and HivemqMqttClientFactory: exceptions are
  now logged correctly with stack trace.
* **[TS]** Remove undocumented dependency to '@types/nanoid'.

# joynr 1.18.3

## API-relevant Changes
None.

## Other Changes
* **[Docker]** Examples and tests now use the Docker image `hivemq/hivemq-ce:latest` instead of
  `hivemq/hivemq-ce:2021.2`.
* **[Java]** In case of embedded clustercontroller runtime, reply messages
  now contain the same custom headers as were contained in the request message
  (i.e. derived from MQTT user properties and regular customHeaders for requests
  received via MQTT).In the Reply message the merged headers are provided as regular
  customHeaders which are then also additionally sent as MQTT user properties.
* **[Java]** Return empty GBID in results when lookup was called with empty GBID.
  In case of bad provisioning, the only known GBID is the empty GBID.
  On a successful lookup, if the received address contained a non-empty GBID,
  communication with the discovered provider would not be possible since the GBID
  is not known.
* **[Java]** Updated net.sf.ehcache:ehcache to 2.10.9.2.
* **[Java]** Updated org.jacoco:jacoco-maven-plugin to 0.8.7
* **[Java]** Updated Mockito dependency to `mockito-core` 4.2.0 and removed `mockito-all`.
* **[Java]** Updated Maven plugin `build-helper-maven-plugin` to version 3.2.0.
* **[Java]** Updated `org.eclipse.jetty.websocket` dependencies to version `11.0.7`.
* **[C++]** Requests (joynr RPC calls) are now aborted with an exception if the corresponding
  request message is dropped because the queue limit is reached. A message is queued when the
  recipient or the required transport, e.g. MQTT, is currently not available. Joynr creates a
  reply message with an exception for each dropped request message.
* **[C++]** Fixed the MQTT maximum message size check to correctly detect too large messages before
  trying to publish the message to the MQTT Broker. The whole MQTT packet is now taken into account
  instead of just the size of the payload.
* **[C++]** Fixed some clang compiler warnings.

## Configuration Property Changes
* **[Java]** The optional MqttModule property
  `PROPERTY_KEY_MQTT_DISABLE_HOSTNAME_VERIFICATION` has been added (default: false).
  See the [Java Configuration Reference](JavaSettings.md) for details about this property.

## Security Fixes
None.

## Bug Fixes
* **[Android]** Fixed log implementation to also log throwables if present as additional parameter.
* **[Java]** Added missing Maven configuration to allow JavaDoc generation by executing
  `mvn javadoc:jar`.
* **[Java]** Fixed log statements in HivemqMqttClient and HivemqMqttClientFactory: exceptions are
  now logged correctly with stack trace.
* **[Java]** Provider registration was not thread safe which could cause `NullPointerException` when
  multiple registrations were executed in parallel.
* **[Java]** Prevented repeated delivery of the same multicast publication:  
  In case of multiple subscriptions for the same multicast from the same runtime, the multicast was
  delivered multiple times to all subscribers in this runtime.
* **[Java]** Fixed unregistration for multicast subscriptions: unregistration was not implemented
  correctly which could cause error logs for non deliverable or expired messages as well as repeated
  delivery of the same multicast publication.
* **[Java]** Fixed a bug in 'GcdTaskSequencer` that could lead to a crash. No further global
  provider registration and provider unregistration is possible afterwards.
* **[Java]** Fixed a bug in `GcdTaskSequencer` that could lead to parallel task execution with
  unpredictable behaviour for global provider registration and unregistration.
* **[Java,C++]** Fixed a bug where GuidedProxyBuilder failed to creata a proxy if interface minor
  number of provider was greater than interface minor number of proxy while major number was
  identical on both sides meaning the provider should have been considered to be compatible.
* **[C++]** Fixed memory leak in `Directory.h`.
* **[C++]** Fixed `CMakeLists.txt` of radio-app example.
* **[C++]** Fixed a race condition that could cause unreachable providers when removal and
  registration of a provider overlapped in the cluster controller (the routing entry of the provider
  was erroneously removed by the callback of the asynchronous remove task in the cluster controller
  after the provider had been registered again).
* **[TS]** Remove undocumented dependency to '@types/nanoid'.
* **[TS]** `joynr.load` now also accepts `UdsLibJoynrProvisioning` interface in addition to
  `WebSocketLibjoynrProvisioning` and `InProcessProvisioning` as provisioning parameter. See
  [JavaScript Configuration Reference](/wiki/JavaScriptSettings.md) for details about the different
  provisioning settings for the different runtimes (`UdsLibJoynrRuntime` and
  `WebSocketLibjoynrRuntime`).

# joynr 1.18.2

## API-relevant Changes
None.

## Other Changes
None.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** Boost EPOLL deactivation does not work with Boost 1.72. Enabled again.
Recommended to use Boost 1.72 or higher with joynr UDS runtime.
* **[C++]** When ACL checks were enabled, it was possible that:
  * incoming request messages to the cluster controller caused unnecessary error logs
  * outgoing request messages were dropped if a provider discovery was required to perform the ACL
    check (this could happen only if a lot of providers had been discovered before and the required
    DiscoveryEntry had to be dropped from the cache because the size limit of 1000 entries had been
    exceeded)

  Discoveries for ACL checks now use a TTL of 60 seconds instead of invalid -1 seconds.

# joynr 1.18.1

## API-relevant Changes
None.

## Other Changes
* **[Android]** Android build is enabled again in Maven profile `android`.
* **[C++]** LibJoynrUdsRuntime calls onFatalRuntimeError and stops processing if:
  * UDS server socket directory does not exist
  * UDS server socket exists but is not readable or not writable
* **[Java]** MQTT layer accepts `mqtts`, `ssl` and `tls` as protocol descriptors
  for TLS MQTT broker connections, and `mqtt` and `tcp` as protocol descriptors
  for non-TLS MQTT broker connections.
* **[Generator]** Removed warning when `--addVersionTo` is set because `--addVersionTo` setting has
  been temporarily reactived again in joynr 1.16.0 and overrules the `#noVersionGeneration` comment
  in fidl files.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** Fixed a potential deadlock when trying to route queued messages.
* **[Android]** Fixed a bug in libjoynr message routing that caused a message loop between libjoynr
  runtime and cluster controller.

# joynr 1.18.0

## API-relevant Changes
* **[Android]** Android support has been temporarily disabled until further notice.
* **[Java,JEE]** Joynr custom headers can now be added or modified by the MQTT broker
  via MQTT user properties, where values modified by the broker take precedence over values
  set by the consumer application. The resulting set of custom headers is available to
  provider applications as before.  
  See the [Java](/wiki/java.md) and [JEE](/wiki/jee.md) documentation for more details.  
  Note: Only custom headers with non empty key and non empty value will be transmitted via MQTT
  user properties.
* **[Java,JEE]** joynr has been migrated to support Java 11.
  Any later Java versions are not supported, even if it may appear to work.
  Java 11 is now required, except for the generator part which remains to be
  compatible to both Java 8 and Java 11 (any other Java version is not supported).
* **[Java,JEE]** The runtime environment requires now use of Payara 5, Payara 4 is no longer
  supported. The dependency to javaee-api 7 has been upgraded to jakartaee-api 8.
* **[Java,JEE]** Several dependencies have been upgraded. Please make sure to
  upgrade any applications / JEE server accordingly to avoid possible
  version conflicts.
  `mvn dependency:tree` from top-level pom.xml reports the following changes:
  Removed or updated old dependencies (old versions derived from joynr 1.17.8):
  * com.google.guava:guava:jar:18.0
  * commons-cli:commons-cli:jar:1.3
  * commons-io:commons-io:jar:2.2
  * commons-lang:commons-lang:jar:2.6
  * com.sun.mail:javax.mail:jar:1.5.0
  * fish.payara.extras:payara-embedded-all:jar:4.1.2.181
  * javax.activation:activation:jar:1.1
  * javax:javaee-api:jar:7.0
  * net.bytebuddy:byte-buddy:jar:1.10.7
  * org.apache.derby:jar:10.10.1.1
  * org.apache.maven:maven-aether-provider:jar:3.2.5
  * org.apache.maven:maven-artifact:jar:2.2.1
  * org.apache.maven:maven-model-builder:jar:3.2.5
  * org.apache.maven:maven-model:jar:2.2.1
  * org.apache.maven:maven-model:jar:3.2.5
  * org.apache.maven:maven-repository-metadata:jar:2.2.1
  * org.apache.maven:maven-repository-metadata:jar:3.2.5
  * org.apache.maven:maven-settings-builder:jar:3.2.5
  * org.apache.maven:maven-settings:jar:2.2.1
  * org.apache.maven:maven-settings:jar:3.2.5
  * org.apache.maven.wagon:wagon-file:jar:2.6
  * org.apache.maven.wagon:wagon-http-lightweight:jar:2.6
  * org.apache.maven.wagon:wagon-http-shared:jar:2.6
  * org.apache.maven.wagon:wagon-provider-api:jar:1.0-beta-6
  * org.apache.maven.wagon:wagon-provider-api:jar:2.6
  * org.codehaus.plexus:plexus-component-annotations:jar:1.5.5
  * org.codehaus.plexus:plexus-interpolation:jar:1.11
  * org.codehaus.plexus:plexus-interpolation:jar:1.21
  * org.codehaus.plexus:plexus-utils:jar:1.5.15
  * org.codehaus.plexus:plexus-utils:jar:3.0.20
  * org.dom4j:dom4j:jar:2.1.1
  * org.eclipse.aether:aether-api:jar:1.0.0.v20140518
  * org.eclipse.aether:aether-connector-basic:jar:1.0.0.v20140518
  * org.eclipse.aether:aether-impl:jar:1.0.0.v20140518
  * org.eclipse.aether:aether-spi:jar:1.0.0.v20140518
  * org.eclipse.aether:aether-transport-wagon:jar:1.0.0.v20140518
  * org.eclipse.aether:aether-util:jar:1.0.0.v20140518
  * org.glassfish.main.extras:glassfish-embedded-all:jar:4.1.2
  * org.hibernate.common:hibernate-commons-annotations:jar:5.1.0.Final
  * org.hibernate:hibernate-core:jar:5.4.12.Final
  * org.hibernate:hibernate-entitymanager:jar:5.4.12.Final
  * org.javassist:javassist:jar:3.24.0-GA
  * org.jboss.arquillian.config:arquillian-config-api:jar:1.1.15.Final
  * org.jboss.arquillian.config:arquillian-config-impl-base:jar:1.1.15.Final
  * org.jboss.arquillian.container:arquillian-container-impl-base:jar:1.1.15.Final
  * org.jboss.arquillian.container:arquillian-container-spi:jar:1.1.15.Final
  * org.jboss.arquillian.container:arquillian-container-test-api:jar:1.1.15.Final
  * org.jboss.arquillian.container:arquillian-container-test-impl-base:jar:1.1.15.Final
  * org.jboss.arquillian.container:arquillian-container-test-spi:jar:1.1.15.Final
  * org.jboss.arquillian.container:arquillian-glassfish-embedded-3.1:jar:1.0.2
  * org.jboss.arquillian.core:arquillian-core-api:jar:1.1.15.Final
  * org.jboss.arquillian.core:arquillian-core-impl-base:jar:1.1.15.Final
  * org.jboss.arquillian.core:arquillian-core-spi:jar:1.1.15.Final
  * org.jboss.arquillian.extension:arquillian-transaction-api:jar:1.0.5
  * org.jboss.arquillian.extension:arquillian-transaction-impl-base:jar:1.0.5
  * org.jboss.arquillian.extension:arquillian-transaction-jta:jar:1.0.5
  * org.jboss.arquillian.extension:arquillian-transaction-spi:jar:1.0.5
  * org.jboss.arquillian.junit:arquillian-junit-container:jar:1.1.15.Final
  * org.jboss.arquillian.junit:arquillian-junit-core:jar:1.1.15.Final
  * org.jboss.arquillian.protocol:arquillian-protocol-servlet:jar:1.1.15.Final
  * org.jboss.arquillian.test:arquillian-test-api:jar:1.1.15.Final
  * org.jboss.arquillian.test:arquillian-test-impl-base:jar:1.1.15.Final
  * org.jboss.arquillian.test:arquillian-test-spi:jar:1.1.15.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-cdi:jar:1.1.15.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-ejb:jar:1.1.15.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-initialcontext:jar:1.1.15.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-resource:jar:1.1.15.Final
  * org.jboss:jandex:jar:2.1.1.Final
  * org.jboss.logging:jboss-logging:jar:3.3.2.Final
  * org.jboss.shrinkwrap.resolver:shrinkwrap-resolver-api:jar:2.2.6
  * org.jboss.shrinkwrap.resolver:shrinkwrap-resolver-api-maven:jar:2.2.6
  * org.jboss.shrinkwrap.resolver:shrinkwrap-resolver-impl-maven:jar:2.2.6
  * org.jboss.shrinkwrap.resolver:shrinkwrap-resolver-spi:jar:2.2.6
  * org.jboss.shrinkwrap.resolver:shrinkwrap-resolver-spi-maven:jar:2.2.6
  * org.jsoup:jsoup:jar:1.7.2
  * org.slf4j:slf4j-api:jar:1.6.6
  * org.slf4j:slf4j-api:jar:1.7.25
  * org.slf4j:slf4j-jdk14:jar:1.7.25
  * org.slf4j:slf4j-log4j12:jar:1.6.6
  * org.slf4j:slf4j-log4j12:jar:1.7.25
  * org.sonatype.plexus:plexus-cipher:jar:1.4
  * org.sonatype.plexus:plexus-sec-dispatcher:jar:1.3
  Added or updated dependencies (new versions derived from this release):
  * fish.payara.arquillian:arquillian-payara-server-embedded:jar:2.4.4
  * fish.payara.extras:payara-embedded-all:jar:5.2021.6
  * jakarta.platform:jakarta.jakartaee-api:jar:8.0.0
  * net.bytebuddy:byte-buddy:jar:1.11.12
  * org.apache.commons:commons-lang3:jar:3.8.1
  * org.apache.maven:maven-artifact:jar:3.6.3
  * org.apache.maven:maven-model:jar:3.6.3
  * org.apache.maven:maven-repository-metadata:jar:3.6.3
  * org.apache.maven:maven-settings:jar:3.6.3
  * org.apache.maven.wagon:wagon-provider-api:jar:3.3.4
  * org.codehaus.plexus:plexus-interpolation:jar:1.25
  * org.codehaus.plexus:plexus-utils:jar:3.2.1
  * org.hibernate.common:hibernate-commons-annotations:jar:5.1.2.Final
  * org.hibernate:hibernate-core:jar:5.5.7.Final
  * org.hibernate:hibernate-entitymanager:jar:5.5.7.Final
  * org.jboss.arquillian.config:arquillian-config-api:jar:1.6.0.Final
  * org.jboss.arquillian.config:arquillian-config-impl-base:jar:1.6.0.Final
  * org.jboss.arquillian.config:arquillian-config-spi:jar:1.6.0.Final
  * org.jboss.arquillian.container:arquillian-container-impl-base:jar:1.6.0.Final
  * org.jboss.arquillian.container:arquillian-container-spi:jar:1.6.0.Final
  * org.jboss.arquillian.container:arquillian-container-test-api:jar:1.6.0.Final
  * org.jboss.arquillian.container:arquillian-container-test-impl-base:jar:1.6.0.Final
  * org.jboss.arquillian.container:arquillian-container-test-spi:jar:1.6.0.Final
  * org.jboss.arquillian.core:arquillian-core-api:jar:1.6.0.Final
  * org.jboss.arquillian.core:arquillian-core-impl-base:jar:1.6.0.Final
  * org.jboss.arquillian.core:arquillian-core-spi:jar:1.6.0.Final
  * org.jboss.arquillian.junit:arquillian-junit-container:jar:1.6.0.Final
  * org.jboss.arquillian.junit:arquillian-junit-core:jar:1.6.0.Final
  * org.jboss.arquillian.protocol:arquillian-protocol-servlet:jar:1.6.0.Final
  * org.jboss.arquillian.test:arquillian-test-api:jar:1.6.0.Final
  * org.jboss.arquillian.test:arquillian-test-impl-base:jar:1.6.0.Final
  * org.jboss.arquillian.test:arquillian-test-spi:jar:1.6.0.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-cdi:jar:1.6.0.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-ejb:jar:1.6.0.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-initialcontext:jar:1.6.0.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-resource:jar:1.6.0.Final
  * org.jboss:jandex:jar:2.2.3.Final
  * org.jboss.logging:jboss-logging:jar:3.4.2.Final
  * org.slf4j:slf4j-api:jar:1.7.28
  * org.slf4j:slf4j-jdk14:jar:1.7.28
  * org.slf4j:slf4j-log4j12:jar:1.7.28

## Other Changes
* **[Java,JEE]** The JEE JDS demo implementation has been discontinued.
  The JDS demo implementation continues to be available as pure Java version.
  Additionally a ready-to-use docker concerto with MQTT broker, database and
  JDS demo implementation is provided for single- and multiple-backend use case.
  See the [starting joynr backend instructions](infrastructure.md) for more details.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
None.

# joynr 1.17.8

## API-relevant Changes
None.

## Other Changes
* **[Android]** Added Gradle 7.0 support for joynr-generator-gradle-plugin.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
None.

# joynr 1.17.7

## API-relevant Changes
None.

## Other Changes
* **[C++]** Custom headers are additionally sent as MQTT5 user properties within PUBLISH.
* **[C++]** Raise required minimum boost version from 1.65.0 to 1.66.0 when building joynr
  with UDS support (default `JOYNR_SUPPORT_UDS=ON`). Minimum required boost version is still 1.65.0
  when building without UDS support (`JOYNR_SUPPORT_UDS=OFF`).

* **[Java, JEE]** Updated hivemq-mqtt-client to 1.2.2 and its transitive dependencies

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** Fixed segmentation fault / abortions in cluster controller and libjoynr runtime
  occurring during UDS socket opening.
* **[Java]** Fixed a bug in `GuidedProxyBuilder`: `DiscoveryResult` now always contains all
  discovered providers (it was erronously prefiltered in case of automatic discovery retries.

# joynr 1.17.6

## API-relevant Changes
None.

## Other Changes
* **[Java]** Custom headers are additionally sent as MQTT5 user properties within PUBLISH.
* **[C++]** Consistently use minimum required boost version 1.65.0 in all joynr CMake projects.
* **[Java]** Removed unused dependency to `io.joynr.mqtt:mqtt-client`.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** Fixed a segmentation fault in libjoynr runtime that could occur during shutdown when
  logging was attempted within a background thread after spdlog was already in the process of
  getting shutdown by libc exit handlers on the main thread.
* **[Java]** Fixed race condition on RPC timeout that sporadically caused a
  `JoynrIllegalStateException` instead of the expected `JoynrCommunicationException`.

# joynr 1.17.5

## API-relevant Changes
* **[TS]** Persistency support for routingTable and subscriptions has been
  discontinued. Affected elements `routingTable` and `publications` with
  boolean values have been removed from persistencyValue object which
  is optional part of provisioning object.
  For the remaining configuration options in joynr TS see `persistencyValue`
  in [JavaScript settings](/wiki/JavaScriptSettings.md).

## Other Changes
* **[Java]** Changed behavior of 'removeStale`:
  * cluster controller now delays `removeStale` for 5 minutes instead of sending it immediately
    after start.
  * Retry of `removeStale` is now limited to 1 hour.
  * TTl for a single try of `removeStale` is now 1 minute instead of 1 hour.

## Configuration Property Changes
* **[TS]** Configuration options for routingTable and subscription persistency
  have been removed, see section API-relevant Changes above.

## Security Fixes
None.

## Bug Fixes
* **[Java]** Fixed a bug where replies for stateless asynchronous remote procedure
  calls could not be handled correctly.
* **[TS]** Added missing dependency to lodash.

# joynr 1.17.4

## API-relevant Changes
* **[Java]** The MessagePersister feature has been discontinued.
  The related classes / interfaces  
  `io.joynr.messaging.persistence.MessagePersister`,  
  `io.joynr.messaging.persistence.NoOpMessagePersister`,  
  `io.joynr.jeeintegration.api.JoynrMessagePersister`  
  have been removed.
* **[C++]** The following configuration related APIs have been removed in LibjoynrSettings:
  * `LibjoynrSettings::getBroadcastSubscriptionRequestPersistenceFilename()`
  * `LibjoynrSettings::setBroadcastSubscriptionRequestPersistenceFilename(...)`
  * `LibjoynrSettings::SETTING_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME()`
  * `LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME()`
  * `LibjoynrSettings::getSubscriptionRequestPersistenceFilename()`
  * `LibjoynrSettings::setSubscriptionRequestPersistenceFilename(...)`
  * `LibjoynrSettings::SETTING_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME()`
  * `LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME()`
  * `LibjoynrSettings::isSubscriptionPersistencyEnabled()`
  * `LibjoynrSettings::setSubscriptionPersistencyEnabled(...)`
  * `LibjoynrSettings::SETTING_SUBSCRIPTION_PERSISTENCY_ENABLED()`
  * `LibjoynrSettings::DEFAULT_SUBSCRIPTION_PERSISTENCY_ENABLED()`
  * `LibjoynrSettings::isMessageRouterPersistencyEnabled()`
  * `LibjoynrSettings::setMessageRouterPersistencyEnabled(...)`
  * `LibjoynrSettings::SETTING_MESSAGE_ROUTER_PERSISTENCY_ENABLED()`
  * `LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCY_ENABLED()`
  * `LibjoynrSettings::getMessageRouterPersistenceFilename()`
  * `LibjoynrSettings::setMessageRouterPersistenceFilename(...)`
  * `LibjoynrSettings::SETTING_MESSAGE_ROUTER_PERSISTENCE_FILENAME()`
  * `LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME()`
  * `LibjoynrSettings::isClearSubscriptionEnabled()`
  * `LibjoynrSettings::setClearSubscriptionEnabled(...)`
  * `LibjoynrSettings::SETTING_CLEAR_SUBSCRIPTION_ENABLED()`
  * `LibjoynrSettings::DEFAULT_CLEAR_SUBSCRIPTION_ENABLED()`
* **[C++]** The following configuration related APIs have been removed in ClusterControllerSettings:
  * `ClusterControllerSettings::getMulticastReceiverDirectoryPersistenceFilename()`
  * `ClusterControllerSettings::setMulticastReceiverDirectoryPersistenceFilename(...)`
  * `ClusterControllerSettings::SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME()`
  * `ClusterControllerSettings::DEFAULT_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCE_FILENAME()`
  * `ClusterControllerSettings::isMulticastReceiverDirectoryPersistencyEnabled()`
  * `ClusterControllerSettings::setMulticastReceiverDirectoryPersistencyEnabled(...)`
  * `ClusterControllerSettings::SETTING_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCY_ENABLED()`
  * `ClusterControllerSettings::DEFAULT_MULTICAST_RECEIVER_DIRECTORY_PERSISTENCY_ENABLED()`

## Other Changes
* **[Java, JEE]** The persistency support for subscription requests has been
  discontinued.
* **[C++]** The features `Message Persistency`, `Routing Table Persistency`, `Subscription Persistency`
  and `Multicast Receiver Directory Persistency` have been discontinued.

## Configuration Property Changes
* **[Java, JEE]** The properties `PROPERTY_SUBSCRIPTIONREQUESTS_PERSISTENCY` and
  `PROPERTY_SUBSCRIPTIONREQUESTS_PERSISTENCE_FILE,` have been removed.
* **[Java]** Removed property `MESSAGE_QUEUE_ID` / `io.joynr.messaging.queue.id`
  related to discontinued MessagePersister feature.
* **[C++]** The following settings have been removed and will be ignored, if present:
```
  [lib-joynr]
  message-router-persistence-file
  message-router-persistency
  subscriptionrequest-persistence-file
  broadcastsubscriptionrequest-persistence-file
  subscription-persistency

  [cluster-controller]
  multicast-receiver-directory-persistence-file
  multicast-receiver-directory-persistency-enabled
```

## Security Fixes
None.

## Bug Fixes
None.

# joynr 1.17.3

## API-relevant Changes
None.

## Other Changes
* **[Java]** Generated classes and their dependencies are now checked for compatibility
  with Java 8, the build fails in case of incompatibilities.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[Android]** Prevent message loop between libjoynr clients (with fixed client address) and
  cluster controller. A loop could occur after client restart:
  * when delivering reply messages for proxies from a previous client lifecycle.
  * when delivering request messages for providers that are not (yet) registered (again) in the new
    client lifecycle.
* **[Java]** Fixed dependency inconsistencies in `tools/generator` and all subprojects: now the
  same versions for all direct and transitive dependencies are used.

# joynr 1.17.2

## API-relevant Changes
None.

## Other Changes
* **[Wiki]** Documented error handling for createProxyBuilder methods in C++.

## Configuration Property Changes
None.

## Security Fixes
* **[JS]** Updated ws to version 7.5.0 and @types/ws to version 7.4.5, fixing `CVE-2021-32640`.

## Bug Fixes
* **[Java]** Fixed dependency problems in `joynr-generator-standalone`: Now the declared versions
  for all dependencies of `joynr-generator-standalone` are used. The build failed because a newer
  transitive dependency is not compatible with Java 8.
* **[Java]** Fixed broken example `custom-headers`.
* **[C++]** Added missing boost includes to JoynrClusterControllerRuntime.h.
* **[Java]** Fixed error handling in `PublicationManager` (subscription handling on provider side):
  Improved handling of proxy routing entries did not work in case of errors.

# joynr 1.17.1

## API-relevant Changes
None.

## Other Changes
* **[Android]** Gradle generator now uses absolute paths for model and output paths.
* **[Android]** Removed unused dependency to `org.apache.httpcomponents:httpclient`.
* **[Java]** Reduced memory consumption by storing multicast subscriptions on provider side only
  if required, i.e. if they have to be queued.
* **[C++]** Changed behavior of 'removeStale`:
  * cluster controller now delays `removeStale` for 5 minutes instead of sending it immediately
    after boot.
  * Retry of `removeStale` is now limited to 1 hour.
  * TTl for a single try of `removeStale` is now 1 minute instead of 1 hour.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** Fixed the UID determination of UDS clients for access control in the cluster controller.
* **[C++]** Delay reconnect when MQTT disconnect with failure occurs instead of attempting to
  reconnect immediately.

# joynr 1.17.0

## API-relevant Changes
* **[Generator]** Setting the `jee` flag does not lead to a warning anymore.
  It is now completely removed from the generator.

## Other Changes
* **[Java]** Global provider registration without awaitGlobalRegistration is now retried
  automatically after timeouts until it succeeds or an unrecoverable error occurs. A single try
  uses the default ttl of 60 seconds towards GlobalCapabilitiesDirectory.  
  Note that the application still does not get informed about the result (success or unrecoverable
  error) of the global registration at GlobalCapabilitiesDirectory.  
  Examples for unrecoverable errors: all types of `DiscoveryError` (e.g. `INVALID_GBID`).
* **[All]** Aligned the internal ttls for lookup calls (provider discovery for proxy creation)
  to make sure that replies or errors from `GlobalCapabilitiesDirectory` are reported via the
  libjoynr `ProxyBuilder` or `GuidedProxyBuilder` instead of generic timeout error.
* **[Java,C++]** No more `SubscriptionStop` message is sent when a multicast (non selective
  broadcast) subscription is unregistered. The message is not required because the provider side
  does not store any information about multicast subscriptions.
* **[C++]** Removed exec permissions on uds socket.
* **[Maven]** Removed unused declared dependencies from `pom.xml` files.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
None.

# joynr 1.16.3

## API-relevant Changes
None.

## Other Changes
None.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[Java]** Fixed a bug where replies for stateless asynchronous remote procedure
  calls could not be handled correctly

# joynr 1.16.2

## API-relevant Changes
None.

## Other Changes
* **[C++]** Changed behavior of 'removeStale`:
  * cluster controller now delays `removeStale` for 5 minutes instead of sending it immediately
    after boot.
  * Retry of `removeStale` is now limited to 1 hour.
  * TTL for a single try of `removeStale` is now 1 minute instead of 1 hour.
* **[C++]** Added boost includes that were accidentally omitted.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** Fixed the UID determination of UDS clients for access control in the cluster controller.
* **[C++]** Delay reconnect when MQTT disconnect with failure occurs instead of attempting to
  reconnect immediately.

# joynr 1.16.1

## API-relevant Changes
None.

## Other Changes
* **[Java]** Fixed logger name in `ProviderDirectory`. Logger name was `ReplyCallerDirectory`
  before.
* **[Java]** Added missing log to `RoutingTableImpl.incrementReferenceCount`.
* **[Java]** Finalized integration tests for improved handling of routing entries.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[Java]** Fixed a potential deadlock on provider side in `PublicationManagerImpl`.

# joynr 1.16.0

## API-relevant Changes
* **[Java,JEE]** Added `buildNone` method to `GuidedProxyBuilder`. Make sure to call this method
  after a successful discovery with the `GuidedProxyBuilder` if you do not want to create a proxy
  for any of the discovered providers. This will trigger the necessary cleanup to avoid unnecessary
  memory usage, see our documentation for [Java](java.md#the-guided-proxy-builder) or
  [JEE](jee.md#the-guided-proxy-builder)
* **[Generator]** The `--addVersionTo` setting is being temporarily reactived again and it overrules
  the `#noVersionGeneration` comment in fidl files. The option `name` for the  `--addVersionTo`
  setting is still no longer supported. See [Generator documentation](generator.md) for more
  information about the generator versioning settings.
* **[Java]** Added additional contructor to `MessagingQos`:
  `MessagingQos(ttl_ms, effort, customHeaders, compress)`
* **[C++]** ProxyBuilder now throws a `DiscoveryException` if the interface the provider discovered
  with arbitration strategy `FIXED_PARTICIPANT` does not match the expected interface.

## Other Changes
* **[Java]** Reduced memory usage by improved handling of discovery and routing entries to prevent
  out of memory problems:  
  * routing entries are now removed when they are not required anymore, independently of their
    expiry date, e.g. after a proxy is garbage collected or when a request is answered.
  * discovery entries of globally registered providers are stored only in the local store of
    LocalCapabilitiesDirectory in the cluster controller. A second entry in the global cache is not
    required anymore.
* **[Java]** Expired incoming messages are now logged as warning instead of error.
* **[C++]** Additional logs for MosquittoConnection start, stop and restart:
  * `...external start() done`
  * `...external stop() done`
  * `...restartThread: stopInternal() done`
  * `...restartThread: waiting 10 secs`
  * `...restartThread: startInternal() done`

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** Fixed a bug where a too short delay between stopping and starting the Mosquitto loop by
  the restartThread could lead to a situation where the MQTT broker repeatedely denied connect
  attempts with CONNACK with RC 135 (client not authorized to connect) due to exceeding the maximum
  connection rate configured at the broker. Now a fixed delay of 10 seconds is used within the
  restartThread.
* **[C++]** Fixed a bug where intentionally stopping the Mosquitto loop caused an invocation of the
  restartThread which could result in a stop/start loop in special cases.
* **[Java]** Fixed a bug that prevented publication deliveries after proxy was garbage collected.
* **[Java]** Fixed memory leak in SubscriptionManager in case PublicationManager signals
  subscription error.
* **[Generator,C++]** Fixed generation of polymorphic subtypes without members or multiple stages
  of inheritance.

# joynr 1.15.12

## API-relevant Changes
* **[Java,JEE]** A GuidedProxyBuilder can now only be used to run one discovery and build one proxy.
  A second attempt at one of those actions will cause an exception to be thrown.
  The GuidedProxyBuilder now throws `JoynrIllegalStateException` instead of `IllegalStateException`.

## Other Changes
* **[C++]** Global provider registration without awaitGlobalRegistration is now retried
  automatically after timeouts until it succeeds or an unrecoverable error occurs. A single try
  uses the default ttl of 60 seconds towards GlobalCapabilitiesDirectory instead of the extended
  timeout of 90 minutes introduced in joynr 1.15.9.  
  Note that the application still does not get informed about the result (success or unrecoverable
  error) of the global registration at GlobalCapabilitiesDirectory.  
  Examples for unrecoverable errors: all types of `DiscoveryError` (e.g. `INVALID_GBID`).
* **[Java,JEE]** Improved logging in ProviderWrapper (log actual provider method invocation):
  `Invoke provider method...` (log level debug)

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[Java]** Fixed a bug that prevented concurrent request execution (concurrent provider method
  invocations) in joynr 1.15.4 or later.
* **[JEE,Docker]** Fixed sporadic deployment problem of discovery-directory-jee standalone and in
  docker image `joynr-backend-jee` (caused by concurrent request execution bug, see above).

# joynr 1.15.11

## API-relevant Changes
None.

## Other Changes
* **[Java]** Logging improvements for expired or non routable messages:
  * Logs for dropped messages now contain the tracking info (e.g. requestReplyId) instead of
    just the messageId.
  * Log for unroutable incoming MQTT message now contains the keyword `incoming` to prevent
    misunderstanding.
  * Error for expired messages is now only logged once: removed unncecessary log in
    `MqttMessagingSkeleton` and reduced log level to `trace` in MessageRouter
  * Expired messages are now logged like in C++: `Received expired message: ...` instead of
    `ttl must be greater than 0 ...`.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** Fix asynchronous message drop on rescheduling when UDS sending-queue-size is reached.
* **[C++]** Fix UDS server SEGV in case of client disconnection.

# joynr 1.15.10

## API-relevant Changes
None.

## Other Changes
* **[Java, JEE, Franca]** Completely removed the GlobalDomainAccessController. Local ACL is
  still possible
* **[Java]** HTTP bridge has been removed. `joynr.jeeintegration.enable.httpbridge` and related
 properties are ignored.
* **[Java]** HTTP messaging has been removed. `joynr.bounceproxy`, `joynr.messaging` and
 `joynr.servlet` and related properties are ignored.
* **[ALL]** Improved documentation (API and Markdown) of provider unregistration to better explain
  what happens internally and what the user can expect.
* **[Java,C++]** Additional logs to log scheduling and start of global add and remove operations
  (`LocalCapabilitiesDirectoryImpl.java` / `LocalCapabilitiesDirectoryImpl.GcdTaskSequencer` and
  `GlobalCapabilitiesDirectoryClient.cpp`):
  * `Global provider registration scheduled...`
  * `Global provider registration started...`
  * `Global remove scheduled...`
  * `Re-Add scheduled.`
  * `Re-Add started.`
* **[Java]** Provider discovery (lookup) for multiple domains now performs a global lookup for all
  domains if there are no local entries for some domains. Before, the global lookup only included
  the missing domains.

## Configuration Property Changes
* **[Java]** Removed settings:
  * `PROPERTY_BOUNCE_PROXY_URL` / `joynr.messaging.bounceproxyurl`
  * `PROPERTY_GLOBAL_DOMAIN_ACCESS_CONTROLLER_URL` / `joynr.messaging.gdac.url`
  * `PROPERTY_HOSTS_FILENAME` / `joynr.messaging.hostsFileName`
  * `PROPERTY_SERVLET_CONTEXT_ROOT` / `joynr.servlet.context.root`
  * `PROPERTY_SERVLET_HOST_PATH` / `joynr.servlet.hostpath`
  * `PROPERTY_SERVLET_SHUTDOWN_TIMEOUT` / `joynr.servlet.shutdown.timeout`
  * `PROPERTY_SERVLET_SKIP_LONGPOLL_DEREGISTRATION` / `joynr.servlet.skiplongpollderegistration`
* **[Java]** Settings no longer support HTTP bridging values:
  * `PROPERTY_GLOBAL_CAPABILITIES_DIRECTORY_URL` / `joynr.messaging.gcd.url`
* **[JEE]** Removed settings:
  * `JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY` / `joynr.jeeintegration.enable.httpbridge`
  * `JEE_INTEGRATION_ENDPOINTREGISTRY_URI` / `joynr.jeeintegration.endpointregistry.uri`

## Security Fixes
None.

## Bug Fixes
* **[C++]** Fix message/memory corruption when UDS sending-queue-size is reached.
* **[JEE]** Delay MQTT shared subscription until providers are registered in order to prevent a
  potential deadlock during application deployment in an environment with high load. It could occur
  if the MQTT Broker had a lot of request messages for a provider in the starting application. The
  incoming messages then blocked all joynr message worker threads because they tried to invoke the
  not yet fully registered and initialized provider bean which required a synchronized block that
  was already locked by another thread that tried to register the provider. The provider
  registration (and the deployment) subsequently failed because all message worker threads were
  blocked and the registration message could not be delivered to the global capabilities directory.
* **[C++]** Fixed a problem that could lead to starvation of global add operations (global provider
  registration) causing high memory consumption and timeouts without reporting the actual reason.

# joynr 1.15.9

## API-relevant Changes
* **[TS]** Marked `internalMessagingQos` in `Provisioning` as deprecated. TTL of
  internal messages cannot be changed anymore, default value 70000 ms will always be used.

## Other Changes
* **[C++]** Provider registration without awaitGlobalRegistration now uses an extended
  timeout of 90 minutes towards GlobalCapabilitiesDirectory. Note this registration is
  still unreliable since the application will not get informed about the result and thus
  has no chance to retry in case of failure.
* **[Java]** Removed the dependency net.sourceforge.htmlunit:htmlunit
* **[Java]** Improved logging of Access Control Entries for better readability.
* **[Java]** Fixed cleanup of queued subscription requests in PublicationManager to free
  unnecessary memory.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[C++]** MosquittoConnection attempts to reconnect when client was not authorized to connect

# joynr 1.15.8

## API-relevant Changes
None.

## Other Changes
* **[Java]** Added javadoc documentation to GuidedProxyBuilder and DiscoveryResult.
* **[Java]** Prevent GuidedProxyBuilder from wrong usage:
  * Proxy version must match the version of the selected provider
  * Setters (to configure discovery) must not be called while discovery is in progress

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
None.

# joynr 1.15.7.1
**[Android]** Release required for Android deployment.

## API-relevant Changes
None.

## Other Changes
* **[Java]** Copy passed DiscoveryQos parameter in GuidedProxyBuilder to guard against accidental
  modification of the DiscoveryQos after it has already been passed in.

## Configuration Property Changes
None.

## Security Fixes
None.

## Bug Fixes
* **[Android]** Fix message TTL with no connectivity.
* **[C++]** Fixed handling of empty broker-url which was converted into an invalid URL.
  E.g. if additional-backend-1-gbid was configured but additional-backend-1-broker-url
  was empty. This resulted in MQTT connection attempts to `channels/://:80`.
* **[C++]** Fixed compatibility with Boost 1.65.0: fall back to operator bool() instead of using
  `has_value()` method for boost::optional which is not yet supported.

# joynr 1.15.7

## API-relevant Changes
None.

## Other Changes
* **[Java]** Removed global domain access controller (GDAC) support from cluster controller.
* **[C++]** Added mosquitto 1.6.8 to CMakeLists.txt of standalone performance test
* **[Docker]** Added specific HiveMQ image to standalone performance test
* **[C++]** No longer required HTTP related communication support has been removed.
* **[C++]** Dependency to curl library has been removed
* **[Java]** Fixed concurrency management in PublicationManager (responsible for subscription
  handling on provider side) to avoid race conditions.
* **[Java,C++]** Improved protection of the cluster controller / LocalCapabilitiesDirectory
  against violation of the uniqueness of participantIds (participantIds must be unique within the
  whole system). If a remote DiscoveryEntry has the same participantId as a local provider (i.e.
  a provider connected to the local cluster controller), the remote provider will be ignored now.
  Such a provider would cause an invalid state in the cluster controller because routing entries are
  already protected so that the routing entry for the provider's participantId still points to the
  local provider.

## Configuration Property Changes
* **[C++]** Messaging settings related to HTTP communication have been removed
  (create-channel-retry-interval, delete-channel-retry-interval, longpoll-retry-interval,
  local-proxy-host, local-proxy-port, http-debug, long-poll-timeout, capabilities-directory-url).

## Security Fixes
None.

## Bug Fixes
* **[Docker]** Introduced better error checking in standalone PT scripts
* **[C++]** Fixed a deadlock that could occur in message router while routing a message to
  a currently not running provider. Such a message was queued and in case there was a
  subscription for the messageQueuedForDelivery filtered broadcast for that specific
  provider participantId it additionally triggered a publication.
  The routing thread acquired and held a read lock for the currently non deliverable message
  and acquired the same lock a second time to route the publication message. Afterwards it
  released both locks. The deadlock occured if the message queue cleanup job (which was
  triggered periodically) tried to acquire the same lock as a write lock when the routing
  thread already held its first lock but had not yet acquired the second one.

# joynr 1.15.6

## API-relevant Changes
* **[Generator]** The option `name` for the `--addVersionTo` setting is no longer supported.
* **[Generator]** The `--addVersionTo` setting is being replaced by the `#noVersionGeneration`
  comment in fidl files. Package versioning is now enabled by default and can be disabled with
  the `#noVersionGeneration` comment. `addVersionTo` is no longer required and should be
  omitted because it will be removed in a future version. If it is still set,
  then it has to match the `#noVersionGeneration` comment:  
  If the comment exists, then the `--addVersionTo` setting has to be `none`.  
  If the comment does not exist, then the `--addVersionTo` setting has to be `package`.  
  In case of an invalid configuration, the code generation will be aborted with an exception.  
  See [Generator documentation](generator.md) for more information about the generator versioning
  settings.

## Other Changes
* **[Maven]** Increased minimum required Maven to 3.3.3.
* **[C++]** Introduced `JOYNR_SUPPORT_WEBSOCKET` and `JOYNR_SUPPORT_UDS` CMake options to
  suppress unused build artifacts on demand. Both are enabled by default.
* **[C++]** Removed global domain access controller (GDAC) support from cluster controller.
* **[C++]** Queued global add (register provider) requests are now aborted when they are expired
  and the application is informed about the abort. This can happen for example when there are too
  many queued requests or the processing of requests takes too much time or, of course, if there
  currently is no global (MQTT) connection. Global remove (unregister provider) requests do not
  expire.
* **[C++]** Globally registered providers are now re-added periodically (every 7 days) to the
  GlobalCapabilitiesDirectory to synchronize the local and global capabilities directory.
* **[Java]** Improved behavior of global remove operations: fixed potential race condition between
  queued global add and subsequent remove.
* **[C++]** Improved behavior of global remove operations: fixed potential race condition between
  queued global add and subsequent remove.

## Configuration Property Changes
* **[C++]** The cluster controller `access-control` settings `global-domain-access-controller-participantid`,
  `global-domain-access-controller-address` and `use-ldas-only` are ignored. The cluster controller behavior
  corresponds to the former default behavior (`use-ldas-only=true`).
* **[Java]** Removed property `PROPERTY_DISCOVERY_GLOBAL_ADD_AND_REMOVE_TTL_MS`.
  Now, the default TTL of 60 seconds is always used.

## Security Fixes
* **[Generator]** Upgraded Java `guava` library from `21.0`/`15.0` to `30.1`, fixing
*CVE-2020-8908* and *CVE-2018-10237*.
* **[Test]** Upgraded Java `junit` library from `4.12` to `4.13.1`, fixing *CVE-2020-15250*.

## Bug Fixes
* **[C++]** Cluster controller now sets MQTT session expiry interval on reconnect. Before it was only
  set on initial connect and the session was lost in case of reconnect. This requires additional
  patches for Mosquitto, see `docker/joynr-base/Dockerfile`.
* **[Java]** Fix starvation of GCD tasks in case of retry of global remove

# joynr 1.15.5

## API-relevant Changes
None.

## Other Changes
* **[Java]** Updated jackson to version 2.11.3
* **[C++]** MosquittoConnection now adds the GBID to each log message to distinguish multiple
  connections.
* **[Java]** Queued global add (register provider) requests are now aborted when they are expired
  and the application is informed about the abort. This can happen for example when there are too
  many queued requests or the processing of requests takes too much time and, of course, if there
  currently is no global (MQTT) connection. Global remove (unregister provider) requests do not
  expire.
* **[Java]** DiscoveryQos parameter for ProxyBuilder.setDiscoveryQos is now copied to avoid
  accidental manipulation, e.g. when the same DiscoveryQos is used for subsequent ProxyBuilder calls
  with different setting.
* **[Java]** Globally registered providers are now re-added periodically (every 7 days) to the
  GlobalCapabilitiesDirectory to synchronize the local and global capabilities directory.

## Configuration Property Changes
None.

## Bug Fixes
* **[Java]** ObjectMapper is now protected by ReentrantReadWriteLock
  to avoid potential occurrence of ConcurrentModificationException

# joynr 1.15.4

## API-relevant Changes
None.

## Other Changes
* **[TS]** Use `onFatalRuntimeError` callback in the examples.
* **[C++]** In UDS case, use stringified UID for ACL checks if username for UID cannot be
  determined.
* **[Java,C++]** Serialized global add (register provider) and remove (unregister provider) requests
  to assure consistent state of local and global capabilities directory.
* **[C++]** Improved documentation of `createRuntimeAsync()`.

## Configuration Property Changes
None.

## Bug Fixes
* **[Java]** Fixed potential NullPointerException and race conditions in RequestReplyManager and
  PublicationManager.
* **[Java]** Added missing removal of routing entry when unregistering a provider to avoid
  unnecessary memory usage.
* **[Java]** Fixed delayed shutdown caused by improper cleanup in HivemqMqttClient.shutdown.

# joynr 1.15.3

## API-relevant Changes
* **[C++]** Implemented the `GuidedProxyBuilder`, which allows extended control
  over how proxies are built after executing a lookup. In particular, the provider
  version can be determined before actually building the proxy. The `GuidedProxyBuilder`
  can be retrieved via the `createGuidedProxyBuilder` method through the `JoynrRuntime` (C++).
  See the [C++ documentation](cplusplus.md#the-guided-proxy-builder).

## Other Changes
* **[TS]** Improved Maven javascript profile and Maven clean phase for radio-node example:
  radio-node is now included in Maven javascript profile and clean plugin removes everything.
* **[C++]** Examples and test projects are now versioned in CMakeLists.txt instead of appending
  the version to the project name.

## Configuration Property Changes
None.

## Bug Fixes
* **[C++]** Fixed pure virtual method call that could occur if internal data got modified while
  being used
* **[C++]** Stabilized broadcast integration tests.
* **[TS]** Fixed UnhandledPromiseRejectionWarnings in unit tests.

# joynr 1.15.2

## API-relevant Changes
* **[JEE]** Interface `ProviderRegistrationSettingsFactory` provides access to the actual
  implementation (the provider bean annotated with @ServiceProvider) of an interface in
  addition to the service interface class itself. This allows multiple instances of a provider
  in the same war file with different registration settings, e.g. different domains. See the
  [JEE documentation](jee.md#implementing-services-joynr-providers) for more details.
* **[Java, JEE]** DelayableImmutableMessage now contains a participantId instead of a set of
  addresses. Any custom message persistency has to be adjusted.
  Have a look at our documentation for more details on [MessagePersister in joynr Java](./java.md)
  and [MessagePersister in joynr JEE](./jee.md).

## Other Changes
* **[C++]** Fixed misleading error logs when attempting to read settings files from different paths.
  Error is now logged only in case no settings can be loaded.

## Configuration Property Changes
* **[Java]** Increased default value of PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS
  from 3600000 ms (1 hour) to 21600000 ms (6 hours).
* **[C++]** Increased default value of SETTING_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS
  from 3600000 ms (1 hour) to 21600000 ms (6 hours).

## Bug Fixes
* **[Java, JEE]** Fixed retransmission of multicast publication messages in case of errors:
  Multicast publications are not multiplicated anymore.

# joynr 1.15.1

## API-relevant Changes
None.

## Other Changes
* **[Java]** Stabilization of flaky tests that sometimes failed in CI
* **[C++]** Use the new `onFatalRuntimeError` callback in the examples and tests

## Configuration Property Changes
None.

## Bug Fixes
* **[C++, Java]** Fix some log statements related to periodic freshness updates
  (aka "touch")
* **[C++]** Fix usage of `boost::optional` in order to still be compatible with
  Boost 1.65.0

# joynr 1.15.0

## API relevant changes
* **[Java, TS]** The API `registerInAllKnownBackends()` is deprecated, since registration in all
  backends is now the default behavior.
* **[C++]** The APIs `registerProviderInAllBackends` and
  `registerProviderInAllBackendsAsync` are deprecated, since registration in all backends
  is now the default behavior.
* **[C++]** The `createRuntime` and `createRuntimeAsync` APIs in `JoynrRuntime` now support a
  `onFatalRuntimeError` callback, which is invoked in exceptional cases that render the runtime
  inoperable. The old APIs without this callback are deprecated now and will be removed in the
  future.
* **[TS]** The `joynr.load` now supports `onFatalRuntimeError` callback, which is invoked in
  exceptional cases that render the runtime inoperable. The callback is optional but it is highly
  recommended to provide an implementation. The old usage of this API does not break.
* **[JEE]** Interface `ProviderRegistrationSettingsFactory` allows to specify domain registration
  setting as well. See the [JEE documentation](jee.md#implementing-services-joynr-providers) for more details.

## Other changes
* **[FIDL]** Changed *add provider* functionality for multiple backends. If no GBIDs specified,
  the registration is applied to all backends instead of the default one. See also documentation of
  [joynr usage with multiple backends](./multiple-backends.md).
* **[C++]** Raise required minimum Boost version from 1.58.0 to 1.65.0.
* **[C++,TS]** Support Unix Domain Sockets for communication between libjoynr and cluster
  controller. The C++ cluster controller accepts UDS client and WebSocket clients in parallel by
  default.  
  For more information about the (optional) UDS configuration options in C++ see
  [Joynr C++ Settings](/wiki/cpp_settings.md).  
  For the (optional) configuration options in joynr TS see
  [JavaScript settings](/wiki/JavaScriptSettings.md).
* **[TS]** Communication via UDS is now default. To use WebSocket communication, call
  `joynr.selectRuntime` with `WebSocketLibjoynrRuntime` before calling `joynr.load()`.
* **[Java, JEE, Generator]** The generator flag for generating JEE code is deprecated now and not
  necessary anymore. The generated code for Java and JEE is identical now.
  See [Generator documentation](generator.md).
* **[Java]** The default discovery scope is consistent with other languages now. It is changed from
    `LOCAL_AND_GLOBAL` to `LOCAL_THEN_GLOBAL`.
* **[C++]** Cluster controller now uses the new `touch` and `removeStale` methods of
  [`GlobalCapabilitiesDirectory.fidl`](../basemodel/src/main/franca/joynr/GlobalCapabilitiesDirectory.fidl).
  The Global Capabilities Directory has to implement version 0.3 of the interface.
* **[Android]** Added support for multi-user in the Android Binder runtime, allowing Android
  Services to be bound either as the system user or as a specific user. The implementation is
  tailored for the CC to run in user 0 (system), and joynr app clients can connect on
  whichever user they decide to in a system with multi-user capabilities.
* **[Android]** Updated joynr Gradle generator to use latest Gradle wrapper (6.1.1) and build
  tools (4.0.0).
* **[Android]** Added ContentProvider that allows apps and other components to  implement
  persistent providers, designed for specific use cases where the providers' configuration can
  occur first thing in the component lifecycle.
* **[JEE]** Added automatic provider registration retries and reporting of unsuccessful provider
  registration. See
  [documentation of joynr JEE integration](jee.md#provider-registration-retries-and-error-handling)
  for more information.
* **[Java]** MqttPahoClient has been removed. Please use HivemqMqttClient instead.
* **[ALL]** Marked broadcast `globalDiscoveryEntryChanged ` in `GlobalCapabilitiesDirectory.fidl`
  as deprecated because it has never been implemented, used or tested.

## Configuration property changes
* **[C++]** The configuration option `mqtt-max-message-size-bytes` was removed from messaging
  settings. Instead the value optionally provided in the CONNACK properties sent by the broker
  as response to CONNECT is used, if available; otherwise the size is limited to the maximum
  value a `std::int64_t` can hold.
* **[JEE]** Introduced `PROPERTY_JEE_PROVIDER_REGISTRATION_RETRIES` and
  `PROPERTY_JEE_PROVIDER_REGISTRATION_RETRY_INTERVAL_MS` to configure the new internal registration
  retry and registration error reporting mechanism.  
  See [Java Configuration Reference](JavaSettings.md#jee-integration) for more details on these
  properties.
* **[Java]** The MqttModule property `PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES` has been removed.
  Instead the value optionally provided in the CONNACK properties sent by the broker
  as response to CONNECT is used, if available; otherwise the size is limited to the default
  value specified by HiveMQ client.

## Bug Fixes
* **[Java, JEE]** When sending replies to requests, the relative TTL was erroneously set to the
    absolute TTL timestamp of the request, resulting in replies that virtually never expired.
    This is fixed and replies will now expire at the same time as the original request.
* **[Java, JEE]** Fixed some cases, where an UndeclaredThrowableException was returned when
    customer provider method implementation code was using an unsupported exception type
    (neither ProviderRuntimeException nor ApplicationException where applicable).
    Now a ProviderRuntimeException will be returned which contains the text of the original
    exception in such cases.
* **[Generator, C++]** Correctly reference enum values with fully qualified name where required
* **[C++]** MosquittoConnection tries to reconnect even in case a fatal error
  occurs after connection has been established

# joynr 1.14.7

## API-relevant Changes
None.

## Other Changes
None.

## Configuration Property Changes
None.

## Bug Fixes
* **[C++]** Fixed a bug where a too short delay between stopping and
  starting the Mosquitto loop by the restartThread could lead to a
  situation where the MQTT broker repeatedely denied connect attempts
  with CONNACK with RC 135 (client not authorized to connect) due to
  exceeding the maximum connection rate configured at the broker.
  Now a fixed delay of 10 seconds is used within the restartThread.
* **[C++]** Fixed a bug where intentionally stopping the Mosquitto loop
  caused an invocation of the restartThread which could result in a
  stop/start loop in special cases.

# joynr 1.14.6

## API-relevant Changes
None.

## Other Changes
* **[Java]** Updated jackson to version 2.11.3

## Configuration Property Changes
None.

## Bug Fixes
* **[C++]** Provider registration without awaitGlobalRegistration now uses an extended
  timeout of 90 minutes towards GlobalCapabilitiesDirectory. Note this registration is
  still unreliable since the application will not get informed about the result and thus
  has no chance to retry in case of failure.
* **[C++]** MosquittoConnection attempts to reconnect when client was not authorized to connect
* **[Java]** ObjectMapper is now protected by ReentrantReadWriteLock
  to avoid potential occurrence of ConcurrentModificationException
* **[JEE]** Delay MQTT shared subscription until providers are registered in order to prevent a
  potential deadlock during application deployment in an environment with high load. It could occur
  if the MQTT Broker had a lot of request messages for a provider in the starting application. The
  incoming messages then blocked all joynr message worker threads because they tried to invoke the
  not yet fully registered and initialized provider bean which required a synchronized block that
  was already locked by another thread that tried to register the provider. The provider
  registration (and the deployment) subsequently failed because all message worker threads were
  blocked and the registration message could not be delivered to the global capabilities directory.
* **[Docker]** Fixed docker image joynr-backend-jee

# joynr 1.14.5

## API-relevant Changes
None.

## Other Changes
None.

## Configuration Property Changes
None.

## Bug Fixes
* **[C++]** Fixed a bug where the CONNACK sent on MQTT reconnect did not
  include the session expiry interval property causing the session to end
  when the network connection is closed.

# joynr 1.14.4

## API-relevant Changes
None.

## Other Changes
* **[Java]** Stabilization of flaky tests that sometimes failed in CI

## Configuration Property Changes
None.

## Bug Fixes
* **[C++, Java]** Fix some log statements related to periodic freshness updates
  (aka "touch")
* **[C++]** Fix usage of `boost::optional` in order to still be compatible with
  Boost 1.65.0

# joynr 1.14.3

## API relevant changes
* **[Java]** The default discovery scope is consistent with other languages now. It is changed from
    `LOCAL_AND_GLOBAL` to `LOCAL_THEN_GLOBAL`.
* **[Java, TS]** The API `registerInAllKnownBackends()` is deprecated, since registration in all
  backends is now the default behavior.
* **[C++]** The APIs `registerProviderInAllBackends` and
  `registerProviderInAllBackendsAsync` are deprecated, since registration in all backends
  is now the default behavior.
* **[C++]** The `createRuntime` and `createRuntimeAsync` APIs in `JoynrRuntime` now support an optional
  `onFatalRuntimeError` callback, which is invoked in exceptional cases that render the runtime
  inoperable. The old APIs without this callback are deprecated now and may be removed in the
  future.
* **[TS]** The `joynr.load` now supports `onFatalRuntimeError` callback, which is invoked in
  exceptional cases that render the runtime inoperable. The callback is optional but it is highly
  recommended to provide an implementation. The old usage of this API does not break.

## Configuration property changes
* **[C++]** The configuration option `mqtt-max-message-size-bytes` was removed from messaging
  settings. Instead the value optionally provided in the CONNACK properties sent by the broker
  as response to CONNECT is used, if available; otherwise the size is limited to the maximum
  value a `std::int64_t` can hold.
* **[JEE]** Introduced `PROPERTY_JEE_PROVIDER_REGISTRATION_RETRIES` and
  `PROPERTY_JEE_PROVIDER_REGISTRATION_RETRY_INTERVAL_MS` to configure the new internal registration
  retry and registration error reporting mechanism.  
  See [Java Configuration Reference](JavaSettings.md#jee-integration) for more details on these
  properties.
* **[Java]** The MqttModule property `PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES` has been removed.
  Instead the value optionally provided in the CONNACK properties sent by the broker
  as response to CONNECT is used, if available; otherwise the size is limited to the default
  value specified by HiveMQ client.

## Bug Fixes
* **[Java, JEE]** Fixed some cases, where an UndeclaredThrowableException was returned when
  customer provider method implementation code was using an unsupported exception type
  (neither ProviderRuntimeException nor ApplicationException where applicable).
  Now a ProviderRuntimeException will be returned which contains the text of the original
  exception in such cases.
* **[C++]** MosquittoConnection tries to reconnect even in case a fatal error
  occurs after connection has been established
* **[Generator, C++]** Correctly reference enum values with fully qualified name where required

## Other Changes
* **[C++]** Cluster controller now uses the new `touch` and `removeStale` methods of
  [`GlobalCapabilitiesDirectory.fidl`](../basemodel/src/main/franca/joynr/GlobalCapabilitiesDirectory.fidl).
  The Global Capabilities Directory has to implement version 0.3 of the interface.
* **[C++]** Raise required minimum Boost version from 1.58.0 to 1.65.0.
* **[Android]** Added ContentProvider that allows apps and other components to  implement
  persistent providers, designed for specific use cases where the providers' configuration can
  occur first thing in the component lifecycle.
* **[JEE]** Added automatic provider registration retries and reporting of unsuccessful provider
  registration. See
  [documentation of joynr JEE integration](jee.md#provider-registration-retries-and-error-handling)
  for more information.
* **[Java]** MqttPahoClient has been removed. Please use HivemqMqttClient instead.
* **[ALL]** Marked broadcast `globalDiscoveryEntryChanged ` in `GlobalCapabilitiesDirectory.fidl`
  as deprecated because it has never been implemented, used or tested.
* **[FIDL]** Changed *add provider* functionality for multiple backends. If no GBIDs specified,
  the registration is applied to all backends instead of the default one. See also documentation of
  [joynr usage with multiple backends](./multiple-backends.md).

# joynr 1.14.2

## Other changes

* **[Android]** Updated joynr Gradle generator to use latest Gradle wrapper (6.1.1) and build
 tools (4.0.0).

## Bug Fixes

* **[C++]** MosquittoConnection tries to reconnect even in case a fatal error
  occurs after connection has been established
* **[C++]** Fixed memory leak in MosquittoConnection
* **[Generator, C++]** Correctly reference enum values with fully qualified name where required
* **[Java]** Expiration date of provisioned discovery entries is now set to maximum value
* **[Java]** Routing entries for RoutingProvider and DiscoveryProvider are now sticky

# joynr 1.14.1

## API relevant changes
None.

## Other changes
* **[Java, JEE, Generator]** The generator flag for generating JEE code is deprecated now and not
  necessary anymore. The generated code for Java and JEE is identical now.
  See [Generator documentation](generator.md).
* **[Android]** Added support for multi-user in the Android Binder runtime, allowing Android
  Services to be bound either as the system user or as a specific user. The implementation is
  tailored for the CC to run in user 0 (system), and joynr app clients can connect on
  whichever user they decide to in a system with multi-user capabilities.

## Bug Fixes
* **[Java, JEE]** When sending replies to requests, the relative TTL was erroneously set to the
    absolute TTL timestamp of the request, resulting in replies that virtually never expired.
    This is fixed and replies will now expire at the same time as the original request.

# joynr 1.14.0

## API relevant changes
* **[Java, JEE]** Changed `JoynrStatusMetrics` to support multiple backends and separate
  connections. `JoynrStatusMetrics` now provide additional connection information.  
  See [Java documentation](java.md#status_monitoring) and
  [JEE documentation](jee.md#status_monitoring) for more details.  
  NOTE: ConnectionStatusMetrics are only available for HivemqMqttClient.
* **[Java]** Replaced `MqttStatusReceiver` with `JoynrStatusMetrics`, see above.

## Other changes
* **[FIDL]** Introduce new touch and removeStale methods in GlobalCapabilitiesDirectory FIDL.
  See [internal documentation of GlobalCapabilitiesDirectory](../docs/gcd-in-multiple-be.md)
  for for additional information about the new methods.
* **[Java, JEE]** Updated hivemq-mqtt-client to 1.1.4
* **[C++]** Mosquitto MQTT Client now sets message expiry interval of
* **[Java, JEE]** Singleton JeeJoynrServiceLocator will now be created at startup time.
* **[Java]** HivemqMqttClient now sets message expiry interval of
  published MQTT messages.
* **[Java]** HivemqMqttClient now checks for maxmimum message size the same way as was
  already implemented for MqttPahoClient, see property `PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES`
  See the [Java Configuration Reference](JavaSettings.md) for details about this property.
* **[Java]** Fixed HivemqMqttClient to not lose messages: messages were sporadically lost
  because hivemq-mqtt-client was not used in a thread safe way when publishing messages.
* **[All, Generator]** The generator can now be optionally be called via command line
  parameter or plugin configuration to restrict code generation to only proxy or provider
  side. The default is to generate code for both sides. Details can be found [here](generator.md).
* **[TS]** Introduced ArbitrationStrategy `FixedParticipant`, for details, see
  [JavaScript documentation](javascript.md#the-discovery-quality-of-service).
* **[TS]** Introduced ArbitrationConstants to be used for arbitration strategies `Keyword` and
  `FixedParticipant`, for more details see
  [JavaScript documentation](javascript.md#the-discovery-quality-of-service).
* **[C++]** Routed messages are now only printed in the cluster controller, with a configurable
  interval.
* **[C++]** Modernize the CMake build system. Please use the targets with the `Joynr::` prefix now.
  Also uses the gold linker, if available.
* **[Overall]** All NOTICE files were removed as they were partially outdated and hard to update.
* **[C++, TS]** Introduced some preliminary changes to introduce Unix Domain Sockets as transport
  protocol. This is not complete yet and can not be used yet.
* **[JEE]** Startup is aborted when `@JoynrLocalDomain` is not set or empty.
* **[Java, JEE]** Logging has been substantially altered to better align with the logging in C++
  and provide better insights into what is happening.

## Configuration property changes
* **[Java, JEE]** Introduced the `PROPERTY_SUBSCRIPTIONREQUESTS_PERSISTENCY` boolean property, which
  can be set via `joynr.dispatching.subscription.subscriptionrequests_persistency`. This property
  allows to disable persistent storage of selective (filtered) broadcast and attribute subscriptions.
  If disabled, then a provider will not know about any earlier subscriptions from proxies after
  restart, i.e. publications for those broadcasts will only be sent to proxies which subscribed
  after the last (re-)start. This setting has no impact on publications for non-selective broadcasts
  (= multicasts).
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** Renamed property `PROPERTY_PARTICIPANTIDS_PERSISISTENCE_FILE,` to
  `PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE,`, fixing a typo.  
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** Renamed property `PROPERTY_SUBSCRIPTIONREQUESTS_PERSISISTENCE_FILE,` to
  `PROPERTY_SUBSCRIPTIONREQUESTS_PERSISTENCE_FILE,`, fixing a typo.  
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[C++]** Added setting `cluster-controller/routed-messages-print-interval-s` for configuring
  the interval in seconds, in which the cluster controller prints the total routed messages.

# joynr 1.13.0

## API relevant changes
None.

## Other changes
* **[All]** For global communication, joynr now requires a MQTT broker that supports MQTT 5 and
  MQTT 3. MQTT 3 is only required for Paho MQTT Client in Java, for JS test execution and for JS
  InProcessRuntime.
* **[C++]** ClusterController runtime now uses MQTT 5 protocol and thus connects only to
  MQTT 5 enabled brokers.
* **[C++]** Updated Mosquitto dependency to 1.6.8
* **[C++]** Added patch for Mosquitto 1.6.8 to add missing constructor API
  `mosquitto_connect_bind_async_v5()`. Joynr cannot be built anymore with unpatched Mosquitto.
* **[Java, JEE]** Switched from MQTT 3 to MQTT 5 for HiveMQ MQTT Client. MQTT Paho Client continues
  to use MQTT 3.
* **[Java]** Fixed and improved reconnect handling and error handling in HivemqMqttClient.
* **[Java]** Improved logging in HivemqMqttClient.
* **[Java]** Removed dependency to spotbugs-annotations.
* **[Java]** Updated jackson to version 2.10.2
* **[Java]** Updated javax.interceptor:javax.interceptor-api to 1.2.2 (only required for joynr
  backend-services examples).
* **[Java]** Updated hibernate version to 5.4.12.Final (only required for joynr backend-services
  examples).
* **[C++]** Fixed memory leaks in MosquittoConnection, AbstractMessageRouter,
  JoynrClusterControllerRuntime.
* **[All]** Removed unecessary participantId section from MQTT topic when publishing.

## Configuration property changes
None.

# joynr 1.12.0

## API relevant changes
None.

## Other changes
* **[Java, JEE]** Integration for HiveMQ MQTT Client. See joynr 1.8.3 - 1.8.6 for details.
* **[Java, JEE]** Updated hivemq-mqtt-client to 1.1.3
* **[Java, JEE]** Removed message worker status metrics (interface `StatusReceiver`), status metrics
  for MQTT (interface `MqttStatusReceiver`) are the only remaining status metrics.
* **[Java]** Removed dependency to commons-lang.
* **[Java, Generator]** Removed ProvidesJoynrTypesInfo annotation from generated classes.
  Classes generated with the new generator will therefore not be compatible with old library versions.
* **[Android]** Fixed Binder race condition on Android version of joynr.
* **[Android]** Added message queue for joynr initialization procedure.

## Configuration property changes
None.

# joynr 1.11.6

This is a non-content release, only necessary due to an error while releasing Java
artifacts on Maven Central.

## API relevant changes
None

## Other changes
None.

## Configuration property changes
None.

# joynr 1.11.5

## API relevant changes
None

## Other changes
None.

## Configuration property changes
None.

## Bug Fixes
* **[Generator, C++]** Correctly reference enum values with fully qualified name where required

# joynr 1.11.4

## API relevant changes
None

## Other changes
None.

## Configuration property changes
None.

## Bug Fixes
* **[C++]** MosquittoConnection tries to reconnect even in case a fatal error
  occurs after connection has been established

# joynr 1.11.3

## API relevant changes
None

## Other changes
* **[C++]** Fixed memory leaks in MosquittoConnection
* **[C++]** Prevent ProxyBuilder callbacks to be called multiple times
  and / or onSuccess and onError callbacks to be both called.

## Configuration property changes
None.

# joynr 1.11.2

## API relevant changes
None

## Other changes
* **[C++]** Fixed an issue where unavailability of a MQTT connection could lead to
  a deadlock situation.

## Configuration property changes
None.

# joynr 1.11.1

## API relevant changes
None

## Other changes
* **[Android]** Release required for Android deployment.

## Configuration property changes
None.

# joynr 1.11.0

## API relevant changes
* **[Java, JEE]** Introduced the `GuidedProxyBuilder`, which allows extended control
  over how proxies are built after executing a lookup. In particular, the provider
  version can be determined before actually building the proxy. The `GuidedProxyBuilder`
  can be retrieved via the `getGuidedProxyBuilder` method through the `JoynrRuntime` (Java) or
  the `ServiceLocator`(JEE).
  See the [Java documentation](java.md#the-guided-proxy-builder) and [JEE documentation](jee.md)
  for details.

* **[C++]** The `registerProviderXXX` APIs in `JoynrRuntime` now support an optional
  parameter `gbids` for multiple backend support. Additionally the new APIs
  `registerProviderInAllBackends` and `registerProviderInAllBackendsAsync` have been
  introduced.
* **[C++]** Add setGbids to ProxyBuilder to select non default GBIDs for provider discovery

* **[JS]** The `joynr` npm package was fully converted to typescript. It's now shipped with
  additional `.d.ts` files supporting the usage of typescript and enabling auto-completion for pure
  JS inside modern IDEs.

* **[JS]** `joynr-generator-standalone-1.11.0.jar` and later versions will now output `.ts` files
  instead of `.js` files. In order to help compile those files to js and for an easier access to
  the generator the new npm package `joynr-generator` was introduced. `joynr-generator` also
  supports `.json` files listing `.fidl` files and the generation of `joynr-includes` facilitating
  the import of generated code.

* **[Android]** Added Android binder transport option to take full advantage of the Android framework,
and use its native way of performing inter-process communication (IPC).

## Other changes
* **[Java]** Orphaned shutdown listener entries related to no longer referenced proxy instances
  are now removed by the Routing Table cleanup background job, provided the related proxy instances
  have already been collected by the Java garbage collector.
* **[Java]** Updated jackson.databind to version 2.9.10, net.sourceforge.htmlunit:htmlunit to 2.36.0
* **[Java]** Added synchronization to ShutdownNotifierList to avoid corruption

## Configuration property changes
None.

# joynr 1.10.1 **[Android only version]**

## API relevant changes
* **[Android]** Added Android binder transport option to take full advantage of the Android framework,
and use its native way of performing inter-process communication (IPC).

## Other changes
* **[Android]** Updated the Android examples to demonstrate how to use the binder version of joynr

## Configuration property changes
None.

# joynr 1.10.0

## API relevant changes
None.

## Other changes
* **[All]** Franca dependencies were updated and are now fetched from Maven Central. These  
  dependencies are no longer packaged with joynr and should be fetched from Maven Central. Please  
  make sure to upgrade any components accordingly to avoid possible version conflicts:
  * org.franca:org.franca.core updated from version 0.13.0 to 0.13.1
  * org.franca:org.franca.core.dsl updated from version 0.13.0 to 0.13.1
  * org.franca:org.franca.deploymodel.dsl updated from version 0.13.0 to 0.13.1
* **[Java]** Updated jackson to version 2.9.9, jackson.databind to 2.9.9.2
* **[C++]** Client Websocket TLS connections will be encrypted by default
* **[JS]** Part of the project was rewritten into TypeScript (TS).
* **[Android]** Added libjoynr-android-websocket-runtime, updated joynr-generator-gradle-plugin
  and added Android examples to examples/android/android-hello-world.


## Configuration property changes
None.

# joynr 1.9.2

## API relevant changes
None.

## Other changes
* **[Java]** Fixed a bug that requests for GlobalCapabilitiesDirectory from a joynr runtime that
  uses the empty GBID could not be answered.
* **[Java]** Updated jackson to version 2.9.10
* **[Java]** Updated net.sourceforge.htmlunit:htmlunit to 2.36.0
* **[Java]** Orphaned shutdown listener entries related to no longer referenced proxy instances
  are now removed by the Routing Table cleanup background job, provided the related proxy instances
  have already been collected by the Java garbage collector.
* **[Java]** Added synchronization to ShutdownNotifierList to avoid corruption

## Configuration property changes
None.

# joynr 1.9.1

## API relevant changes
* **[Java]** Introduced new ProviderRegistrar class for provider registration in the JoynrRuntime.
  This is to prevent future API bloat of the registerProvider() methods. An object of this class
  is retrieved by executing the new getProviderRegistrar() method of the JoynrRuntime.
  Usage of the class is documented in the [Java documentation](java.md).
  Previous interfaces of the registerProvider() method are still available, but are now
  considered deprecated.
* **[Java, JEE]** Global discovery and registration now works as documented:
  * By default, the communication with GlobalCapabilitiesDirectory takes place over the default
    (first configured) backend connection, i.e. the connection identified by the first GBID
    configured at the cluster controller (`PROPERTY_GBIDS`).
  * If GBID(s) are provided to `ProxyBuilder` / `ServiceLocator` or `ProviderRegistrar` /
    `ProviderRegistrationSettingsFactory`, the first one will be used to select the connection for
    the communication with GlobalCapabilitiesDirectory.

## Other changes
* **[Java, C++, JS]** Underscore is now allowed inside custom header values in MessagingQos

## Configuration property changes
None.

# joynr 1.9.0

## API relevant changes
* **[Java]** Introduced new registerProvider() method with GBIDs in the JoynrRuntime.
  This enables using non-default GBIDs for provider registration. The provided GBIDs must
  be configured in `PROPERTY_GBIDS` on the cluster-controller side.
* **[Java]** Introduced new registerInAllKnownBackends() method in the JoynrRuntime.
  This allows the registration of a provider in all backends known to the cluster-controller.
* **[JEE]** ServiceLocator builder now provides the ability to specify a callback when creating a
  proxy, so that you can be notified of proxy creation success or failure. See JEE wiki for details.
* **[JEE]** ServiceLocator builder now allows building a `CompletableFuture<>` of the proxy in order
  to be able to await successful proxy creation before using it. See JEE wiki for details.
* **[JEE]** Interface `ProviderQosFactory` was renamed to `ProviderRegistrationSettingsFactory` as
  now other settings like GBID(s) can be specified as well. You can still provide a single or
  multiple implementations of this interface in order to customize how joynr registers a provider.
  See the Javadoc of the interface as well as the [JEE documentation](jee.md) for more details.
* **[Java]** Add setGbids to ProxyBuilder.
* **[Java]** Contrary to the documentation, the default proxy for GlobalCapabilitiesDirectory will be picked.

## Other changes
* **[C++]** Clustercontroller attempts to auto-unsubcribe a consumer proxy's
  attribute or filtered (selective) broadcast subscription at the associated provider
  when a providers's publication for that subscriptionId cannot be delivered via websocket
  to the consumer proxy (e.g. because the related application has exited without graceful
  shutdown). This is an emergency measure and currently only available when unencrypted
  messaging is used.

## Configuration property changes
* **[Java]** Renamed property `PROPERTY_DISCOVERY_RETRY_INTERVAL_MS` to
  `PROPERTY_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS`.  
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** Renamed property `PROPERTY_ARBITRATION_MINIMUMRETRYDELAY` to
  `PROPERTY_DISCOVERY_MINIMUM_RETRY_INTERVAL_MS`. The new identifier is
  `joynr.discovery.minimumretryintervalms`. This change indicates that the property
  is related to `DiscoveryQos.retryIntervalMs`.  
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** `DISCOVERYDIRECTORYURL` renamed to `PROPERTY_GLOBAL_CAPABILITIES_DIRECTORY_URL` and
  `joynr.messaging.discoverydirectoryurl` changed to `joynr.messaging.gcd.url`.
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** `DOMAINACCESSCONTROLLERURL` renamed to `PROPERTY_GLOBAL_DOMAIN_ACCESS_CONTROLLER_URL`
  and `joynr.messaging.domainaccesscontrollerurl` changed to `joynr.messaging.gdac.url`.
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** Added property `PROPERTY_GBIDS`, which is configurable with `joynr.messaging.gbids`.
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** `PROPERTY_KEY_MQTT_BROKER_URI` renamed to `PROPERTY_MQTT_BROKER_URIS` and
  `joynr.messaging.mqtt.brokeruri` changed to `joynr.messaging.mqtt.brokeruris`.
  Now, multiple Broker URIs can be configured.  
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** `PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMER_SEC` renamed to `PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC` and
  `joynr.messaging.mqtt.keepalivetimersec` changed to `joynr.messaging.mqtt.keepalivetimerssec`.
  Now, multiple keep alive timers can be configured.  
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** `PROPERTY_KEY_MQTT_CONNECTION_TIMEOUT_SEC` renamed to `PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC` and
  `joynr.messaging.mqtt.connectiontimeoutssec` changed to `joynr.messaging.mqtt.connectiontimeoutssec`.
  Now, multiple connection timeouts can be configured.  
  See [Java Configuration Reference](JavaSettings.md) for more details.

# joynr 1.8.12.1

## API relevant changes
None.

## Other changes
* **[Java,JEE]** Improved logging in RequestInterpreter and ProviderWrapper.

## Configuration property changes
None.

# joynr 1.8.12

## API relevant changes
None.

## Other changes
* **[Java]** The cache for GlobalDiscoveryEntries is now limited to 1000 non-sticky
  entries. The oldest entry will be removed first, when the limit is exceeded.

## Configuration property changes
None.

# joynr 1.8.11

## API relevant changes
None.

## Other changes
* **[Java]** HivemqMqttClient now checks for maxmimum message size the same way as was
  already implemented for MqttPahoClient, see property `PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES`
  See the [Java Configuration Reference](JavaSettings.md) for details about this property.
* **[Java, JEE]** Singleton JeeJoynrServiceLocator will now be created at startup time.

## Configuration property changes
None.

# joynr 1.8.10

## API relevant changes
None.

## Other changes
* **[Java, JEE]** Fixed automatically generated `DiscoveryQos` when using the
  `GuidedProxyBuilder` to actually use the default values, instead of invalid
  values.

## Configuration property changes
None.

# joynr 1.8.9

## API relevant changes
None.

## Other changes
* **[Java, JEE]** Updated hivemq-mqtt-client to 1.1.4
* **[Java]** Fixed and improved reconnect handling and error handling in HivemqMqttClient.
* **[Java]** Improved logging in HivemqMqttClient.
* **[Java]** Fixed HivemqMqttClient to not lose messages: messages were sporadically lost
  because hivemq-mqtt-client was not used in a thread safe way when publishing messages.
* **[Java]** Added the changes from joynr 1.8.2.1.

## Configuration property changes
None.

# joynr 1.8.8

## API relevant changes
None.

## Other changes
* **[JS] Fixed potential messages loss when a websocket connection could not be
  established
* **[Java, JEE]** Updated hivemq-mqtt-client to 1.1.3
* **[Java]** Updated jackson to 2.10.2
* **[C++]** Prevent Arbitrator/ProxyBuilder from invocing callbacks multiple times

## Configuration property changes
None.

# joynr 1.8.7

## API relevant changes
* **[Java/JEE]** Introduced the `GuidedProxyBuilder`, which allows extended control
  over how proxies are built after executing a lookup. In particular, the provider
  version can be determined before actually building the proxy. The `GuidedProxyBuilder`
  can be retrieved via the `getGuidedProxyBuilder` method through the `JoynrRuntime` (Java) or
  the `ServiceLocator`(JEE).
  See the [Java documentation](java.md#the-guided-proxy-builder) and [JEE documentation](jee.md)
  for details.

## Other changes
* **[Generator]** `addVersionTo` / DEPRECATED. The addVersionTo option will be removed
  in a future version of the generator. Set the #noVersionGeneration comment
  in the .fidl file instead, see [Disable versioning of generated files](#disable-versioning-of-generated-files).

## Configuration property changes
None.

# joynr 1.8.6

## API relevant changes
None.

## Other changes
* **[Java, JEE]** HivemqMqttClient now waits for `PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS`
  milliseconds before trying to reconnect when the initial connect fails.
* **[Java]** Added synchronization to ShutdownNotifierList to avoid corruption

## Configuration property changes
None.

# joynr 1.8.5

## API relevant changes
None.

## Other changes
* **[Java, JEE]** HivemqMqttClient now reports its status via JoynrStatusMetrics.
* **[Java, JEE]** When the HivemqMqttClient is using TLS and the property
  `PROPERTY_KEY_MQTT_CIPHERSUITES` is not set, all ciphers supported in Java are used
  by default. Currently these are:
  * `TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384`
  * `TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384`
  * `TLS_RSA_WITH_AES_256_CBC_SHA256`
  * `TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384`
  * `TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384`
  * `TLS_DHE_RSA_WITH_AES_256_CBC_SHA256`
  * `TLS_DHE_DSS_WITH_AES_256_CBC_SHA256`
  * `TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA`
  * `TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA`
  * `TLS_RSA_WITH_AES_256_CBC_SHA`
  * `TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA`
  * `TLS_ECDH_RSA_WITH_AES_256_CBC_SHA`
  * `TLS_DHE_RSA_WITH_AES_256_CBC_SHA`
  * `TLS_DHE_DSS_WITH_AES_256_CBC_SHA`
  * `TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256`
  * `TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256`
  * `TLS_RSA_WITH_AES_128_CBC_SHA256`
  * `TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256`
  * `TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256`
  * `TLS_DHE_RSA_WITH_AES_128_CBC_SHA256`
  * `TLS_DHE_DSS_WITH_AES_128_CBC_SHA256`
  * `TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA`
  * `TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA`
  * `TLS_RSA_WITH_AES_128_CBC_SHA`
  * `TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA`
  * `TLS_ECDH_RSA_WITH_AES_128_CBC_SHA`
  * `TLS_DHE_RSA_WITH_AES_128_CBC_SHA`
  * `TLS_DHE_DSS_WITH_AES_128_CBC_SHA`
  * `TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384`
  * `TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256`
  * `TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384`
  * `TLS_RSA_WITH_AES_256_GCM_SHA384`
  * `TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384`
  * `TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384`
  * `TLS_DHE_RSA_WITH_AES_256_GCM_SHA384`
  * `TLS_DHE_DSS_WITH_AES_256_GCM_SHA384`
  * `TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256`
  * `TLS_RSA_WITH_AES_128_GCM_SHA256`
  * `TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256`
  * `TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256`
  * `TLS_DHE_RSA_WITH_AES_128_GCM_SHA256`
  * `TLS_DHE_DSS_WITH_AES_128_GCM_SHA256`
  * `TLS_EMPTY_RENEGOTIATION_INFO_SCSV`

## Configuration property changes
None.

# joynr 1.8.4

## API relevant changes
None.

## Other changes
* **[Java]** Orphaned shutdown listener entries related to no longer referenced proxy instances
  are now removed by the Routing Table cleanup background job, provided the related proxy instances
  have already been collected by the Java garbage collector.
* **[Java]** Updated jackson to version 2.9.9, jackson.databind to 2.9.9.2
* **[Java]** Updated net.sourceforge.htmlunit.htmlunit to 2.35.0
* **[Java]** Updated smrf to 0.3.3
* **[Java]** Updated hivemq-mqtt-client to 1.1.2

## Configuration property changes
* **[Java, JEE]** Introduced the `PROPERTY_KEY_MQTT_CIPHERSUITES` property, which can be set via
  `joynr.messaging.mqtt.ssl.ciphersuites`. This property allows to define the ciphers that are
  used by the HiveMQ MQTT Client. See [Java Configuration Reference](JavaSettings.md) for more details.

# joynr 1.8.3

## API relevant changes
None.

## Other changes
* **[Java, JEE]** Integration for HiveMQ MQTT Client. Also, the mqtt module is split up into the
  base implementation, the Paho implementation and the new HiveMQ MQTT Client integration. The JEE
  integration now uses the HiveMQ MQTT Client integration.
* **[Docs]** Added documentation of configuring the [Java MQTT Clients](./java_mqtt_clients.md)
* **[All]** Franca dependencies were updated and are now fetched from Maven Central. These  
  dependencies are no longer packaged with joynr and should be fetched from Maven Central. Please  
  make sure to upgrade any components accordingly to avoid possible version conflicts:
  * org.franca:org.franca.core updated from version 0.13.0 to 0.13.1
  * org.franca:org.franca.core.dsl updated from version 0.13.0 to 0.13.1
  * org.franca:org.franca.deploymodel.dsl updated from version 0.13.0 to 0.13.1

# joynr 1.8.2.1

## API relevant changes
None.

## Other changes
* **[Java, JEE]** Removed unnecessary persistency for MulticastSubscriptionRequests related to
  subscriptions for non-selective (unfiltered) broadcasts.

## Configuration property changes
* **[Java, JEE]** Introduced the `PROPERTY_SUBSCRIPTIONREQUESTS_PERSISTENCY` boolean property, which
  can be set via `joynr.dispatching.subscription.subscriptionrequests_persistency`. This property
  allows to disable persistent storage of selective (filtered) broadcast and attribute subscriptions.
  If disabled, then a provider will not know about any earlier subscriptions from proxies after
  restart, i.e. publications for those broadcasts will only be sent to proxies which subscribed
  after the last (re-)start. This setting has no impact on publications for non-selective broadcasts
  (= multicasts).
  See [Java Configuration Reference](JavaSettings.md) for more details.

# joynr 1.8.2

## API relevant changes
None.

## Other changes
* **[JEE]** Fixed bug for making stateless-async calls to JEE providers which was resulting in an
  IllegalArgumentException because the method signature was not found in the callback due to the
  void result being wrongly translated to a `null` value.

## Configuration property changes
None.

# joynr 1.8.1

## API relevant changes
None.

## Other changes
* **[JS]** Fixed a bug where fixed participantIds used for provider registrations
  were not getting stored causing provider unregistration to fail.

## Configuration property changes
None.

# joynr 1.8.0

## API relevant changes
None.

## Other changes
* **[Java]** Introduced overridden toString() methods in classes
  io.joynr.arbitration.DiscoveryQos and io.joynr.messaging.MessagingQos
* **[Build]** Dockerfiles with included scripts for building joynr have been updated to use
  Fedora 27 with openssl 1.1.0, MoCOCrW branch openssl1.1, flatbuffers 1.10.0, smrf 0.3.3
  and websocketpp 0.8.1.
* **[C++, Java, JS]** Updated smrf version to 0.3.3
* **[C++]** The `JOYNR_DEFAULT_RUNTIME_LOG_LEVEL` for Release builds has been changed to
  "DEBUG" to support message tracking.
* **[FIDL]** Introduce support for multiple backends in GlobalCapabilitiesDirectory FIDL. See
  `docs/gcd-in-multiple-be.md` for additional documentation.

## Configuration property changes
None.

# joynr 1.7.3

## API relevant changes
None.

## Other changes
* **[Java]** Added synchronization to ShutdownNotifierList to avoid corruption
* **[Java]** Updated net.sourceforge.htmlunit:htmlunit to 2.36.0
* **[Java]** Updated jackson.databind to version 2.9.10

## Configuration property changes
None.

# joynr 1.7.2

## API relevant changes
None.

## Other changes
* **[Java]** Orphaned shutdown listener entries related to no longer referenced proxy instances
  are now removed by the Routing Table cleanup background job, provided the related proxy instances
  have already been collected by the Java garbage collector.
* **[Java]** Updated jackson to version 2.9.9, jackson.databind to 2.9.9.2
* **[Java]** Updated net.sourceforge.htmlunit.htmlunit to 2.35.0
* **[Java]** Updated smrf to 0.3.3

## Configuration property changes
None.

# joynr 1.7.1

## API relevant changes
None.

## Other changes
* **[JS]** Fixed typo in `WebSocketLibjoynrRuntime`, by using correct method
  `terminateAllSubscriptions`.

## Configuration property changes
None.

# joynr 1.7.0

## API relevant changes
* **[JEE]** Fixed typo withDicoveryQos (renamed method withDicoveryQos to withDiscoveryQos) in the
  ServiceProxyBuilder interface of the joynr ServiceLocator.
* **[JS]** Added `terminateAllSubscriptions` method to joynr.js which can be called to terminate all
  active subscriptions (i.e. for broadcasts, attributes' updates etc.).
* **[JS]** In the `process.exit` handler joynr.shutdown now needs to be explicitly called with
  `settings.clearSubscriptionsEnabled=false`.

## Other changes
* **[C++]** Enabled registration of multiple providers in a single runtime.
  ParticipantIdStorage now also stores major versions of providers.
* **[Java, JEE]** Provided an example for the usage of the message persistence feature. Check the
  `examples/message-persistence/` folder.
* **[JS]** Fixed an issue where joynr.shutdown would not wait for clearSubscriptions before shutting
  down.
* **[Java]** Eliminated declared but unneeded dependencies in some of the sub-projects. Also avoided
  defining versions inside the dependencyManagement for transitive dependencies not being directly
  used in joynr.
* **[Generator, Java]** The flag `addVersionTo` now also appends version information at file system
  level to generated types. See [Generator Documentation](generator.md) for additional information.
  The feature has been tested to work in Java, expected to work also in C++ and JS.
* **[C++, Java, JS]** Updated smrf version to 0.3.1

## Configuration property changes
  None.

# joynr 1.6.5

## API relevant changes
None.

## Other changes
* **[Java]** Update jackson to version 2.9.8

## Configuration property changes
None.

# joynr 1.6.4

## API relevant changes
None.

## Other changes
* **[Java]** Update Xtend and Xtext to latest version 2.16.0 to fix issue with transitive dependencies in generator.

## Configuration property changes
None.

# joynr 1.6.3

## API relevant changes
None.

## Other changes
* **[C++]** MQTT connection will not be attempted if MQTT TLS is enabled, but TLS certificates
  do not exist or are inaccessible or TLS options cannot be set.

## Configuration property changes
None.

# joynr 1.6.2

## API relevant changes
None.

## Other changes
* **[C++]** Improved handling of global and local capabilities in cluster-controller to better cope
  with overlapping registrations of the same provider.
* **[Java]** Update mqtt paho client to 0.0.6
* **[Java]** Do not discard message on publish if mqttClient is not yet created.
* **[Java]** Attempt mqtt reconnect also on SSL errors.
* **[Java]** Improved performance of mqtt client (use StampedLock).

## Configuration property changes
None.

# joynr 1.6.1

## API relevant changes
None.

## Other changes
* **[FIDL]** Introduce a new method in GlobalCapabilitiesDirectory which allows applications to get
  notified about changes in JDS. For more information refer to `GlobalCapabilitiesDirectory.fidl`.
* **[Java]** Removed `org.reflections` and `guava` dependencies from joynr runtime code. Both still
  remain needed in the joynr generator. However, it is not deployed when running joynr applications.

## Configuration property changes
None.

# joynr 1.6.0

## API relevant changes
* **[Java, JEE]** added `prepareForShutdown` lifecycle step
* **[Java, JEE]** Added stateless async communication API.
  See the [Java documentation](java.md) and [JEE documentation](jee.md) for details.
* **[Java, JEE]** Added `MessagePersister` interface that can be implemented and allows queued
  messages to be persisted in order to prevent message loss. JEE applications can inject a
  MessagePersister implementation. See the [Java documentation](java.md#message_persistence) and
  [JEE documentation](jee.md#message_persistence) for details.

## Other changes
* **[Java]** Guava (Google Core Libraries for Java) dependency has been removed from
  io.joynr.java.core.libjoynr and io.joynr.java.core.libjoynr-websocket-runtime

## Configuration property changes
* **[Java]** Introduced `PROPERTY_KEY_MQTT_USERNAME` and `PROPERTY_KEY_MQTT_PASSWORD`.
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** Introduced `PROPERTY_MESSAGE_QUEUE_SHUTDOWN_MAX_TIMEOUT`.
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** Introduced `PROPERTY_PREPARE_FOR_SHUTDOWN_TIMEOUT`.
  See [Java Configuration Reference](JavaSettings.md) for more details.
* **[JS]** Moved default settings for discoveryQos from `capabilities.discoveryQos` to `discoveryQos`
    because capabilities was used as an array.
* **[Java]** Introduced `MESSAGE_QUEUE_ID` property.
  See [Java Configuration Reference](JavaSettings.md) for more details.

# joynr 1.5.0

## API relevant changes
* **[Java]** Android support has been disabled until further notice.
* **[Java]** joynr now **requires Java 8** and the plain Java part is no longer compatible with Java 7 (which was previously required for Android).
* **[Java]** The key format of persisted provider participantIDs changed. It now includes the major
 version of the Franca interface implemented by the provider.
 * All providers which do not use a fixed participantID will be registered with a newly generated
   participantID. Existing proxies have to be rebuilt (the provider has to be discovered again).
 * Providers with a fixed participantId might have to be updated in order to use the new key format,
   see [Joynr Java Developer Guide](java.md#register-provider-with-fixed-%28custom%29-participantId)
   for the recommended way of registering providers with fixed participantID.
* **[Java]** Several dependencies have been upgraded. Please make sure to
  upgrade any applications / JEE server accordingly to avoid possible
  version conflicts.
  `mvn dependency:tree` from top-level pom.xml reports the following changes:
  * com.google.code.findbugs:annotations updated from version 2.0.1 to 3.0.1
  * com.google.code.findbugs:jsr305 updated from version 2.0.1 to 3.0.2
  * com.google.errorprone:error_prone_annotations added version 2.1.3
  * com.google.guava:guava updated from version 11.0.2 to 24.1-android
  * com.google.j2objc:j2objc-annotations added version 1.1
  * commons-pool:commons-pool updated from version 1.5.4 to 1.6
  * fish.payara.extras:payara-embedded-all updated from version 4.1.1.161 to 4.1.153
  * javassist:javassist removed version 3.12.1.GA
  * javax.ws.rs:javax.ws.rs-api updated from version 2.0.1 to 2.1-m03
  * net.jcip:jcip-annotations added version 1.0
  * net.sourceforge.serp:serp updated from version 1.14.1 to 1.15.1
  * org.apache.openjpa:openjpa-jdbc updated from version 2.3.0 to 2.4.2
  * org.apache.openjpa:openjpa-kernel updated from version 2.3.0 to 2.4.2
  * org.apache.openjpa:openjpa-lib updated from version 2.3.0 to 2.4.2
  * org.apache.openjpa:openjpa-persistence updated from version 2.3.0 to 2.4.2
  * org.apache.openjpa:openjpa-persistence-jdbc updated from version 2.3.0 to 2.4.2
  * org.apache.xbean:xbean-asm4-shaded removed version 3.14
  * org.apache.xbean:xbean-asm5-shaded added version 3.17
  * org.checkerframework:checker-compat-qual added version 2.0.0
  * org.codehaus.mojo:animal-sniffer-annotations added version 1.14
  * org.eclipse.emf:org.eclipse.emf.codegen removed version 2.11.0-v20150806-0404
  * org.eclipse.equinox:common removed version 3.6.200-v20130402-1505
  * org.eclipse.jdt:org.eclipse.jdt.compiler.apt added version 1.2.100
  * org.eclipse.jdt:org.eclipse.jdt.core added version 3.12.2
  * org.eclipse.platform:org.eclipse.core.commands added version 3.9.100
  * org.eclipse.platform:org.eclipse.core.contenttype added version 3.7.0
  * org.eclipse.platform:org.eclipse.core.expressions added version 3.6.100
  * org.eclipse.platform:org.eclipse.core.filesystem added version 1.7.100
  * org.eclipse.platform:org.eclipse.core.jobs added version 3.10.0
  * org.eclipse.platform:org.eclipse.core.resources added version 3.13.0
  * org.eclipse.platform:org.eclipse.core.runtime added version 3.14.0
  * org.eclipse.platform:org.eclipse.equinox.app added version 1.3.500
  * org.eclipse.platform:org.eclipse.equinox.common added version 3.10.0
  * org.eclipse.platform:org.eclipse.equinox.preferences added version 3.7.100
  * org.eclipse.platform:org.eclipse.equinox.registry added version 3.8.0
  * org.eclipse.platform:org.eclipse.osgi added version 3.13.0
  * org.eclipse.platform:org.eclipse.text added version 3.6.300
  * org.eclipse.tycho:org.eclipse.jdt.core removed version 3.10.0.v20140604-1726
  * org.eclipse.xtend:org.eclipse.xtend.lib updated from version 2.8.4 to 2.11.0
  * org.eclipse.xtend:org.eclipse.xtend.lib.macro updated from version 2.8.4 to 2.11.0
  * org.eclipse.xtext:org.eclipse.xtext updated from version 2.8.4 to 2.11.0
  * org.eclipse.xtext:org.eclipse.xtext.util updated from version 2.8.4 to 2.11.0
  * org.eclipse.xtext:org.eclipse.xtext.xbase updated from version 2.8.4 to 2.11.0
  * org.franca:org.franca.core updated from version 0.10.0 to 0.13.0
  * org.franca:org.franca.core.dsl updated from version 0.10.0 to 0.13.0
  * org.franca:org.franca.deploymodel.dsl updated from version 0.10.0 to 0.13.0
  * org.glassfish.hk2.external:aopalliance-repackaged updated from version 2.4.0-b31 to 2.5.0-b32
  * org.glassfish.hk2.external:javax.inject updated from version 2.4.0-b31 to 2.5.0-b32
  * org.glassfish.hk2:hk2-api updated from version 2.4.0-b31 to 2.5.0-b32
  * org.glassfish.hk2:hk2-locator updated from version 2.4.0-b31 to 2.5.0-b32
  * org.glassfish.hk2:hk2-utils updated from version 2.4.0-b31 to 2.5.0-b32
  * org.glassfish.jersey.bundles.repackaged:jersey-guava updated from version 2.21 to 2.26-b02
  * org.glassfish.jersey.core:jersey-client updated from version 2.21 to 2.26-b02
  * org.glassfish.jersey.core:jersey-common updated from version 2.21 to 2.26-b02
  * org.glassfish.jersey.core:jersey-server updated from version 2.21 to 2.26-b02
  * org.glassfish.jersey.media:jersey-media-jaxb updated from version 2.21 to 2.26-b02
  * org.glassfish.main.extras:glassfish-embedded-all updated from version 4.1.1 to 4.1.2
  * org.javassist:javassist updated from version 3.18.1-GA to 3.22.0-GA
  * org.jboss.arquillian.config:arquillian-config-api updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.config:arquillian-config-impl-base updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.container:arquillian-container-impl-base updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.container:arquillian-container-spi updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.container:arquillian-container-test-api updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.container:arquillian-container-test-impl-base updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.container:arquillian-container-test-spi updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.container:arquillian-glassfish-embedded-3.1 updated from version 1.0.0.CR4 to 1.0.2
  * org.jboss.arquillian.core:arquillian-core-api updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.core:arquillian-core-impl-base updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.core:arquillian-core-spi updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.extension:arquillian-transaction-api updated from version 1.0.3.Final to 1.0.5
  * org.jboss.arquillian.extension:arquillian-transaction-impl-base updated from version 1.0.3.Final to 1.0.5
  * org.jboss.arquillian.extension:arquillian-transaction-jta updated from version 1.0.3.Final to 1.0.5
  * org.jboss.arquillian.extension:arquillian-transaction-spi updated from version 1.0.3.Final to 1.0.5
  * org.jboss.arquillian.junit:arquillian-junit-container updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.junit:arquillian-junit-core updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.protocol:arquillian-protocol-servlet updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.test:arquillian-test-api updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.test:arquillian-test-impl-base updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.test:arquillian-test-spi updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-cdi updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-ejb updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-initialcontext updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.arquillian.testenricher:arquillian-testenricher-resource updated from version 1.1.11.Final to 1.1.15.Final
  * org.jboss.shrinkwrap.descriptors:shrinkwrap-descriptors-api-base updated from version 2.0.0-alpha-8 to 2.0.0
  * org.jboss.shrinkwrap.descriptors:shrinkwrap-descriptors-spi updated from version 2.0.0-alpha-8 to 2.0.0
  * org.jboss.shrinkwrap.resolver:shrinkwrap-resolver-api updated from version 2.2.0 to 2.2.6
  * org.jboss.shrinkwrap.resolver:shrinkwrap-resolver-api-maven updated from version 2.2.0 to 2.2.6
  * org.jboss.shrinkwrap.resolver:shrinkwrap-resolver-impl-maven updated from version 2.2.0 to 2.2.6
  * org.jboss.shrinkwrap.resolver:shrinkwrap-resolver-spi updated from version 2.2.0 to 2.2.6
  * org.jboss.shrinkwrap.resolver:shrinkwrap-resolver-spi-maven updated from version 2.2.0 to 2.2.6
  * org.jboss.shrinkwrap:shrinkwrap-api updated from version 1.2.3 to 1.2.6
  * org.jboss.shrinkwrap:shrinkwrap-impl-base updated from version 1.2.3 to 1.2.6
  * org.jboss.shrinkwrap:shrinkwrap-spi updated from version 1.2.3 to 1.2.6
  * org.reflections:reflections updated from version 0.9.8 to 0.9.10
* **[Java]** Proxy methods can now be passed an optional `io.joynr.messaging.MessagingQos` parameter.
  This allows to overwrite the `MessagingQos` which was specified during proxy building
  for each proxy method call separately.

## Other changes
* **[Java, C++, JS]** Introduced Franca 0.13.0, joynr continues to support
  Franca features that were already supported in previous joynr versions.
* **[Java]** Orphaned routing entries containing addresses for proxy participantIds
  related to no longer referenced proxy instances are now removed by the Routing Table cleanup
  background job, provided the related proxy instances have already been collected by the Java
  garbage collector.
  See also [Java Configuration Reference](JavaSettings.md) for more details about property
  `PROPERTY_ROUTING_TABLE_CLEANUP_INTERVAL_MS`.
  Note that the application is responsible for keeping a reference to the proxy instance until
  method calls have ended with either success or error and broadcasts / attribute subscriptions
  have been terminated since otherwise a potential reply or publication cannot be routed anymore
  once the routing entry has been removed.
* **[JS]** Replaced the joynr dependency smrf-native with smrf, which is a javascript only implementation
  of joynr's secure messaging format. This offers the following benefits.
    * simpler usage of joynr, as no native dependencies are remaining
    * improved performance for serialization and deserialization of messages.
    * prepares future browser support. Rewriting of the websocket related browser part still necessary.
* **[Java]**  Use `/` instead of `:` in shared subscription topic
* **[C++]** Added setting 'messaging/discard-unroutable-replies-and-publications'.
  See default-messaging.settings for more details. Do not enable this setting before all local joynr
  environments have been updated to joynr 1.5.0 or later.

## Configuration property changes
None.

# joynr 1.4.0

## API relevant changes
* **[JS]** Registration of global providers can be made waiting until registration has been
  propagated to GlobalCapabilitiesDirectory by passing an optional boolean flag `awaitGlobalRegistration`
  with value `true` to `registerProvider` or `settings.awaitGlobalRegistration` to `register` APIs of
  joynr.registration.

* **[Java]** Registration of global providers can be made waiting until registration has been
  propagated to GlobalCapabilitiesDirectory by passing a boolean flag `awaitGlobalRegistration`
  with value `true` to overloaded `registerProvider`. The `registerProvider` without the flag
  parameter will still trigger but no longer wait for registration at GlobalCapabilitiesDirectory.
  The default timeout for calls to the GlobalCapabilitiesDirectory has been shortened to
  60 seconds in order to be able to return result / timeout to the caller of `registerProvider`,
  automatically internally undo the local registration as well and allow for a later retry by
  the application.
  The JEE case where `registerProvider` is called internally based on annotations continues
  to use a very long default timeout for the call to GlobalCapabilitiesDirectory as before.

## Other changes
* **[C++, Generator]** Deleted InProcess bypass. Every message has to be now routed
  through message router.
* **[C++]** The application thread will not return immediately if persistency is ON. Persistency of subscriptions
  is being loaded in the same thread as registerProvider.

## Configuration property changes
* **[Java]** Introduced `PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS` to use separate MQTT connections.
  See [Java Configuration Reference](JavaSettings.md) for more details.

# joynr 1.3.2

## API relevant changes
None.

## Configuration property changes
None.

## Other changes
* **[C++]** Additional fix for wildcard storage structure.
* **[C++]** Use cluster-controller ID instead of receiverID in mqttClientID
* **[C++]** Allow local communication before initializing global communication

# joynr 1.3.1

## API relevant changes
None.

## Configuration property changes
None.

## Other changes
* **[C++]** Reduce lookups of subscriptionRequest via subscriptionId.
* **[C++]** Fixed race condition on parentResolveMutex in LibJoynrMessageRouter.
* **[C++]** Fix initialization sequence of libjoynr by calling addRequestCaller before addNextHop.
* **[C++]** Access control: build wildcard storage tree correctly and always return entire branch for lookups.

# joynr 1.3.0

## API relevant changes
* **[C++]** Registration of global providers can be made waiting until registration has been
  propagated to GlobalCapabilitiesDirectory by passing an optional bool flag `awaitGlobalRegistration`
  to `registerProvider` and `registerProviderAsync`.

## Configuration property changes
* **[Java]** Introduced `PROPERTY_KEY_MQTT_KEYSTORE_TYPE` and `PROPERTY_KEY_MQTT_TRUSTSTORE_TYPE` for
specifying the keystore/truststore type. See [Java Configuration Reference](JavaSettings.md)for more details.

## Other changes
* **[JS]** Removed the dependency to wscpp. Joynr uses the ws npm module for websocket
  communication instead.
* **[JS]** The minimum required node version changes to 8.0.0. to allow the usage of
  async await and util.promisify.

* **[C++]** Added a CMake flag `USE_PLATFORM_MOCOCRW` to download mococrw at build time.

# joynr 1.2.0

## API relevant changes
* **[Java]** Renamed property keys defining key- and truststore paths and passwords
used for secure MQTT connections.
See [Java Configuration Reference](JavaSettings.md)for more details.

## Other changes
* **[Generator]** Introduced a new flag `addVersionTo` to append version information at filesystem level
  (package or interface). See [Generator Documentation](generator.md) for additional information.

## Configuration property changes
* **[Java]** Introduced `PROPERTY_MESSAGING_COMPRESS_REPLIES` for compressing all outgoing replies by
  default. See [Java Configuration Reference](JavaSettings.md)for more details.

# joynr 1.1.2

## API relevant changes
None.

## Other changes
* **[JS]** fix issue with missing Object.prototype.hasOwnProperty builtin due to Object.create(null).

# joynr 1.1.1

## API relevant changes
None.

## Other changes
* **[C++]** Fixed race condition for subscriptions / publications.
* **[C++]** Fixed race condition in ParticipantIdStorage.
* **[JS]** fix browserify issue with missing GenerationUtil File.

# joynr 1.1.0

## API relevant changes
* **[C++]** Proxy methods can now be passed an optional `joynr::MessagingQos` parameter.
  This allows to overwrite the `MessagingQos` which was specified during proxy building
  for each proxy method call separately.
* **[C++]** Persistence of participantIds of providers can be disabled upon registration
  by passing an optional parameter to `registerProvider` and `registerProviderAsync`.
* **[Java]** The String constants `PROPERTY_BACKPRESSURE_ENABLED` and
  `PROPERTY_MAX_INCOMING_MQTT_REQUESTS` are moved from class ConfigurableMessagingSettings
  to LimitAndBackpressureSettings. Please adapt the import statements in case you
  use these constants directly.

## Javascript Memory and Performance Changes
* **[Generator]** Generated JS code will support only module.exports as default when exporting.
  This reduces the size of the generated code.
  There is a new generator option requireJSSupport, which will restore the old behavior.
  See the [joynr code Generator Reference](generator.md) for details.
* **[Generator]** Joynr Compound Types and Joynr Enums won't generate their own equals implementation,
  but use a more general implementation provided by libjoynr.
  Extracted some additional functionality to libjoynr by using mixins.
  This further reduces the size of the generated code.
  This change renders the generated code incompatible with previous joynr versions.
* **[JS]** Removed Object.freeze at several API relevant locations and thus allowing libjoynr to
  manipulate those objects freely. This allows joynr the usage of prototypes and thus saving many
  function allocations.
* **[JS]** Fixed a bug where all joynr Runtimes were required. Added a description how to avoid the
  same Problem when using browserify. See [Javascript Configuration Reference](JavaScriptSettings.md)
  for the detailed explanation.
* **[JS]** Replaced log4javascript with a simplified implementation. The same configuration interface
  is still supported apart from some advanced options.
  See [Javascript Configuration Reference](JavaScriptSettings.md) for the detailed explanation.
* **[JS]** Many other internal optimizations which avoid function allocations and thus unnecessary
  GC cycles.

## Configuration property changes
* **[Java]** Introduced `PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_UPPER_THRESHOLD`
  and `PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD` for controlling
  the backpressure mechanism. See [Java Configuration Reference](JavaSettings.md)
  for more details.
* **[Java]** Property `PROPERTY_BACKPRESSURE_MAX_INCOMING_MQTT_MESSAGES_IN_QUEUE`
  was renamed to `PROPERTY_MAX_INCOMING_MQTT_REQUESTS`. The new identifier is
  `joynr.messaging.maxincomingmqttrequests`. This change indicates that the property
  is not more related to a backpressure mechanism but is rather an independent
  self-protection mechanism of the instance from too heavy MQTT requests inflow.
  Furthermore, the default value of the property was changed from 20 to 0 (off).
  Hence, at default the mechanism is off and only the user can configure an appropriate
  value according to his application. See [Java Configuration Reference](JavaSettings.md)
  for more details.
* **[Java]** Removed property `PROPERTY_REPEATED_MQTT_MESSAGE_IGNORE_PERIOD_MS`.
  The behavior of the MqttMessagingSkeleton changes to immediate mqtt message acknowledgment
  and this should eliminate receiving repeated messages from the mqtt broker.
* **[C++]** newly added TLS properties `cluster-controller/mqtt-tls-version` and
  `cluster-controller/mqtt-tls-ciphers` can be used to fine tune the MQTT TLS connection
* **[JS]** changed default value of shutdownSettings.clearSubscriptionsEnabled to true.
  As default behavior Joynr will try to terminate active subscriptions when shut down.

## Other changes
* **[C++]** moved settings `local-capabilities-directory-persistence-file` and
  `local-capabilities-directory-persistency-enabled` from section [lib-joynr] to [cluster-controller].
* **[C++]** added setting 'cluster-controller/global-capabilities-directory-compressed-messages-enabled'
  which specifies whether messages to GlobalCapabilitiesDirectory shall be compressed.
  By default they will be sent uncompressed.
* **[Java]** Fixed a bug that was blocking shutdown if disconnected from MQTT at the same time.
* **[C++]** Upgrade muesli to version 1.0.1.
* **[Java]** joynr exposes status metrics which can be used to monitor instances. See
  [JEE Documentation](jee.md#status_monitoring) for more information on how to use this
  information for JEE and [Java Documentation](java.md#status_monitoring) for plain Java.

# joynr 1.0.5

## API relevant changes
None.

## Other changes
* **[Java]** Reduced cpu load and memory usage by reusing joynr internal proxies instead of
  building a new proxy for every proxy operation.
* **[Java, C++]** Enhanced log output to allow easier tracing of proxy calls: message ID and
  relevant payload are now logged when a joynr message is created to be able to relate later log
  output which only contains the message ID to the corresponding proxy call.
* **[Java]** use SMRF 0.2.3

## Configuration property changes
* **[Java]** Moved property PROPERTY_CAPABILITIES_FRESHNESS_UPDATE_INTERVAL_MS
  from LocalCapabilitiesDirectoryImpl.java to SystemServicesSettings.java.
* **[JS]** Default of `persistency.capabilities` changed to `true`

# joynr 1.0.4

## API relevant changes
None.

## Other changes
* **[C++]** The queue sizes for messages can now additionally be limited by setting the properties
  'cluster-controller/message-queue-limit-bytes' and / or
  'cluster-controller/transport-not-available-queue-limit-bytes' which specify
  the limit in bytes rather than number of messages. By default no queue limit is enforced.

# joynr 1.0.3

## API relevant changes
None.

## Other changes
* **[C++]** Fixed Mosquitto Connection start/stop handling
  Mosquitto background thread got not always joined correctly resulting in memory leak.
* **[C++]** JoynrRuntime::createRuntime APIs now internally catch all exceptions to
  avoid crashes; exceptions will be logged and distributed as JoynrRuntimeException
  to onError() callback, if provided

# joynr 1.0.2

## API relevant changes
None.

## Other changes
* **[Java]** joynr performs an explicit disconnect when the MQTT connection is lost in order to make
  a reconnect more robust.

# joynr 1.0.1

## Other changes

## Configuration property changes
* **[C++]** The queue size for messages, which can not be transmitted because the global transport
  is not available, can be limited by setting the `cluster-controller/transport-not-available-queue-limit`
  property. By default no queue limit is enforced.

# joynr 1.0.0
API Stable

## API relevant changes
None.

## Other changes
* **[C++]** It is now possible to add a limit to the message queue. When this limit
  is reached, the message with the smallest TTL will be removed. By default
  the limit is disabled and can be enabled with the property:
  * `cluster-controller/message-queue-limit`
* **[C++]** The message queue can also be limited per participant id (message's recipient).
  If the limit for a certain recipient is reached, the message with the smallest TTL
  for this recipient will be removed. By default the limit is disabled. In order to
  enable it, set following property: `cluster-controller/per-participantid-message-queue-limit`.
  The same limit value is used for all participant ids. This mechanism will only be enabled, if
  `cluster-controller/message-queue-limit` is set to a value greater than 0.
* **[JS]** Updated wscpp version to 1.0.0
* **[JS]** Verify arrays with Array.isArray() built-in function instead of
  "object.constructor === Array".
* **[Java]** Discovery entries returned by the discovery service will update the routing table.

## Configuration property changes
* **[JS]** See the [Javascript Configuration Reference](JavaScriptSettings.md) for
  details about the newly introduced properties:
  * `persistency`
    * `routingTable`
    * `capabilities`
    * `publications`

# joynr 0.33.1

## API relevant changes
None.

## Other changes
* **[C++]** fixed crash that could occur during shutdown of joynr runtime

# joynr 0.33.0

## API relevant changes
None.

## Configuration property changes
* **[Java]** See the [Java Configuration Reference](JavaSettings.md) for
  details about the newly introduced property:
  * `PROPERTY_DISCOVERY_GLOBAL_ADD_AND_REMOVE_TTL_MS`

## Other changes
* **[C++, Generator]** Fixed a problem with the generator when empty structures
  with extends were specified in the FIDL files.
* **[C++, Java, JS]** Removed DBUS and CommonAPI support
* **[Java, Generator]** Allow disabling of null checks in complex type member setters
  See the [joynr code Generator Reference](generator.md) for details.
  Note: Using types containing null values is incompatible with joynr C++.

# joynr 0.32.2

## API relevant changes
None.

## Other changes
* **[C++]** use correct uid in AccessControlListEditor provider

# joynr 0.32.1

## API relevant changes
None.

## Other changes
* **[C++]** Fixed routing of multicast publications when access controller is enabled:
  route message to all recipients instead of routing it to first found address only

# joynr 0.32.0

## API relevant changes
None.

## Configuration property changes
* **[Java]** See the [Java Configuration Reference](JavaSettings.md) for
  details about the newly introduced property:
  * `PROPERTY_KEY_MQTT_CLEAN_SESSION`
  * `PROPERTY_KEY_MQTT_KEYSTORE_PATH`
  * `PROPERTY_KEY_MQTT_TRUSTSTORE_PATH`
  * `PROPERTY_KEY_MQTT_KEYSTORE_PWD`
  * `PROPERTY_KEY_MQTT_TRUSTSTORE_PWD`
* **[JS]** See the [Javascript Configuration Reference](JavaScriptSettings.md) for
  details about the newly introduced properties:
  * `shutdownSettings`
    * `clearSubscriptionsEnabled`
    * `clearSubscriptionsTimeoutMs`

## Other changes
* **[Java]** added support for MQTT via TLS
* **[C++]** Disabled persistency for the routing table, local discovery cache and multicast
  receiver directory by default. Persistency for subscriptions is still enabled. The
  corresponding properties are called `lib-joynr/message-router-persistency`,
  `lib-joynr/local-capabilities-directory-persistency-enabled`,
  `cluster-controller/multicast-receiver-directory-persistency-enabled` and
  `lib-joynr/subscription-persistency`.
* **[C++]** Update spdlog to interim version >> 0.14.0
  [Git commit 799ba2a57bb16b921099bd9ab64a513e4ebe4217]

# joynr 0.31.1

## API relevant changes
None.

## Other changes
* **[C++]** Fixed provider reregistration trigger call

# joynr 0.29.3

## API relevant changes
None.

## Other changes
* **[C++]** Fixed provider reregistration trigger call

# joynr 0.31.0

## API relevant changes
* **[JS]** Joynr now uses node native Promise if available, otherwise it falls
  back to bluebird as before. Please make sure to use only Promise APIs available
  in the node standard.

## Configuration property changes
* **[C++]** allow to load multiple ACL/RCL Json files for the cluster-controller.
  * `cluster-controller/acl-entries-directory`

* **[C++]** make the delay of the attempt to reconnect in mosquitto loop exponential until the connection succeeds.
     By default this property is not enabled mqtt-exponential-backoff-enabled = false.
  * `messaging/mqtt-reconnect-max-delay`
  * `messaging/mqtt-exponential-backoff-enabled`

## Other changes
* **[C++, Java, JS]** Updated smrf version to 0.2.2

# joynr 0.30.1

## API relevant changes
None.

## Configuration property changes
* **[Java]** PROPERTY_MAX_INCOMING_MQTT_MESSAGES_IN_QUEUE and PROPERTY_REPEATED_MQTT_MESSAGE_IGNORE_PERIOD_MS
 were renamed in order to reflect that they are related to the backpressure
 mechanism. PROPERTY_MAX_INCOMING_MQTT_MESSAGES_IN_QUEUE is now called
 PROPERTY_BACKPRESSURE_MAX_INCOMING_MQTT_MESSAGES_IN_QUEUE and its identifier is
 `joynr.messaging.backpressure.maxincomingmqttmessagesinqueue`.
 PROPERTY_REPEATED_MQTT_MESSAGE_IGNORE_PERIOD_MS is now called
 PROPERTY_BACKPRESSURE_REPEATED_MQTT_MESSAGE_IGNORE_PERIOD_MS and its identifier
 is `joynr.messaging.backpressure.repeatedmqttmessageignoreperiodms`.
* **[Java]** Introduced `PROPERTY_BACKPRESSURE_ENABLED`. See [Java Configuration Reference](JavaSettings.md) for
   details about the new property. The backpressure mechanism is disabled by default.

## Other changes
* **[Java]** If a joynr instance receives a reply for which no receiver exists,
  it will be dropped immediately.
* **[JS]** Once loaded, joynr automatically calls joynr.shutdown() when
  process.exit(...) is called; a loaded joynr thus no longer prevents the
  application from terminating.
* **[JS]** Enhanced tracing output; log level 'debug' now logs messages with
  full details, log level 'info' logs with reduced details (w/o parameters,
  response values, publication details)
* **[JS]** Handle uncaught errors from MessageRouter.route to prevent crashes
  when an incoming message cannot be processed
* **[JS]** Members of top level struct method parameters are now checked for
  existance.
* **[C++]** Do not block main thread for asynchronous provider registration.
* **[C++]** implemented ACL audit mode, which allows to audit whether ACL/RCL
  is configured correctly. By default, it is turned off and can be activated
  via `access-control/audit`.
* **[C++]** Not possible to remove access control from C++ build. At runtime is
   disabled by default and can be enabled via setting the configuration property `access-control/enable`
   in the cluster controller settings.

# joynr 0.30.0

## API relevant changes
* **[Java]** Setters for class members of Java classes
  representing Franca structs no longer accept null
  values. An InvalidArgumentException will be thrown
  in case a setter is called with null.

  The reason is that null values cause incomplete JSON
  objects to be serialized which cannot be deserialized
  on C++ side.

## Configuration property changes
* **[C++]** Made the following properties configurable. See
  default-messaging.settings for more details.
  * `routing-table-grace-period-ms`
  * `routing-table-cleanup-interval-ms`

## Other changes
* **[C++]** The internal routing table can now be cleaned up.
  Routing entries which have been created for handling a request from
  global can be removed when the ttl + grace period has passed.
* **[C++]** TLS client certificates for secure websocket connections with empty common name (CN)
  field are not accepted any longer.
* **[C++]** If a LibJoynr instance was configured to use SSL but no keychain
 was provided to the JoynrRuntime::create method, it will report a fatal error
 and throw a JoynrRuntimeException.
* **[C++]** Access control is included in the c++ build by default. It still must be enabled
 by setting the configuration property `access-control/enable`.

# joynr 0.29.2

## API relevant changes
None.

## Other changes
* **[C++]** The class `Url` now accepts IPv6 addresses in hexadecimal format.
  * If the constructor is called with a single URL string then the IPv6 hex part
  must be enclosed by square brackets as follows:
  ```
  Url("https://user:password@[abcd:dcba:0123:3210:4567:7654:3456:6543]:4040/script?query=somequery#fragment")
  ```
  Please note: Symbolic hostnames of hosts reachable via IPv6 must be specified
  as usual without square brackets.
  * If the constructor is called with multiple parameters, only the address is
  to be used without square brackets, example:
  ```
  Url("https", "user", "password", "abcd:dcba:0123:3210:4567:7654:3456:6543", ...)
  ```
* **[C++]** Properties whose value is of URL format (e.g. `broker-url`) can now be
  configured with URL strings containing IPv6 addresses in hexadecimal format as
  accepted by the `Url` class.

# joynr 0.29.1

## API relevant changes
None.

## Other changes
* **[C++]** Introduced ProviderReregistrationController interface which is implemented
 by the cluster-controller and can be accessed by creating the corresponding proxy.
 It allows to trigger the re-registration of globally visible providers which are
 registered at the corresponding cluster-controller instance.
* **[JS]** Use require instead of requirejs.

# joynr 0.29.0

## API relevant changes
* **[C++]** JoynrRuntime::createRuntimeAsync and JoynrRuntime::createRuntime now accept an
  optional IKeychain argument. See the [C++ Documentation](cplusplus.md) for more information.

## Configuration property changes
* **[Java]** See the [Java Configuration Reference](JavaSettings.md) for
  details about these newly introduced properties:
  * `PROPERTY_DISCOVERY_DEFAULT_TIMEOUT_MS`
  * `PROPERTY_DISCOVERY_RETRY_INTERVAL_MS`
  * `PROPERTY_KEY_MQTT_MAX_MESSAGE_SIZE_BYTES`
  * `PROPERTY_MAX_DELAY_WITH_EXPONENTIAL_BACKOFF_MS`
  * `PROPERTY_REPEATED_MQTT_MESSAGE_IGNORE_PERIOD_MS`
  * `PROPERTY_ROUTING_MAX_RETRY_COUNT`
* **[Java]** Renamed property `PROPERTY_MAX_MESSAGES_INQUEUE` to
  `PROPERTY_MAX_INCOMING_MQTT_MESSAGES_IN_QUEUE`. Please note that
  joynr will ignore messages if the MQTT queue is full by not sending an PUBACK
  for the message (QOS 1). joynr requires that the message is resend by the MQTT
  broker at a later point in time. If the resend time intervall of the broker is
  too high, an additional delay will be introduced. Please make sure to set the
  resend intervall of your MQTT broker appropriately.
* **[JS]** Made default discoveryQos configurable via provisioning. See the
  [Javascript Configuration Reference](JavaScriptSettings.md) for more details.
* **[C++]** Made the following properties configurable. See
  default-messaging.settings for more details.
  * `discovery-default-retry-interval-ms`
  * `discovery-default-timeout-ms`
  * `mqtt-max-message-size-bytes`
* **[C++, Java, JS]** Changed default values for the following properties:
  * Discovery expiry interval set to 6 weeks
  * Discovery timeout interval set to 10 minutes
  * Discovery retry interval set to 10 seconds

## Other changes
None.

# joynr 0.28.1

## API relevant changes
None.

## Other changes
* **[JS]** Changed node node-localstorage to node-persist to avoid too long filenames
* **[C++]** LocalCapabilitiesDirectory does not store multiple entries for a single participantId

# joynr 0.28.0

## API relevant changes
* **[C++, API]** Ease implementation of SubscriptionListener for empty broadcasts.
* **[C++, API]** createJoynrRuntime*(...) APIs now return shared_ptr instead of unique_ptr
* **[C++, API]** createProxyBuilder*(...) APIs now return shared_ptr instead of unique_ptr
* **[C++, API]** proxyBuilder->build*(...) APIs now return shared_ptr instead of unique_ptr

## Other changes
* **[C++]** joynr accepts files which have size at most 2 GB.
* **[Java, Properties]** Changed default values of joynr.messaging.mqtt.keepalivetimersec (new value: 30s) and
 joynr.messaging.mqtt.connectiontimeoutsec (new value: 60s)
* **[JS]** Updated wscpp version to 0.2.4
* **[C++, Java, JS]** Updated smrf version to 0.2.1
* **[C++]** Add cluster-controller property for MQTT CA certificate folder path.
* **[C++, Java]** Always log MQTT client ID.

# joynr 0.27.4

## API relevant changes
None.

## Other changes
* **[C++]** Fixed crash which can occur when a queued message cannot be routed due to expired timeout.

# joynr 0.27.3

## API relevant changes
None.

## Other changes
* **[C++]** Fixed crash which occurs when a LibJoynrRuntime is destroyed before the init method was called.

# joynr 0.27.2

## API relevant changes
None.

## Other changes
* **[JEE]** Applications can inject a MqttClientIdProvider to generate an id for the mqtt client.
  The producer method must be annotated with JoynrMqttClientIdProvider.
* **[Java, C++, JS]** updated SMRF dependency to 0.2.0 which introduces an incompatibility with any previous version
* **[C++]** Fixed potential crash when a proxy is used after a joynr runtime was deleted.

# joynr 0.27.1

## API relevant changes
None.

## Other changes
* **[JEE]** Fixed cleanup of thread pool when application is undeployed
* **[C++]** Mosquitto Connection uses now internal Mosquitto Loop thread handling.
* **[C++]** Cleaned up MessagingSettings. All settings which correspond to `seconds` time unit,
  have now `seconds` suffix.

# joynr 0.27.0

## API relevant changes
* **[C++]** virtual methods (such as `clone()`) are only generated for polymorphic datatypes
* **[C++]** floating point members of generated datatypes are now compared w.r.t. a specific precision;
  `operator==` uses a fixed precision of `4` ULPs (Units in the Last Place); the `equals()` method can be used to perform
  comparison with a custom precision

## Other changes
* **[C++]** The cluster controller can be configured in such a way that access control checks are performed
  only with locally provisioned or cached data. The configuration is done by using the
  `access-control/use-ldas-only` property in the cluster controller settings.
* **[Java]** Introduced property joynr.messaging.maxmessagesinqueue to restrict number of messages being processed
  at a time. See the [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** Introduced property joynr.messaging.routingtablegraceperiodms.
  See the [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** Introduced property joynr.messaging.routingtablecleanupintervalms.
  See the [Java Configuration Reference](JavaSettings.md) for more details.
* **[Java]** RawMessagingPreprocessor will be injected in JeeMqttMessagingSkeletonProvider correctly

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
* **[Java, C++, JS]** HTTP communication is not supported at the moment
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
None.

## Other changes
* **[C++]** setting "discovery-entry-expiry-interval-ms" can now store values up to 2^63-1

# joynr 0.25.2

## API relevant changes
None.

## Other changes
* **[C++]** libCommon has been moved to libJoynr. This fixes issues with static linking with libjoynr.
* **[JEE]** Shutdown MQTT client when undeploying WebApp.

# joynr 0.25.1

## API relevant changes
None.

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
None.

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
None.

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

## API relevant changes
None.

## Other changes
* **[C++]** Bugfix: MQTT sender blocks message router thread in case of connection to broker not
  established.

# joynr 0.22.1

## API relevant changes
None.

## Other changes
* **[JS]** Bugfix: For non-selective broadcast subscriptions the listeners could be called too
  often if multiple matching listeners were found.

# joynr 0.21.4

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

## API relevant changes
None.

## Other changes
* **[JS]** Fix bug which prevents successful restore of persisted broadcast subscription
  requests

# joynr 0.21.2

## API relevant changes
None.

## Other changes
* **[C++]** Fix cluster controller crash if many persisted discovery entries are present

# joynr 0.21.1

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

## API relevant changes
None.

## Other changes
* **[C++]** Fixed an issue which caused a high CPU load when a client disconnected from a
  cluster controller.

# joynr 0.20.3

## API relevant changes
None.

## Other changes
* **[JS]** Fix bug which resulted in improper shutdown of joynr.

# joynr 0.20.2

## API relevant changes
None.

## Other changes
* **[JS]** Fixed bug which caused exception when loading persisted
  subscriptions during startup.

# joynr 0.20.1

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

## API relevant changes
None.

## Other changes
* **[C++]** Fix multi-threading issue in LocalCapabilitiesDirectory.

# joynr 0.19.4

## API relevant changes
None.

## Other changes
* **[C++]** Correctly load persisted routing table in the LibJoynrRuntime.

# joynr 0.19.3

## API relevant changes
* **[C++]** Add new API to create joynr runtime with settings object.

## Other changes
* **[JS]** Support attributes starting with capital letters.

# joynr 0.19.2

## API relevant changes
None.

## Other changes
* **[C++]** Do not crash joynr runtime if writing persistency files fails.

# joynr 0.19.1

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

## API relevant changes
None.

## Other changes
* **[JEE]** Fixed bug with multi-out return values not being translated
  between container classes and multi-valued deferred instances in the
  `ProviderWrapper`.

# joynr 0.18.4

## API relevant changes
None.

## Other changes
* **[C++]** Fixed high cpu load which occurs when the system time is changed
* **[C++]** Fixed persistency of local capability entries
* **[C++]** Stability fixes for proxy arbitration
* **[JS]** Added reconnect after connection loss for websockets
* **[JS]** Support to clear local storage when loading joynr library

# joynr 0.18.3

## API relevant changes
None.

## Other changes
* **[Java]** Enabled Discovery and ACL addresses to use MQTT
* **[JEE]** Introduced example radio JEE app

# joynr 0.18.2

## API relevant changes
None.

## Other changes
* **[JS]** Fixed bug when using joynr with node version >= 6

# joynr 0.18.1

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

## API relevant changes
None.

## Other changes
* **[JS]** Updated dependency for atmoshpere.js to version 2.3.2. This ensures that
  joynr has no native dependencies in its npm package.

# joynr 0.17.1

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
 * Add suffix "_Ms_" to timing related subscription QoS constants (default, min and max values).
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

## API relevant changes
None.

## Other changes
* **[C++]** Fix segmentation fault in cluster-controller when a libjoynr disconnects.
* **[C++]** Define proper import targets for Mosquitto in the joynr package configuration.
* **[Java]** Use correct MQTT topics to fix incompatibilities with joynr C++.
* **[Java]** Improved stability in websocket implementation.

# joynr 0.15.0

## Notes
* **[Java, C++]** Java and C++ cluster controllers are now able to communciate to an MQTT broker as
  a replacement, or in addition to, the original bounceproxy. Java uses the Eclipse Paho client,
  while C++ uses mosquitto as an MQTT client.
* **[C++]** There is a new build and runtime dependency for the clustercontroller to mosquitto 1.4.7
* **[Java]** Handling of different transport middlewares has been refactored to be much more
  extensible. Using Guice Multibinders, it is now possible for external projects to add transport
  middleware implementations and inject these into the runtime. See the ```joynr-mqtt-client```
  project for an example of how this can be done.
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

## API relevant changes
None.

## Other changes
* **[C++]** Removed absolute paths from export targets for the install tree.
* **[C++]** Fix segmentation fault in cluster-controller checkServerTime function.
* **[C++]** Added /etc/joynr to settings search path. This is a workaround for builds with
  incorrect CMAKE_INSTALL_PREFIX.

# joynr 0.14.2

## API relevant changes
None.

## Other changes
* **[C++]** Fix dependency resolution in the CMake package config file for joynr.

# joynr 0.14.1

## API relevant changes
None.

## Other changes
* **[JS]** Fixed bug in generated proxies with broadcast subscription requests
  having no filters.

# joynr 0.14.0

## Notes
* **[Java, JS, C++]** Franca `ByteBuffer` is supported.
* **[Java, JS, C++]** Franca `typedef` is supported. For Java and JS, typedefs
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
* **[Java, JS, C++]** The JSON serializer in all three languages escapes already escaped
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
* **[Java, JS, C++]** Support Franca type Map
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

## API relevant changes
None.

## Other changes
* **[C++]** Selective broadcasts of basic types generate compilable code.

# joynr 0.12.2

## API relevant changes
None.

## Other changes
* **[C++]** Generated enum throws exception in the getLiteral method in case of an
  unresolved value.

# joynr 0.12.1

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
* **[Java, JS, C++]** Harmonized the handling of minimum interval for subscriptions with
  OnChangeSubscriptionQos. Set the MIN value to 0 ms.
* **[Java, JS, C++]** Harmonized the handling of subscription qos parameters for broadcast
  subscriptions. If two subsequent broadcasts occur within the minimum interval, the
  latter broadcast will not be sent to the subscribing entity.

## Other changes
* **[C++]** Fixed bug causing a consumer to crash when subscribing to attributes of type
  enumeration
* **[JS]** Support of methods with multiple output parameters
* **[Java, C++]** Fixed bug with arrays as return parameter types of methods and
  broadcasts and as attribute types of subscriptions
* **[Tooling]** The joynr generator ignores invalid Franca models, and outputs a list of errors to
  the console.

# joynr 0.11.1

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
* **[Java, JS]** Support of exceptions for methods/attributes. Exceptions at provider side are now
  communicated via joynr to the consumer, informing him about unexpected behavior. joynr providers
  are able to reject method calls by using error enum values as associated with the method in the
  Franca model.
* **[JS]** The callback provided at broadcast subscription is now called with key value pairs for
  the broadcast parameters. Previously, the callback has been invoked with individual function
  arguments for each broadcast parameter.
* **[Java, JS, C++]** Harmonized the handling of expiry dates in SubscriptionQos

## Other changes
* **[C++]** Replaced QSharedPointer with std::shared_ptr
* **[C++]** Replaced QDatetime with std counterpart "chrono"
* **[C++]** Replaced log4qt with spdlog
* **[C++]** Fixed bug which prevented the onError callback of async method calls to be called in
  case of unexpected behavior (e.g. timeouts)
* **[Java, JS, C++]** Fixed bug which caused joynr message loss due to wrong time interpreation in
  case of very high expiry dates.
* **[Java, JS]** Enriched the radio example with exception handling

# joynr 0.10.2

## API relevant changes
None.

## Other changes
* **[JS]** Reworked the handling of enums defined in Franca models.
  This resolves issues when using enums as input/output parameter of
  methods in JavaScript.

# joynr 0.10.1

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

## API relevant changes
None.

## Other changes
* **[C++]** Problems with receiving messages in libjoynr via WebSockets have been resolved.
* **[Java, C++]** Default domain for backend services is now "io.joynr".

# joynr 0.9.1

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

## Other changes
None.

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
