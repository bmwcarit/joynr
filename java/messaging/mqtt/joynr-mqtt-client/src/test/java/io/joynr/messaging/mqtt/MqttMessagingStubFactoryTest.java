/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;
import joynr.MutableMessage;
import joynr.system.RoutingTypes.MqttAddress;

@RunWith(MockitoJUnitRunner.class)
public class MqttMessagingStubFactoryTest {

    private final String TESTGBID1 = "testgbid1";
    private final String TESTGBID2 = "testgbid2";
    private final String TESTTOPIC1 = "testtopic1";
    private final String TESTTOPIC2 = "testtopic2";

    @Mock
    private MqttClientFactory mqttClientFactory;

    @Mock
    private JoynrMqttClient joynrMqttClient1;

    @Mock
    private JoynrMqttClient joynrMqttClient2;

    @Before
    public void setUp() {
        doReturn(joynrMqttClient1).when(mqttClientFactory).createSender(TESTGBID1);
        doReturn(joynrMqttClient2).when(mqttClientFactory).createSender(TESTGBID2);
    }

    @Test
    public void testReturnStubWithCorrectClient() throws SecurityException, EncodingException,
                                                  UnsuppportedVersionException {
        MutableMessage mutableMessage = new MutableMessage();
        mutableMessage.setPayload("testPayload".getBytes());
        mutableMessage.setRecipient("test");
        mutableMessage.setSender("test");
        ImmutableMessage message = mutableMessage.getImmutableMessage();
        SuccessAction successAction = new SuccessAction() {

            @Override
            public void execute() {
            }
        };

        FailureAction failureAction = new FailureAction() {

            @Override
            public void execute(Throwable error) {
            }
        };

        ArgumentCaptor<String> topicCaptor = ArgumentCaptor.forClass(String.class);
        String[] gbid_array = new String[]{ TESTGBID1, TESTGBID2 };
        MqttMessagingStubFactory subject = new MqttMessagingStubFactory(mqttClientFactory, gbid_array);

        MqttAddress address1 = new MqttAddress(TESTGBID1, TESTTOPIC1);
        MqttMessagingStub messagingStub1 = subject.createInternal(address1);
        MqttAddress address2 = new MqttAddress(TESTGBID2, TESTTOPIC2);
        MqttMessagingStub messagingStub2 = subject.createInternal(address2);

        messagingStub1.transmit(message, successAction, failureAction);
        verify(joynrMqttClient1).publishMessage(topicCaptor.capture(),
                                                any(byte[].class),
                                                anyInt(),
                                                anyLong(),
                                                any(SuccessAction.class),
                                                any(FailureAction.class));
        verify(joynrMqttClient2, times(0)).publishMessage(anyString(),
                                                          any(byte[].class),
                                                          anyInt(),
                                                          anyLong(),
                                                          any(SuccessAction.class),
                                                          any(FailureAction.class));
        assertTrue(topicCaptor.getValue().startsWith(TESTTOPIC1));
        reset(joynrMqttClient1);
        reset(joynrMqttClient2);
        messagingStub2.transmit(message, successAction, failureAction);
        verify(joynrMqttClient1, times(0)).publishMessage(anyString(),
                                                          any(byte[].class),
                                                          anyInt(),
                                                          anyLong(),
                                                          any(SuccessAction.class),
                                                          any(FailureAction.class));
        verify(joynrMqttClient2).publishMessage(topicCaptor.capture(),
                                                any(byte[].class),
                                                anyInt(),
                                                anyLong(),
                                                any(SuccessAction.class),
                                                any(FailureAction.class));
        assertTrue(topicCaptor.getValue().startsWith(TESTTOPIC2));
    }

    @Test
    public void testReturnNullOnUnknownGbid() {
        String[] gbid_array = new String[]{ TESTGBID1, TESTGBID2 };
        MqttMessagingStubFactory subject = new MqttMessagingStubFactory(mqttClientFactory, gbid_array);
        MqttAddress address = new MqttAddress("UnknownGbid", "UnknownTopic");
        MqttMessagingStub messagingStub = subject.createInternal(address);
        assertNull(messagingStub);
    }

}
