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

import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.messaging.IMessagingSkeletonFactory;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessageInProgressObserver;
import io.joynr.messaging.mqtt.MqttMessagingSkeletonFactory;
import io.joynr.messaging.mqtt.MqttTopicPrefixProvider;
import io.joynr.messaging.routing.MessageProcessedHandler;
import io.joynr.messaging.routing.MessageRouter;

@RunWith(MockitoJUnitRunner.class)
public class JeeMqttMessagingSkeletonProviderTest {

    private JeeMqttMessagingSkeletonProvider subject;

    @Mock
    private MessageRouter mockMessageRouter;
    @Mock
    private MessageProcessedHandler mockMessageProcessedHandler;

    @Mock
    private MqttClientFactory mockMqttClientFactory;

    @Mock
    private MqttTopicPrefixProvider mockMqttTopicPrefixProvider;

    @Mock
    MqttMessageInProgressObserver mockMqttMessageInProgressObserver;

    private void createProvider(boolean enableSharedSubscriptions) {
        subject = new JeeMqttMessagingSkeletonProvider(new String[0],
                                                       enableSharedSubscriptions,
                                                       false,
                                                       mockMessageRouter,
                                                       mockMessageProcessedHandler,
                                                       mockMqttClientFactory,
                                                       "",
                                                       mockMqttTopicPrefixProvider,
                                                       null,
                                                       null,
                                                       null,
                                                       "",
                                                       mockMqttMessageInProgressObserver);
    }

    @Test
    public void createsJeeSpecificFactoryIfSharedSubscriptionsEnabled() {
        createProvider(true);
        IMessagingSkeletonFactory result = subject.get();
        assertTrue(JeeSharedSubscriptionsMqttMessagingSkeletonFactory.class.isInstance(result));
    }

    @Test
    public void createsDefaultFactoryIfSharedSubscriptionsDisabled() {
        createProvider(false);
        IMessagingSkeletonFactory result = subject.get();
        assertTrue(MqttMessagingSkeletonFactory.class.isInstance(result));
    }
}
