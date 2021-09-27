# Using Joynr

A variable number of **Provider** applications can be registered to provide services for **Consumer** applications. A suitable Provider can be selected by the Consumer through a number of discovery lookup strategies.

Joynr supports several programming languages (Java/JEE, C\+\+, JavaScript/TypeScript) within the same system, i.e. a Java Consumer can interact with a C\+\+ Provider etc.

Data serialisation and transfer is transparent to the application. The current implementation supports a number of different transport mechanisms (Web Sockets, MQTT), and serialises to JSON.


## Provider

A provider is an entity that serves as data source for a particular communication interface, and supplies data (attributes) and operations to interested and authorised consumers.

Each provider must be started in a separate directory in order to avoid conflicts with the private settings of other instances.


## Consumer

A consumer is an entity that wants to retrieve data from providers. It can either subscribe to data attribute changes, query providers by calling their operations or subscribe to specific broadcasts.

Each consumer must be started in a separate directory in order to avoid conflicts with the private settings of other instances.


## Communication Interface

Communication interfaces are contracts between providers and consumers defining the data and methods that may be exchanged/communicated via joynr.


## Definition of communication interfaces

The interfaces between the Provider and Consumer applications are defined using the
**Franca Interface Definition Language**. The files will be used as input to automatically generate
Java, C++, etc. program code depending on the target language. A Franca file must be named with
extension ".fidl" and be placed at the correct location in the source tree. The generated code is
then used to implement the Application modelled by the corresponding Franca files.


## Cluster Controller

The Cluster Controller (CC) is a collective term used to group a group of services that assist one or more applications (Providers and Consumers) with joynr discovery, registration and communication. Services that fall under the category of belonging to a Cluster Controller include:

* Registration and discovery – manages the registration and lookup of local and global capabilities (providers).
* Message routing – on message receipt, processes the message and distributes it to the intended recipient. Please note that joynr does not guarantee that messages are delivered in the order in which they were sent.
* Access Control


## Proxy

From within an application, communication with providers is initiated via proxy objects.

Proxies are objects that are representations of the remote provider that communication is initiated with. They are used by consumers to access providers' functionality.

For example, in Berlin, there is a provider providing GPS information for a vehicle. In Munich, we have a consumer that creates a GPS proxy for this vehicle, and uses this object to read information from the Berlin vehicle.

Proxy objects are created using a Proxy Factory. The logic behind the proxy object is to initiate communication with the joynr network and handle the received response while fulfilling the application’s communication requirements - for example, maximum amount of time to wait for a response.


## Capabilities (discovery) directory

A capability (DiscoveryEntry) states the domain and interface name for which a provider is registered.

These entries contain access information as well as supported Qos (Quality of Service). This
information is used in the arbitration process to pick a provider to create a proxy for.

A capabilities directory is a list of capabilities for providers currently registered in a joynr network, and is available globally and locally.

* The global capabilities directory (GCD) is accessible from any joynr-enabled endpoint, and contains a global list of capabilities that may be accessed by joynr consumers.
* The local capabilities directory (LCD) serves as a cache for the global capabilities directory, but also maintains a list of capabilities that only should be discovered locally. Only local consumers attached to the same cluster controller are able to use these local capabilities.


## Arbitration

Arbitration is concerned with the rules in determining the preferred provider to communicate with. In the case where an application requests to communicate with a provider for which there are several matching implementations available (for example, a consumer requests a weather service for Munich, and joynr has several registered for Munich), the consumer must provide an arbitration strategy, which will be used to make the selection (cf. also section [Discovery](#discovery))


## Discovery

Consumers need to create a **Proxy** in order to use Provider services for a specific interface. Providers are registered in either the **local** or **local and global** capabilities directory, have a given priority and, if applicable, one or more **keywords**.

Discovery is the process of looking up providers in the joynr capabilities directory (see the
following section). The discovery is triggered by a consumer application that wants to communicate
with a provider which offers a required service, e.g. weather service. If discovery and arbitration
are successful, joynr creates a proxy for that provider which allows the consumer to communicate
with the selected provider.

In order to find a suitable provider, the consumer must select a **scope** and an **arbitration strategy**.
Optionally, a maximum age for cached remote entries can be required as well, i.e. older entries from cache will be ignored.

The **scope** can be
* **local only** (only providers registered in the local capabilities directory are considered for discovery)
* **local then global** (only if no one exist locally, global ones will be used)
* **local and global** (providers are selected from the local and global capabilities directory)
* **global only** (only global providers will be used)

In case of global lookup, providers are looked up in all backends (all global transports) known to the cluster-controller. Additionally, the discovery can be restricted to some of these backends by providing a list of GBIDs (Global Backend IDentifiers) to the proxy builder services. Usually, this is not necessary. For further information, please refer to our [Multiple backends documentation](multiple-backends.md).

The entries found that match the selected scope are then evaluated based on the arbitration strategy.

The **arbitration strategy** can be one of the following:
* **LAST_SEEN (Java/JS/C++)** the participant that was last refreshed (i.e. with the most current
last seen date) will be selected
* **NotSet (Java)** (not allowed in the app, otherwise arbitration will throw DiscoveryException)
* **Nothing (JS)** use DefaultArbitrator which picks the first discovered entry with
   compatible version
* **HighestPriority (Java/JS) / HIGHEST_PRIORITY (C++)** Entries will be considered according to
   priority
* **Keyword** (Java/JS) / KEYWORD (C++) Only entries that have a matching keyword will be considered
* **FixedChannel (Java) / FIXED_PARTICIPANT (C++)** select provider which matches the participantId
   provided as custom parameter in DiscoveryQos, if existing
* **LOCAL_ONLY (C++)** (not implemented yet, will throw DiscoveryException)
* **Custom (Java/JS)** provide a custom ArbitrationStrategyFunction to allow custom selection of
   discovered entries

**Default arbitration strategy:** LAST_SEEN

The required **keyword** for the arbitration strategy *Keyword* has to be specified by the consumer,
if this kind of strategy has been selected.

Further details can be found in our developer documentations for each supported language.


## Quality of Service

Quality of Service objects allow the application developer to tailor discovery, messaging and subscriptions.
The Qos objects are described in more detail in the Developer Guides separately for each programming language.


## Runtime Environment / Infrastructure
joynr requires the following components to run:

### MQTT Broker
The default configuration communicates via MQTT and needs a MQTT broker (e.g.
[Mosquitto](http://mosquitto.org)) listening on port 1883.  
See [Infrastructure](infrastructure.md).

### Global Capabilities (Discovery) Directory (GCD)
Centralized directory to discover providers for a given domain and interface.
The discovery directory is using MQTT communication.  
See [Infrastructure](infrastructure.md).


## Further Reading

General:
* For details about Franca IDL see [Franca IDL Guide](franca.md).
* For details about the joynr Code Generator see [joynr Code Generator Guide](generator.md)
* For details about the required joynr infrastructure components see
  [joynr infrastructure Guide](infrastructure.md)
* For details about joynr with multiple backends (multiple global connections) see
  [joynr multiple backends Guide](multiple-backends.md)

See our main [Readme with the table of contents](../README.md) for our developer guides and further
reading.

## Tutorials

* **[A tour through a simple radio application (Java and C++)](Tutorial.md):**
This tutorial guides you through a simple joynr application, explaining essential concepts such as
communication interfaces, consumers, providers and how they communicate.
* **[Using selective broadcast to implement a geocast (Java and C++)](Broadcast-Tutorial.md):**
In that tutorial the [RadioApp example](Tutorial.md) is extended by a selective broadcast and filter
logics that implements a [geocast](http://en.wikipedia.org/wiki/Geocast).
* **[Create your first joynr JavaScript application](JavaScriptTutorial.md):**
This tutorial presents the JavaScript implementation of the radio application of the previous tutorials.
