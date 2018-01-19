# Using Joynr

A variable number of **Provider** applications can be registered to provide services for **Consumer** applications. A suitable Provider can be selected by the Consumer through a number of discovery lookup strategies.

Joynr supports several programming languages (Java, C\+\+, JavaScript) within the same system, i.e. a Java Consumer can interact with a C\+\+ Provider etc.

Data serialisation and transfer is transparent to the application. The current implementation supports a number of different transport mechanisms (Web Sockets, HTTP Long Polling), and serialises to JSON.


## Provider

A provider is an entity that serves as data source for a particular communication interface, and supplies data (attributes) and operations to interested and authorised consumers.

Each provider must be started in a separate directory in order to avoid conflicts with the private settings of other instances.


## Consumer

A consumer is an entity that wants to retrieve data from providers. It can either subscribe to data attribute changes, query providers by calling their operations or subscribe to specific broadcasts.

Each consumer must be started in a separate directory in order to avoid conflicts with the private settings of other instances.


## Communication Interface

Communication interfaces are contracts between providers and consumers defining the data and methods that may be exchanged/communicated via joynr.


## Arbitration

Arbitration is concerned with the rules in determining the preferred provider to communicate with. In the case where an application requests to communicate with a provider for which there are several matching implementations available (for example, a consumer requests a weather service for Munich, and joynr has several registered for Munich), the consumer must provide an arbitration strategy, which will be used to make the selection (cf. also section [Discovery](#discovery))


## Capabilities (discovery) directory

A capability states the domain and interface name for which a provider is registered.

 These information entries contain access information as well as supported Qos (Quality of Service). This
information is used in the arbitration process to pick a provider for a proxy.

A capabilities directory is a list of capabilities for providers currently registered in a joynr network, and is available globally and locally.

* The global directory is accessible from any joynr-enabled endpoint, and contains a global list of capabilities that may be accessed by joynr consumers.
* The local capabilities directory serves as a cache for the global directory, but also maintains a list of capabilities that only should be discovered locally. Only local consumers attached to the same cluster controller are able to use these capabilities.


## Cluster Controller

The Cluster Controller (CC) is a collective term used to group a group of services that assist one or more applications with joynr discovery, registration and communication. Services that fall under the category of belonging to a Cluster Controller include:

* Registration and discovery – manages the registration of local and global capabilities.
* Message routing – on message receipt, processes the message and distributes it to the intended recipient.
* Access Control


## Discovery

Consumers need to create a **Proxy** in order to use Provider services for a specific interface. Providers are registered in either the **local** or **local and global** capabilities directory, have a given priority and, if applicable, one or more **keywords**.

In order to find a suitable provider, the consumer must select a **scope** and an **arbitration strategy**.
Optionally, a maximum age for cached entries can be required as well, i.e. older entries from cache will be ignored.

The **scope** can be
* **local only** (only providers registered in the local capabilities directory are considered for discovery)
* **local then global** (only if no one exist locally, global ones will be used)
* **local and global** (providers are selected from the local and global capabilities directory)
* **global only** (only global providers will be used)

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


## Proxy

From within an application, communication is initiated via proxy objects.

Proxies are objects that are representations of the remote provider that communication is initiated with. They are used by consumers to access providers' functionality.

For example, in Berlin, there is a provider providing GPS information for a vehicle. In Munich, we have a consumer that creates a GPS proxy for this vehicle, and uses this object to read information from the Berlin vehicle.

Proxy objects are created using a Proxy Factory. The logic behind the proxy object is to initiate communication with the joynr network and handle the received response while fulfilling the application’s communication requirements - for example, maximum amount of time to wait for a response.


## Quality of Service

Quality of Service objects allow the application developer to tailor discovery and messaging.
The Qos objects are described in more detail in the Developer Guides separately for each programming language.


## Definition of communication interfaces

The interfaces between the Provider and Consumer applications are defined using the
**Franca Interface Definition Language**. The files will be used as input to automatically generate
Java, C++, etc. program code depending on the target language. A Franca file must be named with
extension ".fidl" and be placed at the correct location in the source tree. The generated code is
then used to implement the Application modelled by the corresponding Franca files.

>*Note: Since the necessary Franca dependencies are currently not available from
>[Maven Central Repository](http://search.maven.org/), we ship Franca dependencies together with the
>joynr source code in the `<JOYNR>/tools/generator/dependency-libs/` directory.*
>
>*If you build joynr yourself using the provided docker and / or Maven infrastructure, the Franca
>dependencies are installed to your local Maven repository during the build.*


## Runtime Environment
joynr requires the following components to run:

### MQTT Broker / HTTP Bounceproxy
The default configuration communicates via MQTT and needs a MQTT broker (e.g.
[Mosquitto](http://mosquitto.org)) listening on port 1883.
If you configure your application to communicate via HTTP, the HTTP bounceproxy is required
instead of the MQTT broker.

#### HTTP bounceproxy
joynr can also be configured to communicate via HTTP using Comet (currently long poll), based on the
Atmosphere Framework. Instead of the MQTT broker, the HTTP bounceproxy is responsible for message
store and forward in this case.

After joynr has been built (see [Building joynr Java and common components](java_building_joynr.md)),
you can run the bounceproxy directly within Maven (for test purposes). Just go into the bounceproxy
project and run
```bash
<JOYNR>$ cd java/messaging/bounceproxy/single-bounceproxy
<JOYNR>/java/messaging/bounceproxy/single-bounceproxy$ mvn jetty:run
```
The bounceproxy is also tested with glassfish 3.1.2.2. See [Glassfish settings]
(Glassfish-settings.md) for configuration details.

>*Note: This is only for test purposes. You need to have Maven installed. Joynr is built and tested
>with Maven 3.3.3, but more recent versions of Maven might also work.*

### Global Discovery Directory
Centralized directory to discover providers for a given domain and interface.
There are two variant of the discovery directory, one using MQTT communication (default) and one
using HTTP communication.

#### MQTT (default)
See [Infrastructure](infrastructure.md).

#### HTTP
Run the discovery directories locally along with the bounceproxy:

1. Build and install the whole joynr project from the root directory (see [Building joynr Java and
common components](java_building_joynr.md))
1. start directories **and** bounceproxy on default jetty port 8080:
```bash
<JOYNR>$ cd java/backend-services/discovery-directory-servlet
<JOYNR>/java/backend-services/discovery-directory-servlet$ mvn jetty:run
```

>*Note: This is only for test purposes. You need to have Maven installed. Joynr is built and tested
>with Maven 3.3.3, but more recent versions of Maven might also work.*

Use the following links to check whether all components are running:

| Service Link | Description |
| ------------ | ----------- |
| <http://localhost:8080/bounceproxy/time/> | Returns the current time in current milliseconds since Unix Epoch. This can be used to test whether the bounceproxy is up and running. |
| <http://localhost:8080/bounceproxy/channels.html> | Lists all channels (message queues) that are currently registered on the bounceproxy instance |
| <http://localhost:8080/discovery/capabilities.html> | Lists all capabilities (providers) currently registered with joynr. Note: After starting the discovery directories and the bounceproxy only, there must be two capabilities registered (channel URL directory and global capabilities directory). |

You can also deploy one or more joynr applications to a servlet engine without reconfiguring the
applications themselves:

1. Simply create a WAR maven project
1. include your application(s) as dependency in the pom.xml
1. include messaging-servlet as a dependency in the pom.xml.
1. create the war file (mvn package)
1. The war created should contain JARs for each of your applications plus the messaging-servlet
   (and other transitive dependencies).
1. Set the JVM properties etc for your servlet engine as described on [Glassfish settings]
   (Glassfish-settings.md).
1. deploy this war to your servlet engine.

All applications deployed should then register themselves with the discovery directory. Messages
will be sent directly to the url registered in hostPath.


## Further Reading

* For details about Franca see [Franca Guide](franca.md).
* For details about the joynr Code Generator see [joynr Code Generator guide](generator.md)


* For details about using joynr with Java see [joynr Java Developer Guide](java.md).
* For details about using joynr with C++ see [joynr C\+\+ Developer Guide](cplusplus.md).
* For details about using joynr with JavaScript see [joynr JavaScript Developer Guide](javascript.md).
* For details about using joynr Javascript tests see [joynr Javascript Testing](javascript_testing.md)


* For building joynr Java yourself see [Building joynr Java and common components](java_building_joynr.md)
* For building joynr C\+\+ yourself see [Building joynr C++](cpp_building_joynr.md)
* For building joynr JavaScript yourself see [Building joynr JavaScript](javascript_building_joynr.md)


## Tutorials

* **[A tour through a simple radio application](Tutorial.md):**
This tutorial guides you through a simple joynr application, explaining essential concepts such as
communication interfaces, consumers, providers and how they communicate.
* **[Using selective broadcast to implement a geocast](Broadcast-Tutorial.md):**
In this tutorial [RadioApp example](Tutorial.md) is extended by a selective broadcast and filter
logics that implements a [geocast](http://en.wikipedia.org/wiki/Geocast).
