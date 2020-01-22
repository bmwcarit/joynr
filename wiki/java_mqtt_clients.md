# Java MQTT Clients

If you want to use MQTT as joynr's transport layer, then
you have a choice of using either the Paho MQTT client or the HiveMQ MQTT Client.

If you use the JEE Integration, then this will automatically use the HiveMQ MQTT Client.

## Choosing the client in Java

In order to select which MQTT Client integration you want to use, simply include one of the
following dependencies in your application (Maven dependency syntax shown):

### HiveMQ MQTT Client
uses MQTT 5

	<dependency>
		<groupId>io.joynr.java.messaging.mqtt</groupId>
		<artifactId>hivemq-mqtt-client</artifactId>
		<version>${joynr.version}</version>
	</dependency>

### Paho MQTT Client
uses MQTT v3

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
