package io.joynr.messaging.mqtt;

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

import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.when;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.MessagingQosEffort;
import io.joynr.messaging.SuccessAction;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.MqttAddress;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

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
        subject = new MqttMessagingStub(mqttAddress, mqttClient);
    }

    @Test
    public void testMessagePublishedWithNormalEffort() {
        when(joynrMessage.getEffort()).thenReturn(String.valueOf(MessagingQosEffort.NORMAL));

        subject.transmit(joynrMessage, successAction, failureAction);

        Mockito.verify(mqttClient).publishMessage(anyString(),
                                                  any(byte[].class),
                                                  eq(MqttMessagingStub.DEFAULT_QOS_LEVEL));
    }

    @Test
    public void testReactToBestEffortQosInJoynrMessageHeader() {
        when(joynrMessage.getEffort()).thenReturn(String.valueOf(MessagingQosEffort.BEST_EFFORT));

        subject.transmit(joynrMessage, successAction, failureAction);

        Mockito.verify(mqttClient).publishMessage(anyString(),
                                                  any(byte[].class),
                                                  eq(MqttMessagingStub.BEST_EFFORT_QOS_LEVEL));
    }

    @Test
    public void testSuccessActionCalled() {
        when(joynrMessage.getEffort()).thenReturn(String.valueOf(MessagingQosEffort.NORMAL));

        subject.transmit(joynrMessage, successAction, failureAction);

        Mockito.verify(successAction).execute();
    }

    @Test
    public void testFailureActionCalled() {
        when(joynrMessage.getEffort()).thenReturn(String.valueOf(MessagingQosEffort.NORMAL));
        JoynrRuntimeException exception = new JoynrRuntimeException("testException");
        doThrow(exception).when(mqttClient).publishMessage(anyString(), any(byte[].class), anyInt());

        subject.transmit(joynrMessage, successAction, failureAction);

        Mockito.verify(failureAction).execute(exception);
    }
}
