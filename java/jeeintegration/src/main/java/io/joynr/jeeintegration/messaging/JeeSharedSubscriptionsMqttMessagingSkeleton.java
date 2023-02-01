/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

import java.util.Set;

import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessageInProgressObserver;
import io.joynr.messaging.mqtt.MqttTopicPrefixProvider;
import io.joynr.messaging.mqtt.SharedSubscriptionsMqttMessagingSkeleton;
import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;

/**
 * Almost the same as {@link SharedSubscriptionsMqttMessagingSkeleton} but separates the subscription to the replyTo
 * topic from the subscription to the shared topic.
 * <p>
 * It subscribes automatically to the replyTo topic when {@link #subscribe()} is called.<br>
 * The subscription to the global topic has to be triggered manually if required.
 */
public class JeeSharedSubscriptionsMqttMessagingSkeleton extends SharedSubscriptionsMqttMessagingSkeleton {

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public JeeSharedSubscriptionsMqttMessagingSkeleton(String ownTopic,
                                                       String replyToTopic,
                                                       MessageRouter messageRouter,
                                                       MessageProcessedHandler messageProcessedHandler,
                                                       MqttClientFactory mqttClientFactory,
                                                       String channelId,
                                                       MqttTopicPrefixProvider mqttTopicPrefixProvider,
                                                       RawMessagingPreprocessor rawMessagingPreprocessor,
                                                       Set<JoynrMessageProcessor> messageProcessors,
                                                       String ownGbid,
                                                       RoutingTable routingTable,
                                                       boolean separateReplyMqttClient,
                                                       String backendUid,
                                                       MqttMessageInProgressObserver mqttMessageInProgressObserver) {
        super(ownTopic,
              replyToTopic,
              messageRouter,
              messageProcessedHandler,
              mqttClientFactory,
              channelId,
              mqttTopicPrefixProvider,
              rawMessagingPreprocessor,
              messageProcessors,
              ownGbid,
              routingTable,
              separateReplyMqttClient,
              backendUid,
              mqttMessageInProgressObserver);
    }

    @Override
    protected void subscribe() {
        super.subscribeToReplyTopic();
    }

    public void subscribeToSharedTopic() {
        super.subscribeToSharedTopic();
    }

}
