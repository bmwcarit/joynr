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
package io.joynr.messaging.mqtt;

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
import com.google.inject.Provider;
import com.google.inject.name.Named;

import io.joynr.messaging.IMessagingSkeletonFactory;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * A provider for {@link IMessagingSkeletonFactory} instances which checks with the property configured under
 * {@link io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS} If shared subscriptions are
 * enabled, it returns an instance of {@link SharedSubscriptionsMqttMessagingSkeleton}. Otherwise (default behaviour),
 * it returns an instance of the normal {@link MqttMessagingSkeleton}.
 */
public class MqttMessagingSkeletonProvider implements Provider<IMessagingSkeletonFactory> {

    private final static Logger logger = LoggerFactory.getLogger(MqttMessagingSkeletonProvider.class);

    protected MqttClientFactory mqttClientFactory;
    private boolean sharedSubscriptionsEnabled;
    private MqttAddress ownAddress;
    private int maxIncomingMqttRequests;
    private boolean backpressureEnabled;
    private int backpressureIncomingMqttRequestsUpperThreshold;
    private int backpressureIncomingMqttRequestsLowerThreshold;
    private MqttAddress replyToAddress;
    private MessageRouter messageRouter;
    private String channelId;
    private MqttTopicPrefixProvider mqttTopicPrefixProvider;
    private RawMessagingPreprocessor rawMessagingPreprocessor;
    private Set<JoynrMessageProcessor> messageProcessors;
    private JoynrStatusMetricsReceiver joynrStatusMetricsReceiver;
    protected final String[] gbids;
    protected final RoutingTable routingTable;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public MqttMessagingSkeletonProvider(@Named(GBID_ARRAY) String[] gbids,
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
                                         JoynrStatusMetricsReceiver joynrStatusMetricsReceiver,
                                         RoutingTable routingTable) {
        sharedSubscriptionsEnabled = enableSharedSubscriptions;
        this.rawMessagingPreprocessor = rawMessagingPreprocessor;
        this.messageProcessors = messageProcessors;
        this.ownAddress = ownAddress;
        this.maxIncomingMqttRequests = maxIncomingMqttRequests;
        this.backpressureEnabled = backpressureEnabled;
        this.backpressureIncomingMqttRequestsUpperThreshold = backpressureIncomingMqttRequestsUpperThreshold;
        this.backpressureIncomingMqttRequestsLowerThreshold = backpressureIncomingMqttRequestsLowerThreshold;
        this.replyToAddress = replyToAddress;
        this.messageRouter = messageRouter;
        this.mqttClientFactory = mqttClientFactory;
        this.channelId = channelId;
        this.mqttTopicPrefixProvider = mqttTopicPrefixProvider;
        this.joynrStatusMetricsReceiver = joynrStatusMetricsReceiver;
        this.gbids = gbids.clone();
        this.routingTable = routingTable;
        logger.debug("Created with sharedSubscriptionsEnabled: {} ownAddress: {} channelId: {}",
                     sharedSubscriptionsEnabled,
                     ownAddress,
                     channelId);
    }

    @Override
    public IMessagingSkeletonFactory get() {
        if (sharedSubscriptionsEnabled) {
            return new SharedSubscriptionsMqttMessagingSkeletonFactory(gbids,
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
                                                                       joynrStatusMetricsReceiver,
                                                                       routingTable);
        }
        return new MqttMessagingSkeletonFactory(gbids,
                                                ownAddress,
                                                maxIncomingMqttRequests,
                                                messageRouter,
                                                mqttClientFactory,
                                                mqttTopicPrefixProvider,
                                                rawMessagingPreprocessor,
                                                messageProcessors,
                                                joynrStatusMetricsReceiver,
                                                routingTable);
    }

}
