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

* MqttModule.PROPERTY_KEY_MQTT_RECONNECT_SLEEP_MS
* MqttModule.PROPERTY_MQTT_BROKER_URIS
* MqttModule.MQTT_BROKER_URI_ARRAY
* MqttModule.MQTT_GBID_TO_BROKERURI_MAP
* MqttModule.PROPERTY_KEY_MQTT_CLIENT_ID_PREFIX
* MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC
* MqttModule.MQTT_TO_KEEP_ALIVE_TIMER_SEC_MAP
* MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC
* MqttModule.MQTT_GBID_TO_CONNECTION_TIMEOUT_SEC_MAP
* MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PATH
* MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PATH
* MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_TYPE
* MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_TYPE
* MqttModule.PROPERTY_KEY_MQTT_KEYSTORE_PWD
* MqttModule.PROPERTY_KEY_MQTT_TRUSTSTORE_PWD
* MqttModule.PROPERTY_KEY_MQTT_CIPHERSUITES
* MqttModule.MQTT_CIPHERSUITE_LIST
* MqttModule.PROPERTY_KEY_MQTT_SEPARATE_CONNECTIONS
* MqttModule.PROPERTY_KEY_MQTT_USERNAME
* MqttModule.PROPERTY_KEY_MQTT_PASSWORD
* MqttModule.PROPERTY_KEY_MQTT_DISABLE_HOSTNAME_VERIFICATION
* MqttModule.PROPERTY_KEY_MQTT_RECEIVE_MAXIMUM
* MqttModule.PROPERTY_KEY_MQTT_CONNECT_ON_START
* MqttModule.PROPERTY_KEY_MQTT_RETAIN
* MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS
* MqttModule.PROPERTY_MQTT_CLEAN_SESSION
