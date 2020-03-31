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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.common.ExpiryDate;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.MessagingQosEffort;
import io.joynr.messaging.SuccessAction;
import joynr.ImmutableMessage;
import joynr.Message.MessageType;
import joynr.system.RoutingTypes.MqttAddress;

/**
 * Unit tests for {@link MqttMessagingStub}.
 */
@RunWith(MockitoJUnitRunner.class)
public class MqttMessagingStubTest {
    @Mock
    private MqttAddress mqttAddress;

    @Mock
    private JoynrMqttClient mqttClient;

    @Mock
    private ImmutableMessage joynrMessage;

    @Mock
    private FailureAction failureAction;

    @Mock
    private SuccessAction successAction;

    private MqttMessagingStub subject;

    @Before
    public void setup() {
        doReturn(new byte[0]).when(joynrMessage).getSerializedMessage();
        subject = new MqttMessagingStub(mqttAddress, mqttClient);
    }

    @Test
    public void testMessagePublishedToCorrectTopic() {
        final String testTopic = "topicTestTopic";
        final String expectedTopic = testTopic + "/low";
        when(mqttAddress.getTopic()).thenReturn(testTopic);
        subject.transmit(joynrMessage, successAction, failureAction);

        verify(mqttClient).publishMessage(eq(expectedTopic),
                                          any(byte[].class),
                                          eq(MqttMessagingStub.DEFAULT_QOS_LEVEL),
                                          anyLong());
    }

    @Test
    public void testMulticastMessagePublishedToCorrectTopic() {
        final String testTopic = "topicTestTopic";
        final String expectedTopic = testTopic;
        when(mqttAddress.getTopic()).thenReturn(testTopic);
        when(joynrMessage.getType()).thenReturn(MessageType.VALUE_MESSAGE_TYPE_MULTICAST);
        subject.transmit(joynrMessage, successAction, failureAction);

        verify(mqttClient).publishMessage(eq(expectedTopic),
                                          any(byte[].class),
                                          eq(MqttMessagingStub.DEFAULT_QOS_LEVEL),
                                          anyLong());
    }

    @Test
    public void testMessagePublishedWithNormalEffort() {
        when(joynrMessage.getEffort()).thenReturn(String.valueOf(MessagingQosEffort.NORMAL));

        subject.transmit(joynrMessage, successAction, failureAction);

        verify(mqttClient).publishMessage(anyString(),
                                          any(byte[].class),
                                          eq(MqttMessagingStub.DEFAULT_QOS_LEVEL),
                                          anyLong());
    }

    @Test
    public void testMessagePublishedWithMsgTtlSecAlwaysRoundedUp() {
        when(joynrMessage.getEffort()).thenReturn(String.valueOf(MessagingQosEffort.NORMAL));

        final long expectedRoundedMsgTtlSec = 61;
        // check number that is closer to the larger value (61)
        final long messageTtlMs1 = System.currentTimeMillis() + 60700;
        when(joynrMessage.getTtlMs()).thenReturn(messageTtlMs1);

        subject.transmit(joynrMessage, successAction, failureAction);

        long relativeTtl1 = ExpiryDate.fromAbsolute(messageTtlMs1).getRelativeTtl();
        assertTrue(relativeTtl1 % 1000 > 600 && relativeTtl1 % 1000 < 800);

        // check number that is closer to the lower value (60)
        final long messageTtlMs2 = System.currentTimeMillis() + 60200;
        when(joynrMessage.getTtlMs()).thenReturn(messageTtlMs2);

        subject.transmit(joynrMessage, successAction, failureAction);

        long relativeTtl2 = ExpiryDate.fromAbsolute(messageTtlMs2).getRelativeTtl();
        assertTrue(relativeTtl2 % 1000 > 100 && relativeTtl2 % 1000 < 300);

        verify(mqttClient, times(2)).publishMessage(anyString(),
                                                    any(byte[].class),
                                                    eq(MqttMessagingStub.DEFAULT_QOS_LEVEL),
                                                    eq(expectedRoundedMsgTtlSec));
    }

    @Test
    public void testMessagePublishedWithMsgTtlSecGreaterThanMaxIntervalAlwaysSetToMaxInterval() {
        when(joynrMessage.getEffort()).thenReturn(String.valueOf(MessagingQosEffort.NORMAL));

        final long MESSAGE_EXPIRY_MAX_INTERVAL = 4294967295L;
        final long expectedMaxMsgTtlSec = MESSAGE_EXPIRY_MAX_INTERVAL;

        // MessageExpiryInterval is > MESSAGE_EXPIRY_MAX_INTERVAL
        final long messageTtlMs1 = System.currentTimeMillis() + MESSAGE_EXPIRY_MAX_INTERVAL * 1000 + 60000;
        when(joynrMessage.getTtlMs()).thenReturn(messageTtlMs1);
        subject.transmit(joynrMessage, successAction, failureAction);

        // MessageExpiryInterval is < 0
        final long messageTtlMs2 = -10;
        when(joynrMessage.getTtlMs()).thenReturn(messageTtlMs2);
        subject.transmit(joynrMessage, successAction, failureAction);

        verify(mqttClient, times(2)).publishMessage(anyString(),
                                                    any(byte[].class),
                                                    eq(MqttMessagingStub.DEFAULT_QOS_LEVEL),
                                                    eq(expectedMaxMsgTtlSec));
    }

    @Test
    public void testMessagePublishedWithoutEffort() {
        when(joynrMessage.getEffort()).thenReturn(null);

        subject.transmit(joynrMessage, successAction, failureAction);

        verify(mqttClient).publishMessage(anyString(),
                                          any(byte[].class),
                                          eq(MqttMessagingStub.DEFAULT_QOS_LEVEL),
                                          anyLong());
    }

    @Test
    public void testReactToBestEffortQosInJoynrMessageHeader() {
        when(joynrMessage.getEffort()).thenReturn(String.valueOf(MessagingQosEffort.BEST_EFFORT));

        subject.transmit(joynrMessage, successAction, failureAction);

        verify(mqttClient).publishMessage(anyString(),
                                          any(byte[].class),
                                          eq(MqttMessagingStub.BEST_EFFORT_QOS_LEVEL),
                                          anyLong());
    }

    @Test
    public void testSuccessActionCalled() {
        when(joynrMessage.getEffort()).thenReturn(String.valueOf(MessagingQosEffort.NORMAL));

        subject.transmit(joynrMessage, successAction, failureAction);

        verify(successAction).execute();
    }

    @Test
    public void testExceptionIsForwardedToCaller() {
        when(joynrMessage.getEffort()).thenReturn(String.valueOf(MessagingQosEffort.NORMAL));
        JoynrRuntimeException exception = new JoynrRuntimeException("testException");
        JoynrRuntimeException expectedException = new JoynrRuntimeException(exception.getMessage());
        doThrow(exception).when(mqttClient).publishMessage(anyString(), any(byte[].class), anyInt(), anyLong());

        try {
            subject.transmit(joynrMessage, successAction, failureAction);
            fail("expected exception");
        } catch (Exception e) {
            assertEquals(expectedException, e);
        }
        verify(successAction, times(0)).execute();
        verify(failureAction, times(0)).execute(exception);
    }
}
