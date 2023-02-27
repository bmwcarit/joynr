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
import static io.joynr.messaging.MessagingPropertyKeys.PROPERTY_BACKEND_UID;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS;
import static io.joynr.messaging.MessagingPropertyKeys.PROPERTY_KEY_SEPARATE_REPLY_RECEIVER;

import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.IMessagingSkeletonFactory;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * A provider for {@link IMessagingSkeletonFactory} instances which checks with the property configured under
 * {@link io.joynr.messaging.mqtt.MqttModule#PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS} If shared subscriptions are
 * enabled, it returns an instance of {@link SharedSubscriptionsMqttMessagingSkeleton}. Otherwise (default behaviour),
 * it returns an instance of the normal {@link MqttMessagingSkeleton}.
 */
public class MqttMessagingSkeletonProvider implements Provider<IMessagingSkeletonFactory> {

    private final static Logger logger = LoggerFactory.getLogger(MqttMessagingSkeletonProvider.class);

    protected MqttClientFactory mqttClientFactory;
    protected boolean sharedSubscriptionsEnabled;
    @Inject(optional = true)
    @Named(MessagingPropertyKeys.GLOBAL_ADDRESS)
    private Address ownAddressInjected = new Address();
    @Inject(optional = true)
    @Named(MessagingPropertyKeys.REPLY_TO_ADDRESS)
    private Address replyToAddressInjected = new Address();
    protected MqttAddress ownAddress;
    protected MqttAddress replyToAddress;
    protected MessageRouter messageRouter;
    protected MessageProcessedHandler messageProcessedHandler;
    protected String channelId;
    protected MqttTopicPrefixProvider mqttTopicPrefixProvider;
    protected RawMessagingPreprocessor rawMessagingPreprocessor;
    protected Set<JoynrMessageProcessor> messageProcessors;
    protected final String[] gbids;
    protected final RoutingTable routingTable;
    protected final boolean separateMqttReplyReceiver;
    protected final MqttMessageInProgressObserver mqttMessageInProgressObserver;

    protected final String backendUid;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public MqttMessagingSkeletonProvider(@Named(GBID_ARRAY) String[] gbids,
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

        sharedSubscriptionsEnabled = enableSharedSubscriptions;
        this.rawMessagingPreprocessor = rawMessagingPreprocessor;
        this.messageProcessors = messageProcessors;
        this.messageRouter = messageRouter;
        this.messageProcessedHandler = messageProcessedHandler;
        this.mqttClientFactory = mqttClientFactory;
        this.channelId = channelId;
        this.mqttTopicPrefixProvider = mqttTopicPrefixProvider;
        this.gbids = gbids.clone();
        this.routingTable = routingTable;
        this.separateMqttReplyReceiver = separateMqttReplyReceiver;
        this.backendUid = backendUid;
        this.mqttMessageInProgressObserver = mqttMessageInProgressObserver;
    }

    @Inject
    public void init() {
        if (ownAddressInjected instanceof MqttAddress) {
            this.ownAddress = (MqttAddress) ownAddressInjected;
        } else {
            throw new JoynrIllegalStateException("Global address is not a MqttAddress!");
        }
        if (replyToAddressInjected instanceof MqttAddress) {
            this.replyToAddress = (MqttAddress) replyToAddressInjected;
        } else {
            throw new JoynrIllegalStateException("Reply-to address is not a MqttAddress!");
        }
        logger.debug("Created with sharedSubscriptionsEnabled: {} ownAddress: {} channelId: {} backendUid: {}",
                     sharedSubscriptionsEnabled,
                     ownAddress,
                     channelId,
                     backendUid);
    }

    @Override
    public IMessagingSkeletonFactory get() {
        if (sharedSubscriptionsEnabled) {
            return createSharedSubscriptionsFactory();
        }
        return createFactory();
    }

    protected IMessagingSkeletonFactory createSharedSubscriptionsFactory() {
        return new SharedSubscriptionsMqttMessagingSkeletonFactory(gbids,
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

    protected IMessagingSkeletonFactory createFactory() {
        return new MqttMessagingSkeletonFactory(gbids,
                                                ownAddress,
                                                messageRouter,
                                                messageProcessedHandler,
                                                mqttClientFactory,
                                                mqttTopicPrefixProvider,
                                                rawMessagingPreprocessor,
                                                messageProcessors,
                                                routingTable,
                                                backendUid,
                                                mqttMessageInProgressObserver);
    }

}
