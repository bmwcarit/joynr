/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
package io.joynr.jeeintegration.messaging;

import static io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys.JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY;
import static io.joynr.messaging.MessagingPropertyKeys.CHANNELID;
import static io.joynr.messaging.MessagingPropertyKeys.GBID_ARRAY;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_MQTT_REPLY_TO_ADDRESS;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_ENABLED;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_UPPER_THRESHOLD;
import static io.joynr.messaging.mqtt.settings.LimitAndBackpressureSettings.PROPERTY_MAX_INCOMING_MQTT_REQUESTS;

import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.joynr.messaging.IMessagingSkeletonFactory;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessagingSkeletonProvider;
import io.joynr.messaging.mqtt.MqttTopicPrefixProvider;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Like {@link MqttMessagingSkeletonProvider}. It checks the property configured under
 * {@link io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys#JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY} to see if
 * messages should be received via HTTP instead of MQTT. In this case, it returns an instance of
 * {@link NoOpMqttMessagingSkeleton}. Otherwise it behaves like {@link MqttMessagingSkeletonProvider}.
 */
public class JeeMqttMessagingSkeletonProvider extends MqttMessagingSkeletonProvider {

    private final static Logger logger = LoggerFactory.getLogger(MqttMessagingSkeletonProvider.class);

    private boolean httpBridgeEnabled;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public JeeMqttMessagingSkeletonProvider(@Named(GBID_ARRAY) String[] gbids,
                                            @Named(JEE_ENABLE_HTTP_BRIDGE_CONFIGURATION_KEY) boolean enableHttpBridge,
                                            @Named(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS) boolean enableSharedSubscriptions,
                                            @Named(PROPERTY_MQTT_GLOBAL_ADDRESS) MqttAddress ownAddress,
                                            @Named(PROPERTY_MAX_INCOMING_MQTT_REQUESTS) int maxIncomingMqttRequests,
                                            @Named(PROPERTY_BACKPRESSURE_ENABLED) boolean backpressureEnabled,
                                            @Named(PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_UPPER_THRESHOLD) int backpressureIncomingMqttRequestsUpperThreshold,
                                            @Named(PROPERTY_BACKPRESSURE_INCOMING_MQTT_REQUESTS_LOWER_THRESHOLD) int backpressureIncomingMqttRequestsLowerThreshold,
                                            @Named(PROPERTY_MQTT_REPLY_TO_ADDRESS) MqttAddress replyToAddress,
                                            MessageRouter messageRouter,
                                            MqttClientFactory mqttClientFactory,
                                            @Named(CHANNELID) String channelId,
                                            MqttTopicPrefixProvider mqttTopicPrefixProvider,
                                            RawMessagingPreprocessor rawMessagingPreprocessor,
                                            Set<JoynrMessageProcessor> messageProcessors,
                                            JoynrStatusMetricsReceiver jeeJoynrStatusMetrics,
                                            RoutingTable routingTable) {
        super(gbids,
              enableSharedSubscriptions,
              ownAddress,
              maxIncomingMqttRequests,
              backpressureEnabled,
              backpressureIncomingMqttRequestsUpperThreshold,
              backpressureIncomingMqttRequestsLowerThreshold,
              replyToAddress,
              messageRouter,
              mqttClientFactory,
              channelId,
              mqttTopicPrefixProvider,
              rawMessagingPreprocessor,
              messageProcessors,
              jeeJoynrStatusMetrics,
              routingTable);
        httpBridgeEnabled = enableHttpBridge;
        logger.debug("Created with httpBridgeEnabled: {} ownAddress: {} channelId: {}",
                     httpBridgeEnabled,
                     ownAddress,
                     channelId);
    }

    @Override
    public IMessagingSkeletonFactory get() {
        if (httpBridgeEnabled) {
            return new NoOpMqttMessagingSkeletonFactory(mqttClientFactory, gbids, routingTable);
        } else {
            return super.get();
        }
    }
}
