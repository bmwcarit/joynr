This tutorial will guide you through a simple joynr radio application, explaining three essential
joynr concepts:

* A simple radio **communication interface**
* A **consumer** interested in knowing about radio information
* A **provider**, that provides the radio information

# Prerequisites
If you haven't built joynr yet, please do so first:

* [Building joynr Java](java_building_joynr.md).

This will install the necessary dependencies to your local Maven repository and generate the radio
application source files. In particular, the
[Franca IDL](https://code.google.com/a/eclipselabs.org/p/franca/) dependencies that are currently
not available from [Maven Central Repository](http://search.maven.org/) are installed. Since Franca
is needed for joynr code generation, we ship Franca dependencies together with the joynr source code
in the `<JOYNR>/tools/generator/dependency-libs/` directory.

## Building joynr JavaScript

The joynr JS binding is build using Maven:

```bash
<JOYNR>/javascript$ mvn clean install -DskipTests
```

# Exploring the JavaScript demo

The JavasciptRadioApp demo is located in `<JOYNR>/javascipt/apps/radio`. We refer to this location as
`RADIO_HOME`.

Have a look into the [Radio Tutorial](Tutorial.md) to get an understading of the communication
interface and the basic joynr concepts.

The generated source code from the Radio model is located in `<RADIO_HOME>/src/main/generated`.

## Providers

Have a look into [\<RADIO_HOME\>/src/main/webapp/js/provider.js]
(/javascript/apps/radio/src/main/webapp/js/provider.js) for provider implementation and
registration details.

## Consumers

Have a look into [\<RADIO_HOME\>/src/main/webapp/js/consumer.js]
(/javascript/apps/radio/src/main/webapp/js/consumer.js) for consumer implementation details.

## In Action

### Prerequisite
You need to have Maven installed. Joynr is tested with Maven 3.2.5, but more recent versions should
also work here.

For both, consumer and provider, the backend (Bounceproxy and Discovery) has to be started first.

### Starting the Backend
The following Maven command will start a [Jetty Server](http://eclipse.org/jetty/) on
`localhost:8080` and automatically deploy Bounceproxy and Discovery services:

```bash
<RADIO_HOME>$ mvn jetty:run-war
```

This will also deploy the provider and consumer webapp on the jetty server.

### Running the Provider and Consumer

To run the provider and consumer open the following URLs in your browser (tested with Chrome and
Firefox):

* Provider: http://localhost:8080/provider.html
* Consumer: http://localhost:8080/consumer.html
