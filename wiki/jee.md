# joynr JEE Integration

The `io.joynr.jeeintegration` project provides an integration layer for
joynr in JEE Applications.
The features supported are:

* Expose session beans as joynr providers via the `@ServiceProvider` annotation
* Inject a `ServiceLocator` in order to obtain consumer proxies for calling
other services
* Internally uses EE container managed thread pools
* Uses the EE container's JAX RS to receive joynr messages via HTTP(s)

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

    <dependency>
      <groupId>io.joynr.java</groupId>
      <artifactId>jeeintegration</artifactId>
      <version>${joynr.version}</version>
    </dependency>

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
`javax.enterprise.inject.Produces`.

#### Manadatory Properties

* `MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT` - this property needs
to be set to the context root of your deployed application, with `/messaging`
added to the end. E.g.: `/myapp/root/messaging`.
* `MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH` - this property needs to
be set to the URL under which the application server you are running on can be
reached, e.g. `https://myapp.mycompany.net`.
* `MessagingpropertyKeys.CHANNELID` - this property should be set to the
application's unique DNS entry, e.g. `myapp.mycompany.net`. This is important,
so that all nodes of the cluster are identified by the same channel ID.
* `MqttModule.PROPERTY_KEY_MQTT_BROKER_URI` - use this to configure the URL for
connecting to the MQTT broker being used for communication.
E.g. `tcp://mqtt.mycompany.net:1883`.

#### Optional Properties

* `MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS` - enables the [HiveMQ](http://www.hivemq.com) specific 'shared subscription' feature, which allows clustering of JEE applications using just MQTT for communication. Set this to `true` to enable the feature. Defaults to `false`.
* `JeeIntegrationPropertyKeys.JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY` -
set this property to `true` if you want to use the HTTP Bridge functionality. In this
configuration incoming messages are communicated via HTTP and can then be load-balanced
accross a cluster via, e.g. nginx, and outgoing messages are communicated directly
via MQTT. If you activate this mode, then you must also provide an endpoint registry
(see next property).
* `JeeIntegrationPropertyKeys.JEE_INTEGRATION_ENDPOINTREGISTRY_URI` -
this property needs to
point to the endpoint registration service's URL with which the
JEE Integration will register itself for its channel's topic.
E.g. `http://endpointregistry.mycompany.net:8080`.
* `MessagingPropertyKeys.DISCOVERYDIRECTORYURL` and
`MessagingPropertyKeys.DOMAINACCESSCONTROLLERURL` - configure the addresses for the
discovery directory and domain access control services.
* `MessagingPropertyKeys.PERSISTENCE_FILE` - if you are deploying multiple joynr-enabled
applications to the same container instance, then you will need to set a different filename
for this property for each application. E.g.: `"my-app-joynr.properties"` for one and
`"my-other-app-joynr.properties"` for another. Failing to do so can result in unexpected
behaviour, as one app will be using the persisted properties and IDs of the other app.

You are principally free to provide any other valid joynr properties via these
configuration methods. See the [official joynr documentation](./JavaSettings.md)
for details.

#### Example

An example of a configuration EJB is:

	@Singleton
	public class JoynrConfigurationProvider {

	  @Produces
	  @JoynrProperties
	  public Properties joynrProperties() {
		Properties joynrProperties = new Properties();
		joynrProperties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT,
			"/myapp/messaging");
		joynrProperties.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH,
			"http://myapp.com:8080");
		joynrProperties.setProperty(MessagingPropertyKeys.CHANNELID,
			"provider.domain");
		joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS,
			Boolean.TRUE.toString());
		joynrProperties.setProperty(MqttModule.PROPERTY_KEY_MQTT_BROKER_URI,
			"tcp://mqttbroker.com:1883");
		joynrProperties.setProperty(MessagingPropertyKeys.DISCOVERYDIRECTORYURL,
			"http://joynrbackend/discovery/channels/discoverydirectory_channelid/");
		joynrProperties.setProperty(MessagingPropertyKeys.DOMAINACCESSCONTROLLERURL,
			"http://joynrbackend/discovery/channels/discoverydirectory_channelid/");
		joynrProperties.setProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL,
			"http://joynrbackend/bounceproxy/");

		return joynrProperties;
	  }

	  @Produces
	  @JoynrLocalDomain
	  public String joynrLocalDomain() {
		return "provider.domain";
	  }

	}


### JEE Container configuration

Configure your container runtime with a `ManagedScheduledExecutorService`
resource which has the name `'concurrent/joynrMessagingScheduledExecutor'`.

For example for Glassfish/Payara:
`asadmin create-managed-scheduled-executor-service --corepoolsize=100 concurrent/joynrMessagingScheduledExecutor`

Note the `--corepoolsize=100` option. The default will only create one thread, which can lead to
blocking. 100 threads should be sufficient for quite a few joynr applications.
Depending on your load, you can experiment with different values. Use higher values to enable more
concurrency when communicating joynr messages.

### Generating the interfaces

When generating the interfaces for use in a JEE environment, you have to
activate an additional parameter in the joynr generator (see also [the generator documentation](generator.md)).

Here's an example of what the plugin configuration might look like:

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

			  <!-- ACTIVATE THIS TO GENERATE JEE COMPATIBLE INTERFACES -->
			  <parameter>
				<jee>true</jee>
			  </parameter>

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

Note the `<parameter><jee>true</jee></parameter>` part. This is essential for
generating artefacts which are compatible with the JEE integration.

### Implementing services

Annotate your business beans with `@ServiceProvider` additionally to the usual
JEE annotations (e.g. `@Stateless`).

For example, if we have defined a `MyService` interface in Franca for which
we want to provide an implementation, then:

    @ServiceProvider(serviceInterface = MyServiceSync.class)
    @Stateless
    public class MyServiceImpl implements MyServiceSync {
    	...
    }

In order to set Provider QoS parameters, provide an implementation of the
`ProviderQosFactory` interface. You can provide multiple beans implementing
the `ProviderQosFactory` interface. The first one found which returns `true`
to `providesFor(Class)` for a given service interface will be used to obtain
a `ProviderQos` instance via the `create()` method in order to perform the
service registration. There is no guarantee in which order the factories will
be asked, so it's safer to just provide one factory for a given service
interface type. If no factory is found to provide the QoS information,
then the default values are used.

Here's an example of what that could look like for the `MyService` we used above:

	@Singleton
	public class MyServiceQosProviderFactory implements ProviderQosFactory {

	    @Override
	    public ProviderQos create() {
	        ProviderQos providerQos = new ProviderQos();
	        providerQos.setCustomParameters(new CustomParameter[]{ new CustomParameter("name", "value") });
	        providerQos.setPriority(1L);
	        return providerQos;
	    }

	    @Override
	    public boolean providesFor(Class<?> serviceInterface) {
	        return MyServiceSync.class.equals(serviceInterface);
	    }

	}

#### Injecting the calling principal

In order to find out who is calling you, you can inject the `JoynrCallingPrincipal`
to your EJB. This will only ever be set if your EJB is called via joynr (i.e. if you
are also calling your EJB via other routes, e.g. JAX-RS, then the principal will not
be set).

See the
[System Integration Test](../tests/system-integration-test/sit-jee-app/src/main/java/io/joynr/systemintegrationtest/jee/SystemIntegrationTestBean.java)
for an example of its usage.

#### <a name="provider_domain"></a> Customising the registration domain

In some cases you might want to register your providers under a different domain than the
application default (specified via `@JoynrLocalDomain`, see configuration documentation above).

In order to do so, use the `@ProviderDomain` annotation on your implementing bean in addition
to the `@ServiceProvider` annotation. The value you provide will be used as the domain when
registering the bean as a joynr provider.
Here is an example of what that looks like:

    @ServiceProvider(serviceInterface = MyServiceSync.class)
    @ProviderDomain(MY_CUSTOM_DOMAIN)
    private static class CustomDomainMyServiceBean implements MyServiceSync {
        ...
    }

#### <a name="publishing_multicasts"></a> Publishing Multicasts

If you have `broadcast` definitions in your Franca file which are __not__ `selective`, then
you can `@Inject` the corresponding `SubscriptionPublisher` in your service implementation
and use that to fire multicast messages to consumers.

In order for the injection to work, you have to use the generated, specific subscription
publisher interface, and have to additionally decorate the injection with the
`@io.joynr.jeeintegration.api.SubscriptionPublisher` qualifier annotation.

Here is an example of what that looks like:

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

See also the
[Radio JEE provider bean](../examples/radio-jee/radio-jee-provider/src/main/java/io/joynr/examples/jee/RadioProviderBean.java)
for a working example.

#### Injecting a RawMessagingPreprocessor

if you need to inspect or modify incoming joynr messages, you can provide a producer of
@JoynrRawMessagingPreprocessor, whose process method will be called for each incoming MQTT
message.

For example:

	@Produces
	@JoynrRawMessagingPreprocessor
	RawMessagingPreprocessor rawMessagingPreprocessor() {
		return new RawMessagingPreprocessor() {
			@Override
			public String process(String rawMessage, @Nonnull Map<String, Serializable> context) {
				// do something with the message here, and add entries to the context
				return rawMessage;
			}
		};
	}

Inject `JoynrJeeMessageMetaInfo` to your EJB in order to retrieve the context for a received message.

The context can be accessed by calling `JoynrJeeMessageMetaInfo.getMessageContext()`:

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

### Calling services

In order to call services provided by other participants (e.g. applications
running in vehicles or providers of other JEE applications), inject the
`ServiceLocator` utility into the class which wants to call a service,
obtain a reference to a proxy of the business interface, and call the
relevant methods on it.

For example, if we wanted to call the `MyService` provider as implemented
in the above example:

	@Stateless
	public class MyConsumer {

		private final ServiceLocator serviceLocator;

        @Inject
        public MyConsumer(ServiceLocator serviceLocator) {
        	this.serviceLocator = serviceLocator;
        }

		public void performCall() {
            MyServiceSync myServiceProxy = serviceLocator.get(MyServiceSync.class, "my.service.domain");
            myServiceProxy.myMethod();
        }

    }

The time-to-live for joynr messages can be set through an additional parameter
in the ServiceLocator.get method.

It is also possible to target multiple providers with one proxy. You can achieve
this by either spcifying a set of domains during lookup, or a custom
`ArbitrationStrategyFunction` in the `DiscoveryQos`, or combine both approaches.
See the [Java Developer Guide](java.md) for details.

__IMPORTANT__: if you intend to have your logic make multiple calls to the same
provider, then you should locally cache the proxy instance returned by the
ServiceLocator, as the operation of creating a proxy is expensive.

## Clustering

The joynr JEE integration currently supports two forms of enabling clustering.
We recommend using the 'shared subscription' model, as this provides the most
functionality and best performance. It does, however, rely on a proprietary
feature from the [HiveMQ](http://www.hivemq.com) MQTT broker.

The other alternative is to use the so-called 'HTTP Bridge' mode. This requires
implementing a plugin for an MQTT broker which forwards incoming messages
to clustered applications which have previously registered themselves to an
endpoint registry application. The application will register a URL and a topic
which it is interested in. The plugin must then forward all incoming messages
on that topic to the given URL. See also
`io.joynr.jeeintegration.httpbridge.HttpBridgeEndpointRegistryClient`.
The joynr project only provides the hook-in points as part of the project. The
plugin for the MQTT broker and the endpoint registry must be implemented
separately.

### Shared Subscriptions

This solution offers load balancing on incoming messages via HiveMQ shared
subscriptions across all nodes in the cluster, and enables reply messages
for requests originating from inside the cluster to be routed directly
to the correct node.

Activate this mode with the
`MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS`
property.

### HTTP Bridge

The HTTP Bridge solution to clustering requires an HTTP load balancer to be
in front of the cluster (which will probably be the case anyway), as well as
an MQTT broker with a suitable plugin installed to perform the forwarding of
the MQTT messages to the cluster and also an endpoint registry application
which holds the mappings between application URLs and the topics for which
they are interested in.

A limitation of this solution is that replies for requests which originated
from a clustered application are not guaranteed to be received by the cluster
node which sent the request. A possible solution could be to provide extra
logic on the load balancer. However, without this you should only use
fire-and-forget semantics for outgoing messages.

## Overriding Jackson library used at runtime

### glassfish-web.xml

joynr ships with a newer version of Jackson than is used by Glassfish / Payara 4.1.
Generally, this shouldn't be a problem. If, however, you observe errors relating
to the JSON serialisation, try setting up your WAR to use the Jackson libraries
shipped with joynr.

In order to do this, you must provide the following content in the
`glassfish-web.xml` file (situated in the `WEB-INF` folder of your WAR):

	<?xml version="1.0" encoding="UTF-8"?>
	<!DOCTYPE glassfish-web-app PUBLIC "-//GlassFish.org//DTD GlassFish Application Server 3.1 Servlet 3.0//EN"
		"http://glassfish.org/dtds/glassfish-web-app_3_0-1.dtd">
	<glassfish-web-app>
	  <class-loader delegate="false" />
	</glassfish-web-app>

... here, the `<class-loader delegate="false" />` part is the relevant bit.

### Deactivate MoxyJson

Apparently Glassfish / Payara 4.1 ships with MoxyJson which may cause problems like
the following, when you try to parse JSON within your application.

    [2016-09-01T16:27:37.782+0200] [Payara 4.1] [SEVERE] []   [org.glassfish.jersey.message.internal.WriterInterceptorExecutor] [tid: _ThreadID=28 _ThreadName=http-listener-1(3)] [timeMillis: 1472740057782] [levelValue: 1000] [[MessageBodyWriter not found for media type=application/json, type=class xxx.JsonClass, genericType=class xxx.JsonClass.]]

To integrate your own dependency you need to disable the MoxyJson with the below code.

    @ApplicationPath("/")
    public class ApplicationConfig extends Application {

      @Override
      public Map<String, Object> getProperties() {
        final Map<String, Object> properties = new HashMap<String, Object>();
        properties.put("jersey.config.server.disableMoxyJson", true);

        return properties;
      }
    }

Then add your own dependencies, e.g. in this case only the following because the others
are referenced by the joynr lib itself. Be aware to check the version of the joynr
referenced libs.

    <dependency>
      <groupId>com.fasterxml.jackson.jaxrs</groupId>
      <artifactId>jackson-jaxrs-json-provider</artifactId>
      <version>2.6.2</version>
    </dependency>
    <dependency>
      <groupId>com.fasterxml.jackson.dataformat</groupId>
      <artifactId>jackson-dataformat-xml</artifactId>
      <version>2.6.2</version>
    </dependency>

Finally in case you're using JSON: Not setting a value to the @JsonProperty annotations
will cause a NoMessageBodyWriter found exception. To avoid that use the following on
relevant getters of your class.

    @JsonProperty("randomName")
    public String getRandomName(){
    ...
    }

Here are some references:

* [Moxy in general](https://blogs.oracle.com/theaquarium/entry/moxy_is_the_new_default)
* [Jersey configuration reference](https://jersey.java.net/documentation/latest/appendix-properties.html)
* [Jersey deployment reference](https://jersey.java.net/documentation/latest/deployment.html)
* [Payara blog re. JEE Microservices](http://blog.payara.fish/building-restful-java-ee-microservices-with-payara-embedded)

## Message Processors

By providing EJBs (or CDI Beans) which implement the `JoynrMessageProcessor` interface
you are able to hook into the message creation process. Each filter is called in turn
after the message has been created and is given the chance to perform any application
specific customisations, such as adding custom headers, or mutating any existing ones.

Be careful that your processor doesn't perform any changes which render the message
un-transmittable, such as changing the `to` or the `from` headers! Generally you
should only be, e.g., adding custom headers (via the `JoynrMessage#setCustomHeaders()`).

Here's an example of a message processor:

```
@Stateless
public class MyMessageProcessor implements JoynrMessageProcessor {
    public JoynrMessage processOutgoing(JoynrMessage joynrMessage) {
        Map<String, String> myCustomHeaders = new HashMap<>();
        myCustomHeaders.put("my-correlation-id", UUID.randomUuid().toString());
        joynrMessage.setCustomHeaders(myCustomHeaders);
        return joynrMessage;
    }

    public JoynrMessage processIncoming(JoynrMessage joynrMessage) {
        return joynrMessage;
    }
}
```

This would add the custom header `my-correlation-id` to the joynr message with a random
UUID as the value.

## Example Application

Under `examples/radio-jee` you can find an example application which is based on the
[Radio App example](./Tutorial.md). It uses the same `radio.fidl` file from the tutorial
but implements it as a JEE provider application and a separate JEE consumer application.

The project is sub-divided into one multi-module parent project and three subprojects:

```
 - radio-jee
   |- radio-jee-api
   |- radio-jee-provider
   |- radio-jee-consumer
```

In order to build the project you first have to have built the rest of joynr by executing
`mvn install` from the root of the directory where you checked out joynr to. Next change
to the `radio-jee` directory and call `mvn install`.

The following describes running the example on [Payara 4.1](http://www.payara.fish). First,
install the application server and you will also need to install an MQTT broker, e.g.
[Mosquitto](http://mosquitto.org).

You need to configure Payara with a ManagedScheduledExecutorService, see
[JEE Container configuration](#jee-container-configuration).

You also need a connection pool for the database which shall be used by the backend services
to persist data.
For this example, we'll create a database on the JavaDB (based on Derby) database which is
installed as part of Payara:
```
    bin/asadmin create-jdbc-connection-pool \
        --datasourceclassname org.apache.derby.jdbc.ClientDataSource \
        --restype javax.sql.XADataSource \
        --property portNumber=1527:password=APP:user=APP:serverName=localhost:databaseName=joynr-discovery-directory:connectionAttributes=\;create\\=true JoynrPool
```
Next, create a datasource resource pointing to that database connection. Here's an
example of what that would look like when using the connection pool created above:
```
`bin/asadmin create-jdbc-resource --connectionpoolid JoynrPool joynr/DiscoveryDirectoryDS`
`bin/asadmin create-jdbc-resource --connectionpoolid JoynrPool joynr/DomainAccessControllerDS`
```

After this, you can start the database:

`bin/asadmin start-database`

Start the MQTT broker, and make sure it's accepting traffic on `1883`.

Then start up the Payara server by changing to the Payara install directory and executing
`bin/asadmin start-domain`. Follow the instructions above for configuring the required
managed executor service and databse.

Next, fire up the joynr backend services:
- `bin/asadmin deploy <joynr home>/asadmin deploy <JOYNR_REPO>/examples/radio-jee/radio-jee-backend-services/target/discovery-jee.war`
- `bin/asadmin deploy <joynr home>/asadmin deploy <JOYNR_REPO>/examples/radio-jee/radio-jee-backend-services/target/accesscontrol-jee.war`

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
