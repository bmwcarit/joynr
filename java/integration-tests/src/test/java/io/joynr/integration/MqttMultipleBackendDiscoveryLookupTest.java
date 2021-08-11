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
import static org.mockito.Matchers.anyMapOf;
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
import io.joynr.proxy.ProxyBuilder;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.tests.testProxy;

/**
 * Test that the correct backend connection is used for global lookup calls.
 */
@RunWith(MockitoJUnitRunner.class)
public class MqttMultipleBackendDiscoveryLookupTest extends MqttMultipleBackendDiscoveryAbstractTest {

    private void testCorrectBackendIsContactedForLookup(String[] gbidsForLookup,
                                                        JoynrMqttClient expectedClient,
                                                        JoynrMqttClient otherClient) throws Exception {
        String gcdTopic = getGcdTopic();

        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        doAnswer(createVoidCountDownAnswer(publishCountDownLatch)).when(expectedClient)
                                                                  .publishMessage(eq(gcdTopic),
                                                                                  any(byte[].class),
                                                                                  anyMapOf(String.class, String.class),
                                                                                  anyInt(),
                                                                                  anyLong(),
                                                                                  any(SuccessAction.class),
                                                                                  any(FailureAction.class));

        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(TESTDOMAIN, testProxy.class);
        proxyBuilder.setDiscoveryQos(discoveryQos);
        if (gbidsForLookup != null) {
            proxyBuilder.setGbids(gbidsForLookup);
        }
        proxyBuilder.build();

        assertTrue(publishCountDownLatch.await(500, TimeUnit.MILLISECONDS));
        verify(otherClient, times(0)).publishMessage(anyString(),
                                                     any(byte[].class),
                                                     anyMapOf(String.class, String.class),
                                                     anyInt(),
                                                     anyLong(),
                                                     any(SuccessAction.class),
                                                     any(FailureAction.class));
        ArgumentCaptor<byte[]> messageCaptor = ArgumentCaptor.forClass(byte[].class);
        verify(expectedClient).publishMessage(eq(gcdTopic),
                                              messageCaptor.capture(),
                                              anyMapOf(String.class, String.class),
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
    public void testCorrectBackendIsContactedForLookup_noGbids() throws Exception {
        testCorrectBackendIsContactedForLookup(null, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForLookup_singleDefaultGbid() throws Exception {
        testCorrectBackendIsContactedForLookup(new String[]{ TESTGBID1 }, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForLookup_singleNonDefaultGbid() throws Exception {
        testCorrectBackendIsContactedForLookup(new String[]{ TESTGBID2 }, joynrMqttClient2, joynrMqttClient1);
    }

    @Test
    public void testCorrectBackendIsContactedForLookup_multipleGbids() throws Exception {
        testCorrectBackendIsContactedForLookup(new String[]{ TESTGBID1, TESTGBID2 },
                                               joynrMqttClient1,
                                               joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForLookup_multipleGbidsReversed() throws Exception {
        testCorrectBackendIsContactedForLookup(new String[]{ TESTGBID2, TESTGBID1 },
                                               joynrMqttClient2,
                                               joynrMqttClient1);
    }

}
