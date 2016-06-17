# joynr JEE Integration

The `io.joynr.jeeintegration` project provides an integration layer for
joynr in JEE Applications.
The features supported are:

* Expose session beans as joynr providers via the `@ServiceProvider` annotation
* Inject a `ServiceLocator` in order to obtain consumer proxies for calling
other services
* Internally uses EE container managed thread pools
* Uses the EE container's JAX RS to receive joynr messages

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

Properties you have to set for the integration to work are:

* `MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT` - this property needs
to be set to the context root of your deployed application, with `/messaging`
added to the end. E.g.: `/myapp/root/messaging`.
* `MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH` - this property needs to
be set to the URL under which the application server you are running on can be
reached, e.g. `https://myapp.mycompany.net`.
* `MessagingpropertyKeys.CHANNELID` - this property should be set to the
application's unique DNS entry, e.g. `myapp.mycompany.net`. This is important,
so that all nodes of the cluster are identified by the same channel ID.
* `JeeIntegrationPropertyKeys.JEE_INTEGRATION_ENDPOINTREGISTRY_URI` -
this property needs to
point to the endpoint registration service's URL with which the
JEE Integration will register itself for its channel's topic.
E.g. `http://endpointregistry.mycompany.net:8080`.
* `joynr.messaging.mqtt.brokeruri` - use this to configure the URL for
connecting to the MQTT broker being used for communication.
E.g. `tcp://mqtt.mycompany.net:1883`.

You are principally free to provide any other valid joynr properties via these
configuration methods. See the [official joynr documentation](http://joynr.io)
for details.

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
		joynrProperties.setProperty("joynr.jeeintegration.endpointregistry.uri",
			"http://endpointregistry:8080/registry/endpoint");
		joynrProperties.setProperty("joynr.messaging.mqtt.brokeruri",
			"tcp://mqttbroker.com:1883");
		joynrProperties.setProperty(MessagingPropertyKeys.DISCOVERYDIRECTORYURL,
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
`asadmin create-managed-scheduled-executor-service --corepoolsize=10 concurrent/joynrMessagingScheduledExecutor`

Note the `--corepoolsize=10` option. The default will only create one thread,
which can lead to blocking.

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

## Overriding Jackson library used at runtime

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

