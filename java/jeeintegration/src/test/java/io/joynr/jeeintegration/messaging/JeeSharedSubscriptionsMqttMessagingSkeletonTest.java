/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.startsWith;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.HashSet;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.mqtt.AbstractSharedSubscriptionsMqttMessagingSkeletonTest;

/**
 * Unit tests for {@link JeeSharedSubscriptionsMqttMessagingSkeleton}.
 */
@RunWith(MockitoJUnitRunner.class)
public class JeeSharedSubscriptionsMqttMessagingSkeletonTest
        extends AbstractSharedSubscriptionsMqttMessagingSkeletonTest {

    private JeeSharedSubscriptionsMqttMessagingSkeleton subject;

    @Override
    protected void initAndSubscribe() {
        subject.init();
        // JeeSharedSubscriptionsMqttMessagingSkeleton does not automatically subscribe to shared topic in init()
        subject.subscribeToSharedTopic();
        verify(mqttClient).subscribe(startsWith("$share/"));
    }

    @Override
    protected void createSkeleton(String channelId) {
        subject = new JeeSharedSubscriptionsMqttMessagingSkeleton(ownTopic,
                                                                  replyToTopic,
                                                                  messageRouter,
                                                                  messageProcessedHandler,
                                                                  mqttClientFactory,
                                                                  channelId,
                                                                  mqttTopicPrefixProvider,
                                                                  new NoOpRawMessagingPreprocessor(),
                                                                  new HashSet<JoynrMessageProcessor>(),
                                                                  ownGbid,
                                                                  routingTable,
                                                                  separateReplyConnection,
                                                                  "",
                                                                  mqttMessageInProgressObserver);
        super.subject = subject;
    }

    @Test
    public void initSubscribesToReplyToTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.init();
        verify(mqttClient).subscribe(eq(replyToTopic + "/#"));
    }

    @Test
    public void initSubscribesToReplyToTopic_separateReplyConnection() {
        initMocks(true);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.init();
        verify(mqttReplyClient).subscribe(replyToTopic + "/#");
        verify(mqttClient, times(0)).subscribe(any(String.class));
    }

    @Test
    public void initDoesNotSubscribeToSharedTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.init();
        verify(mqttClient, times(0)).subscribe(startsWith("$share/"));
    }

    @Test
    public void subscribeSubscribesToReplyToTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribe();
        verify(mqttClient).subscribe(eq(replyToTopic + "/#"));
    }

    @Test
    public void subscribeSubscribesToReplyToTopic_separateReplyConnection() {
        initMocks(true);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribe();
        verify(mqttReplyClient).subscribe(replyToTopic + "/#");
        verify(mqttClient, times(0)).subscribe(any(String.class));
    }

    @Test
    public void subscribeDoesNotSubscribeToSharedTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribe();
        verify(mqttClient, times(0)).subscribe(startsWith("$share/"));
    }

}
