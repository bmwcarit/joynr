# Joynr Javascript Developer Guide

## Conversion of Franca entries

### Place holders

Note that the following notations in the code examples below must be replaced by actual values from
Franca:

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
The Franca ```<Package>``` will be transformed to the Javascript module ```joynr.<Package>```.

### Type collection name

The Franca ```<TypeCollection>``` is not used in the generated code. It is therefore important that
all typeNames within a particular package be unique.

### Complex type name

Any Franca complex type ```<TypeCollection>.<Type>``` will result in the creation of an object
```joynr.<Package>.<Type>``` (see above).

The same ```<Type>``` will be used for all elements in the event that this type is used as an
element of other complex types, as a method input or output argument, or as a broadcast output
argument.

### Interface name

The Franca ```<Interface>``` will be used as a prefix to create the following JavaScript objects:

```
<Interface>Provider
<Interface>Proxy
```

### Attribute, Method and Broadcast names

In Javascript the names of attributes, methods and broadcasts within the same interface must be
unique, as each name will become a property of the Proxy object.

# Building a Javascript consumer application

A Javascript joynr consumer application must "require" or otherwise load the ```joynr.js``` module
and call its ```joynr.load()``` method with provisioning arguments in order to create a **joynr
object**.

Next, for all Franca interfaces that are to be used, a **proxy** must be created using the
```build()``` method of the ```joynr.proxyBuilder``` object, with the provider's domain passed as
an argument.

Once the proxy has been successfully created, the application can add any attribute or broadcast
subscriptions it needs, and then enter its event loop where it can call the interface methods.

## Required imports

The following Javascript modules must be made available using require or some other loading
mechanism:
```
// for each type <Type>
"import" js/joynr/<Package>/<Type>.js
// for each interface <Interface>
"import" js/joynr/<Package>/<Interface>Proxy.js
"import" js/joynr.js
"import" js/joynrprovisioning.common.js
"import" js/joynrprovisioning.consumer.js
```

## Base implementation

The Javascript application must load and initialize the joynr runtime environment prior to calling
any other Joynr API.

```javascript
joynr.load(provisioning).then(function(loadedJoynr) {
    joynr = loadedJoynr;

    // build one or more proxies and optionally set up event handlers
}).catch(function(error) {
    // error handling
});

```

## The discovery quality of service

The ```DiscoveryQos``` configures how the search for a provider will be handled. It has the
following members:

* **discoveryTimeoutMs**  Timeout for the discovery process (milliseconds). A timeout triggers an exception.
* **cacheMaxAgeMs** Defines the maximum allowed age of cached entries (milliseconds); only younger
entries will be considered. If no suitable providers are found, depending on the discoveryScope,
a remote global lookup may be triggered.
* **arbitrationStrategy** (details see below)
* **additionalParameters** special application-specific parameters that must match, e.g. a keyword
* **discoveryRetryDelayMs** The time to wait between discovery retries after encountering a discovery
error.
* **discoveryScope** (details see below)

The **discoveryScope** defines whether a suitable provider will be searched only in the local
capabilities directory or also in the global one.

Available values are as follows:

* **LOCAL\_ONLY** Only entries from local capability directory will be searched
* **LOCAL\_THEN\_GLOBAL** Entries will be taken from local capabilities directory, unless no such
entries exist, in which case global entries will be considered as well.
* **LOCAL\_AND\_GLOBAL** Entries will be taken from local capabilities directory and from global
capabilities directory.
* **GLOBAL\_ONLY** Only the global entries will be looked at.

Whenever global entries are involved, they are first searched in the local cache. In case no global
entries are found in the cache, a remote lookup is triggered.

The **arbitration strategy** defines how the results of the scoped lookup will be sorted
and / or filtered. The arbitration strategy is a function with one parameter (the array of
capability entries found). It can either be selected from the predefined arbitration strategies
in ArbitrationStrategyCollection or provided as user-defined function. If this user-defined
function `myFunction` needs additional filter criteria like the arbitration strategy *Keyword*,
the result of `myFunction.bind(myParam)` has to be used as arbitration strategy.

**Predefined arbitration strategies:**
* **ArbitrationStrategyCollection.Nothing**
* **ArbitrationStrategyCollection.HighestPriority** Highest priority provider will be selected
* **ArbitrationStrategyCollection.Keyword** Only a Provider that has keyword set will be selected

The priority used by the arbitration strategy *HighestPriority* is set by the provider in its
providerQos settings.

Example for setting up a ```DiscoveryQos``` object:
```javascript
// additionalParameters unclear
// there is currently no ArbitrationConstants in Javascript
// { "keyword" : "someKeyword" }
// { "fixedParticipantId" : "someParticipantId" }
// { }
//
var discoveryQos = new joynr.proxy.DiscoveryQos({
    discoveryTimeoutMs : 30000,
    cacheMaxAgeMs : 0,
    arbitrationStrategy : ArbitrationStrategyCollection.HighestPriority,
    additionalParameters: {},
    discoveryScope : DiscoveryScope.LOCAL\_ONLY,
    discoveryRetryDelayMs : 1000
});
```

## The message quality of service

The ```MesssagingQos``` object defines the roundtrip timeout for RPC requests in milliseconds.
If no specific setting is given, the default is 60 seconds.

Example:

```javascript
var messagingQos = new joynr.messaging.MessagingQos({
    ttl: 60000
});
```

## Building a proxy

Proxy creation is necessary before services from a provider can be called:
* call its **methods** (RPC) **asynchronously**
* **subscribe** or **unsubscribe** to its **attributes** or **update** a subscription
* **subscribe** or **unsubscribe** to its **broadcasts** or **update** a subscription

The call requires **messagingQos** and **discoveryQos** settings as well as the provider's domain.

```javascript
var messagingQos, discoveryQos;

// setup messagingQos, discoveryQos
var domain = "<ProviderDomain>";
joynr.proxyBuilder.build(<Interface>Proxy, {
    domain: domain,
    discoveryQos: discoveryQos, // optional
    messagingQos: messagingQos  // optional
}).then(function(<interface>Proxy) {
    // subscribe to attributes (optional)

    // subscribe to broadcasts (optional)

    // call methods or setup event handlers which call methods
}).catch(function(error) {
    // handle error
});
```

## Method calls (RPC)

In Javascript all method calls are asynchronous. Since the local proxy method returns a Promise,
the reaction to the resolving or rejecting of the Promise can be immediately defined.
Note that the message order on Joynr RPCs will not be preserved; if calling order is required,
then the subsequent dependent call should be made in the following then() call.
```javascript
<interface>Proxy.<method>(... optional arguments ...).then(function(response) {
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

* **expiry dateMs** Absolute Time until notifications will be send (in milliseconds)
* **publicationTtlMs** Lifespan of a notification (in milliseconds), the notification will be deleted
afterwards

```javascript
var subscriptionQos = new joynr.proxy.SubscriptionQos({
    expiryDateMs : 1000,
    publicationTtlMs : 1000
});
```

The default values are as follows:

```
{
    expiryDateMs: SubscriptionQos.NO\_EXPIRY\_DATE,  // 0
    publicationTtlMs : SubscriptionQos.DEFAULT\_PUBLICATION\_TTL // 10000
}
```

### PeriodicSubscriptionQos

```PeriodicSubscriptionQos``` has the following additional members:

* **periodMs** defines how long to wait before sending an update even if the value did not change
* **alertAfterIntervalMs** Timeout for notifications, afterwards a missed publication notification
will be sent (milliseconds)

This object can be used for subscriptions to attributes. Note that updates will be sent only based
on the specified interval, and not as a result of an attribute change.

```javascript
var subscriptionQosPeriodic = new joynr.proxy.PeriodicSubscriptionQos({
    periodMs : 1000,
    alertAfterIntervalMs : 1000
});
```
The default values are as follows:
```
{
    periodMs: PeriodicSubscriptionQos.MIN\_PERIOD // 50
    alertAfterIntervalMs: PeriodicSubscriptionQos.NEVER\_ALERT // 0
}
```

### OnchangeSubscriptionQos

The object ```OnChangeSubscriptionQos``` inherits from ```SubscriptionQos``` and has the following
additional members:

* **minIntervalMs** Minimum time to wait between successive notifications (milliseconds)

This object should be used for subscriptions to broadcasts. It can also be used for subscriptions
to attributes if no periodic update is required.

Example:
```javascript
var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
    minIntervalMs : 50
});
```
The default is as follows:
```
{
    minIntervalMs: OnChangeSubscriptionQos.MIN\_INTERVAL // 50
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
```javascript
var subscriptionQosOnChangeWithKeepAlive = new joynr.proxy.OnChangeWithKeepAliveSubscriptionQos({
    maxIntervalMs : 2000
});
```
The default is as follows:
```
{
    maxIntervalMs : 0
    alertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.NEVER\_ALERT // 0
}
```

## Subscribing to an attribute

Attribute subscription - depending on the subscription quality of service settings used - informs
an application either periodically and / or on change of an attribute about the current value.
The **subscriptionId** returned asynchronously in case of a successful call can be used later to
update the subscription or to unsubscribe from it.

```javascript
<interface>Proxy.<Attribute>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    onReceive : function(value) {
        // handle subscription broadcast
    },
    onError: function(error) {
        // handle subscription error
        // (e.g. JoynrRuntimeException, PublicationMissedException)
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

```javascript
// subscriptionId from earlier subscribe call
<interface>Proxy.<Attribute>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    subscriptionId: subscriptionId,
    onReceive : function(value) {
        // handle subscription broadcast
    },
    onError: function(error) {
        // handle subscription error
        // (e.g. JoynrRuntimeException, PublicationMissedException)
    }
}).then(function(subscriptionId) {
    // subscription update successful, the subscriptionId should be the same as before
}).catch(function(error) {
    // handle error case
});
```

## Unsubscribing from an attribute

Unsubscribing from an attribute subscription requires the **subscriptionId** returned by the
ealier subscribe call.

```javascript
<interface>Proxy.<Attribute>.unsubscribe({
    subscriptionId: subscriptionId
}).then(function() {
    // handle success case
}).catch(function(error) {
    // handle error case
});
```

## Subscribing to a broadcast unconditionally

Broadcast subscription informs the application in case a broadcast is fired from provider side
and provides the output values via a callback function.
The **subscriptionId** returned by the call can be used later to update the subscription or to
unsubscribe.

```javascript
<interface>Proxy.<Broadcast>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    onReceive : function(value) {
        // handle subscription broadcast
    },
    onError: function(error) {
        // handle subscription error
        // (e.g. JoynrRuntimeException)
    }
}).then(function(subscriptionId) {
    // subscription successful, store subscriptionId for later use
}).catch(function(error) {
    // handle error case
});
```

## Updating an unconditional broadcast subscription

The ```subscribe()``` method can also be used to update an existing subscription, when the
**subscriptionId** is passed as an additional parameter as follows:
```javascript
<interface>Proxy.<Broadcast>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    subscriptionId: subscriptionId,
    onReceive : function(value) {
        // handle subscription broadcast
    },
    onError: function(error) {
        // handle subscription error
        // (e.g. JoynrRuntimeException)
    }
}).then(function(subscriptionId) {
    // subscription update successful, the subscriptionId should be the same as before
}).catch(function(error) {
    // handle error case
});

```

## Subscribing to a broadcast with filter parameters

Broadcast subscription with a **filter** informs the application in case a **selected broadcast
which matches filter criteria** is fired from the provider side. The output values are returned
via callback.
The **subscriptionId** returned by the call can be used later to update the subscription or to
unsubscribe.

```javascript
var fParam = <interface>Proxy.<broadcast>.createFilterParameters();
// for each parameter
fParam.set<Parameter>(parameterValue);
<interface>Proxy.<Broadcast>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    filterParameters : fParam,
    onReceive : function(value) {
        // handle subscription broadcast
    },
    onError: function(error) {
        // handle subscription error
        // (e.g. JoynrRuntimeException)
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


```javascript
var fParam = <interface>Proxy.<broadcast>.createFilterParameters();
// for each parameter
fParam.set<Parameter>(parameterValue);
<interface>Proxy.<Broadcast>.subscribe({
    subscriptionQos : subscriptionQosOnChange,
    subscriptionId: subscriptionId,
    filterParameters : fParam,
    onReceive : function(value) {
        // handle subscription broadcast
    },
    onError: function(error) {
        // handle subscription error
        // (e.g. JoynrRuntimeException)
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

```javascript
<interface>Proxy.<Broadcast>.unsubscribe({
    subscriptionId : subscriptionId,
}).then(function() {
    // call successful
}).catch(function(error) {
    // handle error case
});

```

# Building a Javascript Provider application

A Javascript joynr **provider** application must "require" or otherwise load the ```joynr.js```
module and call its ```joynr.load()``` method with provisioning arguments in order to create a
joynr object.

Next, for all Franca interfaces that are being implemented, a **providerBuilder** must be created
using the ```build()``` method of the ```joynr.providerBuilder``` object supplying the providers
domain as argument.

Upon successful creation, the application can register all **Capabilities** it provides, and enter
its event loop where it can handle calls from the proxy side.

## Required imports

The following Javascript modules must be made "required" or otherwise loaded:
```
// for each type <Type>
"import" js/joynr/<Package>/<Type>.js
// for each interface <Interface>
"import" js/joynr/<Package>/<Interface>Proxy.js
"import" js/joynr.js
"import" js/joynrprovisioning.common.js
"import" js/joynrprovisioning.provider.js
```

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
```javascript
var providerQos = new joynr.types.ProviderQos({
    customParameters: [],
    priority : 100,
    scope: joynr.types.ProviderScope.GLOBAL,
    supportsOnChangeSubscriptions : true
});
```

## Provider
A provider application must load joynr and when this has been successfully finished, it can
register a Provider implementation for each Franca interface it implements.
It is also possible to unregister that implementation again, e.g. on shutdown.

While the implementation is registered, the provider will respond to any method calls from outside,
can report any value changes via publications to subscribed consumers, and may fire broadcasts, as
defined in the Franca interface.
```javascript
$(function() {
    // for each <Interface> where a Provider should be registered for later
    var <interface>provider = null;
    var <interface>ProviderImpl = new <Interface>ProviderImpl();

    var provisioning = {};
    provisioning.channelId = "someChannel";

    joynr.load(provisioning).then(function(loadedJoynr) {
        joynr = loadedJoynr;

        // when applications starts up:
        // register <Interface>provider
        ...
        // main loop here
        ...
        // when application ends:
        // unregister <Interface>provider
    }).catch(function(error){
        if (error) {
            throw error;
        }
    });
})();
```

## Register a Provider implementation
When registering a provider implementation for a specific Franca interface, the object implementing
the interface, the provider's domain and the provider's quality of service settings are passed as
parameters.

```javascript
var <interface>ProviderQos;
<interface>Provider = joynr.providerBuilder.build(<Interface>Provider, <interface>ProviderImpl);

// for any filter of a broadcast with filter
<interface>Provider.<broadcast>.addBroadcastFilter(new <Filter>BroadcastFilter());

// setup <interface>ProviderQos
joynr.registration.registerProvider(
    domain,
    <interface>Provider,
    <interface>ProviderQos
).then(function() {
    // registration successful
}).catch(function() {
    // registration failed
});
```

## Unregister a provider
Unregistering a previously registered provider requires the provider's domain and the object that
represents the provider implementation.

```javascript
// provider should have been set and registered previously
joynr.registration.unregisterProvider(
    domain,
    <Interface>provider
).then(function() {
    // unregistration successful
}).catch(function() {
    // unregistration failed
});
```

## The Provider implementation for an interface
The function implementing the interface must provide code for all its methods and a getter function
for every attribute.
```javascript
var <Interface>ProviderImpl =
    function <Interface>ProviderImpl() {
        var self = this;

        // define <method> handler

        // define internal representation of <attribute> and
        // getter handlers per <attribute>
        // wrappers to fire broadcasts
    };
```

## Method handler
Each handler for a Franca method for a specific interface is implemented as a function object
member of **this**. The parameters are provided as objects. The implementation can be done
either asynchronously or synchronously.

### Synchronous implementation
```javascript
this.<method> = function(parameters) {
    // handle method, return returnValue of type <returnType> with
    // return returnValue;
    // - or -
    // throw errorEnumerationValue;
    // (wrt. error enumeration value range please refer to the Franca specification of the method)
    // - or -
    // throw new joynr.exceptions.ProviderRuntimeException({ detailMessage: "reason" });
};
```

### Asynchronous implementation
```javascript
this.<method> = function(parameters) {
    var result = new Promise(function(resolve, reject) {
        // handle method, then either return the value
        // of type <returnType> with
        // resolve(returnValue);
        // - or -
        // reject(errorEnumerationValue);
        // (wrt. error enumeration value range please refer to the Franca specification of the method)
        // - or -
        // reject(new ProviderRuntimeException({ detailMessage: "reason" }));
    });

    // handle method, return returnValue of type <returnType>
    return result;
};
```

## Attribute handler
For each Franca attribute of an interface, a member of **this** named after the
```<attribute>``` has to be created which consists of an object which includes a getter function
as attribute that returns the current value of the ```<attribute>```. Also an internal
representation of the Franca attribute value has to be created and properly intialized.
```javascript
// for each <attribute> of the <interface> provide an internal representation
// and a getter
var internal<Attribute> = <initialValue>;

this.<attribute> = {
    get: function() {
        return attributeValue;
    }
};
```

## Attribute change broadcast
The provider implementation must inform about any change of an attribute by calling valueChanged
on the given attribute.
```javascript
this.<attribute>.valueChanged(newValue);
```
## Sending a broadcast
For each Franca broadcast, a member of **this** named after the ```<broadcast>```
has to be created which consists of an empty object.

```javascript
this.<broadcast> = {};
```

The broadcast can then later be fired using
```javascript
this.fire<Broadcast> = function() {
    var outputParameters;
    outputParameters = self.<broadcast>.createBroadcastOutputParameters();
    // foreach output parameter of the broadcast
    outputParameters.set<Parameter>(value);
    self.<broadcast>.fire(outputParameters);
}
```

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

```javascript
(function(undefined) {
    var <Filter>BroadcastFilter = function <Filter>BroadcastFilter() {
        if (!(this instanceof <Filter>BroadcastFilter)) {
            return new <Filter>BroadcastFilter();
        }

        Object.defineProperty(this, 'filter', {
            enumerable: false,
            value: function(broadcastOutputParameters, filterParameters) {
                // Parameter value can be evaluated by calling getter functions, e.g.
                // broadcastOutputParameters.get<OutputParameter>()
                // filterParameters can be evaluated by using properties, e.g.
                // filterParameters.<property>
                //
                // Evaluate whether the broadcastOutputParameters fulfill
                // the filterParameter here, then return true, if this is
                // the case and the publication should be done, false
                // otherwise.

                return <booleanValue>;
            };
        });
    };

    return <Filter>BroadcastFilter;
}());
```
