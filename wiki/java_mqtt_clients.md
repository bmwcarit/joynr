# Java MQTT Clients

If you want to use MQTT as joynr's transport layer, then
you have a choice of using either the Paho MQTT client or the HiveMQ MQTT Client.

If you use the JEE Integration, then this will automatically use the HiveMQ MQTT Client.

## Choosing the client in Java

In order to select which MQTT Client integration you want to use, simply include one of the
following dependencies in your application (Maven dependency syntax shown):

### HiveMQ MQTT Client

	<dependency>
		<groupId>io.joynr.java.messaging.mqtt</groupId>
		<artifactId>hivemq-mqtt-client</artifactId>
		<version>${joynr.version}</version>
	</dependency>

### Paho MQTT Client

	<dependency>
		<groupId>io.joynr.java.messaging.mqtt</groupId>
		<artifactId>paho-mqtt-client</artifactId>
		<version>${joynr.version}</version>
	</dependency>

## Configuration

Both MQTT Integrations are configured with the same set of properties, as documented in the
[Java ConfigurationReference](./JavaSettings.md).

However, the HiveMQ MQTT Client integration only supports a subset of the configuration settings
(because some are not relevant to the client). The following is a list of the settings supported by
the integration:

* MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH
* MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH
* MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_TYPE
* MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_TYPE
* MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD
* MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD
* MqttModule.PROPERTY_KEY_MQTT_USERNAME
* MqttModule.PROPERTY_KEY_MQTT_PASSWORD
* MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS
* MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS
* MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMER_SEC
* MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUT_SEC
* MqttModule.PROPERTY_MQTT_CLEAN_SESSION

## JEE 7

If you're running in a JEE 7 container, such as Payara 5.x or Glassfish 5.x, then you have to
exclude CDI scanning of the HiveMQ MQTT Client library (a future version of the library will do this
itself by including a beans.xml de-activating CDI discovery, but as of joynr 1.8.3 this has not been
released yet).

### Example for Payara 5 / Glassfish 5

In your WAR, include a `glassfish-web.xml` under `WEB-INF` with (at least) the following content:

	<?xml version="1.0" encoding="UTF-8"?>
	<!DOCTYPE glassfish-web-app PUBLIC "-//GlassFish.org//DTD GlassFish Application Server 3.1 Servlet 3.0//EN"
		"http://glassfish.org/dtds/glassfish-web-app_3_0-1.dtd">
	<glassfish-web-app>
	  <scanning-exclude>hivemq-mqtt-client*</scanning-exclude>
	</glassfish-web-app>

