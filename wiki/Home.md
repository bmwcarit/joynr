joynr is an Internet-ready communications framework, designed to communicate between:
* Hand-held devices
* PCs
* Backend Services
* Vehicles
* ...

joynr supports attribute Get/Set, Subscriptions and Remote Procedure Call (RPC). 

joynr is made up of three layers:

1. Interface definitions, defined using the Franca Interface Definition Language (IDL)
1. Java or C++ programming interfaces that are used to provide or consume the attributes and operations defined in Franca IDL.
1. Messaging Layer, which sends joynr messages via http, serialised as json.

## Development Environment
### Modelling
Modelling is done using Franca IDL, which can be written in any text editor. 

We recommend using the Franca Eclipse tooling to simplify the process (syntax highlighting, code completion, model validation etc.) :
Franca IDL (https://code.google.com/a/eclipselabs.org/p/franca/) 

Code generation itself is integrated into the Maven build process. See the Radio App tutorial and sample code for more information.

### Java Development
* Maven 3

NOTE: we implement in Eclipse with the following plugins:
* M2E - Maven Integration for Eclipse
* M2E connector for build-helper-maven-plugin
* M2E connector for the Eclipse JDT Compiler

and optionally for Android development:
* Android Configurator for M2E	
* Android Development Tools

... but any IDE should work just fine. Netbeans for instance also has nice Maven integration.

Tip: If using Eclipse, use the Maven importer to import your project from the POM. After a change to your model, run as... Maven install in order to regenerate the interfaces etc.

### C++ Development
* CMake 2.8.9
* Qt SDK 5.1.x (http://qt-project.org/downloads)
* cURL
* g++
* ccache

## Runtime Environment
joynr requires the following components to run:
### Bounceproxy
responsible for message store and forward using Comet (currently long poll), based on the Atmosphere Framework. 

For test purposes you can run the bounceproxy directly within Maven. Just go into the bounceproxy project and run
```
    JOYNR>/java/messaging/bounceproxy/bounceproxy$ mvn jetty:run  
```

The bounceproxy is also tested with glassfish 3.1.2.2. See [[Glassfish settings]] for configuration details.

### Discovery Directories
Centralised directory to discover providers for a given domain and interface. 

Run the discovery directories locally along with the bounceproxy:

1. Use maven to build and install the whole joynr project from the root directory
1. start directories and bounceproxy on default jetty port 8080

```
<JOYNR>$ mvn clean install -DskipTests
<JOYNR>$ cd java/backend-services/discoverydirectoryservlet
<JOYNR>/java/backend-services/discoverydirectoryservlet$ mvn jetty:run
```

You can also deploy one or more joynr applications to a servlet engine without reconfiguring the applications themselves:

1. Simply create a WAR maven project
1. include your application(s) as dependency in the pom.xml
1. include messaging-servlet as a dependency in the pom.xml. 
1. create the war file (mvn package)
1. The war created should contain JARs for each of your applications plus the messaging-servlet (and other transitive dependencies). 
1. Set the JVM properties etc for your servlet engine as described on [[Glassfish settings]].
1. deploy this war to your servlet engine.

All applications deployed should then register themselves with the discovery directory. Messages will be sent directly to the url registered in hostPath.

## Tutorials
**[A tour through a simple radio application](Tutorial)**
This tutorial guides you through a simple joynr application, explaining essential concepts such as communication interfaces, consumers, providers and how they communicate.