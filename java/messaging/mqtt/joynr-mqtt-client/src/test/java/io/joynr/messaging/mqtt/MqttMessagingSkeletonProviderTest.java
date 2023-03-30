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
package io.joynr.messaging.mqtt;

import static io.joynr.messaging.MessagingPropertyKeys.GBID_ARRAY;
import static io.joynr.messaging.MessagingPropertyKeys.GLOBAL_ADDRESS;
import static io.joynr.messaging.MessagingPropertyKeys.PROPERTY_KEY_SEPARATE_REPLY_RECEIVER;
import static io.joynr.messaging.MessagingPropertyKeys.REPLY_TO_ADDRESS;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS;
import static org.junit.Assert.assertTrue;

import java.util.HashSet;
import java.util.Set;

import org.junit.runner.RunWith;
import org.junit.Test;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.Mock;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Module;
import com.google.inject.ProvisionException;
import com.google.inject.TypeLiteral;
import com.google.inject.name.Names;

import io.joynr.messaging.IMessagingSkeletonFactory;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

@RunWith(MockitoJUnitRunner.class)
public class MqttMessagingSkeletonProviderTest {

    private MqttMessagingSkeletonProvider subject;

    @Mock
    private MessageRouter mockMessageRouter;
    @Mock
    private MessageProcessedHandler mockMessageProcessedHandler;

    @Mock
    private MqttClientFactory mockMqttClientFactory;

    @Mock
    private MqttTopicPrefixProvider mockMqttTopicPrefixProvider;

    @Mock
    protected MqttMessageInProgressObserver mqttMessageInProgressObserver;

    @Mock
    private RawMessagingPreprocessor rawMessagingPreprocessor;

    @Mock
    private RoutingTable routingTable;

    private void createProvider(boolean enableSharedSubscriptions, Address globalAddress, Address replyToAddress) {
        Module injectionModule = new AbstractModule() {
            @Override
            protected void configure() {
                bind(String[].class).annotatedWith(Names.named(GBID_ARRAY)).toInstance(new String[0]);
                bind(Boolean.class).annotatedWith(Names.named(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS))
                                   .toInstance(enableSharedSubscriptions);
                bind(Boolean.class).annotatedWith(Names.named(PROPERTY_KEY_SEPARATE_REPLY_RECEIVER)).toInstance(false);
                bind(MessageRouter.class).toInstance(mockMessageRouter);
                bind(MessageProcessedHandler.class).toInstance(mockMessageProcessedHandler);
                bind(MqttClientFactory.class).toInstance(mockMqttClientFactory);
                bind(String.class).annotatedWith(Names.named(MessagingPropertyKeys.CHANNELID)).toInstance("");
                bind(MqttTopicPrefixProvider.class).toInstance(mockMqttTopicPrefixProvider);
                bind(String.class).annotatedWith(Names.named(MessagingPropertyKeys.PROPERTY_BACKEND_UID))
                                  .toInstance("");
                bind(MqttMessageInProgressObserver.class).toInstance(mqttMessageInProgressObserver);
                bind(new TypeLiteral<Set<JoynrMessageProcessor>>() {
                }).toInstance(new HashSet<>());
                bind(RawMessagingPreprocessor.class).toInstance(rawMessagingPreprocessor);
                bind(RoutingTable.class).toInstance(routingTable);
                bind(Address.class).annotatedWith(Names.named(GLOBAL_ADDRESS)).toInstance(globalAddress);
                bind(Address.class).annotatedWith(Names.named(REPLY_TO_ADDRESS)).toInstance(replyToAddress);
            }
        };
        subject = Guice.createInjector(injectionModule).getInstance(MqttMessagingSkeletonProvider.class);
    }

    @Test
    public void createsJeeSpecificFactoryIfSharedSubscriptionsEnabled() {
        createProvider(true, new MqttAddress("global", "address"), new MqttAddress("replyTo", "address"));
        IMessagingSkeletonFactory result = subject.get();
        assertTrue(SharedSubscriptionsMqttMessagingSkeletonFactory.class.isInstance(result));
    }

    @Test
    public void createsDefaultFactoryIfSharedSubscriptionsDisabled() {
        createProvider(false, new MqttAddress("global", "address"), new MqttAddress("replyTo", "address"));
        IMessagingSkeletonFactory result = subject.get();
        assertTrue(MqttMessagingSkeletonFactory.class.isInstance(result));
    }

    @Test(expected = ProvisionException.class)
    public void initializeProviderWithNonMqttGlobalAddress() {
        createProvider(false, new Address(), new MqttAddress("replyTo", "address"));
    }

    @Test(expected = ProvisionException.class)
    public void initializeProviderWithNonMqttReplyToAddress() {
        createProvider(false, new MqttAddress("global", "address"), new Address());
    }
}
