# Shortcomings found yet

## Java

Franca `UInt8` are stored as Java `Byte` - too low range (128..255 not covered), 
negative range possible. \
Franca `UInt16` are stored as Java `Short` - too low range (32768..65535 not covered), 
negative range possible. \
Franca `UInt32` are stored as Java `Integer` - too low range (2^31..2^32-1 not covered), 
negative range possible. \
Franca `UInt64` are stored as Java `Long` - too low range (2^63..2^64-1 not covered), 
negative range possible.

It is not possible to code tests with too low range representation.
Those tests will however fail, when coded in C++.

# joynr JEE inter-language-test

The `io.joynr.tests.inter-language-test` project provides an inter-language
test environment using JEE applications in catalogs named `inter-language-test-jee-*`.

As of now, the following features are supported:

* Synchronous method calls
* Synchronous Getter / Setter calls for attributes

## Installation

If you're building from source, then you can build and install the artifacts to
your local maven repo with:

```
cd <repository>/tests/inter-language-test
mvn install
```

## Usage

This section describes the general usage of the joynr inter-language-test for jee
and provides simple examples of the usage where possible.

### Configuration

The configuration is included in the files
`src/main/java/io/joynr/test/interlanguage/jee/JoynrConfigurationProvider.java`
in the inter-language-test-jee-provider and inter-language-test-jee-consumer
directories.

A `@Singleton` EJB which has no business interface (i.e. does not
implement any interfaces) provides methods annotated with:

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

* `MessagingpropertyKeys.CHANNELID` - this property should be set to the
  application's unique DNS entry, e.g. `myapp.mycompany.net`. This is important,
  so that all nodes of the cluster are identified by the same channel ID.
* `ConfigurableMessagingSettings.PROPERTY_GBIDS` - use this to configure the GBIDs for
  connecting to the MQTT brokers being used for communication.
  E.g. `gbid1,gbid2`.
        * A GBID (Global Backend IDentifier) identifies a single backend independently of its uri.
          This is necessary for multiple backends, since the broker-uri of a backend can differ
          depending on location.
  This property is required if Mqtt is configured to be used as global transport.
* `MqttModule.PROPERTY_MQTT_BROKER_URIS` - use this to configure the URLs for
  connecting to the MQTT brokers identified by `ConfigurableMessagingModule.PROPERTY_GBIDS`.
  If used, the number of configured broker-uris must be equal to the number of configured gbids.
  E.g. `tcp://mqtt.mycompany.net:1883,tcp://mqtt.othercompany.net:1883`.


#### Optional Properties

* `MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS` - enables the
  [HiveMQ](http://www.hivemq.com) specific 'shared subscription' feature, which allows
  clustering of JEE applications using just MQTT for communication. Set this to `true`
  to enable the feature. Defaults to `false`.
* `MessagingPropertyKeys.PROPERTY_GLOBAL_CAPABILITIES_DIRECTORY_URL` configure the address  
  for the discovery directory service.
* `MessagingPropertyKeys.PERSISTENCE_FILE` - if you are deploying multiple joynr-enabled
  applications to the same container instance, then you will need to set a different filename
  for this property for each application. E.g.: `"my-app-joynr.properties"` for one and
  `"my-other-app-joynr.properties"` for another. Failing to do so can result in unexpected
  behaviour, as one app will be using the persisted properties and IDs of the other app.

You are principally free to provide any other valid joynr properties via these
configuration methods. See the [official joynr documentation](./JavaSettings.md)
for details.

#### Example

An example of a configuration EJB, which uses `mqtt` as primary global transport is:

```
@Singleton
public class JoynrConfigurationProvider {

  @Produces
  @JoynrProperties
  public Properties joynrProperties() {
    Properties joynrProperties = new Properties();
    joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID,
        "io.joynr.test.interlanguage.jee.provider");
    joynrProperties.setProperty(ConfigurableMessagingSettings.PROPERTY_GBIDS,
        "joynrdefaultgbid");
    joynrProperties.setProperty(MqttModule.PROPERTY_MQTT_BROKER_URIS,
        "tcp://localhost:1883");

    return joynrProperties;
  }

  @Produces
  @JoynrLocalDomain
  public String joynrLocalDomain() {
    return "joynr-inter-language-test-domain";
  }
}
```


### JEE Container configuration

Configure your container runtime with a `ManagedScheduledExecutorService`
resource which has the name `'concurrent/joynrMessagingScheduledExecutor'`.

For example for Glassfish/Payara:
`asadmin create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor`

>Note the `--corepoolsize=100` option. The default will only create one thread,
>which can lead to blocking

## Example

Under `tests/inter-language-test` you can find the test applications.
It uses an `InterLanguageTest.fidl` file based on the one used for the
standard inter-language-test without JEE, but implements it as a JEE provider
application and a separate JEE consumer application.

The project is sub-divided into one multi-module parent project and four subprojects:

```
 - inter-language-test
   |- inter-language-test-base
   |- inter-language-test-jee-api
   |- inter-language-test-jee-provider
   |- inter-language-test-jee-consumer
```

In order to build the project, change to the `inter-language-test` directory and call `mvn install`.

Next, fire up the joynr infrastructure components with default configuration (single backend),
see [joynr infrastructure](../../wiki/infrastructure.md).

### Running JEE tests separately

The following describes running the example on [Payara 5](http://www.payara.fish). First,
install the application server.

Start up the Payara server by changing to the Payara install directory and executing
`bin/asadmin start-domain`. Follow the instructions above for configuring the required
managed executor service.

Depending on whether only consumer, only provider or both should run as JEE applications,
deploy the required WAR files:

- `bin/asadmin deploy <joynr home>/tests/inter-language-test/inter-language-test-jee-provider/target/inter-language-test-jee-provider.war`
- `bin/asadmin deploy <joynr home>/tests/inter-language-test/inter-language-test-jee-consumer/target/inter-language-test-jee-consumer.war`

Make sure that any involved external embedded or standalone cluster controller is configured
correctly.

In case just the JEE provider should be tested, then once the provider application has started
successfully, you can start up the external consumer as normal. Note that an external consumer
might start test cases which are not supported by the JEE interlanguage test environment.
The resulting failure can be ignored.

If the JEE consumer application should be used, then once the consumer application and the JEE or
external provider application have started up successfully, you can use an HTTP client
(e.g. `curl` on the command line or [Paw](https://luckymarmot.com/paw) on Mac OS X) to trigger
the start of the JUnit test cases on the consumer side.

- `curl http://localhost:8080/inter-language-test-jee-consumer/start-tests'
  - This starts the tests, and you should see some output similar to Surefire JUnit XML reports,
    but in condensed JSON format.

Example JSON output (pretty-printed):
```
{
  "testSuiteResults": [
    {
      "name": <nameOfTestSuite>,
      "tests": <numberOfTests>,
      "errors": <numberOfErrors>,
      "skipped": <numberOfSkippedTests>,
      "testCaseResults" : [
        {
          "classname": "io.joynr.test.interlanguage.jee.<TestSuiteClass>",
          "name": "<nameOfTestCase>,
          "status": "ok",
          "time": "<timeUsed>"
        },
        ...
        {
          "classname": "io.joynr.test.interlanguage.jee.<TestSuiteClass>",
          "name": "<nameOfTestCase>,
          "status": "failed",
          "time": "<timeUsed>",
          "failure": {
            "message": "<failureMessage>",
            "text": "<detailedInfoIfAny",
            "type": "<typeIfAny>"
          }
        },
        ...
      ]
    }
  ]
}
```

Note that by definition the order of JSON elements in a JSON object is undefined;
only JSON arrays have a defined order - so your output might look a bit different.
