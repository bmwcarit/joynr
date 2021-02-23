# Joynr Javascript Developer Guide

## Conversion of Franca entries

### Place holders

Note that the following notations in the code examples below must be replaced by actual values from
Franca:

```
// "«Attribute»" the Franca name of the attribute
// "«AttributeType»" the Franca name of the attribute type
// "«broadcast»" the Franca name of the broadcast, starting with a lowercase letter
// "«Broadcast»" the Franca name of the broadcast, starting with capital letter
// "BroadcastFilter«Attribute»" Attribute is the Franca attributes name
// "«Filter»" the Franca name of the broadcast filter
// "«interface»" the Franca interface name, starting with a lowercase letter
// "«Interface»" the Franca interface name, starting with capital letter
// "«method»" the Franca method name, starting with a lowercase letter
// "«Method»" the Franca method name, starting with capital letter
// "«OutputType»" the Franca broadcast output type name
// "«Package»" the Franca package name
// "«ProviderDomain»" the provider domain name used by provider and client
// "«ReturnType»" the Franca return type name
```

### Package name
The Franca ```«Package»``` will be transformed to the Javascript module ```joynr.«Package»```.

### Type collection name

The Franca ```«TypeCollection»``` will be transformed to the Javascript
module ```joynr.«Package».«TypeCollection»```.

### Complex type name

Any Franca complex type ```«TypeCollection».«Type»``` will result in the creation of an object
```joynr.«Package».«TypeCollection».«Type»``` (see above).

The same ```«Type»``` will be used for all elements in the event that this type is used as an
element of other complex types, as a method input or output argument, or as a broadcast output
argument.

### Interface name

The Franca ```«Interface»``` will be used as a prefix to create the following JavaScript objects:

```
«Interface»Provider
«Interface»Proxy
```

### Attribute, Method and Broadcast names

In Javascript the names of attributes, methods and broadcasts within the same interface must be
unique, as each name will become a property of the Proxy object.

# Building a Javascript application

A Javascript joynr application must import `joynr` and call
`joynr.load(provisioning, onFatalRuntimeError?)` in order to load the joynr API. The callback
 `onFatalRuntimeError` is optional but highly recommended to provide an implementation. After the
 promise resolves, joynr can be used to build either a
 [consumer application](#Building a Javascript consumer application), a [provider application]
 (#Building a Javascript Provider application) or a combination of both.

## Required imports

The joynr API uses common-js based exports. Files should be imported using require.
Joynr types may be added on top of the exported namespaces or in separate es-module based files.

```typescript
// common-js based import of joynr
import joynr = require("joynr");
// es-module based import of joynr Provisioning
import {WebSocketLibjoynrProvisioning} from "joynr/joynr/start/interface/Provisioning";
// common-js based import of Enum DiscoveryScope and DiscoveryScopeMembers;
import DiscoveryScope = require("joynr/generated/joynr/types/DiscoveryScope");
import { DiscoveryScopeMembers } from "joynr/generated/joynr/types/DiscoveryScope";
// or es-module based with "esModuleInterop": true flag
import DiscoveryScope, {
    DiscoveryScopeMembers
} from "joynr/generated/joynr/types/DiscoveryScope";
```

Joynr exports all depending objects on top of the main file.  
An overview can be found by looking at libjoynr-deps.ts or looking at the typescript signatures.
Either importing the files directly or using them from on top of joynr gives the same reference,
such that both ways of using them are possible.  
Generally, joynr used to advocate the usage from on top of joynr, but there are several arguments
against it and it's now recommended to use the types directly.
* Auto import features of IDEs facilitate direct imports
* Typescript types need to be imported by file and aren't available on top of joynr.ts
* Importing from on top of joynr makes mocking more difficult.
* Importing from on top of joynr creates unnecessary dependencies to the rest of the joynr code.

Any other objects created by the runtime should be taken from the runtime and not imported directly.

```typescript
import joynr = require("joynr");
import DiscoveryScope = require("joynr/generated/joynr/types/DiscoveryScope");
let isEqual = DiscoveryScope === joynr.types.DiscoveryScope; // true

import MessagingQos = require("joynr/messaging/MessagingQos");
isEqual = MessagingQos === joynr.messaging.MessagingQos; // true

import {Settings} from "joynr/messaging/MessagingQos";
// type Settings = joynr["messaging"]["MessagingQos"]["Settings"];
// Does not work as tsc -d swallows the types from the overloaded namespaces.
```

## Base implementation

The Javascript application must load and initialize the joynr runtime environment prior to calling
any other Joynr API.

At the end of the application lifetime, the joynr runtime must be shutdown in order to properly
deallocate resources and end any background activity. Providers can be automatically unregistered,
by setting `provisioning.shutdownSettings.clearSubscriptionsEnabled` to true.

```typescript
import joynr = require("joynr");
import {WebSocketLibjoynrProvisioning} from "joynr/joynr/start/interface/Provisioning"
const provisioning: WebSocketLibjoynrProvisioning = {
  shutdownSettings: {
    clearSubscriptionsEnabled: true
  }
}; // ignore other settings for this example.

// onFatalRuntimeError callback is optional, but it is highly recommended to provide an
// implementation. Here is an example:
const onFatalRuntimeError = (error: JoynrRuntimeException) => {
    console.log(`Unexpected joynr runtime error occurred: ${error}`);
};

await joynr.load(provisioning, onFatalRuntimeError);

// build proxies/provider here as described later.

// main application code

// shutdown the runtime at the end of the application's lifetime
await joynr.shutdown();

```

## Provisioning

The Typescript interface can be found in `"joynr/joynr/start/interface/Provisioning"`
Textual explanation of most settings can be found in the [javascript tutorial](JavaScriptSettings.md)

# Building a Javascript consumer application

For all Franca interfaces that are to be used, a **proxy** must be created using the
```build()``` method of the ```joynr.proxyBuilder``` object, with the provider's domain passed as
an argument.

Once the proxy has been successfully created, the application can add any attribute or broadcast
subscriptions it needs, and then enter its event loop where it can call the interface methods.

## The discovery quality of service

The ```DiscoveryQos``` configures how the search for a provider will be handled. It has the
following members:

* **discoveryTimeoutMs**  Timeout for the discovery process (milliseconds) if no compatible
  provider was found within the given time. A timeout triggers a DiscoveryException or
  NoCompatibleProviderFoundException containing the versions of the discovered incompatible
  providers.
* **discoveryRetryDelayMs** The time to wait between discovery retries after encountering a
  discovery error.
* **arbitrationStrategy** The arbitration strategy (details see below)
* **cacheMaxAgeMs** Defines the maximum allowed age of cached entries (milliseconds); only younger
  entries will be considered. If no suitable providers are found, depending on the discoveryScope,
  a remote global lookup may be triggered.
* **discoveryScope** The discovery scope (details see below)
* **providerMustSupportOnChange** If set to true, select only providers which support onChange
  subscriptions (set by the provider in its providerQos settings)
* **additionalParameters** special application-specific parameters that must match, e.g. a keyword

The enumeration **discoveryScope** defines options to decide whether a suitable provider will be
searched in the local capabilities directory or in the global one.

Available values are as follows:

* **LOCAL\_ONLY** Only entries from local capability directory will be searched
* **LOCAL\_THEN\_GLOBAL** Entries will be taken from local capabilities directory, unless no such
  entries exist, in which case global entries will be considered as well.
* **LOCAL\_AND\_GLOBAL** Entries will be taken from local capabilities directory and from global
  capabilities directory.
* **GLOBAL\_ONLY** Only the global entries will be looked at.

**Default discovery scope:** ```LOCAL_THEN_GLOBAL```

Whenever global entries are involved, they are first searched in the local cache. In case no global
entries are found in the cache, a remote lookup is triggered.

The **arbitration strategy** defines how the results of the scoped lookup will be sorted
and / or filtered. The arbitration strategy is a function with one parameter (the array of
capability entries found). It can either be selected from the predefined arbitration strategies
in ArbitrationStrategyCollection or provided as user-defined function. If this user-defined
function `myFunction` needs additional filter criteria like the arbitration strategy *Keyword*,
the result of `myFunction.bind(myParam)` has to be used as arbitration strategy.

**Predefined arbitration strategies:**
* **ArbitrationStrategyCollection.LastSeen** The participant that was last refreshed (i.e. with the
  most current last seen date) will be selected
* **ArbitrationStrategyCollection.Nothing** use DefaultArbitrator which picks the first discovered
   entry with compatible version
* **ArbitrationStrategyCollection.HighestPriority** Highest priority provider will be selected
* **ArbitrationStrategyCollection.FixedParticipant** select provider which matches the
  participantId provided as a custom parameter in DiscoveryQos (see below), if existing
* **ArbitrationStrategyCollection.Keyword** Only a Provider that has keyword set will be selected

**Default arbitration strategy:** ```ArbitrationStrategyCollection.LastSeen```

The priority used by the arbitration strategy *HighestPriority* is set by the provider in its
providerQos settings.

**Predefined arbitration constants**

defines keys for the key-value pair for the custom Parameters of discoveryScope:

* ** KEYWORD_PARAMETER**: required custom parameter for ArbitrationStrategy.Keyword
* ** FIXED_PARTICIPANT_PARAMETER**: required custom parameter for
  ArbitrationStrategy.FixedParticipant

Example for setting up a ```DiscoveryQos``` object:
```typescript
import DiscoveryQos from 'joynr/joynr/proxy/DiscoveryQos';
import {LastSeen} from 'joynr/joynr/types/ArbitrationStrategyCollection';
import {LOCAL_THEN_GLOBAL} from 'joynr/joynr/types/DiscoveryScope';
import {KEYWORD_PARAMETER} from "joynr/joynr/types/ArbitrationConstants";
import {FIXED_PARTICIPANT_PARAMETER} from "joynr/joynr/types/ArbitrationConstants";

const discoveryQos = new DiscoveryQos({
    discoveryTimeoutMs : 30000,
    discoveryRetryDelayMs : 1000,
    arbitrationStrategy : LastSeen,
    cacheMaxAgeMs : 0,
    discoveryScope : LOCAL_THEN_GLOBAL,
    providerMustSupportOnChange : false,
    // additional parameters are used for predefined arbitration strategies:
    // Keyword and FixedParticipant
    // or can be used for custom arbitration strategies
    additionalParameters : {
        // required parameter for arbitration strategy FixedParticipant:
        [FIXED_PARTICIPANT_PARAMETER]: "expectedParticipantId",
        // required parameter for arbitration strategy Keyword:
        [KEYWORD_PARAMETER]: "expectedKeyword",
        // additional parameters for custom arbitration strategy:
        "key1": "value1",
        //    ...
        "keyN": "valueN"
    }
});
```

Missing parameters will be replaced by the default settings.

## The message quality of service

The ```MesssagingQos``` object defines the **roundtrip timeout in milliseconds** for
**RPC requests** (getter/setter/method calls) and unsubscribe requests and it allows
definition of additional custom message headers.
The ttl for subscription requests is calculated from the ```expiryDateMs```
in the [SubscriptionQos](#subscription-quality-of-service) settings.
The ttl of internal joynr messages cannot be changed (corresponding setting
is deprecated and will be ignored).

If no specific setting is given, the default roundtrip timeout is 60 seconds.
The keys of custom message headers may contain ascii alphanumeric or hyphen.
The values of custom message headers may contain alphanumeric, space, semi-colon, colon,
comma, plus, ampersand, question mark, hyphen, dot, star, forward slash and back slash.
If a key or value is invalid, the API method called to introduce the custom message
header throws an Error.

Example:

```typescript
import MessagingQos from "joynr/joynr/messaging/MessagingQos";
const messagingQos = new MessagingQos({
    ttl: 60000,
    // optional custom headers
    customHeaders: {
        "key1": "value1",
    //    ...
        "keyN": "valueN"
    }
});

// optional
messagingQos.putCustomMessageHeader("anotherKey", "anotherValue");
```

## Building a proxy

Proxy creation is necessary before services from a provider can be called:
* call its **methods** (RPC) **asynchronously**
* **subscribe** or **unsubscribe** to its **attributes** or **update** a subscription
* **subscribe** or **unsubscribe** to its **broadcasts** or **update** a subscription

The ProxyBuilder.build call requires the provider's domain. Optionally, **messagingQos** and
**discoveryQos** settings can be specified if the default settings are not suitable.

When looking up a globally registered provider, all backends known to the clustercontroller will be
considered by default. This can be constrained by specifying the optional **GBIDs** setting,
narrowing the search to the specified **GBIDs**. These **GBIDs** must be valid and preconfigured at
the local cluster-controller.  
The global discovery will be performed via the GblobalCapabilitiesDirectory instance in the backend
identified by the first GBID in the list of provided GBIDs.

In case no suitable provider can be found during discovery, a `DiscoveryException` or
`NoCompatibleProviderFoundException` is thrown.

```typescript
import joynr from "joynr";
import Proxy from "joynr/«path/to/Proxy»";
const domain = "«ProviderDomain»";

let messagingQos, discoveryQos;
let gbids = ['joynrdefaultgbid'];
// setup messagingQos, discoveryQos

// the signature is available at joynr/joynr/proxy/ProxyBuilder
joynr.proxyBuilder.build<Proxy>(Proxy, {
    domain: domain,
    discoveryQos, // optional
    messagingQos,  // optional
    gbids // optional
}).then((buildProxy: Proxy) => {
    // subscribe to attributes (optional)

    // subscribe to broadcasts (optional)

    // call methods or setup event handlers which call methods
}).catch((error) => {
    // handle error (usually DiscoveryException of NoCompatibleProviderFoundException)
});
```

## Method calls (RPC)

In Javascript all method calls are asynchronous. Since the local proxy method returns a Promise,
the reaction to the resolving or rejecting of the Promise can be immediately defined.
Note that the message order on Joynr RPCs will not be preserved; if calling order is required,
then the subsequent dependent call should be made in the following then() call.

The typescript methods signatures are fully typed. On the top level multiple arguments are
transformed into a settings object, merging overloaded franca methods into a single method.
All method signatures, args and return values are typed by the generator.

```typescript
«interface»Proxy.«method»(... optional arguments ...).then(function(response) {
	// call successful, handle response value
}).catch(function(error) {
    // call failed, execute error handling
    // The following objects are used to receive error details from provider side:
    // - joynr.exceptions.ApplicationException (its member 'error' holds the error enumeration value,
    //   wrt. error enumeration value range please refer to the Franca specification of the method)
    // - joynr.exceptions.ProviderRuntimeException (with embedded 'detailMessage')
});
```

## Subscription quality of service

A subscription quality of service setting is required for subscriptions to broadcasts or attribute
changes. The following sections cover the 4 quality of service objects available.

### SubscriptionQos

```SubscriptionQos``` has the following members:

* **expiryDateMs** Absolute Time until notifications will be send (in milliseconds)

```typescript
import SubscriptionQos from "joynr/joynr/proxy/SubscriptionQos"
const subscriptionQos = new SubscriptionQos({
    expiryDateMs : 0,
});
```

The default values are as follows:

```
{
    expiryDateMs: SubscriptionQos.NO_EXPIRY_DATE,  // 0
    publicationTtlMs : SubscriptionQos.DEFAULT_PUBLICATION_TTL // 10000
}
```

### MulticastSubscriptionQos

The object ```MulticastSubscriptionQos``` inherits from ```SubscriptionQos```.

This object should be used for subscriptions to non-selective broadcasts.

Example:
```typescript
import MulticastSubscriptionQos from "joynr/joynr/proxy/MulticastSubscriptionQos"
const multicastSubscriptionQos = new MulticastSubscriptionQos({
    validityMs : 3000000
});
```

or alternatively

```typescript
import MulticastSubscriptionQos from "joynr/joynr/proxy/MulticastSubscriptionQos"
const multicastSubscriptionQos = new MulticastSubscriptionQos({
    expiryDateMs : Date.now() + 3000000
});
```

The default is as follows:
```
{
    expiryDateMs: SubscriptionQos.NO_EXPIRY_DATE // 0
}
```

### PeriodicSubscriptionQos

```PeriodicSubscriptionQos``` has the following additional members:

* **periodMs** defines how long to wait before sending an update even if the value did not change
* **alertAfterIntervalMs** Timeout for notifications, afterwards a missed publication notification
  will be sent (milliseconds)
* **publicationTtlMs** Lifespan of a notification (in milliseconds), the notification will be
  deleted afterwards
  The API will be changed in the future: proxy subscribe calls will no longer take a
  subscriptionQos; instead the publication TTL will be settable on the provider side.

This object can be used for subscriptions to attributes.

Note that updates will be sent only based on the specified interval, and not as a result of an
attribute change.

```typescript
import PeriodicSubscriptionQos from "joynr/joynr/proxy/PeriodicSubscriptionQos"
const subscriptionQosPeriodic = new PeriodicSubscriptionQos({
    periodMs : 60000,
    alertAfterIntervalMs : 0,
    publicationTtlMs : 10000
});
```
The default values are as follows:
```
{
    periodMs: PeriodicSubscriptionQos.DEFAULT_PERIOD_MS, // 60000
    alertAfterIntervalMs: PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL, // 0
    publicationTtlMs : SubscriptionQos.DEFAULT_PUBLICATION_TTL // 10000
}
```

### OnchangeSubscriptionQos

The object ```OnChangeSubscriptionQos``` inherits from ```SubscriptionQos``` and has the following
additional members:

* **minIntervalMs** Minimum time to wait between successive notifications (milliseconds)
* **publicationTtlMs** Lifespan of a notification (in milliseconds), the notification will be
  deleted afterwards
  The API will be changed in the future: proxy subscribe calls will no longer take a
  subscriptionQos; instead the publication TTL will be settable on the provider side.

This object should be used for subscriptions to selective broadcasts. It can also be used for
subscriptions to attributes if no periodic update is required.

Example:
```typescript
import OnChangeSubscriptionQos from "joynr/joynr/proxy/OnChangeSubscriptionQos"
const subscriptionQosOnChange = new OnChangeSubscriptionQos({
    minIntervalMs : 1000,
    publicationTtlMs : 10000
});
```
The default is as follows:
```
{
    minIntervalMs: OnChangeSubscriptionQos.DEFAULT_MIN_INTERVAL_MS, // 1000
    publicationTtlMs : SubscriptionQos.DEFAULT_PUBLICATION_TTL // 10000
}
```

### OnchangeWithKeepAliveSubscriptionQos

The object ```OnChangeWithKeepAliveSubscriptionQos``` inherits from ```OnChangeSubscriptionQos```
and has the following additional members:

* **maxIntervalMs** Maximum time to wait between notifications, if value has not changed
* **alertAfterIntervalMs** Timeout for notifications, afterwards a missed publication notification
  will be sent (milliseconds)

This object can be used for subscriptions to attributes. Updates will then be sent both
periodically and after a change (i.e. this acts like a combination of PeriodicSubscriptionQos
and OnChangeSubscriptionQos).

Using it for subscriptions to broadcasts is theoretically possible because of inheritance but
makes no sense (in this case the additional members will be ignored).

Example:
```typescript
import OnChangeWithKeepAliveSubscriptionQos from "joynr/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos"
const subscriptionQosOnChangeWithKeepAlive = new OnChangeWithKeepAliveSubscriptionQos({
    maxIntervalMs : 60000,
    alertAfterIntervalMs: 0
});
```
The default is as follows:
```
{
    maxIntervalMs : OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS // 60000
    alertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL // 0
}
```

## Subscribing to an attribute

Attribute subscription - depending on the subscription quality of service settings used - informs
an application either periodically and / or on change of an attribute about the current value.

The **subscriptionId** is returned asynchronously after the subscription is successfully registered
at the provider. It can be used later to update the subscription or to unsubscribe from it.
If the subscription failed, a SubscriptionException will be returned via the callback (onError) and
via the Promise.

To receive the subscription, **callback functions** (onReceive, onSubscribed, onError) have to be
provided as outlined below.

```typescript
«interface»Proxy.«Attribute».subscribe({
    subscriptionQos : subscriptionQosOnChange,
    // Gets called on every received publication
    onReceive : function(value) {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully registered at the provider
    onSubscribed : function(subscriptionId) { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: function(error) {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    }
}).then(function(subscriptionId) {
    // subscription successful, store subscriptionId for later use
}).catch(function(error) {
    // handle error case
});
```

## Updating an attribute subscription

The ```subscribe()``` method can also be used to update an existing subscription, by passing the
**subscriptionId** as an additional parameter as follows:

```typescript
// subscriptionId from earlier subscribe call
«interface»Proxy.«Attribute».subscribe({
    subscriptionQos : subscriptionQosOnChange,
    subscriptionId,
    // Gets called on every received publication
    onReceive : (value) => {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully updated at the provider
    onSubscribed : (subscriptionId) => { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: (error) => {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    }
}).then((subscriptionId) => {
    // subscription update successful, the subscriptionId should be the same as before
}).catch((error) => {
    // handle error case
});
```

## Unsubscribing from an attribute

Unsubscribing from an attribute subscription requires the **subscriptionId** returned by the
earlier subscribe call.

```typescript
«interface»Proxy.«Attribute».unsubscribe({
    subscriptionId
}).then(function() {
    // handle success case
}).catch(function(error) {
    // handle error case
});
```

## Subscribing to a (non-selective) broadcast

A Broadcast subscription informs the application in case a broadcast is fired by a provider.
The output values are returned via a callback function.

A broadcast is selective only if it is declared with the `selective` keyword in Franca, otherwise it
is non-selective.

Non-selective broadcast subscriptions can be passed optional **partitions**. A partition is a
hierarchical list of strings similar to a URL path. Subscribing to a partition will cause only those
broadcasts to be sent to the consumer that match the partition. Note that the partition is set when
subscribing on the consumer side, and must match the partition set on the provider side when the
broadcast is performed.

Example: a consumer could set a partition of "europe", "germany", "munich" to receive broadcasts for
Munich only. The matching provider would use the same partition when sending the broadcast.

The **subscriptionId** is returned asynchronously after the subscription is successfully registered
at the provider. It can be used later to update the subscription or to unsubscribe from it.
If the subscription failed, a SubscriptionException will be returned via the callback (onError) and
via the Promise.

To receive the subscription, **callback functions** (onReceive, onSubscribed, onError) have to be
provided as outlined below.

```typescript
«Interface»Proxy.«Broadcast».subscribe({
    subscriptionQos : multicastSubscriptionQos,
    // Gets called on every received publication
    onReceive : function(value) {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully registered at the provider
    onSubscribed : function(subscriptionId) { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: function(error) {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    }

    // optional parameter for multicast subscriptions (subscriptions to non-selective broadcasts)
    partitions: [partitionLevel1,
                 ...
                 partitionLevelN]
}).then(function(subscriptionId) {
    // subscription successful, store subscriptionId for later use
}).catch(function(error) {
    // handle error case
});
```
The [partition syntax is explained in the multicast concept](../docs/multicast.md#partitions)

## Updating a (non-selective) broadcast subscription

The ```subscribe()``` method can also be used to update an existing subscription, when the
**subscriptionId** is passed as an additional parameter as follows:
```typescript
«Interface»Proxy.«Broadcast».subscribe({
    subscriptionQos : multicastSubscriptionQos,
    subscriptionId: subscriptionId,
    // Gets called on every received publication
    onReceive : function(value) {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully updated at the provider
    onSubscribed : function(subscriptionId) { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: function(error) {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    },

    // optional parameter for multicast subscriptions (subscriptions to non-selective broadcasts)
    partitions: [partitionLevel1,
                 ...
                 partitionLevelN]
}).then(function(subscriptionId) {
    // subscription update successful, the subscriptionId should be the same as before
}).catch(function(error) {
    // handle error case
});

```

## Subscribing to a selective broadcast, i.e. a broadcast with filter parameters

Broadcast subscription with a **filter** informs the application in case a **selective broadcast
which matches filter criteria** is fired from the provider side. The output values are returned
via callback.

The **subscriptionId** is returned asynchronously after the subscription is successfully registered
at the provider. It can be used later to update the subscription or to unsubscribe from it.
If the subscription failed, a SubscriptionException will be returned via the callback (onError) and
via the Promise.

To receive the subscription, **callback functions** (onReceive, onSubscribed, onError) have to be
provided as outlined below.

In addition to the normal broadcast subscription, the filter parameters for this broadcast must be
created and initialized as additional parameters to the ```subscribe``` method. These filter
parameters are used to receive only those broadcasts matching the provided filter criteria.

```typescript
const fParam = «interface»Proxy.«broadcast».createFilterParameters();
// for each parameter
fParam.set«Parameter»(parameterValue);
«interface»Proxy.«Broadcast».subscribe({
    subscriptionQos : subscriptionQosOnChange,
    filterParameters : fParam,
    // Gets called on every received publication
    onReceive : function(value) {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully registered at the provider
    onSubscribed : function(subscriptionId) { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: function(error) {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    }
}).then(function(subscriptionId) {
    // subscription successful, store subscriptionId for later use
}).catch(function(error) {
    // handle error case
});
```

## Updating a broadcast subscription with filter parameters

The **subscribeTo** method can also be used to update an existing subscription, by passing the
**subscriptionId** as an additional parameter as follows:


```typescript
const fParam = «interface»Proxy.«broadcast».createFilterParameters();
// for each parameter
fParam.set«Parameter»(parameterValue);
«interface»Proxy.«Broadcast».subscribe({
    subscriptionQos : subscriptionQosOnChange,
    subscriptionId: subscriptionId,
    filterParameters : fParam,
    // Gets called on every received publication
    onReceive : function(value) {
        // handle subscription publication
    },
    // Gets called when the subscription is successfully updated at the provider
    onSubscribed : function(subscriptionId) { // optional
        // save the subscriptionId for updating the subscription or unsubscribing from it
        // the subscriptionId can also be taken from the Promise returned by the subscribe call
    },
    // Gets called on every error that is detected on the subscription
    onError: function(error) {
        // handle subscription error, e.g.:
        // - SubscriptionException if the subscription registration failed at the provider
        // - PublicationMissedException if a periodic subscription publication does not arrive in time
    }
}).then(function(subscriptionId) {
    // subscription update successful, the subscriptionId should be the same as before
}).catch(function(error) {
    // handle error case
});
```

## Unsubscribing from a broadcast

Unsubscribing from a broadcast subscription requires the **subscriptionId** returned asynchronously
by the earlier subscribe call.

```typescript
«interface»Proxy.«Broadcast».unsubscribe({
    subscriptionId : subscriptionId,
}).then(function() {
    // call successful
}).catch(function(error) {
    // handle error case
});

```

# Building a Javascript Provider application

For all Franca interfaces that are being implemented, a **provider** object must be created
using the ```build()``` method of ```joynr.providerBuilder``` supplying the ProviderConstructor and
ProviderImplementation as argument.

The signature of the ProviderImplementation is included in the generated code.

Upon successful creation, the application can register all **Capabilities** it provides by calling
`joynr.registration.register(settings)`.

## The Provider quality of service

The ```ProviderQos``` has the following members:

* **customParameters** e.g. the key-value for the arbitration strategy Keyword during discovery
* **priority** the priority used for arbitration strategy HighestPriority during discovery
* **scope** the scope (see below), used in discovery
* **supportsOnChangeSubscriptions** whether the provider supports subscriptions on changes

The **scope** can be
* **LOCAL** The provider will be registered in the local capability directory
* **GLOBAL** The provider will be registered in the local and global capability directory

Example:
```typescript
import ProviderQos from "joynr/generated/joynr/types/ProviderQos";
import ProviderScope from "joynr/generated/joynr/types/ProviderScope";
const providerQos = new ProviderQos({
    customParameters: [],
    priority : 100,
    scope: ProviderScope.GLOBAL,
    supportsOnChangeSubscriptions : true
});
```

## Register a Provider implementation
After an application is successfully loaded, it can register provider implementations for
Franca interfaces it implements.

While the implementation is registered, the provider will respond to any method calls from outside,
can report any value changes via publications to subscribed consumers, and may fire broadcasts, as
defined in the Franca interface.

When registering a provider implementation for a specific Franca interface, the object implementing
the interface, the provider's domain and the provider's quality of service settings are passed as
parameters.

If registration to local and global scope is requested by 'ProviderQos', the provider is registered
to all GBIDs configured in the cluster controller.

The 'gbids' parameter can be provided to override the GBIDs selection in the cluster controller.
The global capabilities directory identified by the first selected GBID performs the registration.

* awaitGlobalRegistration: optional - set to true in case the registration promise should wait till
global registration is also completed in the GlobalCapabilitiesDirectory instead of only the
LocalCapabilitiesDirectory in the cluster-controller.
* gbids: Optional subset of GBIDs configured in the cluster controller for custom global
registration.

```typescript
import joynr from "joynr";
import ProviderQos from "joynr/generated/joynr/types/ProviderQos";
import ProviderScope from "joynr/generated/joynr/types/ProviderScope";
import ApplicationException = require("joynr/joynr/exceptions/ApplicationException");
import DiscoveryError = require("joynr/generated/joynr/types/DiscoveryError");
const providerQos = new ProviderQos({
  customParameters: [],
  priority: Date.now(),
  scope: ProviderScope.GLOBAL,
  supportsOnChangeSubscriptions: true
});
const domain = "test";
const gbids = ['joynrdefaultgbid', 'otherbackend'];
const provider = joynr.providerBuilder.build<«Interface»Provider>(«Interface»Provider, providerImpl);
const awaitGlobalRegistration = true;

// for any filter of a broadcast with filter
provider.«broadcast».addBroadcastFilter(new «Filter»BroadcastFilter());

try {
    await joynr.registration.register(
    {
        domain,
        provider,
        providerQos,
        awaitGlobalRegistration,
        gbids
    });
} catch (e) {
    if (e instanceof ApplicationException) {
        // handle modelled DiscoveryError
        switch (e.error) {
            case DiscoveryError.UNKNOWN_GBID:
                // one of the selected GBIDs is not known
                // either at the cluster-controller or at the GlobalCapabilitiesDirectory
                break;
            case DiscoveryError.INVALID_GBID:
                // one of the selected GBIDs is invalid, e.g. empty or duplicated
                break;
            case DiscoveryError.INTERNAL_ERROR:
                // other error at the cluster-controller or GlobalCapabilitiesDirectory
                break;
            case DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS:
                // not applicable for provider registration
                break;
            case DiscoveryError.NO_ENTRY_FOR_PARTICIPANT:
                // not applicable for provider registration
                break;
            default:
                // handle default
                break;
        }
    } else {
        // handle other errors
    }
}
```

## Unregister a provider
Unregistering a previously registered provider requires the provider's domain and the object that
represents the provider implementation.

```typescript
// provider should have been set and registered previously
await joynr.registration.unregisterProvider(
    domain,
    «Interface»provider
);
```
If the provider is registered globally (default), i.e `ProviderScope.GLOBAL` has been set in `ProviderQos`
when registering the provider, then this function triggers a global remove operation in the local
capabilities directory of the cluster controller (standalone or embedded within the same runtime)
and returns.

If the provider is registered only locally, i.e `ProviderScope.LOCAL` has been set in `ProviderQos`
when registering the provider, then this function removes the provider from the local capabilities directory
of the cluster controller (standalone or embedded within the same runtime) and waits for the result.

__IMPORTANT__: the `unregisterProvider` function does not wait for a response from the global capabilities
directory and do not get informed about errors or success. The cluster controller will repeat the global
remove operation until it succeeds or the cluster controller is shut down. This function does not guarantee
a successful execution of provider's removal from the global capabilities directory.

## The Provider implementation for an interface
The function implementing the interface must provide code for all its methods and a getter function
for every attribute.
Type signatures of ProviderImplementations are available in the respective generated code.
```typescript
const «Interface»ProviderImpl = {
    // define «method» handler

    // define internal representation of «attribute» and
    // getter handlers per «attribute»
    // wrappers to fire broadcasts
}
```

## Method handler
Each handler for a Franca method for a specific interface is implemented as a function object
member of **this**. The parameters are provided as objects. The implementation can be done
either asynchronously or synchronously. The methods will be wrapped into a Promise in any case.

```typescript
this.«method» = async (parameters) => {
    // handle method, return returnValue of type «returnType» with
    // return returnValue;
    // - or -
    // throw errorEnumerationValue;
    // (wrt. error enumeration value range please refer to the Franca specification of the method)
    // - or -
    // throw new joynr.exceptions.ProviderRuntimeException({ detailMessage: "reason" });
};
```

## Attribute handler
For each Franca attribute of an interface, a member of **this** named after the
```«attribute»``` has to be created which consists of an object which includes a getter function
as attribute that returns the current value of the ```«attribute»```. Also an internal
representation of the Franca attribute value has to be created and properly intialized.
```typescript
// for each «attribute» of the «interface» provide an internal representation
// and a getter
const attributeValue = «initialValue»;

this.«attribute» = {
    get: function() {
        return attributeValue;
    }
};
```

## Attribute change broadcast
The provider implementation must inform about any change of an attribute which is not done via a
remote setter call by calling valueChanged on the given attribute. If an attribute setter is called
remotely, an attribute change publication will be triggered automatically.
```typescript
this.«attribute».valueChanged(newValue);
```
## Sending a broadcast
For each Franca broadcast, a member of **this** named after the ```«broadcast»```
has to be created which consists of an empty object.

```typescript
this.«broadcast» = {};
```

The broadcast can then later be fired using
```typescript
this.fire«Broadcast» = function() {
    var outputParameters;
    outputParameters = self.«broadcast».createBroadcastOutputParameters();
    // foreach output parameter of the broadcast
    outputParameters.set«Parameter»(value);

    // optional: the partitions to be used for the broadcast
    // Note: wildcards are only allowed on consumer side
    var partitions = [partitionLevel1,
                      ...
                      partitionLevelN];

    self.«broadcast».fire(outputParameters[, partitions]);
}
```
The [partition syntax is explained in the multicast concept](../docs/multicast.md#partitions)

## Selective (filtered) broadcasts

In contrast to unfiltered broadcasts, to realize selective (filtered) broadcasts, the filter logic
has to be implemented and registered by the provider. If multiple filters are registered on the
same provider and broadcast, all filters are applied in a chain and the broadcast is only
delivered if all filters in the chain return true.

### The broadcast filter object
A broadcast filter object implements a filtering function called ```filter()``` which returns a
boolean value indicating whether the broadcast should be delivered. The input parameters of the
```filter()``` method consist of the output parameters of the broadcast and the filter parameters
used by the consumer on subscription.

```typescript
(function(undefined) {
    var «Filter»BroadcastFilter = function «Filter»BroadcastFilter() {
        if (!(this instanceof «Filter»BroadcastFilter)) {
            return new «Filter»BroadcastFilter();
        }

        Object.defineProperty(this, 'filter', {
            enumerable: false,
            value: function(broadcastOutputParameters, filterParameters) {
                // Parameter value can be evaluated by calling getter functions, e.g.
                // broadcastOutputParameters.get«OutputParameter»()
                // filterParameters can be evaluated by using properties, e.g.
                // filterParameters.«property»
                //
                // Evaluate whether the broadcastOutputParameters fulfill
                // the filterParameter here, then return true, if this is
                // the case and the publication should be done, false
                // otherwise.

                return «booleanValue»;
            };
        });
    };

    return «Filter»BroadcastFilter;
}());
```
