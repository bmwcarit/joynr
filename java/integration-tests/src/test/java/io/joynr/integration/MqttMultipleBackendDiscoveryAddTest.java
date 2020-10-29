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
package io.joynr.integration;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.messaging.FailureAction;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.runtime.ProviderRegistrar;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.tests.DefaulttestProvider;

/**
 * Test that the correct backend connection is used for global add calls.
 */
@RunWith(MockitoJUnitRunner.class)
public class MqttMultipleBackendDiscoveryAddTest extends MqttMultipleBackendDiscoveryAbstractTest {

    private void testCorrectBackendIsContactedForAdd(String[] gbidsForAdd,
                                                     JoynrMqttClient expectedClient,
                                                     JoynrMqttClient otherClient) throws Exception {
        String gcdTopic = getGcdTopic();

        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        doAnswer(createVoidCountDownAnswer(publishCountDownLatch)).when(expectedClient)
                                                                  .publishMessage(eq(gcdTopic),
                                                                                  any(byte[].class),
                                                                                  anyInt(),
                                                                                  anyLong(),
                                                                                  any(SuccessAction.class),
                                                                                  any(FailureAction.class));

        ProviderRegistrar registrar = joynrRuntime.getProviderRegistrar(TESTDOMAIN, new DefaulttestProvider())
                                                  .withProviderQos(providerQos)
                                                  .awaitGlobalRegistration();
        if (gbidsForAdd != null) {
            registrar.withGbids(gbidsForAdd);
        }
        registrar.register();

        assertTrue(publishCountDownLatch.await(1500, TimeUnit.MILLISECONDS));
        verify(otherClient, times(0)).publishMessage(anyString(),
                                                     any(byte[].class),
                                                     anyInt(),
                                                     anyLong(),
                                                     any(SuccessAction.class),
                                                     any(FailureAction.class));
        ArgumentCaptor<byte[]> messageCaptor = ArgumentCaptor.forClass(byte[].class);
        verify(expectedClient).publishMessage(eq(gcdTopic),
                                              messageCaptor.capture(),
                                              anyInt(),
                                              anyLong(),
                                              any(SuccessAction.class),
                                              any(FailureAction.class));
        byte[] serializedMessage = messageCaptor.getValue();
        ImmutableMessage capturedMessage = new ImmutableMessage(serializedMessage);
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST, capturedMessage.getType());
        assertEquals(getGcdParticipantId(), capturedMessage.getRecipient());
    }

    @Test
    public void testCorrectBackendIsContactedForAdd_noGbids() throws Exception {
        testCorrectBackendIsContactedForAdd(null, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForAdd_singleDefaultGbid() throws Exception {
        testCorrectBackendIsContactedForAdd(new String[]{ TESTGBID1 }, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForAdd_singleNonDefaultGbid() throws Exception {
        testCorrectBackendIsContactedForAdd(new String[]{ TESTGBID2 }, joynrMqttClient2, joynrMqttClient1);
    }

    @Test
    public void testCorrectBackendIsContactedForAdd_multipleGbids() throws Exception {
        testCorrectBackendIsContactedForAdd(new String[]{ TESTGBID1, TESTGBID2 }, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForAdd_multipleGbidsReversed() throws Exception {
        testCorrectBackendIsContactedForAdd(new String[]{ TESTGBID2, TESTGBID1 }, joynrMqttClient2, joynrMqttClient1);
    }

    @Test
    @Deprecated
    public void testCorrectBackendIsContactedForAddToAll() throws Exception {
        String gcdTopic = getGcdTopic();

        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        doAnswer(createVoidCountDownAnswer(publishCountDownLatch)).when(joynrMqttClient1)
                                                                  .publishMessage(eq(gcdTopic),
                                                                                  any(byte[].class),
                                                                                  anyInt(),
                                                                                  anyLong(),
                                                                                  any(SuccessAction.class),
                                                                                  any(FailureAction.class));

        joynrRuntime.getProviderRegistrar(TESTDOMAIN, new DefaulttestProvider())
                    .withProviderQos(providerQos)
                    .awaitGlobalRegistration()
                    .registerInAllBackends();

        assertTrue(publishCountDownLatch.await(1500, TimeUnit.MILLISECONDS));
        verify(joynrMqttClient2, times(0)).publishMessage(anyString(),
                                                          any(byte[].class),
                                                          anyInt(),
                                                          anyLong(),
                                                          any(SuccessAction.class),
                                                          any(FailureAction.class));
        ArgumentCaptor<byte[]> messageCaptor = ArgumentCaptor.forClass(byte[].class);
        verify(joynrMqttClient1).publishMessage(eq(gcdTopic),
                                                messageCaptor.capture(),
                                                anyInt(),
                                                anyLong(),
                                                any(SuccessAction.class),
                                                any(FailureAction.class));
        byte[] serializedMessage = messageCaptor.getValue();
        ImmutableMessage capturedMessage = new ImmutableMessage(serializedMessage);
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST, capturedMessage.getType());
        assertEquals(getGcdParticipantId(), capturedMessage.getRecipient());
    }

}
