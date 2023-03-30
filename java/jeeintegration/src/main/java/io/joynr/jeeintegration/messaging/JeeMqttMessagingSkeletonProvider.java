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

import static io.joynr.messaging.MessagingPropertyKeys.CHANNELID;
import static io.joynr.messaging.MessagingPropertyKeys.GBID_ARRAY;
import static io.joynr.messaging.MessagingPropertyKeys.PROPERTY_BACKEND_UID;
import static io.joynr.messaging.MessagingPropertyKeys.PROPERTY_KEY_SEPARATE_REPLY_RECEIVER;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS;

import java.util.Set;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.IMessagingSkeletonFactory;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessageInProgressObserver;
import io.joynr.messaging.mqtt.MqttMessagingSkeletonProvider;
import io.joynr.messaging.mqtt.MqttTopicPrefixProvider;
import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;

/**
 * Like {@link MqttMessagingSkeletonProvider}.
 * <p>
 * If shared subscriptions are enabled, it returns an instance of
 * {@link JeeSharedSubscriptionsMqttMessagingSkeletonFactory}
 * instead of {@link io.joynr.messaging.mqtt.SharedSubscriptionsMqttMessagingSkeleton}.
 * <p>
 * Otherwise it returns an instance of the normal {@link io.joynr.messaging.mqtt.MqttMessagingSkeleton} like
 * {@link MqttMessagingSkeletonProvider}.
 */
public class JeeMqttMessagingSkeletonProvider extends MqttMessagingSkeletonProvider {

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public JeeMqttMessagingSkeletonProvider(@Named(GBID_ARRAY) String[] gbids,
                                            @Named(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS) boolean enableSharedSubscriptions,
                                            @Named(PROPERTY_KEY_SEPARATE_REPLY_RECEIVER) boolean separateMqttReplyReceiver,
                                            MessageRouter messageRouter,
                                            MessageProcessedHandler messageProcessedHandler,
                                            MqttClientFactory mqttClientFactory,
                                            @Named(CHANNELID) String channelId,
                                            MqttTopicPrefixProvider mqttTopicPrefixProvider,
                                            RawMessagingPreprocessor rawMessagingPreprocessor,
                                            Set<JoynrMessageProcessor> messageProcessors,
                                            RoutingTable routingTable,
                                            @Named(PROPERTY_BACKEND_UID) String backendUid,
                                            MqttMessageInProgressObserver mqttMessageInProgressObserver) {
        super(gbids,
              enableSharedSubscriptions,
              separateMqttReplyReceiver,
              messageRouter,
              messageProcessedHandler,
              mqttClientFactory,
              channelId,
              mqttTopicPrefixProvider,
              rawMessagingPreprocessor,
              messageProcessors,
              routingTable,
              backendUid,
              mqttMessageInProgressObserver);
    }

    @Override
    protected IMessagingSkeletonFactory createSharedSubscriptionsFactory() {
        return new JeeSharedSubscriptionsMqttMessagingSkeletonFactory(gbids,
                                                                      ownAddress,
                                                                      replyToAddress,
                                                                      messageRouter,
                                                                      messageProcessedHandler,
                                                                      mqttClientFactory,
                                                                      channelId,
                                                                      mqttTopicPrefixProvider,
                                                                      rawMessagingPreprocessor,
                                                                      messageProcessors,
                                                                      routingTable,
                                                                      separateMqttReplyReceiver,
                                                                      backendUid,
                                                                      mqttMessageInProgressObserver);
    }
}
