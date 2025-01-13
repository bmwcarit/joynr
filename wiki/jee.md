# joynr JEE Integration

The `io.joynr.jeeintegration` project provides an integration layer for
joynr in JEE Applications.
The features supported are:

* Expose session beans as joynr providers via the `@ServiceProvider` annotation
* Inject a `ServiceLocator` in order to obtain consumer proxies for calling
other services
* Internally use EE container managed thread pools
* Provide mqtt based cluster-aware communication abilities
* Use the EE container's JAX RS to receive joynr messages via HTTP(s)
* Provide joynr configuration via an EJB

There is also an example application based on the Radio App example. See the end of this
document for a description of the example.

## Installation

If you're building from source, then you can build and install the artifact to
your local maven repo with:
`mvn install`

## Usage

This section describes the general usage of the joynr JEE integration module
and provides simple examples of the usage where possible.

### Build

Add the `io.joynr.java:jeeintegration` dependency to your project.

For Maven, use the following dependency:

```xml
<dependency>
  <groupId>io.joynr.java</groupId>
  <artifactId>jeeintegration</artifactId>
  <version>${joynr.version}</version>
</dependency>
```

For Gradle, in your build.gradle dependencies add:
`compile 'io.joynr.java:jeeintegration:${joynr.version}'`

### Application configuration

Create a `@Singleton` EJB which has no business interface (i.e. does not
implement any interfaces) and provide methods annotated with:

* `@JoynrProperties`
	* The method decorated with this annotation must return a `Properties`
	  object which contains the joynr properties which the application
	  wants to set.
* `@JoynrLocalDomain`
	* The method decorated with this annotation must return a string value
	  which is the local domain used by the application to register all
	  providers with.

Additionally, the method must be annotated with
`jakarta.enterprise.inject.Produces`.

#### Mandatory Properties
See Java Configuration Reference for more details on the available properties.

* `MessagingPropertyKeys.CHANNELID` - this property should be set to the
application's unique DNS entry, e.g. `myapp.mycompany.net`. This is important,
so that all nodes of the cluster are identified by the same channel ID.
* `ConfigurableMessagingSettings.PROPERTY_GBIDS` - use this to configure the GBIDs for
the backends to be used, e.g. `gbid1,gbid2`.
* `MqttModule.PROPERTY_MQTT_BROKER_URIS` - use this to configure the URLs for
the backends identified by `ConfigurableMessagingModule.PROPERTY_GBIDS`.
If used, the number of configured broker-uris must be equal to the number of configured GBIDs.
E.g. `tcp://mqtt.mycompany.net:1883,tcp://mqtt.othercompany.net:1883`.

 #### Optional Properties

* `MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS` - enables the [HiveMQ](http://www.hivemq.com) specific 'shared subscription' feature, which allows clustering of JEE applications using just MQTT for communication. Set this to `true` to enable the feature. Defaults to `false`.
* `MessagingPropertyKeys.PERSISTENCE_FILE` - if you are deploying multiple joynr-enabled
applications to the same container instance, then you will need to set a different filename
for this property for each application. E.g.: `"my-app-joynr.properties"` for one and
`"my-other-app-joynr.properties"` for another. Failing to do so can result in unexpected
behaviour, as one app will be using the persisted properties and IDs of the other app.
* `JeeIntegrationPropertyKeys.PROPERTY_KEY_JEE_SUBSCRIBE_ON_STARTUP` - allows disabling the automatic subscription to the MQTT topic when the runtime starts. If the automatic subscription has been disabled this way, it has to be triggered manually through the `JoynrConnectionService` (see [below](#disabling-automatic-subscriptions) for more details).
  Defaults to `true`.

You are principally free to provide any other valid joynr properties via these
configuration methods. See the [official joynr documentation](./JavaSettings.md)
for details.

#### Example

An example of a configuration EJB is:

```java
@Singleton
public class JoynrConfigurationProvider {

    @Produces
    @JoynrProperties
    public Properties joynrProperties() {
        Properties joynrProperties = new Properties();
        joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID,
            "provider.domain");
        joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS,
            Boolean.TRUE.toString());
        joynrProperties.setProperty(ConfigurableMessagingSettings.PROPERTY_GBIDS,
            "joynrdefaultgbid");
        joynrProperties.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS,
            "tcp://mqttbroker.com:1883");

        return joynrProperties;
    }

    @Produces
    @JoynrLocalDomain
    public String joynrLocalDomain() {
        return "provider.domain";
    }

}
```


### JEE Container configuration

Configure your container runtime with a `ManagedScheduledExecutorService`
resource which has the name `'concurrent/joynrMessagingScheduledExecutor'`.

For example for Glassfish/Payara:
`asadmin create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor`

Note the `--corepoolsize=100` option. The default will only create one thread, which can lead to
blocking. 100 threads should be sufficient for quite a few joynr applications.

Depending on your load, you can experiment with different values. Use higher values to enable more
concurrency when communicating joynr messages.

As a rule of thumb consider

```
corepoolsize =
    (
        5 + joynr.messaging.maximumParallelSends
        + ((joynr.messaging.mqtt.separateconnections == true) ? 5 : 0 +
           10) * number of connected brokers/GBIDS
        + any number of additional threads the application needs internally
    ) * numberOfJoynrRuntimes per container
```

Typically there is one joynr runtime per deployed application (WAR file).

#### Payara Micro

When deploying to a Payara Micro instance, you can package the configuration in a separate
text file, e.g. `post-boot.txt`, and then have Payara Micro execute this automatically by
specifying the command line option `--postbootcommandfile post-boot.txt`.

With the postboot command file, you don't specify the `asadmin` command explicitely, as
each line is executed as if called with it. Hence, the content of the file will be:

    create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor

When using the `payara-micro-maven-plugin`, specify the command line options as:

```xml
<commandLineOptions>
    <option>
        <key>--postbootcommandfile</key>
        <value>${basedir}/post-boot.txt</value>
    </option>
</commandLineOptions>
```

Lastly, if you are using Docker to create a container with your Payara Micro application, then
specify the `ENTRYPOINT` as:

    ENTRYPOINT ["java", "-jar", "/app.jar", "--postbootcommandfile", "/post-boot.txt"]

Where `/app.jar` is the name of the uberjar you created with Payara Micro and your application.

To see a full example of this have a look at the
[custom-headers example project](../examples/custom-headers/README.md).


### Generating the interfaces

When generating the interfaces for use in a JEE environment, use the Java code generator, see
[the generator documentation](generator.md) for further information about the generator.

Here's an example of what the plugin configuration might look like:

```xml
<plugin>
  <groupId>io.joynr.tools.generator</groupId>
  <artifactId>joynr-generator-maven-plugin</artifactId>
  <version>${joynrVersion}</version>
  <executions>
    <execution>
      <id>generate-code</id>
      <phase>generate-sources</phase>
      <goals>
        <goal>generate</goal>
      </goals>
      <configuration>
        <model>${basedir}/src/main/resources/fidl</model>
        <generationLanguage>java</generationLanguage>
        <outputPath>${project.build.directory}/generated-sources</outputPath>
      </configuration>
    </execution>
  </executions>
  <dependencies>
    <dependency>
      <groupId>io.joynr.tools.generator</groupId>
      <artifactId>java-generator</artifactId>
      <version>${joynrVersion}</version>
    </dependency>
  </dependencies>
</plugin>
```


### Implementing services (joynr providers)

Annotate your business beans with `@ServiceProvider` additionally to the usual
JEE annotations (e.g. `@Stateless`).

For example, if we have defined a `MyService` interface in Franca for which
we want to provide an implementation, then:

```java
@ServiceProvider(serviceInterface = MyServiceSync.class)
@Stateless
public class MyServiceImpl implements MyServiceSync {
    ...
}
```

#### Provider registration: retries and error handling

Joynr JEE integration automatically registers the providers in the GlobalCapabilitiesDirectory with
the default registration parameters during startup (deployment).

In case of errors, it will retry the registration until it succeeds or the maximum number of retry
attempts is reached. Once a provider could be registered successfully, the loop continues with the
next provider.  
The retry behavior can be configured with the following properties:
* [JeeIntegrationPropertyKeys.PROPERTY_JEE_PROVIDER_REGISTRATION_RETRIES](JavaSettings.md#property_jee_provider_registration_retries)
* [JeeIntegrationPropertyKeys.PROPERTY_JEE_PROVIDER_REGISTRATION_RETRY_INTERVAL_MS](JavaSettings.md#property_jee_provider_registration_retry_interval_ms)
If the registration does not succeed within the configured limit, the deployment of the application
will fail with an exception that reports the registration problems.

> Note:
> A single registration attempt may take up about to 60 seconds. The next registration attempt
> will be triggered after the configured retry interval.  
> The timout for the deployment in your application server may be reached before all registration
> retries are finished.


#### <a name="provider_domain"></a> Customizing the registration domain

In some cases you might want to register your providers under a different domain than the
application default (specified via `@JoynrLocalDomain`, see configuration documentation above).

In order to do so, provide an implementation of `ProviderRegistrationSettingsFactory` (see
the following section) or use the `@ProviderDomain` annotation on your implementing bean
in addition to the `@ServiceProvider` annotation.

The value you provide will be used as the domain when registering the bean as a joynr provider.
The `@ProviderDomain` annotation will only be used if your `ProviderRegistrationSettingsFactory`
does not override the method `createDomain` or you do not provide an implementation of
`ProviderRegistrationSettingsFactory` at all.

Here is an example of what the `@ProviderDomain` annotation looks like:

```java
@ServiceProvider(serviceInterface = MyServiceSync.class)
@ProviderDomain(MY_CUSTOM_DOMAIN)
private static class CustomDomainMyServiceBean implements MyServiceSync {
    ...
}
```


#### Customizing the provider registration parameters

In order to set parameters like Provider QoS, GBID(s) or domain, provide an implementation of the
`ProviderRegistrationSettingsFactory` interface. 

You can provide multiple beans implementing this interface. The first one found which returns
`true` to `providesFor(serviceInterface, serviceProviderBean)` for a given service interface and
implementing class will be used by joynr to obtain settings needed to perform the service registration.
There is no guarantee in which order the factories will be asked, so it's safer to just provide
one factory for a given service interface type and its implementation.

By default (if you do not provide an implementation for `providesFor(serviceInterface, serviceProviderBean)`),
the `serviceProviderBean` class is ignored and the selection of a matching factory is just based on the
`serviceInterface` class. This makes the extended `providesFor` method backwards compatible with the original
implementation that just considered the service interface.

If no factory is found to provide settings, then the joynr runtime uses default values.

Note that you can also implement the interface just partially with the settings you care about. For
the rest the default methods of the interface will be invoked. Here is an example of what a (full)
implementation could look like for the `MyService` we used above:

```java
@Singleton
public class MyServiceProviderSettingsFactory implements ProviderRegistrationSettingsFactory {

    @Override
    public ProviderQos createProviderQos() {
        ProviderQos providerQos = new ProviderQos();
        providerQos.setCustomParameters(new CustomParameter[]{ new CustomParameter("name", "value") });
        providerQos.setPriority(1L);
        return providerQos;
    }

    @Override
    public String[] createGbids() {
        // Make sure these GBIDs are valid and are part of ConfigurableMessagingSettings.PROPERTY_GBIDS
        String[] gbidsForRegistration = { "testbackend1", "testbackend2" };
        return gbidsForRegistration;
    }

    @Override
    public String createDomain() {
        String domain = "testdomain";
        return domain;
    }

    @Override
    public boolean providesFor(Class<?> serviceInterface) {
        return MyServiceSync.class.equals(serviceInterface);
    }

    @Override
    public boolean providesFor(Class<?> serviceInterface, Class<?> serviceProviderBean) {
        return (MyServiceSync.class.equals(serviceInterface) && MyServiceBean.class.equals(serviceProviderBean));
    }

}
```

#### <a name="publishing_multicasts"></a> Publishing Multicasts

If you have `broadcast` definitions in your Franca file which are __not__ `selective`, then
you can `@Inject` the corresponding `SubscriptionPublisher` in your service implementation
and use that to fire multicast messages to consumers.

In order for the injection to work, you have to use the generated, specific subscription
publisher interface, and have to additionally decorate the injection with the
`@io.joynr.jeeintegration.api.SubscriptionPublisher` qualifier annotation.

Here is an example of what that looks like:

```java
@Stateless
@ServiceProvider(serviceInterface = MyServiceSync.class)
public class MyBean implements MyServiceSync {
    private MyServiceSubscriptionPublisher myServiceSubscriptionPublisher;

    @Inject
    public MyBean(@SubscriptionPublisher MyServiceSubscriptionPublisher myServiceSubscriptionPublisher) {
        this.myServiceSubscriptionPublisher = myServiceSubscriptionPublisher;
    }

    ... other method implementations ...

    @Override
    public void myMethod() {
        myServiceSubscriptionPublisher.fireMyMulticast("Some value");
    }
}
```

See also the
[Radio JEE provider bean](../examples/radio-jee/radio-jee-provider/src/main/java/io/joynr/examples/jee/RadioProviderBean.java)
for a working example.

#### Error handling

Joynr provider are expected to throw only `ProviderRuntimeException`.
If errors are modeled in the corresponding Franca interface, also `ApplicationException` can be
thrown.

All other (unexpected) exceptions (e.g. NullPointerException), are caught by joynr and wrapped in a
`ProviderRuntimeException`. Usually, exceptions should be handled by the provider implementation.
Errors shall be reported via `ProviderRuntimeException` or `ApplicationException`.

Providers are not expected to throw any other joynr internal exceptions (e.g. JoynrRuntimeException)
though this is possible and might happen when a joynr provider calls another joynr service.
These exceptions are then also wrapped by joynr in a ProviderRuntimeException.

Errors from providers are reported by joynr to the calling proxies in the consumer application.
In case of async calls, the errors are reported to the Callback/Future, in case of sync calls, the
exceptions are thrown and have to be handled in the consumer application.

Note that `JoynrRuntimeException` is annotated with `@ApplicationException` to allow better error
handling in JEE. This annotation is not to be confused with the joynr type `ApplicationException`.

#### Injecting the calling principal

In order to find out who is calling you, you can inject the `JoynrCallingPrincipal`
to your EJB. This will only ever be set if your EJB is called via joynr (i.e. if you
are also calling your EJB via other routes, e.g. JAX-RS, then the principal will not
be set).

See the
[System Integration Test](../tests/system-integration-test/sit-jee-app/src/main/java/io/joynr/systemintegrationtest/jee/SystemIntegrationTestBean.java)
for an example of its usage.

#### Injecting a RawMessagingPreprocessor

if you need to inspect or modify incoming joynr messages, you can provide a producer of
`@JoynrRawMessagingPreprocessor`, whose process method will be called for each incoming MQTT
message.

For example:

```java
@Produces
@JoynrRawMessagingPreprocessor
public RawMessagingPreprocessor rawMessagingPreprocessor() {
    return new RawMessagingPreprocessor() {
        @Override
        public String process(String rawMessage, @Nonnull Map<String, Serializable> context) {
            // do something with the message here, and add entries to the context
            return rawMessage;
        }
    };
}
```

#### Injecting a JoynrJeeMessageMetaInfo

Inject `JoynrJeeMessageMetaInfo` to your EJB in order to retrieve the context (including custom
headers) for a received message.

CustomHeaders can be provided on the consumer side via the `MessagingQos` when building a proxy
or as optional `MessagingQos` parameter when executing a method call.

Additional CustomHeaders can be added and existing CustomHeaders can be modified by the MQTT broker
if the broker is part of the communication process. In that case, the value set by the MQTT broker
takes precedence over a value set by `MessagingQos` on the consumer side as mentioned above.

The context can be accessed by calling `JoynrJeeMessageMetaInfo.getMessageContext()`:

```java
@Stateless
@ServiceProvider(serviceInterface = MyServiceSync.class)
public class MyBean implements MyServiceSync {
    private JoynrJeeMessageMetaInfo messageMetaInfo;

    @Inject
    public MyBean(JoynrJeeMessageMetaInfo messageMetaInfo) {
        this.messageMetaInfo = messageMetaInfo;
    }

    ... other method implementations ...

    @Override
    public void myMethod() {
        ...
        Map<String, Serializable> context = messageMetaInfo.getMessageContext();
        ...
    }
}
```

See also [examples/custom-headers](../examples/custom-headers/README.md).

### Calling services

In order to call services provided by other participants (e.g. applications
running in vehicles or providers of other JEE applications), inject the
`ServiceLocator` utility into the class which wants to call a service,
obtain a reference to a proxy of the business interface, and call the
relevant methods on it.

The service locator utility offers a proxy builder for specifying
the meta data to use in building the proxy in a fluent API style.

For example, if we wanted to call the `MyService` provider as implemented
in the above example:

```java
@Stateless
public class MyConsumer {

    private final ServiceLocator serviceLocator;

    @Inject
    public MyConsumer(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;
    }

    public void performCall() {

        // set optional gbids as required
        String[] gbids = String[] { "gbid1", "gbid2", ... };

        // set optional discoveryQos as required
        DiscoveryQos discoveryQos = new DiscoveryQos();

        // set optional messagingQos as required
        MessagingQos messagingQos = new MessagingQos();

        MyServiceSync myServiceProxy =
            serviceLocator.builder(MyServiceSync.class, "my.service.domain")
            .withTtl(ttl).                   // optional, overrides .withMessagingQos(...)
            .withMessagingQos(messagingQos). // optional, overrides .withTtl(...)
            .withDiscoveryQos(discoveryQos). // optional,
            .withGbids(gbids).               // optional
            .withCallback(callback).         // optional, see below
            .withUseCase(useCase).           // optional, see below
            .build();
        myServiceProxy.myMethod();
    }

}
```

The time-to-live for joynr messages can be set through either withTtl(ttl) API
or inside messagingQos using withMessagingQos(messagingQos) API. Please use only
one of both APIs, if at all, since they may override each others settings.

By default providers are looked up in all known backends.  
In case of global discovery, the default backend connection is used (identified by
the first GBID configured at the cluster controller).  
The withGbids(gbids) API can be used to discover providers registered for specific
backend global ids (GBIDs).  
If withGbids(gbids) API is used, then the global discovery will take place over the
connection to the backend identified by the first gbid in the list of provided GBIDs.

__IMPORTANT__: if you intend to have your logic make multiple calls to the same
provider, then you should locally cache the proxy instance returned by the
ServiceLocator, as the operation of creating a proxy is expensive.

#### Multi-proxies

It is also possible to target multiple providers with one proxy. You can achieve
this by specifying a set of domains for the ProxyBuilder / ServiceLocator and a custom
`ArbitrationStrategyFunction` in the `DiscoveryQos`, that selects a set of providers.
See the [Java Developer Guide](java.md#multi-proxies) for details.

#### The guided proxy builder
For enhanced control over the proxy creation process, the GuidedProxyBuilder can be used.
It separates the provider lookup / discovery (`guidedProxyBuilder.discover()` or
`guidedProxyBuilder.discoverAsync()`) from the actual proxy creation
`guidedProxyBuilder.buildProxy()`. This adds more flexibility to the provider
selection than the [arbitration strategies from DiscoveryQos](#the-discovery-quality-of-service).
In particular, a proxy can be easily built for an unknown provider version.
See also [Generator documentation](generator.md) for versioning of the generated
interface code.  
The `buildProxy` and the corresponding `discover` methods must be called on the
same instance of GuidedProxyBuilder.
An instance of a GuidedProxybuilder can only be used for performing one discovery
and building one proxy afterwards. Any attempt to do a second `discover` or
`build` call will result in an exception being thrown.

Note: The GuidedProxyBuilder can be be configured with DiscoveryQos except for the
arbitration strategy settings. Arbitation strategy from DiscoveryQos will be ignored as
the provider selection is a manual step in GuidedProxyBuilder between `discover()` and
`buildProxy()`.

Note: make sure to call either `buildNone()` or `buildProxy()` after a successful
discovery.
Call `buildNone()` instead of `buildProxy()` if you do not want to build a proxy for any of the
discovered providers. `buildNone()` will trigger the necessary cleanup after a successful discovery
and release previously allocated resources to avoid unnecessary memory usage (it decrements the
reference counts of the routing entries of all the discovered providers because none of them is
required anymore for this `GuidedProxyBuilder` instance).

Example for the usage of a GuidedProxyBuilder:

```java
...
// getGuidedProxyBuilder(...) requires some existing proxy class version as parameter
// (e.g. v4), but the concrete version does not matter since the interfaceName will
// be derived from it, which is identical for all proxy class versions.
Set<String> domains = new HashSet<>();
domains.add(providerDomainOne);
GuidedProxyBuilder guidedProxyBuilder =
    serviceLocator.getGuidedProxyBuilder(domains, joynr.<Package>.v4.<Interface>Proxy.class);
DiscoveryResult discoveryResult;
try {
    discoveryResult = guidedProxyBuilder
        .setMessagingQos(...) //optional
        .setDiscoveryQos(...) //optional
        .setGbids(...) //optional
        // same optional setters as in ProxyBuilder
        .discover();
        // GuidedProxyBuilder also offers a discoverAsync() method which returns a CompletableFuture
} catch (DiscoveryException e) {
    //handle errors
}
// Call buildNone() if you do not want to create a proxy for any of the discovered providers after successful discovery
// to do the necessary cleanup to avoid unnecessary memory usage
guidedProxyBuilder.buildNone();

// Select a DiscoveryEntry to build a proxy for the corresponding provider:
DiscoveryEntry lastSeenEntry = discoveryResult.getLastSeen();
/* Other possibilities to retrieve DiscoveryEntries from DiscoveryResult:
    DiscoveryEntry highestPriorityEntry = discoveryResult.getHighestPriority();
    DiscoveryEntry latestVersionEntry = discoveryResult.getLastestVersion();
    DiscoveryEntry entry = discoveryResult.getParticipantId(participantId);
    Collection<DiscoveryEntry> allDiscoveryEntries = discoveryResult.getAllDiscoveryEntries();
    Collection<DiscoveryEntry> discoveryEntriesWithKey = discoveryResult.getWithKeyword(keyword);
*/
if (lastSeenEntry.getProviderVersion().getMajorVersion() == 4) {
    // use the generated proxy interface for version 4.x
    joynr.<Package>.v4.<Interface>Proxy proxy = guidedProxyBuilder
        .buildProxy(joynr.<Package>.v4.<Interface>Proxy.class, lastSeenEntry.getParticipantId());
} else {
    ...
}
...
```

#### Proxy Futures and Callbacks

When using the builder API you can optinally also provide a callback instance or request you be
returned a `CompletableFuture` for the proxy in order to be able to wait until the proxy is actually
connected to the provider (i.e. arbitration is completed), or be notified of any failures in
connecting the proxy. This way your application is able to either wait until it knows that messages
can be sent to the provider, or react in some way to the proxy never being successfully created.

It is possible to use both a callback and the future at the same time, but if you want to do that
make sure to call `.withCallback(...)` before calling `.useFuture()`, otherwise you will encounter
an exception at runtime.

Here are some simple examples of both using a callback and using the future API:

```java
@Singleton
public class MyConsumer {

    private static final Logger LOGGER = LoggerFactory.getLogger(MyConsumer.class);

    @Inject
    private ServiceLocator serviceLocator;

    private MyServiceSync proxy;

    @PostConstruct
    public void initialise() {
        // Using a callback
        proxy = serviceLocator.builder(MyServiceSync.class, "my.service.domain")
            .withCallback(new ProxyBuilder.ProxyCreatedCallback<MyServiceSync>() {
                @Override
                public void onProxyCreationFinished(MyServiceSync result) {
                    LOGGER.info("Proxy created successfully.");
                }
                @Override
                public void onProxyCreationError(JoynrRuntimeException error) {
                    LOGGER.error("Unable to create proxy.", error);
                }
                @Override
                public void onProxyCreationError(DiscoveryException error) {
                    LOGGER.error("Discovery failed.", error);
                }
            })
            .build();

        // Using a completable future
        CompletableFuture<MyServiceSync> future = serviceLocator.builder(
                MyServiceSync.class, "my.service.domain"
            )
            .useFuture()
            .build();

        // ... non blocking
        future.whenCompleteAsync((proxy, error) -> {
            if (proxy != null) {
                MyConsumer.this.proxy = proxy;
                LOGGER.info("Proxy created successfully.");
            } else if (error != null) {
                LOGGER.error("Unable to create proxy.", error);
            }
        });

        // ... blocking
        try {
            proxy = future.get();
        } catch (ExecutionException e) {
            LOGGER.error("Unable to create proxy.", error.getCause());
        }
    }
}
```

See the JavaDoc of `io.joynr.jeeintegration.api.ServiceLocator` and
`io.joynr.jeeintegration.JeeJoynrServiceLocator` for more details.


#### Stateless Async

If you want to call a service in a stateless fashion, i.e. any node in a cluster
can handle the reply, then you need to provide a `@CallbackHandler` bean and
request the `*StatelessAsync` instead of the `*Sync` interface from the
`ServiceLocator`.

The methods in the `*StatelessAsync` interface have a `MessageIdCallback` as the
last parameter, which is a consumer of a String value. This value is the unique
ID of the request being sent out, and when the reply arrives, that same ID will
accompany the result data as part of the `ReplyContext` passed in as last parameter.
This way, your application can persist or otherwise share context information between
nodes in a cluster, so that any node can process the replies.  
__IMPORTANT__: it is not guaranteed that the message has actually left the system
when the `MessageIdCallback` is called. It is possible that the message gets stuck
in the lower layers due to, e.g., infrastructure issues. If the application persists
data for the message IDs returned, it may also want to run periodic clean-up jobs
to see if there are any stale entries due to messages not being transmitted
successfully.

The handling of the replies is done by a bean implementing the `*StatelessAsyncCallback`
interface corresponding to the `*StatelessAsync` interface which is called for
making the request, and which is additionally annotated with `@CallbackHandler`.  
These are automatically discovered at startup time, and registered as stateless async
callback handlers with the joynr runtime.

In order to allow the same service to be called from multiple parts of an application
in different ways, you must also provide a unique 'use case' name for each of the
proxy / callback pairs.

##### Example

For a full example project, see
[examples/stateless-async](../examples/stateless-async/README.md).

The following are some code snippets to exemplify the usage of the stateless async
API:

```java
@Stateless
@CallbackHandler
public class MyServiceCallbackBean implements MyServiceStatelessAsyncCallback {

    private final static String MY_USE_CASE = "get-stuff-done";

    @Inject
    private MyContextService myContextService;

    @Override
    public String getUseCase() {
        return MY_USE_CASE;
    }

    // This method is called in response to receiving a reply for calls to the provider
    // method 'myServiceMethod', and the reply context will contain the same message ID
    // which the request was sent with. This is then used to retrieve some persisted
    // context, which was created at the time of sending the request - potentially by
    // a different node in the cluster.
    @Override
    public void myServiceMethodSuccess(String someParameter, ReplyContext replyContext) {
        myContextService.handleReplyFor(replyContext.getMessageId(), someParameter);
    }

}

@Singleton
public class MyServiceBean {

    private MyServiceStatelessAsync proxy;

    @Inject
    private ServiceLocator serviceLocator;

    @Inject
    private MyContextService myContextService;

    @PostConstruct
    public void initialise() {
        proxy = serviceLocator.builder(MyServiceStatelessAsync.class, "my-provider-domain")
                    .withUseCase(MyServiceCallbackBean.MY_USE_CASE)
                    .build();
    }

    // Called by the application to trigger a call to the provider, persisting some
    // context information for the given messageId, which can then be retrieved in the
    // callback handler and used to process the reply.
    public void trigger() {
        proxy.myServiceMethod("some input value",
                                messageId -> myContextService.persistContext(messageId)
        );
    }

}
```

### Disabling automatic subscriptions

In case your provider application is not ready to process requests immediately after it started,
you can use the property `JeeIntegrationPropertyKeys.PROPERTY_KEY_JEE_SUBSCRIBE_ON_STARTUP` to disable
the automatic subscription to the MQTT topic when the joynr runtime starts.
If you do this, the subscription can be triggered by injecting the `JoynrConnectionService` and calling
`notifyReadyForRequestProcessing()`.
Call this method when your providers are ready to handle requests.

```Java
@Singleton
public class MyServiceBean {

    @Inject
    private JoynrConnectionService joynrConnectionService;

    public void readyToAcceptRequests() {
        joynrConnectionService.notifyReadyForRequestProcessing();
    }
}
```


## Clustering

Clustering is enabled by the 'shared subscription' model.
The solution offers load balancing on incoming messages via shared
subscriptions (part of MQTT v5 and proprietary to the HiveMQ broker prior thereto)
across all nodes in the cluster, and enables reply messages
for requests originating from inside the cluster to be routed directly
to the correct node.

Activate this mode with the
`MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS`
property.

Make sure to use the same fixed participant IDs for the providers in all nodes of the cluster. See
[Joynr Java Developer Guide](java.md#register-provider-with-fixed-%28custom%29-participantId).

## <a name="status_monitoring"></a> Joynr Status Monitoring

Joynr provides metrics which can be used to detect invalid states and situations which require a
restart of an instance. In order to access this information, inject an object of type
`JoynrStatusMetrics`. See the documentation of `JoynrStatusMetrics` and the
[Java Developer Guide](java.md#status_monitoring) for more information.

```java
import io.joynr.jeeintegration;
import io.joynr.statusmetrics.JoynrStatusMetrics;

@Stateless
public class MyHealthCheck {

    private final JoynrStatusMetrics joynrStatusMetrics;

    @Inject
    public MyConsumer(JoynrStatusMetrics joynrStatusMetrics) {
        this.joynrStatusMetrics = joynrStatusMetrics;
    }

    public boolean getVerdictExample() {
        // Evaluate data from joynrStatusMetrics here
    }
}
```

## Message Processors

By providing EJBs (or CDI Beans) which implement the `JoynrMessageProcessor` interface
you are able to hook into the message creation process. Each filter is called in turn
after the message has been created and is given the chance to perform any application
specific customizations, such as adding custom headers, or mutating any existing ones.

Be careful that your processor doesn't perform any changes which render the message
un-transmittable, such as changing the `to` or the `from` headers! Generally you
should only be, e.g., adding custom headers (via the `JoynrMessage#setCustomHeaders()`).

Here's an example of a message processor:

```java
@Stateless
public class MyMessageProcessor implements JoynrMessageProcessor {
    public MutableMessage processOutgoing(MutableMessage joynrMessage) {
        Map<String, String> myCustomHeaders = new HashMap<>();
        myCustomHeaders.put("my-correlation-id", UUID.randomUuid().toString());
        joynrMessage.setCustomHeaders(myCustomHeaders);
        return joynrMessage;
    }

    public ImmutableMessage processIncoming(ImmutableMessage joynrMessage) {
        return joynrMessage;
    }
}
```

This would add the custom header `my-correlation-id` to every outgoing joynr message with a random
UUID as the value. See also [examples/custom-headers](../examples/custom-headers/README.md).


## Shutting Down

If you want to perform a graceful shutdown of your application and the joynr runtime,
then inject a reference to the `io.joynr.jeeintegration.api.JoynrShutdownService`
into one of your beans, and call its `prepareForShutdown` and potentially the
`shutdown` method thereafter.

Calling `prepareForShutdown` gives the joynr runtime a chance to stop receiving
incoming requests and work off any messages still in its queue. Thus your application
logic may still be called from joynr after your call to `prepareForShutdown`. Be
aware that if your logic requires to make calls via joynr in response to those received messages,
you will only be able to call non-stateful methods such as fire-and-forget. Calls to stateful
methods, such as those in the Sync interface, will result in a `JoynrIllegalStateException`
being thrown.

The `prepareForShutdown` method will block until either the joynr runtime has finished processing
all messages in the queue or a timeout occurs. The timeout can be configured via the
`PROPERTY_PREPARE_FOR_SHUTDOWN_TIMEOUT` property. See the
[Java Configuration Guide](./JavaSettings.md) for details.

In order to be able to stop receiving messages but still be able to send stateless messages out,
you must use two MQTT connections, which can be activated using the
`PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS` property. Again see the
[Java Configuration Guide](./JavaSettings.md) for details.


## Example Application

Under `examples/radio-jee` you can find an example application which is based on the
[Radio App example](./Tutorial.md). It uses the same `radio.fidl` file from the tutorial
but implements it as a JEE provider application and a separate JEE consumer application.

The project is sub-divided into one multi-module parent project and four subprojects:

```
 - radio-jee
   |- radio-jee-api
   |- radio-jee-backend-services
   |- radio-jee-consumer
   |- radio-jee-provider
```

In order to build the project you have to built joynr by executing
`mvn install` from the root of the directory where you checked out joynr to. Next change
to the `radio-jee` directory and find the .war-archives in the corresponding `target` subfolders.

First, fire up the joynr backend service as illustrated in:
[starting joynr backend instructions](../wiki/infrastructure.md)

Finally, deploy the provider and consumer applications:

- `bin/asadmin deploy <joynr home>/examples/radio-jee/radio-jee-provider/target/radio-jee-provider.war`
- `bin/asadmin deploy <joynr home>/examples/radio-jee/radio-jee-consumer/target/radio-jee-consumer.war`

Once both applications have started up successfully, you can use an HTTP client (e.g. `curl`
on the command line or [Paw](https://luckymarmot.com/paw) on Mac OS X) to trigger calls
from the consumer to the client:

- `curl -X POST http://localhost:8080/radio-jee-consumer/radio-stations/shuffle`
	- This method has no explicit return value, if you don't get an error, it worked
- `curl http://localhost:8080/radio-jee-consumer/radio-stations/current-station`
	- This gets the current station, and you should see some output similar to:
	  `{"country":"GERMANY","name":"Bayern 3","trafficService":true}`

Note: If you use a joynr docker container to run the JEE application server
the payara's default http port 8080 won't be available. Use the https port 8181 instead.

Next, explore the code in the `radio-jee-provider` and `radio-jee-consumer` projects.
Note the `radio-jee-provider/src/main/java/io/joynr/examples/jee/RadioProviderBean.java`
and `radio-jee-consumer/src/main/java/io/joynr/examples/jee/RadioConsumerRestEndpoint.java`
classes in particular, which represent the implementation of the joynr provider for the
Radio service, and the consumer thereof.

If you want to use the radio JEE example as a template for building a joynr based JEE application
don't forget to change the joynr dependency version in the Maven POMs to a release version. If you
change the parent POM, which is also likely, don't forget to pull the necessary `dependencyManagement`
entries from the joynr parent POM into your own POM.
You'll also likely want to change the way the FIDL file is included in the API project. In this
example it is obtained from the `radio-app` Maven dependency, but you will probably want to have it
in, e.g., `${project.root}/my-api/src/main/model/my.fidl`, and then reference that file directly
in the generator configuration. See the [joynr Generator Documentation](generator.md) for details.

## Overriding Jackson library used at runtime

The following information only applies to Glassfish / Payara 4.1.

### glassfish-web.xml

joynr ships with a newer version of Jackson than is used by Glassfish / Payara 4.1.
Generally, this shouldn't be a problem. If, however, you observe errors relating
to the JSON serialisation, try setting up your WAR to use the Jackson libraries
shipped with joynr.

In order to do this, you must provide the following content in the
`glassfish-web.xml` file (situated in the `WEB-INF` folder of your WAR):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE glassfish-web-app PUBLIC "-//GlassFish.org//DTD GlassFish Application Server 3.1 Servlet 3.0//EN"
    "http://glassfish.org/dtds/glassfish-web-app_3_0-1.dtd">
<glassfish-web-app>
  <class-loader delegate="false" />
</glassfish-web-app>
```

... here, the `<class-loader delegate="false" />` part is the relevant bit.

### Deactivate MoxyJson

Apparently Glassfish / Payara 4.1 ships with MoxyJson which may cause problems like
the following, when you try to parse JSON within your application.

    [2016-09-01T16:27:37.782+0200] [Payara 4.1] [SEVERE] []   [org.glassfish.jersey.message.internal.WriterInterceptorExecutor] [tid: _ThreadID=28 _ThreadName=http-listener-1(3)] [timeMillis: 1472740057782] [levelValue: 1000] [[MessageBodyWriter not found for media type=application/json, type=class xxx.JsonClass, genericType=class xxx.JsonClass.]]

To integrate your own dependency you need to disable the MoxyJson with the below code.

```java
@ApplicationPath("/")
public class ApplicationConfig extends Application {

  @Override
  public Map<String, Object> getProperties() {
    final Map<String, Object> properties = new HashMap<String, Object>();
    properties.put("jersey.config.server.disableMoxyJson", true);

    return properties;
  }
}
```

Then add your own dependencies, e.g. in this case only the following because the others
are referenced by the joynr lib itself. Be aware to check the version of the joynr
referenced libs.

```xml
<dependency>
  <groupId>com.fasterxml.jackson.jaxrs</groupId>
  <artifactId>jackson-jaxrs-json-provider</artifactId>
  <version>2.9.8</version>
</dependency>
<dependency>
  <groupId>com.fasterxml.jackson.dataformat</groupId>
  <artifactId>jackson-dataformat-xml</artifactId>
  <version>2.9.8</version>
</dependency>
```

Finally in case you're using JSON: Not setting a value to the @JsonProperty annotations
will cause a NoMessageBodyWriter found exception. To avoid that use the following on
relevant getters of your class.

```java
@JsonProperty("randomName")
public String getRandomName(){
...
}
```

Here are some references:

* [Moxy in general](https://blogs.oracle.com/theaquarium/entry/moxy_is_the_new_default)
* [Jersey configuration reference](https://jersey.github.io/documentation/latest/appendix-properties.html)
* [Jersey deployment reference](https://jersey.github.io/documentation/latest/deployment.html)
* [Payara blog re. JEE Microservices](http://blog.payara.fish/building-restful-java-ee-microservices-with-payara-embedded)
