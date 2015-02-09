joynr is an Internet-ready communications framework, designed to communicate between:
* Hand-held devices
* PCs
* Backend Services
* Vehicles
* ...

joynr supports three communication paradigms

1. attribute Get/Set and Subscriptions (Pub/Sub),
1. Remote Procedure Call (RPC),
1. and Broadcasts or event subscriptions.

joynr is made up of three layers:

1. Interface definitions, defined using the Franca Interface Definition Language (IDL)
1. Java or C++ programming interfaces that are used to provide or consume the attributes and operations defined in Franca IDL.
1. Messaging Layer, which sends joynr messages via http, serialised as json.

# Development Environment
## Modelling
Modelling is done using Franca IDL, which can be written in any text editor. 

We recommend using the Franca Eclipse tooling to simplify the process (syntax highlighting, code completion, model validation etc.) :
Franca IDL (https://code.google.com/a/eclipselabs.org/p/franca/) 

Code generation itself is integrated into the Maven build process. See the Radio App tutorial and sample code for more information.

## Java Development
* JDK 1.7
* Maven 3

NOTE: we implement in Eclipse with the following plugins:
* M2E - Maven Integration for Eclipse
* M2E connector for build-helper-maven-plugin
* M2E connector for the Eclipse JDT Compiler

and optionally for Android development:
* Android for Maven Eclipse
* Android Development Tools

... but any IDE should work just fine. Netbeans for instance also has nice Maven integration.

Tip: If using Eclipse, use the Maven importer to import your project from the POM. After a change to your model, execute Maven install (select `Run As -> Maven install` from the context menu) in order to regenerate the interfaces etc.

## C++ Development
* CMake (2.8.12)
* Qt SDK (5.3.2, http://qt-project.org/downloads)
* cURL (7.32.0)
* GNU (4.8.3)

Versions mentioned in parentheses are currently used to build joynr by our continuous integration system and therefore verified to be working. However, also slightly different versions of these dependencies might work too.

# Building joynr
## Building Code Generators and joynr Java
Use Maven to build joynr Code Generators and joynr Java. To speed up the build process you can skip test execution.
```bash
<JOYNR>$ mvn clean install -DskipTests
```
This command will build and install following components (listed by subfolder):
* `tools`
  * build resources needed during build
  * dependency libraries needed during build not available in Maven Central
  * generator framework and a corresponding Maven Plugin
* `basemodel`
  * Franca files describing communication interfaces to infrastructure services
* `java`
  * Java code generator
  * joynr Java API
  * generated Java source code (needed to access infrastructure services and tests)
  * infrastructure services (bounceproxy and discovery directories)
* `cpp`
  * C++ code generator
  * generated C++ source code (needed to access infrastructure services and tests)
* `examples`
  * Radio app example (including code generation for Java and C++)

## Building C++
Use CMake and Make to build joynr C++.
```bash
# PREREQUISITE: tools and basemodel are built and installed
<JOYNR>$ cd cpp
# generate C++ sources needed to access infrastructure services
<JOYNR>/cpp$ mvn generate-sources
# build joynr C++
# disable tests and code formatter
<JOYNR>/cpp$ mkdir build
<JOYNR>/cpp$ cd build
<JOYNR>/cpp/build$ cmake -DBUILD_TESTS=OFF -DENABLE_CLANG_FORMATTER=OFF ..
<JOYNR>/cpp/build$ make -j8
```

# Runtime Environment
joynr requires the following components to run:
## Bounceproxy
Responsible for message store and forward using Comet (currently long poll), based on the Atmosphere Framework. 

For test purposes you can run the bounceproxy directly within Maven. Just go into the bounceproxy project and run
```bash
<JOYNR>$ mvn clean install -DskipTests
<JOYNR>$ cd java/messaging/bounceproxy/single-bounceproxy
<JOYNR>/java/messaging/bounceproxy/single-bounceproxy$ mvn jetty:run  
```

The bounceproxy is also tested with glassfish 3.1.2.2. See [Glassfish settings](Glassfish-settings.md) for configuration details.

## Discovery Directories
Centralized directory to discover providers for a given domain and interface.

Run the discovery directories locally along with the bounceproxy:

1. Use maven to build and install the whole joynr project from the root directory
1. start directories and bounceproxy on default jetty port 8080

```bash
<JOYNR>$ mvn clean install -DskipTests
<JOYNR>$ cd java/backend-services/discovery-directory-servlet
<JOYNR>/java/backend-services/discovery-directory-servlet$ mvn jetty:run
```

Use the following links to check whether all components are running:

| Service Link | Description |
| ------------ | ----------- |
| <http://localhost:8080/bounceproxy/time/> | Returns the current time in current milliseconds since Unix Epoch. This can be used to test whether the bounceproxy is up and running. |
| <http://localhost:8080/bounceproxy/channels.html> | Lists all channels (message queues) that are currently registered on the bounceproxy instance |
| <http://localhost:8080/discovery/capabilities.html> | Lists all capabilities (providers) currently registered with joynr. Note: After starting the discovery directories and the bounceproxy only, there must be two capabilities registered (channel URL directory and global capabilities directory). |

You can also deploy one or more joynr applications to a servlet engine without reconfiguring the applications themselves:

1. Simply create a WAR maven project
1. include your application(s) as dependency in the pom.xml
1. include messaging-servlet as a dependency in the pom.xml. 
1. create the war file (mvn package)
1. The war created should contain JARs for each of your applications plus the messaging-servlet (and other transitive dependencies). 
1. Set the JVM properties etc for your servlet engine as described on [Glassfish settings](Glassfish-settings.md).
1. deploy this war to your servlet engine.

All applications deployed should then register themselves with the discovery directory. Messages will be sent directly to the url registered in hostPath.

# Tutorials
* **[A tour through a simple radio application](Tutorial.md):**
This tutorial guides you through a simple joynr application, explaining essential concepts such as
communication interfaces, consumers, providers and how they communicate.

* **[Using selective broadcast to implement a geocast](Broadcast-Tutorial.md):**
In this tutorial [RadioApp example](Tutorial.md) is extended by a selective broadcast and filter
logics that implements a [geocast](http://en.wikipedia.org/wiki/Geocast).

# Releases
joynr is currently at released version 0.8.0. See the [release notes](ReleaseNotes.md).
