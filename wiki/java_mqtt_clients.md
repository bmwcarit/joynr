# Java MQTT Client

joynr supports the HiveMQ MQTT Client with MQTT v5 protocol as transport layer.

## JEE applications

HiveMQ MQTT client is automatically used by the JEE Integration.

## Pure Java applications

For pure Java applications please include the following dependency in your application
(Maven dependency syntax shown):

```
	<dependency>
		<groupId>io.joynr.java.messaging.mqtt</groupId>
		<artifactId>hivemq-mqtt-client</artifactId>
		<version>${joynr.version}</version>
	</dependency>
```

## Configuration

For detailed info about MQTT related configuration settings please refer to
[Java ConfigurationReference](./JavaSettings.md).

The following is a list of supported settings:

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
