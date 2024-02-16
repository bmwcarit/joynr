# Joynr Java Developer Guide

## Conversion of Franca entries

### Place holders

Note that the following elements in the code examples below must be replaced by actual values from Franca:

```
// "<Attribute>" the Franca name of the attribute
// "<AttributeType>" the Franca name of the attribute type
// "<broadcast>" the Franca name of the broadcast, starting with a lowercase letter
// "<Broadcast>" the Franca name of the broadcast, starting with capital letter
// "BroadcastFilter<Attribute>" Attribute is the Franca attributes name
// "<Filter>" the Franca name of the broadcast filter
// "<interface>" the Franca interface name, starting with a lowercase letter
// "<Interface>" the Franca interface name, starting with capital letter
// "<method>" the Franca method name, starting with a lowercase letter
// "<Method>" the Franca method name, starting with capital letter
// "<OutputType>" the Franca broadcast output type name
// "<Package>" the Franca package name
// "<ProviderDomain>" the provider domain name used by provider and client
// "<ReturnType>" the Franca return type name
```

### Package name
The Franca ```<Package>``` will be transformed to the Java package ```joynr.<Package>```.

### Type collection name
The Franca ```<TypeCollection>``` will be transformed to the Java package ```joynr.<Package>.<TypeCollection>```.

### Complex type name

Any Franca complex type ```<TypeCollection>.<Type>``` will result in the creation of a ```class joynr.<Package>.<TypeCollection>.<Type>``` (see above).

The same ```<Type>``` will be used for all elements in the event that this type is used as an element of other complex types, as a method input or output argument, or as a broadcast output argument.

Getter and Setter methods will be created for any element of a struct type. Also a standard constructor, full arguments constructor and object argument constructor will be created automatically.

Note that in order to use an instance of the class directly or indirectly as input or output argument
for any joynr call (e.g. method call, broadcast publication etc.), all its members must be properly
initialized; especially any references must be != null (i.e. point to initialized instances).

### Interface name

The Franca ```<Interface>``` will be used as a prefix to create the following Java classes or interfaces:

```java
public abstract class joynr.<Package>.<Interface>AbstractProvider
public class joynr.<Package>.Default<Interface>Provider
public interface joynr.<Package>.<Interface>Async
public interface joynr.<Package>.<Interface>BroadcastInterface
public interface joynr.<Package>.<Interface>FireAndForget
public interface joynr.<Package>.<Interface>
public abstract class joynr.<Package>.<Interface><Broadcast>BroadcastFilter
public interface joynr.<Package>.<Interface>Provider
public interface joynr.<Package>.<Interface>Proxy
public interface joynr.<Package>.<Interface>SubscriptionInterface
public interface joynr.<Package>.<Interface>SubscriptionPublisher
public class joynr.<Package>.<Interface>SubscriptionPublisherImpl
public interface joynr.<Package>.<Interface>Sync
```
# Setting up a joynr deployment
Choose how you want your application to connect to the joynr network by initializing the
JoynrRuntime with the appropriate joynr RuntimeModule, which uses guice to inject the desired
functionality.

If you have multiple nodes running locally, they can share a cluster controller, which handles
access control, local discovery and message routing for the local cluster. joynr currently supports
connecting Java nodes to a cluster controller via WebSockets. The individual applications are
configured using a ```LibjoynrWebSocketRuntimeModule```.

For a single node deployment, it may however be simpler to combine the cluster controller logic and
the application in a single Java process. Use a ```CCInProcessRuntimeModule``` in this case.

See the Radio example, in particular ```MyRadioConsumerApplication``` and
```MyRadioProviderApplication```, for a detailed example of how this is done.

See the [Java Configuration Reference](JavaSettings.md) for a complete listing of all available
configuration properties available to use in joynr Java applications.

## The external (global) transport middlewares
joynr is able to communicate to other clusters via MQTT using HiveMQ Client.
Guice is also used to inject the required functionality.

For details on configuring MQTT as the transport layer, please see the
[Java MQTT Clients](./java_mqtt_clients.md) documentation.

After choosing which RuntimeModule you are using, override it with the
```HivemqMqttClientModule```. See the Radio example, in particular
```MyRadioConsumerApplication``` and ```MyRadioProviderApplication``` for a detailed example of how
this is done.

# Building a Java consumer application

A java joynr application inherits from ```AbstractJoynrApplication``` class and contains at least a
```main()```, ```run()``` and ```shutdown()``` method.

## Required imports

The following base imports are required for a Java Consumer application:

```java
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.ApplicationException;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import java.io.IOException;
import java.util.Properties;
import joynr.<Package>.<Interface>Proxy;
import com.google.inject.Inject;
import com.google.inject.Module;
import com.google.inject.name.Named;
import com.google.inject.util.Modules;
```

## The base class

```java
// required imports
...
public class MyConsumerApplication extends AbstractJoynrApplication {

    @Inject
    @Named(APP_CONFIG_PROVIDER_DOMAIN)
    private String providerDomain;

    private <Interface>Proxy <interface>Proxy;

    public static void main(String[] args) throws IOException {
        // initialization, perhaps including setting the domain
        // for this instance of the application.
    }

    public void run() {
        // main application logic
    }

    public void shutdown() {
       // unregister and cleanup
    }
}
```

## The main method

The ```main()``` method must setup the configuration (provider domain etc.) and create the
```JoynrApplication``` instance by instantiating a new ```JoynrApplicationModule```. Then the
```run()``` method of the consumer application can be called to do the work.

As a prerequisite, the **provider** and **consumer domain** need to be defined using ```Properties```
as shown below.

```java
public static void main(String[] args) throws IOException {
    String providerDomain = "<ProviderDomain>";

    Properties joynrConfig = new Properties();
    joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
    joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, "my_local_domain");

    Properties appConfig = new Properties();
    appConfig.setProperty(APP_CONFIG_PROVIDER_DOMAIN, providerDomain);

    Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(new HivemqMqttClientModule());

    JoynrApplication myConsumerApp =
      new JoynrInjectorFactory(joynrConfig, runtimeModule).createApplication(
        new JoynrApplicationModule(MyApplication.class, appConfig));

    myConsumerApp.run();
    myConsumerApp.shutdown();
}
```
## The discovery quality of service

The class ```DiscoveryQos``` configures how the search for a provider will be handled. It has the following members:

* **discoveryTimeoutMs**  Timeout for discovery process (milliseconds)  if no compatible
  provider was found within the given time. A timeout triggers a DiscoveryException or
  NoCompatibleProviderFoundException containing the versions of the discovered incompatible
  providers.  
  See also [`PROPERTY_DISCOVERY_DEFAULT_TIMEOUT_MS`](JavaSettings.md#property_discovery_default_timeout_ms).
* **retryIntervalMs** The time to wait between discovery retries after encountering a discovery error.  
  See also [`PROPERTY_DISCOVERY_DEFAULT_RETRY_INTERVAL_MS`](JavaSettings.md#property_discovery_default_retry_interval_ms)
  and [`PROPERTY_DISCOVERY_MINIMUM_RETRY_INTERVAL_MS`](JavaSettings.md#property_discovery_minimum_retry_interval_ms).
* **cacheMaxAgeMs** Defines the maximum allowed age of cached entries (milliseconds), only younger
  entries will be considered. If no suitable providers are found, then depending on the
  discoveryScope, a remote global lookup may be triggered.
* **arbitrationStrategy** The arbitration strategy (details see below)
* **discoveryScope** The discovery scope (details see below)
* **providerMustSupportOnChange** If set to true, select only providers which support onChange
  subscriptions (set by the provider in its providerQos settings)
* **customParameters** special parameters, that must match, e.g. keyword (see below)

The enumeration **discoveryScope** defines options to decide whether a suitable provider will be
searched in the local capabilities directory or in the global one.

Available values are as follows:

* **LOCAL_ONLY** Only entries from local capability directory will be searched
* **LOCAL_THEN_GLOBAL** Entries will be taken from local capabilities directory, unless no such
  entries exist, in which case global entries will be looked at as well.
* **LOCAL_AND_GLOBAL** Entries will be taken from local capabilities directory and from global
  capabilities directory.
* **GLOBAL_ONLY** Only the global entries will be looked at.

**Default discovery scope:** ```LOCAL_THEN_GLOBAL```

Whenever global entries are involved, they are first searched in the local cache. In case no global
entries are found in the cache, a remote lookup is triggered.

The enumeration ```ArbitrationStrategy``` defines how the results of the scoped lookup will be
sorted and / or filtered to select a Provider:

* **LastSeen** The participant that was last refreshed (i.e. with the most current last seen date)
  will be selected
* **NotSet** (not allowed in the app, otherwise arbitration will throw DiscoveryException)
* **HighestPriority** Entries will be considered according to priority
* **Keyword** Only entries that have a matching keyword will be considered
* **FixedChannel** select provider which matches the participantId provided as custom parameter in
   DiscoveryQos (see below), if existing
* **Custom** Allows you to provide a `ArbitrationStrategyFunction` to allow custom
  selection of discovered entries (only possible via constructor)

**Default arbitration strategy:** ```LastSeen```

The priority used by the arbitration strategy *HighestPriority* is set by the provider through the
call ```providerQos.setPriority()```.

Class ```ArbitrationConstants``` provides keys for the key-value pair for the custom Parameters of
discoveryScope:

* **PRIORITY_PARAMETER** (apparently not implemented as of now)
* **KEYWORD_PARAMETER**
* **FIXEDPARTICIPANT_KEYWORD**

Example for **Keyword** arbitration strategy:

```java
discoveryQos.addCustomParameter(ArbitrationConstants.KEYWORD_PARAMETER, "keyword");
```
Example for **FixedChannel** arbitration strategy:

```java
discoveryQos.addCustomParameter(ArbitrationConstants.FIXED_PARTICIPANT_KEYWORD, participantId);
```

Example for the creation of a DiscoveryQos class object:

```java
DiscoveryQos discoveryQos = new DiscoveryQos();

discoveryQos.setDiscoveryTimeoutMs(10000); // optional, default 30000
discoveryQos.setRetryIntervalMs(1000); // optional, default 1000
discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE); // optional, default 0
// optional, default as stated above
discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
// optional, default as stated above
discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
discoveryQos.setProviderMustSupportOnChange(true); // optional, default false
discoveryQos.addCustomParameter(key, value); // optional, default none
```

## The message quality of service

The ```MesssagingQos``` class defines the **roundtrip timeout in milliseconds** for **RPC requests**
(getter/setter/method calls) and unsubscribe requests and it allows definition of additional custom
message headers.

The ttl for subscription requests is calculated from the ```expiryDateMs```
in the [SubscriptionQos](#quality-of-service-settings-for-subscriptions) settings.
For internal joynr messages, the value of PROPERTY_MESSAGING_MAXIMUM_TTL_MS is used.
*The GlobalCapabilitiesDirectoryClient uses TTL_30_DAYS_IN_MS (30 days)*

If no specific setting is given, the default roundtrip timeout is 60 seconds.
The keys of custom message headers may contain ascii alphanumeric or hyphen.
The values of custom message headers may contain alphanumeric, space, semi-colon, colon,
comma, plus, ampersand, question mark, hyphen, dot, star, forward slash and back slash.
If a key or value is invalid, the API method called to introduce the custom message
header throws an IllegalArgumentException.

Example:

```java
long ttl_ms = 60000;

MessagingQos messagingQos = new MessagingQos(ttl_ms);
// optional custom headers
Map<String, String> customMessageHeaders = new Map<String, String>();
customMessageHeaders.put("key1", "value1");
...
customMessageHeaders.put("keyN", "valueN");
messagingQos.putAllCustomMessageHeaders(customMessageHeaders);
...
messagingQos.putCustomMessageHeader("anotherKey", "anotherValue");
```

## The run method

Inside the ```run()``` method, the consumer application instance must create one **proxy** per used Franca interface in order to be able to
* call its **methods** (RPC) either **synchronously** or **asynchronously**
* **subscribe** or **unsubscribe** to its **attributes** or **update** a subscription
* **subscribe** or **unsubscribe** to its **broadcasts** or **update** a subscription

In case no suitable provider can be found during discovery, a `DiscoveryException` or
`NoCompatibleProviderFoundException` is thrown.  
In case of communication errors, a `JoynrRuntimeException` (usually `JoynrTimeoutException) is
thrown.

```java
@Override
public void run() {
    DiscoveryQos discoveryQos = new DiscoveryQos();
    MessagingQos messagingQos = new MessagingQos();
    String[] gbids = new String[] { "gbid1","gbid2" };
    // the qos can be fine tuned here by calling setters

    ProxyBuilder<<Interface>Proxy> proxyBuilder =
        runtime.getProxyBuilder(providerDomain, <Interface>Proxy.class);
    try {
        // Also can call proxyBuilder.build(callback)
        <interface>Proxy = proxyBuilder.
            setMessagingQos(messagingQos). // optional
            setDiscoveryQos(discoveryQos). // optional
            setGbids(gbids).               // optional
            build();
        // call methods, subscribe to broadcasts etc.
        // enter some event loop
    } catch (DiscoveryException|NoCompatibleProviderFoundException e) {
        // no provider found
    } catch (JoynrRuntimeException e) {
        // handle other JoynrRuntimeException types, e.g. JoynrTimeoutException
    }
}
```
A callback can also be added to the proxyBuilder.build() call, allowing application code to be
notified when the discovery process has completed.

By default, providers are looked up in all known backends.  
In case of global discovery, the default backend connection is used (identified by the
first GBID configured at the cluster controller).  
The discovery of providers can be restricted to certain backends by specifying one or more
global backend ids (GBIDs) using the setGbids(gbids) API.  
If setGbids(gbids) API is used, then the global discovery will take place over the
connection to the backend identified by the first GBID in the list of provided GBIDs.

## Multi-proxies

It is also possible to obtain a proxy for targeting multiple providers that implement the same
interface (multi-proxy). You can achieve this by specifying a set of domains for the ProxyBuilder
and a custom `ArbitrationStrategyFunction` in the `DiscoveryQos`(see section
[The discovery quality of service](#the-discovery-quality-of-service) above), that returns a set of
providers.

> Note: The proxy creation is only successful if at least one provider could be discovered for each
specified domain.

When you create such a multi-proxy, a call to a method on that proxy will result in 'n'
calls to the providers, where 'n' is the number of providers targeted.
It is only possible to send calls to multiple providers if the methods are fire-and-forget.
Attempts to make calls to non-fire-and-forget methods from a multi-proxy will result in an
exception being thrown. In case of the async API, the exception will be reported via the callback
and the future.

So, for example, if we change the code above to target two domains, we get:

```java
...
    Set<String> domains = new HashSet<>();
    domains.add(providerDomainOne);
    domains.add(providerDomainTwo);

    discoveryQos = new DiscoveryQos(<discoveryTimeoutMs>,
                                    <retryIntervalMs>,
                                    new ArbitrationStrategyFunction() {
        @Override
        protected Set<DiscoveryEntryWithMetaInfo> select(
                    Map<String, String> parameters,
                    Collection<DiscoveryEntryWithMetaInfo> capabilities) {
            // filter capabilities here as needed
            // the proxy will be created for the returned entries
            return new HashSet<>(capabilities);
        }
    }, <cacheMaxAgeMs>, <discoveryScope>);

    ProxyBuilder<<Interface>Proxy> proxyBuilder =
        runtime.getProxyBuilder(domains, <Interface>Proxy.class);

    <interface>Proxy = proxyBuilder.
    setMessagingQos(messagingQos). // optional
    setDiscoveryQos(discoveryQos). // mandatory for a multi-proxy
    setGbids(gbids).               // optional
    build();
...
```

## The guided proxy builder
For enhanced control over the proxy creation process, the GuidedProxyBuilder can be used.
It separates the provider lookup / discovery (`guidedProxyBuilder.discover()` or
`guidedProxyBuilder.discoverAsync()`) from the actual proxy creation
`guidedProxyBuilder.buildProxy()`. This adds more flexibility to the provider
selection than the [arbitration strategies from DiscoveryQos](#the-discovery-quality-of-service).
In particular, a proxy can be easily built for an unknown provider version.
See also [Generator documentation](generator.md) for versioning of the generated
interface code.  
The `buildProxy` and the corresponding `discover` method must be called on the
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
    runtime.getGuidedProxyBuilder(domains, joynr.<Package>.v4.<Interface>Proxy.class);
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

## Synchronous Remote procedure calls
While the provider executes the call asynchronously in any case, the consumer will wait until the call is finished, i.e. the thread will be blocked.
Note that the message order on Joynr RPCs will not be preserved.

Example for calls with single return parameter:

```java
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
public void run() {
    // setup proxy named <interface>Proxy
    ...
    try {
        <ReturnType> retval;
        retval = <interface>Proxy.<method>([inputVal1, ..., inputValN]);
    } catch (ApplicationException e) {
        // optional special error handling in case model contains error enumeration
    } catch (JoynrRuntimeException e) {
        // error handling
    }
}
```

In case of multiple return parameters the parameters will be wrapped into a class named
```<Method>Returned```. Each parameter value is available through a public member variable inside this class.

Example:
```java
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
public void run() {
    // setup proxy named <interface>Proxy
    ...
    try {
        <Method>Returned retval;
        retval = <interface>Proxy.<method>([inputVal1, ..., inputValN]);
        // handle return parameters
        //   retval.<returnParameter1>
        //   ...
        //   retval.<returnParameterN>
    } catch (ApplicationException e) {
        // optional special error handling in case model contains error enumeration
    } catch (JoynrRuntimeException e) {
        // error handling
    }
}
```

For methods which are modelled with error enumerations, additionally, ApplicationExceptions have to be caught.
The ApplicationException serves as container for the actual error enumeration which can be retrieved by calling e.getError().


## Asynchronous Remote Procedure calls
Using asynchronous method calls allows the current thread to continue its work. For this purpose a callback has to be provided for the API call in order to receive the result and error respectively. Note the current thread will still be blocked until the Joynr message is internally set up and serialized. It will then be enqueued and handled by a Joynr Middleware thread.
The message order on Joynr RPCs will not be preserved.
If no return type exists, the term ```Void``` is used instead.

### Asynchronous Remote Procedure calls with single return parameter
Example for calls with single return parameter:

```java
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
public class MyCallback implements Callback<<ReturnType>> {
    @Override
    void onSuccess(<ReturnType> result) {
        // handle result
    }

    @Override
    void onFailure(JoynrRuntimeException error) {
        // handle error
    }
}
...
public void run() {
    // setup proxy named "<interface>Proxy"
    ...
    public Future<<ReturnType>> future;
    MyCallback myCallback = new MyCallback();

    future = <interface>Proxy.<method>(
        myCallback,
        [inputVal1, ..., inputValN]
    );
    try {
        long timeoutInMilliseconds;
        // set timeout value here
        <ReturnType> result = future.get(timeOutInMilliseconds);
    } catch (InterruptedException|JoynrRuntimeException e) {
        // handle error
    } catch (ApplicationException e) {
        // optional special error handling in case model contains error enumeration
    }
    ...
}
```
If the Franca model includes error enums, then the Callback will also need to implement onFailure for the modeled error:

```java
@Override
public void onFailure(<Method>ErrorEnum errorEnum) {
    switch (errorEnum) {
    case <ENUM_LITERAL_A>:
        break;
    case <ENUM_LITERAL_B>:
        break;
    default:
    // handle default error case
        break;
    }

}
```

### Async Remote Procedure calls with Multiple Return Parameters

In case of multiple return parameters the parameters will be wrapped into a class named
```<Method>Returned```. Each parameter value is available through a public member variable inside this class.

```java
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection.<Type>;
...
public class MyCallback implements Callback<<Method>Returned> {
    void onSuccess(<Method>Returned result) {
        // handle result
    }

    void onFailure(JoynrRuntimeException error) {
        // handle error
    }
}
...
public void run() {
    // setup proxy named "<interface>Proxy"
    ...
    public Future<<Method>Returned> future;
    MyCallback myCallback = new MyCallback();

    future = <interface>Proxy.<method>(
        myCallback,
        [inputVal1, ..., inputValN]
    );
    try {
        long timeoutInMilliseconds;
        // set timeout value here
        <Method>Returned result = future.get(timeOutInMilliseconds);
        // handle return parameters
        //   result.<returnParameter1>
        //   ...
        //   result.<returnParameterN>
    } catch (InterruptedException|JoynrRuntimeException e) {
        // handle error
    } catch (ApplicationException e) {
        // optional special error handling in case model contains error enumeration
    }
    ...
}
```

If the Franca model includes error enums, then the Callback will also need to implement onFailure for the modeled error:

```java
@Override
public void onFailure(<Method>ErrorEnum errorEnum) {
    switch (errorEnum) {
    case <ENUM_LITERAL_A>:
        break;
    case <ENUM_LITERAL_B>:
        break;
    default:
		 // handle default error case
        break;
    }

}
```

## Stateless Asynchronous Remote Procedure Calls

In contrast to both the synchronous and asynchronous RPC mechanisms described above, the
stateless asynchronous replies can be handled by any runtime in a cluster of an application.

In order to accomplish this, the application provides a callback implementation by
registering it with the joynr runtime similar to the way providers are registered.
Then, when a request is made for a service for which a callback was registered, the
reply data is routed to that callback.

In order to be able to logically match the request with the reply, a unique ID is provided
when sending the request, via the `MessageIdCallback`, which the application can use to
persist any relevant context information. When the reply arrives, the same ID is provided to
the callback as part of the `ReplyContext` as the last parameter, and the application can
then use this to load the context information.  
__IMPORTANT__: it is not guaranteed that the message has actually left the system
when the `MessageIdCallback` is called. It is possible that the message gets stuck
in the lower layers due to, e.g., infrastructure issues. If the application persists
data for the message IDs returned, it may also want to run periodic clean-up jobs
to see if there are any stale entries due to messages not being transmitted
successfully.

So that an application can use the same service in multiple use cases, during registration
of the callback and when creating the service proxy, a unique 'use case' name must be
provided, matching the proxy to the callback.

__IMPORTANT__: due to the stateless nature of this communication pattern, there are some
cases where the message can't be delivered but will not trigger a callback. Specifically
if the TTL of the message has expired when it reaches the provider, no error callback
will be triggered. Equally, if the message gets lost by the infrastructure en route, no
error callback will be triggered. If required, you must guard against these cases in
your application code by, e.g. storing a timestamp in the context data you persist for
a given message ID, and track the success of the request / reply rountrip using the
callback methods. If the status is then not set within a given timeframe, you can react
accordingly.

### Example

For a full example showing how to use the stateless async API, see
[examples/stateless-async](../examples/stateless-async/README.md).

### Registering the callback

    ...
    public MyStatelessAsyncCallback implements <interface>StatelessAsyncCallback {
        @Override
        public String getUseCase() {
            return <usecase>;
        }

        // Called for replies to proxy.myMethod(...)
        @Override
        public void myMethodSuccess(<service output parameters>, ReplyContext replyContext) {
            ... handle the reply data ...
        }
        @Override
        public void myMethodFailed(<application error enum>, ReplyContext replyContext) {
            ... handle business errors ...
        }
        @Override
        public void myMethodFailed(JoynrRuntimeException e, ReplyContext replyContext) {
            ... handle ProviderRuntimeExceptions and other runtime exceptions ...
        }
    }
    ...
    public void run() {
        ...
        runtime.registerStatelessAsyncCallback(new MyStatelessAsyncCallback());
        ...
    }
    ...

### Building the Proxy

    ...
    public void run() {
        ...
        // callback already registered as above
        ...
        ProxyBuilder builder = runtime.getProxyBuilder(<domain>, <interface>Proxy.class);
        ...
        builder.setStatelessAsyncCallbackUseCase(<usecase>);
        <interface>StatelessAsync proxy = builder.build();
        proxy.myMethod(<input parameters>, messageId -> this::persistMyMethodContext);
		// ^ Reply handled by the myMethod* callbacks
        ...
    }
    ...

It's essential that for a given piece of business logic the use case of the callback to be
used matches that passed into the proxy being built. The requests sent from that proxy, will
then be handled by the callback with the same use case identifier.

## Quality of Service settings for subscriptions

### SubscriptionQos

The abstract class ```SubscriptionQos``` has the following members:

* **expiryDateMs** Absolute time until notifications will be send (milliseconds)
* **publicationTtlMs** Lifespan of a notification (milliseconds), the notification will be deleted
  afterwards
  Known Issue: subscriptionQos passed when subscribing to a non-selective broadcast are ignored.
  The API will be changed in the future: proxy subscribe calls will no longer take a
  subscriptionQos; instead the publication TTL will be settable on the provider side.

### MulticastSubscriptionQos

The class ```MulticastSubscriptionQos``` inherits from ```SubscriptionQos```.

This class should be used for subscriptions to non-selective broadcasts.

### PeriodicSubscriptionQos

The class ```PeriodicSubscriptionQos``` inherits from ```SubscriptionQos``` and has the following
additional members:

* **periodMs** defines how long to wait before sending an update even if the value did not change
* **alertAfterIntervalMs** Timeout for notifications, afterwards a missed publication notification
  will be raised (milliseconds)

This class can be used for subscriptions to attributes.

Note that updates will be send only based on the specified interval and not based on changes of the
attribute.

### OnChangeSubscriptionQos

The class ```OnChangeSubscriptionQos``` inherits from ```UnicastSubscriptionQos``` which inherits
from ```SubscriptionQos```. It has the following additional members:

* **minIntervalMs** Minimum time to wait between successive notifications (milliseconds)
* **publicationTtlMs** Notification messages will be sent with this time-to-live. If a notification
  message can not be delivered within its time to live, it will be deleted from the system. This
  value is provided in milliseconds.

This class should be used for subscriptions to selective broadcasts.
It can also be used for subscriptions to attributes if no periodic update is required.

### OnchangeWithKeepAliveSubscriptionQos

The class ```OnChangeWithKeepAliveSubscriptionQos``` inherits from ```OnChangeSubscriptionQos```
and has the following additional members:

* **maxIntervalMs** Maximum time to wait between notifications, if value has not changed
* **alertAfterIntervalMs** Timeout for notifications, afterwards a missed publication notification
  will be raised (milliseconds)

This class can be used for subscriptions to attributes. Updates will then be sent based both
periodically and after a change (i.e. this acts like a combination of ```PeriodicSubscriptionQos```
and ```OnChangeSubscriptionQos```).

Using it for subscriptions to broadcasts is theoretically possible because of inheritance but makes
no sense (in this case the additional members will be ignored).

## Subscribing to an attribute

Attribute subscription - depending on the subscription quality of service settings used - informs
the application either periodically and / or on change of an attribute about the current value.

The **subscriptionId** can be retrieved via the callback (onSubscribed) and via the future returned
by the subscribeTo call. It can be used later to update the subscription or to unsubscribe from it.
The subscriptionId will be available when the subscription is successfully registered at the
provider. If the subscription failed, a SubscriptionException will be returned via the callback
(onError) and thrown by future.get().

To receive the subscription, a **callback** has to be provided which is done providing a listener
class as outlined below. Since the callback is called by a communication middleware thread, it
should not be blocked, wait for user interaction, or do larger computation. The callback methods
(onReceive, onSubscribed, onError) are optional. Only the required methods have to be implemented.

```java
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
...
public void run() {
    // setup proxy named "<interface>Proxy"
    ...
    // Quality of service settings can be provided as either
    // "PeriodicSubscriptionQos",
    // "OnChangeSubscriptionQos" or "OnChangeWithKeepAliveSubscriptionQos" class
    // referenced by <QosClass> below

    <QosClass> qos = new <QosClass>(... parameters ...);
    ...
    Future<String> subscriptionIdFuture;
    try {
        subscriptionIdFuture = <interface>Proxy.subscribeTo<Attribute>(
            new AttributeSubscriptionAdapter<AttributeType>() {
                // Gets called on every received publication
                @Override
                public void onReceive(<AttributeType> value) {
                    // handle info
                }

                // Gets called when the subscription is successfully registered at the provider
                @Override
                public void onSubscribed(String subscriptionId) {
                    // save the subscriptionId for updating the subscription or unsubscribing from it
                    // the subscriptionId can also be taken from the future returned by the subscribeTo call
                }

                // Gets called on every error that is detected on the subscription
                @Override
                public void onError(JoynrRuntimeException e) {
                    // handle subscription error, e.g.:
                    // - SubscriptionException if the subscription registration failed at the provider
                    // - PublicationMissedException if a periodic subscription publication does not
                    //   arrive in time

                }
            },
            qos
            );
    } catch (JoynrRuntimeException e) {
        // handle error
    }
    ...
    // get the subscriptionId from the Future when needed
    String subscriptionId;
    if (subscriptionIdFuture != null) {
        try {
            subscriptionId = subscriptionIdFuture.get();
        } catch (JoynrRuntimeException | InterruptedException | ApplicationException e) {
            // handle error
        }
    }
}
```

## Updating an attribute subscription

The subscribeTo method can also be used to update an existing subscription, when the **subscriptionId** is given as additional parameter as follows:

```java
    try {
        subscriptionIdFuture = <interface>Proxy.subscribeTo<Attribute>(
            subscriptionId,
            new AttributeSubscriptionAdapter<AttributeType>() {
                // Gets called on every received publication
                @Override
                public void onReceive(<AttributeType> value) {
                    // handle info
                }

                // Gets called when the subscription is successfully updated at the provider
                @Override
                public void onSubscribed(String subscriptionId) {
                    // save the subscriptionId for updating the subscription or unsubscribing from it
                    // the subscriptionId can also be taken from the future returned by the subscribeTo call
                }

                // Gets called on every error that is detected on the subscription
                @Override
                public void onError(JoynrRuntimeException e) {
                    // handle subscription error, e.g.:
                    // - SubscriptionException if the subscription registration failed at the provider
                    // - PublicationMissedException if a periodic subscription publication does not
                    //   arrive in time
                }
            },
            qos
        );
    } catch (JoynrRuntimeException e) {
        // handle error
    }
```

## Unsubscribing from an attribute

Unsubscribing from an attribute subscription also requires the **subscriptionId** returned by the ealier subscribeTo call.

```java
// for required imports see subscription info
...
public void run() {
    // setup proxy named "<interface>Proxy"
    ...
    private String subscriptionId;
    ...
    try {
        // subscriptionId must have been assigned by previous call
        ...
        <interface>Proxy.unsubscribeFrom<Attribute>(subscriptionId);
    } catch (JoynrRuntimeException e) {
        // handle error
    }
}
```

## Subscribing to a (non-selective) broadcast

A broadcast subscription informs the application in case a broadcast is fired by a provider.
The output value is returned to the consumer via a callback function.

A broadcast is selective only if it is declared with the selective keyword in Franca, otherwise it
is non-selective.

Non-selective broadcast subscriptions can be passed optional **partitions**. A partition is a
hierarchical list of strings similar to a URL path. Subscribing to a partition will cause only those
broadcasts to be sent to the consumer that match the partition. Note that the partition is set when
subscribing on the consumer side, and must match the partition set on the provider side when the
broadcast is performed.

Example: a consumer could set a partition of "europe", "germany", "munich" to receive broadcasts for
Munich only. The matching provider would use the same partition when sending the broadcast.

The **subscriptionId** can be retrieved via the callback (onSubscribed) and via the future returned
by the subscribeTo call. It can be used later to update the subscription or to unsubscribe from it.
The subscriptionId will be available when the subscription is successfully registered at the
provider. If the subscription failed, a SubscriptionException will be returned via the callback
(onError) and thrown by future.get().

To receive the subscription, a **callback** has to be provided which is done providing a listener
class as outlined below. Since the callback is called by a communication middleware thread, it
should not be blocked, wait for user interaction, or do larger computation. The callback methods
(onReceive, onSubscribed, onError) are optional. Only the required methods have to be implemented.

```java
import joynr.MulticastSubscriptionQos;
...
// for any Franca broadcast named "<Broadcast>" used
import joynr.<Package>.<Interface>BroadcastInterface.<Broadcast>BroadcastAdapter;
...
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
public void run() {
    // setup proxy named "<interface>Proxy"
    ...
    private Future<String> subscriptionIdFuture;
    ...
    try {
        long expiryDateMs;
        int publicationTtlMs;
        String partitionLevel1;
        String partitionLevel2;
        ...
        // provide values for expiryDateMs, publicationTtlMs here
        ...
        MulticastSubscriptionQos qos =
            new MulticastSubscriptionQos()
                .setExpiryDateMs(expiryDateMs)
                .setPublicationTtlMs(publicationTtlMs);
        ...
        subscriptionIdFuture = <interface>Proxy.subscribeTo<Broadcast>Broadcast(
            new <Broadcast>BroadcastAdapter() {
                // Gets called on every received publication
                @Override
                public void onReceive(<AttributeType> value) {
                    // handle broadcast info
                }

                // Gets called when the subscription is successfully registered at the provider
                @Override
                public void onSubscribed(String subscriptionId) {
                    // save the subscriptionId for updating the subscription or unsubscribing from it
                    // the subscriptionId can also be taken from the future returned by the subscribeTo call
                }

                // Gets called on every error that is detected on the subscription
                @Override
                public void onError(JoynrRuntimeException e) {
                    // handle error
                }
            },
            qos,
            partitionLevel1, // optional partitions
            ...
            partitionLevelN // optional partitions
        );
        ...
    } catch (DiscoveryException e) {
        // handle error
    } catch (JoynrCommunicationExceptin e) {
        // handle error
    }
    ...
    // get the subscriptionId from the Future when needed
    String subscriptionId;
    if (subscriptionIdFuture != null) {
        try {
            subscriptionId = subscriptionIdFuture.get();
        } catch (JoynrRuntimeException | InterruptedException | ApplicationException e) {
            // handle error
        }
    }
}
```

## Updating a (non-selective) broadcast subscription

The subscribeTo method can also be used to update an existing subscription, when the
**subscriptionId** is passed as additional parameter as follows:

```java
subscriptionIdFuture = <interface>Proxy.subscribeTo<Broadcast>Broadcast(
    subscriptionId,
    new <Broadcast>BroadcastAdapter() {
        // Gets called on every received publication
        @Override
        public void onReceive(<AttributeType> value) {
            // handle broadcast info
        }

        // Gets called when the subscription is successfully updated at the provider
        @Override
        public void onSubscribed(String subscriptionId) {
            // save the subscriptionId for updating the subscription or unsubscribing from it
            // the subscriptionId can also be taken from the future returned by the subscribeTo call
        }

        // Gets called on every error that is detected on the subscription
        @Override
        public void onError(JoynrRuntimeException e) {
            // handle error
        }
    },
    qos,
    partitionLevel1, // optional partitions
    ...
    partitionLevelN // optional partitions
);
```

## Subscribing to a selective broadcast, i.e. a broadcast with filter parameters

Selective Broadcasts use filter logic implemented by the provider and filter parameters set by the
consumer to send only those broadcasts from the provider to the consumer that pass the filter. The
broadcast output values are passed to the consumer via callback.

The **subscriptionId** can be retrieved via the callback (onSubscribed) and via the future returned
by the subscribeTo call (see section
[Subscribing to a (non-selective) broadcast](#subscribing-to-a-%28non-selective%29-broadcast)).

To receive the subscription, a **callback** has to be provided (cf. section
[Subscribing to a (non-selective) broadcast](#subscribing-to-a-%28non-selective%29-broadcast)).

In addition to the normal broadcast subscription, the filter parameters for this broadcast must be
created and initialized as additional parameters to the ```subscribeTo``` method. These filter
parameters are used to receive only those broadcasts matching the provided filter criteria.

```java
import joynr.OnChangeSubscriptionQos;
...
// for any Franca broadcast named "<Broadcast>" used
import joynr.<Package>.<Interface>BroadcastInterface.<Broadcast>BroadcastAdapter;
import joynr.<Package>.<Interface>BroadcastInterface.<Broadcast>FilterParameters;
...
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
public void run() {
    // setup proxy named "<interface>Proxy"
    ...
    private Future<String> subscriptionIdFuture;
    ...
    try {
        long minIntervalMs;
        long expiryDateMs;
        long publicationTtlMs;
        long validityMs;
        ...
        // provide values for minIntervalMs, expiryDateMs, publicationTtlMs, validityMs here
        ...
        OnChangeSubscriptionQos qos =
            new OnChangeSubscriptionQos()
                .setMinIntervalMs(minIntervalMs)
                .setExpiryDateMs(expiryDateMs)
                .setPublicationTtlMs(publicationTtlMs)
                .setValidityMs(validityMs);

        <Broadcast>FilterParameters filter = new <Broadcast>FilterParameters();
        // foreach BroadcastFilterAttribute of that filter
        filter.setBroadcastFilter<Attribute>(value);
        ...
        subscriptionIdFuture = <interface>Proxy.subscribeTo<Broadcast>Broadcast(
            new <Broadcast>BroadcastAdapter() {
                // Gets called on every received publication
                @Override
                public void onReceive(<AttributeType> value) {
                    // handle broadcast info
                }

                // Gets called when the subscription is successfully registered at the provider
                @Override
                public void onSubscribed(String subscriptionId) {
                    // save the subscriptionId for updating the subscription or unsubscribing from it
                    // the subscriptionId can also be taken from the future returned by the subscribeTo call
                }

                // Gets called on every error that is detected on the subscription
                @Override
                public void onError(JoynrRuntimeException e) {
                    // handle error
                }
            },
            qos,
            filter
        );
        ...
    } catch (DiscoveryException e) {
        // handle error
    } catch (JoynrCommunicationExceptin e) {
        // handle error
    }
    ...
    // to retrieve the subscriptionId, please refer to section "subscribing to a broadcast unconditionally"
}
```

## Updating a broadcast subscription with filter parameters

The subscribeTo method can also be used to update an existing subscription, when the
**subscriptionId** is passed as additional parameter as follows:

```java
        subscriptionIdFuture = <interface>Proxy.subscribeTo<Broadcast>Broadcast(
            subscriptionId,
            new <Broadcast>BroadcastAdapter() {
                // Gets called on every received publication
                @Override
                public void onReceive(... OutputParameters ...) {
                    // handle broadcast info
                }

                // Gets called when the subscription is successfully updated at the provider
                @Override
                public void onSubscribed(String subscriptionId) {
                    // save the subscriptionId for updating the subscription or unsubscribing from it
                    // the subscriptionId can also be taken from the future returned by the subscribeTo call
                }

                // Gets called on every error that is detected on the subscription
                @Override
                public void onError(JoynrRuntimeException e) {
                    // handle error
                }
            },
            qos,
            filter
        );
```

## Unsubscribing from a broadcast

Unsubscribing from a broadcast subscription requires the **subscriptionId** returned by the earlier
subscribe call.

```java
public void run() {
    // setup proxy named "<interface>Proxy"
    ...
    private String subscriptionId;
    ...
    try {
        <interface>Proxy.unsubscribeFrom<Broadcast>Broadcast(subscriptionId);
        ...
    } catch (DiscoveryException e) {
        // handle error
    } catch (JoynrCommunicationExceptin e) {
        // handle error
    }
    ...
}
```

## The shutdown method
The shutdown method should be called on exit of the application. Inside the ```shutdown()```
method , the consumer should unsubscribe from any attributes and broadcasts it was subscribed to and
terminate the instance.

```java
@Override
public void shutdown() {
    // for all proxies
    if (<interface>Proxy != null) {
         if (subscribed) {
             // unsubscribe from attributes
             // unsubscribe from broadcasts
        }
    }

    runtime.shutdown(true);
    try {
         Thread.sleep(3000);
    } catch (InterruptedException e) {
          // handle error
    }
    System.exit(0);
}
```

# Building a Java Provider application

The Java Provider mainly consists of the following classes:

* A  generic **Provider Application Class**
* One **Provider Class** for each Franca interface to be supported

## The MyProviderApplication class

The provider application class is used to register a provider class for each Franca interface to be
supported.

### Required Imports

```java
import io.joynr.accesscontrol.StaticDomainAccessControlProvisioning;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import java.io.IOException;
import java.util.Properties;
import com.google.inject.Inject;
import com.google.inject.Module;
import edu.umd.cs.findbugs.annotations.SuppressWarnings;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
```

### The base class
The class must extend ```AbstractJoynrApplication``` and can theoretically serve multiple Franca
interfaces.

For each Franca interface implemented, the providing application creates an instance of
```My<Interface>Provider```, which implements the service for that particular interface, and
registers it as a provider at the Joynr Middleware.

The example below shows the code for one interface:

```java
package myPackage;
...
// required imports
...
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
public class MyProviderApplication extends AbstractJoynrApplication {
    private static final String AUTH_TOKEN = "MyProvider_authToken";
    public static final String STATIC_PERSISTENCE_FILE = "provider-joynr.properties";
    private My<Interface>Provider <interface>Provider = null;

    public static void main(String[] args) {
        // ...
    }

    @Override
    public void run() {
        // ...
    }

    @Override
    public void shutdown() {
        // ...
    }
}

```

### The main method

```java

public static void main(String[] args) {
    String localDomain = "<ProviderDomain>";
    Properties joynrConfig = new Properties();
    joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
    joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, localDomain);
    Properties appConfig = new Properties();

    // OPTIONAL: configure access control if required (access control is diabled by default)
    provisionAccessControl(joynrConfig, localDomain);

    Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(new HivemqMqttClientModule());
    JoynrApplication joynrApplication =
        new JoynrInjectorFactory(joynrConfig,
                                 runtimeModule).createApplication(
            new JoynrApplicationModule(MyProviderApplication.class, appConfig)
        );
    joynrApplication.run();
    joynrApplication.shutdown();
}
```

### The Provider quality of service

The ```ProviderQos``` has the following members:

* **customParameters** e.g. the key-value for the arbitration strategy Keyword during discovery
* **priority** the priority used for arbitration strategy HighestPriority during discovery
* **scope** the Provider scope (see below), used in discovery
* **supportsOnChangeSubscriptions** whether the provider supports subscriptions on changes

The **ProviderScope** can be
* **LOCAL** The provider will be registered in the local capability directory
* **GLOBAL** The provider will be registered in the local and global capability directory

Example:

```java
ProviderQos providerQos = new ProviderQos();
providerQos.setCustomParameters(customParameters);
providerQos.setPriority(100);
providerQos.setScope(ProviderScope.GLOBAL);
providerQos.setSupportsOnChangeSubscriptions(true);
```

### The run method
The run method registers the interface specific provider class instance. From that
time on, the provider will be reachable from outside and react on incoming requests (e.g. method
RPC etc.). It can be found by consumers through Discovery.
Any specific broadcast filters must be added prior to registry.

```java
@Override
public void run() {
    <interface>Provider = new My<Interface>Provider();

    // for any filter of a broadcast with filter
    <interface>Provider.addBroadcastFilter(new <Filter>BroadcastFilter());
    ProviderQos providerQos = new ProviderQos();
    // use setters on providerQos as required
    // set the priority, used for arbitration by highest priority
    long priorityValue;
    // set priorityValue
    providerQos.setPriority(priorityValue);

    Future<Void> future = runtime.getProviderRegistrar(localDomain, <interface>Provider)
                                 // if no providerQos is set, a default providerQos will be used
                                 .withProviderQos(providerQos) // optional, see above
                                 .withGbids(gbids) // optional, see below
                                 .awaitGlobalRegistration() // optional
                                 .register();
    try {
        future.get();
    }
    catch (ApplicationException a) {
        // handle modelled DiscoveryError
        switch ((DiscoveryError) a.getError()) {
            case DiscoveryError.UNKNOWN_GBID:
                // one of the selected GBIDs is not known
                // either at the cluster-controller or at the GlobalCapabilitiesDirectory
            case DiscoveryError.INVALID_GBID:
                // one of the selected GBIDs is invalid, e.g. empty or duplicated
                break;
            case DiscoveryError.INTERNAL_ERROR:
                // other error at the cluster-controller or GlobalCapabilitiesDirectory
            case DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS:
                // not applicable for provider registration
                break;
            case DiscoveryError.NO_ENTRY_FOR_PARTICIPANT:
                // not applicable for provider registration
                break;
            default:
                // handle default
        }
    }
    catch (JoynrRuntimeException e) {
        // handle other errors, e.g. JoynrTimeoutException, ProviderRuntimeException
    }

    // loop here
}
```

### Register provider

If not specified otherwise, a new provider is registered in all known backends (defined by
`PROPERTY_GBIDS` in [Joynr Java Settings](JavaSettings.md) at the cluster controller).
To register a provider only to a subset of the known backends, the custom **GBID**s have to be
defined by calling `withGbids(selectedGbids)` in the `ProviderRegistrar`.
The `selectedGbids` must contain a subset of the **GBID**s configured at the cluster controller.
The global capabilities directory identified by the first selected **GBID** performs the
registration.

```java
...
@Override
public void run() {
...
    Future<Void> future = runtime.getProviderRegistrar(localDomain, <interface>Provider)
                                 .withProviderQos(someProviderQos) // optional
                                 .withGbids(new String[] { "gbid1","gbid2" }) // optional
                                 .awaitGlobalRegistration() // optional
                                 .register();
    try {
        future.get();
    }
    catch (ApplicationException a) {
        // handle modelled DiscoveryError, see above
    }
    catch (JoynrRuntimeException e) {
        // handle JoynrRuntimeException
    }
...
}

```

The passed GBIDs have to be known to the cluster-controller, which means that they also have to be
defined in `PROPERTY_GBIDS`[Joynr Java Settings](JavaSettings.md).

### Register provider with fixed (custom) participantId

By default, joynr generates a participantID for the provider instance. This participantID is
persisted (see PROPERTY_PARTICIPANTIDS_PERSISTENCE_FILE in [Joynr Java Settings](JavaSettings.md))
and reused when a provider for the same interface is registered again on the same domain
(e.g. after a restart).

A provider can also be registered with a fixed (predefined) participantID by adding a property to
the joynrConfig, see [The main method](#the-main-method), before the runtimeModule is created.

```java
import io.joynr.capabilities.ParticipantIdKeyUtil;
...
public static void main(String[] args) {
...
    joynrConfig.setProperty(
            ParticipantIdKeyUtil.getProviderParticipantIdKey(localDomain, <interface>Provider.class),
            <customProviderParticipantID>);
...
}
```

The property key is also used as key in the persistence file. It is created as follows:
`<JOYNR_PARTICIPANT_PREFIX><DOMAIN>.<INTERFACE_NAME>.v<MAJOR_VERSION>`
It is strongly recommended to use ParticipantIdKeyUtil to create the key, as the key format might
change again in the future.

> Note:
> The provided fixed participantId is only used if there is no entry in the persistence file.
> If the provider has already been registered with a generated (default) participantId before, the
> persistence file or the entry for the provider has to be deleted to enable the fixed participantId.

### The shutdown method
The `shutdown` method should be called on exit of the application. It should unregister any
providers the application had registered earlier.

```java
@Override
@SuppressWarnings(value = "DM_EXIT", justification = "WORKAROUND to be removed")
public void shutdown() {
    if (<interface>Provider != null) {
        try {
            runtime.unregisterProvider(localDomain, <interface>Provider);
        } catch (JoynrRuntimeException e) {
            // handle error
        }
    }
    runtime.prepareForShutdown();
    runtime.shutdown(true);
    try {
        Thread.sleep(3000);
    } catch (InterruptedException e) {
        // do nothing; exiting application
    }
    System.exit(0);
}
```
The `unregisterProvider` method just triggers the removal of the provider. It does not wait for a response and does not
get informed about errors or success. During the `shutdown`, the joynr runtime waits for a maximum number of 5
seconds for a response from the local capabilities directory of the cluster controller (standalone or embedded
within the same runtime).

__IMPORTANT__: A successful response from the local capabilities directory does not guarantee a successful execution
of provider's removal from the global capabilities directory in case the provider is registered globally, i.e
`ProviderScope.GLOBAL` has been set in `ProviderQos` when registering the provider.

If the provider is running in libjoynr runtime connected to a standalone cluster controller, the cluster controller
will still repeat the global remove operation until it succeeds or the cluster controller is shut down.

If the cluster controller is embedded within the same runtime (cluster controller runtime) and the provider is
registered globally, consider waiting some grace period after calling `unregisterProvider` before calling `shutdown`
to give the joynr framework a chance to perform the provider removal.

**Graceful shutdown**: joynr runtime implements `prepareForShutdown` method which allows to perform
a graceful shutdown. Calling `prepareForShutdown` gives the joynr runtime a chance to stop receiving
incoming requests and work off any messages still in its queue.
Thus your application logic may still be called from joynr after your call to `prepareForShutdown`. Be
aware that if your logic requires to make calls via joynr in response to those received messages,
you will only be able to call non-stateful methods such as fire-and-forget. Calls to stateful
methods, such as those in the Sync interface, will result in a `JoynrIllegalStateException`
being thrown.

The `prepareForShutdown` method will block until either the joynr runtime has finished processing
all messages in the queue or a timeout occurs. The timeout can be configured via the
`PROPERTY_PREPARE_FOR_SHUTDOWN_TIMEOUT` property. See the
[Java Configuration Guide](./JavaSettings.md) for details.

More detailed view on how does `prepareForShutdown` work:
ShutdownNotifier will call `prepareForShutdown` methods implementations in a certain order:
1. HivemqMqttClientFactory - it will shut down all of its receiver and will not receive new messages
2. ProxyInvocationHandler - it will set `preparingForShutdown` to true
3. MessageTrackerForGracefulShutdown - it will wait for a specified amount of time to let messages unregister
4. All remaining `prepareForShutdownListener` implementations

### Access control

The following allows anyone to access interface:

```java
private static void provisionAccessControl(Properties properties, String domain) throws Exception {
   ObjectMapper objectMapper = new ObjectMapper();
   objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
   MasterAccessControlEntry newMasterAccessControlEntry = new MasterAccessControlEntry(
       "*",
       domain,
       MyProvider.INTERFACE_NAME,
       TrustLevel.LOW,
       Arrays.asList(TrustLevel.LOW),
       TrustLevel.LOW,
       Arrays.asList(TrustLevel.LOW),
       "*",
       Permission.YES,
       Arrays.asList(Permission.YES)
   );
   MasterAccessControlEntry[] provisionedAccessControlEntries = { newMasterAccessControlEntry };
   String provisionedAccessControlEntriesAsJson = objectMapper.writeValueAsString(provisionedAccessControlEntries);
   properties.setProperty(StaticDomainAccessControlProvisioning.PROPERTY_PROVISIONED_MASTER_ACCESSCONTROLENTRIES,
       provisionedAccessControlEntriesAsJson);
}
```
> NOTE: This configuration is only required if access control is enabled at the cluster controller.
By default, it is disabled.

### Accessing custom headers from a provider

In case a provider method has to access custom headers it can do so using the following code snippet:

```java
    HashMap <String, String> customHeadersMap = new HashMap<>();
    for (Map.Entry<String, Serializable> entry : AbstractJoynrProvider.getCallContext()
                                                                      .getContext()
                                                                      .entrySet()) {
        customHeadersMap.put(entry.getKey(), entry.getValue().toString());
    }
```

Check out our [custom headers example](../examples/custom-headers/README.md) for a more comprehensive example.

CustomHeaders can be provided on the consumer side via the `MessagingQos` when building a proxy
or as optional `MessagingQos` parameter when executing a method call.

Additional CustomHeaders can be added and existing CustomHeaders can be modified by the MQTT broker
if the broker is part of the communication process. In that case, the value set by the MQTT broker
takes precedence over a value set by `MessagingQos` on the consumer side as mentioned above.

## The My&lt;Interface>Provider class

The provider class implements the **attributes**, **methods** and **broadcasts** of a particular
Franca interface.

### Required imports
The following Joynr Java imports are required:

```java
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.<Package>.<Interface>AbstractProvider;
```

### The base class
The provider class must either extend the generated class `Default<Interface>Provider` or
alternatively at least its super class ```<Interface>AbstractProvider```.

In the latter case it must as well implement getter and / or setter methods itself for each Franca
attribute (where required). The class `Default<Interface>Provider` already includes default
implementations of getter and setter methods (where required).

In both cases it must implement a method for each method of the Franca interface. In order to send
broadcasts the generated code of the super class ```<Interface>AbstractProvider``` can be used.

If the value of a notifiable attribute gets changed directly inside the implementation of a method
or (non-default) setter, the `<Attribute>Changed(<Attribute>)` method needs to be called in order
to inform subscribers about the value change.

```java
package myPackage;
...
// required imports
...
public class My<Interface>Provider extends <Interface>AbstractProvider {
    // member variables realizing the Franca interfaces Attributes go here, if any
    <AttributeType> <Attribute>;
    ...
    // default constructor
    public My<Interface>Provider() {
        // initialize members and attributes here, if any
    }
    ...
    // foreach Franca interface "<Attribute>" provide a getter method
    ...
    // foreach Franca interface "<method>" provide an implementation
    ...
    // foreach Franca "<broadcast>" you can use the provided method to fire the event
    ...
}
```

### Providing attribute getters
The asynchronous getter methods return the current value of an attribute. Since the current thread
is blocked while the getter runs, activity should be kept as short as possible. In most cases, when
a simple element is returned, the method can resolve the Promise immediately. However, if longer
activity is required, it should be done in the background and the deferred should also be resolved
by a background thread.

```java
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
@Override
public Promise<Deferred<<AttributeType>>> get<Attribute>() {
    Deferred<<AttributeType>> deferred = new Deferred<<AttributeType>>();
    <AttributeType> value;
    ...
    // start some activity to get the value
    // if complex, execute this asynchronously;
    // once the value is available, resolve the Promise
    // may be run from background thread, if required
    deferred.resolve(value);
    // if an error occurs, the Deferred can be rejected with a ProviderRuntimeException
    deferred.reject(new ProviderRuntimeException(<errorMessage>));
    ...
    // from current thread
    return new Promise<Deferred<<AttributeType>>>(deferred);
}
```

### Providing attribute setters
Since the current thread is blocked while the setter runs, activity should be kept as short as
possible. In most cases, when a simple element is returned, the method can resolve the Promise
immediately. However, if longer activity is required, it should be done in the background and the
deferred should also be resolved by a background thread.

```java
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
@Override
public Promise<DeferredVoid> set<Attribute>(<AttributeType> <attribute>) {
    DeferredVoid deferred = new DeferredVoid();
    ...
    // start some activity to set the value
    // if complex, execute this asynchronously;
    // once the value is set, resolve the Promise
    // may be run from background thread, if required
    deferred.resolve();
    // if an error occurs, the Deferred can be rejected with a ProviderRuntimeException
    deferred.reject(new ProviderRuntimeException(<errorMessage>));
    // if attribute is notifiable (not marked as noSubscriptions in the Franca model),
    // inform subscribers about the value change
    <Attribute>Changed(<Attribute>);
    ...
    // from current thread
    return new Promise<DeferredVoid>(deferred);
}
```

### Implementing a Franca RPC method
The provider should always implement RPC calls asynchronously in order to not block the main thread
longer than required. Also it needs to take care not to overload the server, e.g. it must not accept
unlimited amount of RPC requests causing background activity. After exceeding a limit, further calls
should be rejected until the number of outstanding activities falls below the limit again.

```java
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
// in case of single return parameter

@Override
public Promise<Deferred<<ReturnType>>> <method>(... parameters ...) {
    Deferred<<ReturnType>> deferred = new Deferred<<ReturnType>>();
    <ReturnType> returnValue;
    ...
    // start some activity to perform the task;
    // if complex, execute this asynchronously by background thread;
    // once the task is finished, resolve the Promise providing
    // the returnValue, if any (see following line).
    deferred.resolve(returnValue);

    // For methods which are modelled with error enumerations, the Promise can be rejected with such
    // an error enumeration. It is then wrapped in an ApplicationException which serves as container
    // for the actual error enumeration.
    deferred.reject(<ErrorEnum>.<VALUE>);

    // If no errors are modelled, the Deferred can be rejected with a ProviderRuntimeException
    deferred.reject(new ProviderRuntimeException(<errorMessage>));
    ...
    // from current thread
    return new Promise<Deferred<<ReturnType>>>(deferred);
}

// in case of multiple return parameters

@Override
public Promise<<Method>Deferred>> <method>(... parameters ...) {
    <Method>Deferred deferred = new <Method>Deferred();
    <ReturnType1> returnValue1;
    ...
    <ReturnTypeN> returnValueN;
    ...
    // start some activity to perform the task;
    // if complex, execute this asynchronously by background thread;
    // once the task is finished, resolve the Promise providing
    // the returnValue, if any (see following line).
    deferred.resolve(returnValue1, ..., returnValueN);

    // For methods which are modelled with error enumerations, the Promise can be rejected with such
    // an error enumeration. It is then wrapped in an ApplicationException which serves as container
    // for the actual error enumeration.
    deferred.reject(<ErrorEnum>.<VALUE>);

    // If no errors are modelled, the Deferred can be rejected with a ProviderRuntimeException
    deferred.reject(new ProviderRuntimeException(<errorMessage>));
    ...
    // from current thread
    return new Promise<<Method>Deferred>(deferred);
}
```

### Firing a broadcast
A broadcast can be emitted using the following method. Note that firing a broadcast blocks the
current thread until the message is serialized.

```java
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
public void fire<Broadcast>Event {
    <OutputValueType1> outputValue1;
    ...
    <OutputValueTypeN> outputValueN;
    ...
    // setup outputValue(s)
    ...
    // use the method provided by generators to send the broadcast
    fire<Broadcast>(outputValue1, ... , outputValueN);
}
```

Optionally a **partition** can be set when firing a (non-selective) broadcast:

```java
fire<Broadcast>(outputValue1,
                outputValue2,
                partitionLevel1,
                ...
                partitionLevelN);
```
// Note: wildcards are only allowed on consumer side
The [partition syntax is explained in the multicast concept](../docs/multicast.md#partitions)

## Selective (filtered) broadcasts
In contrast to unfiltered broadcasts, to realize selective (filtered) broadcasts, the filter logic
has to be implemented and registered by the provider. If multiple filters are registered on the same
provider and broadcast, all filters are applied in a chain and the broadcast is only delivered if
all filters in the chain return true.

### The broadcast filter classes
A broadcast filter class implements a filtering function called ```filter()``` which returns a
boolean value indicating whether the broadcast should be delivered. The input parameters of the
```filter()``` method reflect the output values of the broadcast.

```java
import joynr.<Package>.<Interface>BroadcastInterface.<Broadcast>BroadcastFilterParameters;
import joynr.<Package>.<Interface><Broadcast>BroadcastFilter;
// for any Franca type named "<Type>" used
import joynr.<Package>.<TypeCollection>.<Type>;
...
public class <Filter>BroadcastFilter extends <Interface><Broadcast>BroadcastFilter {
    ...
    @Override
    public boolean filter(
        <OutputValueType1> outputValue1,
        ...
        <OutputValueTypeN> outputValueN,
        <Broadcast>BroadcastFilterParameters filterParameters
    ) {
        boolean returnValue;
        ...
        // calculate result
        ...
        return returnValue;
    }
}
```

# Joynr Status Monitoring

Joynr provides metrics to monitor the connectivity status of its connections which can be used to
detect invalid states and situations which require a restart of an instance. Currently,
ConnectionStatusMetrics are only available for HivemqMqttClient. In order to access this information,
inject an implementation of the following interface via Guice:
* `io.joynr.statusmetrics.JoynrStatusMetrics`

Joynr provides `io.joynr.statusmetrics.JoynrStatusMetricsAggregator`
as implementation for this interface by default.

Via the JoynrStatusMetrics interface, you can call the method `getConnectionStatusMetrics(gbid)`
to retrieve a list of ConnectionStatusMetrics objects for all connections that are established
to the specified gbid. You can also retrieve all available metrics (independent of a GBID) via the method `getAllConnectionStatusMetrics()`.  
JoynrStatusMetrics also offers the method `getNumDroppedMessages()` that returns the accumulated
amount of messages dropped by all connections.

The ConnectionStatusMetrics class offers the following metrics:
* boolean isSender() // Returns whether the represented connection is configured to be a sender.
* boolean isReceiver() // Returns whether the represented connection is configured to be a receiver.
* boolean isReplyReceiver() // Returns whether the represented connection is configured to be a reply receiver. If true, isReceiver() is also true.
* boolean isConnected() // Returns whether the represented connection is currently connected.
* Instant getLastStateChange() // Returns the point in time where the connection state of the
  represented connection last changed.
* long getReceivedMessages() // Returns the total amount of messages received via this connection.
* long getSentMessages() // Returns the total amount of messages sent via this connection.
* long getConnectionDrops() // Returns the number of times this connection dropped.
* long getConnectionAttempts() // Returns the total number of performed connection attempts on the
  represented connection.  
  NOTE: HivemqMqttClient currently only reports initial connection attempts. Reconnect attempts after
  a connection loss are not available.

See the documentation of the `JoynrStatusMetrics` interface for more information.
