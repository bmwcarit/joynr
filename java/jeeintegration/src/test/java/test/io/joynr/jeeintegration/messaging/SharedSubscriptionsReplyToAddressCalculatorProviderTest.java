package test.io.joynr.jeeintegration.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static joynr.JoynrMessage.MESSAGE_TYPE_REQUEST;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.hamcrest.Matchers;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.jeeintegration.messaging.SharedSubscriptionReplyToAddressCalculatorProvider;
import io.joynr.messaging.mqtt.MqttMessageReplyToAddressCalculator;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Unit tests for {@link SharedSubscriptionReplyToAddressCalculatorProvider}.
 */
@RunWith(MockitoJUnitRunner.class)
public class SharedSubscriptionsReplyToAddressCalculatorProviderTest {

    @Captor
    private ArgumentCaptor<String> stringCaptor;

    @Test
    public void testReplyToAddedWhenActive() {
        MqttMessageReplyToAddressCalculator subject = getAddressCalculator("tcp://broker:1883", "test/topic", true);
        JoynrMessage message = mockMessage(MESSAGE_TYPE_REQUEST);

        subject.setReplyTo(message);

        verify(message).setReplyTo(stringCaptor.capture());
        Assert.assertThat(stringCaptor.getValue(), Matchers.containsString("replyto/"));
    }

    @Test
    public void testReplyToNotAddedWhenInactive() {
        MqttMessageReplyToAddressCalculator subject = getAddressCalculator("tcp://broker:1883", "test/topic", false);
        JoynrMessage message = mockMessage(MESSAGE_TYPE_REQUEST);

        subject.setReplyTo(message);

        verify(message).setReplyTo(stringCaptor.capture());
        Assert.assertThat(stringCaptor.getValue(), Matchers.not(Matchers.containsString("replyto/")));
    }

    private JoynrMessage mockMessage(String messageType) {
        JoynrMessage message = Mockito.mock(JoynrMessage.class);
        when(message.getType()).thenReturn(messageType);
        return message;
    }

    private MqttMessageReplyToAddressCalculator getAddressCalculator(String brokerUri, String topic, boolean enabled) {
        MqttAddress replyToMqttAddress = new MqttAddress(brokerUri, topic);
        String enableSharedSubscriptions = Boolean.valueOf(enabled).toString();
        SharedSubscriptionReplyToAddressCalculatorProvider subject = new SharedSubscriptionReplyToAddressCalculatorProvider(replyToMqttAddress,
                                                                                                                            enableSharedSubscriptions);
        return subject.get();
    }

}
