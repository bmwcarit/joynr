/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
package io.joynr.messaging.mqtt;

import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_BROKER_URIS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_GBIDS;

import java.util.Arrays;
import java.util.HashMap;

import javax.inject.Named;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.exceptions.JoynrIllegalStateException;

public class MqttMultipleBackendPropertyProvider {

    private static final Logger logger = LoggerFactory.getLogger(MqttMultipleBackendPropertyProvider.class);

    private String[] brokerUriArray;
    private final HashMap<String, String> gbidToBrokerUriMap;
    private final HashMap<String, Integer> gbidToKeepAliveTimerSecMap;
    private final HashMap<String, Integer> gbidToConnectionTimeoutSecMap;

    @Inject
    public MqttMultipleBackendPropertyProvider(@Named(PROPERTY_MQTT_BROKER_URIS) String brokerUris,
                                               @Named(PROPERTY_GBIDS) String gbids,
                                               @Named(PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC) String keepAliveTimersSec,
                                               @Named(PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC) String connectionTimeoutsSec) {
        String[] gbidArray = Arrays.stream(gbids.split(",")).map(a -> a.trim()).toArray(String[]::new);

        brokerUriArray = Arrays.stream(brokerUris.split(",")).map(a -> a.trim()).toArray(String[]::new);
        if (brokerUriArray.length != gbidArray.length) {
            logger.error("The amount of defined BrokerUris is not equal to the amount of defined GBIDs!");
            throw new JoynrIllegalStateException("The amount of defined BrokerUris is not equal to the amount of defined GBIDs!");
        }
        // Validation of the brokerUris happens in the MqttClient.
        if (Arrays.stream(brokerUriArray).anyMatch(brokerUri -> brokerUri.isEmpty())) {
            logger.error("BrokerUri must not be empty: {}!", brokerUris);
            throw new JoynrIllegalStateException("BrokerUri must not be empty: " + brokerUris + "!");
        }

        Integer[] keepAliveTimerSecArray;
        try {
            keepAliveTimerSecArray = stringArrayToIntArray(keepAliveTimersSec.replaceAll("\\s", "").split(","));
        } catch (NumberFormatException e) {
            logger.error("Invalid entry in: {}!", PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC);
            throw new JoynrIllegalStateException("Invalid entry in " + PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC + "!");
        }
        if (keepAliveTimerSecArray.length != gbidArray.length) {
            logger.error("The amount of defined MQTT keep alive times is not equal to the amount of defined GBIDs!");
            throw new JoynrIllegalStateException("The amount of defined MQTT keep alive times is not equal to the amount of defined GBIDs!");
        }

        Integer[] connectionTimeoutTimerSecArray;
        try {
            connectionTimeoutTimerSecArray = stringArrayToIntArray(connectionTimeoutsSec.replaceAll("\\s", "")
                                                                                        .split(","));
        } catch (NumberFormatException e) {
            logger.error("Invalid entry in: {}!", PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC);
            throw new JoynrIllegalStateException("Invalid entry in " + PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC + "!");
        }
        if (connectionTimeoutTimerSecArray.length != gbidArray.length) {
            logger.error("The amount of defined MQTT connection timeout times is not equal to the amount of defined GBIDs!");
            throw new JoynrIllegalStateException("The amount of defined MQTT connection timeout times is not equal to the amount of defined GBIDs!");
        }

        gbidToBrokerUriMap = arraysToHashMap(gbidArray, brokerUriArray);
        gbidToConnectionTimeoutSecMap = arraysToHashMap(gbidArray, connectionTimeoutTimerSecArray);
        gbidToKeepAliveTimerSecMap = arraysToHashMap(gbidArray, keepAliveTimerSecArray);
    }

    private static Integer[] stringArrayToIntArray(String[] stringArray) {
        Integer[] intArray = new Integer[stringArray.length];
        for (int i = 0; i < intArray.length; i++) {
            intArray[i] = Integer.parseInt(stringArray[i]);
        }
        return intArray;
    }

    private static <T> HashMap<String, T> arraysToHashMap(String[] keyArray, T[] valueArray) {
        HashMap<String, T> hashMap = new HashMap<>();
        for (int i = 0; i < keyArray.length; i++) {
            hashMap.put(keyArray[i], valueArray[i]);
        }
        return hashMap;
    }

    public String[] provideBrokerUris() {
        return brokerUriArray.clone();
    }

    public HashMap<String, String> provideGbidToBrokerUriMap() {
        return gbidToBrokerUriMap;
    }

    public final HashMap<String, Integer> provideGbidToKeepAliveTimerSecMap() {
        return gbidToKeepAliveTimerSecMap;
    }

    public final HashMap<String, Integer> provideGbidToConnectionTimeoutSecMap() {
        return gbidToConnectionTimeoutSecMap;
    }

}
