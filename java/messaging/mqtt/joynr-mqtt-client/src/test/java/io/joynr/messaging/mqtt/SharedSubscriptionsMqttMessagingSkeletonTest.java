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
package io.joynr.messaging.mqtt;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.startsWith;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import java.util.HashSet;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;

/**
 * Unit tests for {@link SharedSubscriptionsMqttMessagingSkeleton}.
 */
@RunWith(MockitoJUnitRunner.class)
public class SharedSubscriptionsMqttMessagingSkeletonTest extends AbstractSharedSubscriptionsMqttMessagingSkeletonTest {

    @Override
    protected void initAndSubscribe() {
        subject.init();
        // SharedSubscriptionsMqttMessagingSkeleton automatically subscribes to shared topic in init()
        verify(mqttClient).subscribe(startsWith("$share/"));
    }

    @Override
    protected void createSkeleton(String channelId) {
        subject = new SharedSubscriptionsMqttMessagingSkeleton(ownTopic,
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
    }

    @Test
    public void initSubscribesToSharedAndReplyToTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.init();

        verify(mqttReplyClient, never()).setMessageListener(any());
        verify(mqttReplyClient, never()).start();
        verify(mqttReplyClient, never()).subscribe(any(String.class));
        verify(mqttClientFactory, never()).connect(mqttReplyClient);

        verify(mqttClient).setMessageListener(any());
        verify(mqttClient).subscribe(replyToTopic + "/#");
        verify(mqttClient).subscribe("$share/channelId/" + ownTopic + "/#");

    }

    @Test
    public void initSubscribesToSharedAndReplyToTopic_separateReplyConnection() {
        initMocks(true);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.init();

        verify(mqttReplyClient).setMessageListener(any());
        verify(mqttReplyClient).start();
        verify(mqttClientFactory).connect(mqttReplyClient);
        verify(mqttReplyClient).subscribe(replyToTopic + "/#");

        verify(mqttClient).setMessageListener(any());
        verify(mqttClient).subscribe("$share/channelId/" + ownTopic + "/#");

    }

    @Test
    public void subscribeSubscribesToSharedAndReplyToTopic() {
        initMocks(false);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribe();
        verify(mqttClient).subscribe(replyToTopic + "/#");
        verify(mqttClient).subscribe("$share/channelId/" + ownTopic + "/#");
    }

    @Test
    public void subscribeSubscribesToSharedAndReplyToTopic_separateReplyConnection() {
        initMocks(true);
        createSkeleton("channelId");
        verify(mqttClient, times(0)).subscribe(any(String.class));
        subject.subscribe();
        verify(mqttReplyClient).subscribe(replyToTopic + "/#");
        verify(mqttClient).subscribe("$share/channelId/" + ownTopic + "/#");
    }

}
