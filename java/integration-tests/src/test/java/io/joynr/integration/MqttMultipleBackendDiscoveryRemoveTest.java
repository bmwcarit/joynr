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
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.dispatching.MutableMessageFactory;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttMessagingSkeletonFactory;
import io.joynr.messaging.mqtt.MqttMessagingSkeletonProvider;
import io.joynr.proxy.Future;
import io.joynr.runtime.ProviderRegistrar;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;
import joynr.Reply;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProvider;

/**
 * Test that the correct backend connection is used for global remove calls.
 */
@RunWith(MockitoJUnitRunner.class)
public class MqttMultipleBackendDiscoveryRemoveTest extends MqttMultipleBackendDiscoveryAbstractTest {

    private void fakeVoidReply(String targetGbid, ImmutableMessage requestMessage) throws EncodingException,
                                                                                   UnsuppportedVersionException {
        String requestReplyId = requestMessage.getCustomHeaders().get(Message.CUSTOM_HEADER_REQUEST_REPLY_ID);
        MutableMessageFactory messageFactory = injector.getInstance(MutableMessageFactory.class);
        MutableMessage replyMessage = messageFactory.createReply(requestMessage.getRecipient(),
                                                                 requestMessage.getSender(),
                                                                 new Reply(requestReplyId),
                                                                 new MessagingQos());

        MqttMessagingSkeletonProvider skeletonProvider = injector.getInstance(MqttMessagingSkeletonProvider.class);
        MqttMessagingSkeletonFactory skeletonFactory = (MqttMessagingSkeletonFactory) skeletonProvider.get();
        IMqttMessagingSkeleton skeleton = (IMqttMessagingSkeleton) skeletonFactory.getSkeleton(new MqttAddress(targetGbid,
                                                                                                               ""));
        skeleton.transmit(replyMessage.getImmutableMessage().getSerializedMessage(), new FailureAction() {
            @Override
            public void execute(Throwable error) {
                fail("fake reply failed in skeleton.transmit: " + error);
            }
        });
    }

    private testProvider registerProvider(String[] targetGbids,
                                          JoynrMqttClient expectedClient,
                                          JoynrMqttClient otherClient) throws InterruptedException, EncodingException,
                                                                       UnsuppportedVersionException {
        final String gcdTopic = getGcdTopic();

        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        doAnswer(createVoidCountDownAnswer(publishCountDownLatch)).when(expectedClient)
                                                                  .publishMessage(eq(gcdTopic),
                                                                                  any(byte[].class),
                                                                                  anyInt(),
                                                                                  anyLong(),
                                                                                  any(SuccessAction.class),
                                                                                  any(FailureAction.class));

        DefaulttestProvider provider = new DefaulttestProvider();
        Future<Void> future;

        ProviderRegistrar registrar = joynrRuntime.getProviderRegistrar(TESTDOMAIN, provider)
                                                  .withProviderQos(providerQos)
                                                  .awaitGlobalRegistration();
        if (targetGbids != null) {
            registrar.withGbids(targetGbids);
        }
        future = registrar.register();

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

        reset(expectedClient);

        fakeVoidReply(targetGbids == null ? gbids[0] : targetGbids[0], capturedMessage);

        try {
            future.get(10000);
        } catch (Exception e) {
            fail("registerProvider failed: " + e);
        }
        return provider;
    }

    private void unregisterProvider(Object provider,
                                    String targetGbid,
                                    JoynrMqttClient expectedClient,
                                    JoynrMqttClient otherClient) throws InterruptedException, EncodingException,
                                                                 UnsuppportedVersionException {
        final String gcdTopic = getGcdTopic();

        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        doAnswer(createVoidCountDownAnswer(publishCountDownLatch)).when(expectedClient)
                                                                  .publishMessage(eq(gcdTopic),
                                                                                  any(byte[].class),
                                                                                  anyInt(),
                                                                                  anyLong(),
                                                                                  any(SuccessAction.class),
                                                                                  any(FailureAction.class));

        joynrRuntime.unregisterProvider(TESTDOMAIN, provider);

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

        reset(expectedClient);

        fakeVoidReply(targetGbid, capturedMessage);
        // wait for delivery
        Thread.sleep(100);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForRemove_providerInSelectedDefaultBackend() throws InterruptedException,
                                                                                            EncodingException,
                                                                                            UnsuppportedVersionException {
        final String expectedGbid = TESTGBID1;
        final String[] targetGbids = new String[]{ expectedGbid };
        testProvider provider = registerProvider(targetGbids, joynrMqttClient1, joynrMqttClient2);
        unregisterProvider(provider, expectedGbid, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForRemove_providerInNonSelectedDefaultBackend() throws InterruptedException,
                                                                                               EncodingException,
                                                                                               UnsuppportedVersionException {
        final String expectedGbid = TESTGBID1;
        final String[] targetGbids = null; // no selection (old API)
        testProvider provider = registerProvider(targetGbids, joynrMqttClient1, joynrMqttClient2);
        unregisterProvider(provider, expectedGbid, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForRemove_providerInNonDefaultBackend() throws InterruptedException,
                                                                                       EncodingException,
                                                                                       UnsuppportedVersionException {
        final String expectedGbid = TESTGBID2;
        final String[] targetGbids = new String[]{ expectedGbid };
        testProvider provider = registerProvider(targetGbids, joynrMqttClient2, joynrMqttClient1);
        unregisterProvider(provider, expectedGbid, joynrMqttClient2, joynrMqttClient1);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForRemove_providerInMultipleBackends() throws InterruptedException,
                                                                                      EncodingException,
                                                                                      UnsuppportedVersionException {
        final String expectedGbid = TESTGBID1;
        final String[] targetGbids = new String[]{ expectedGbid, TESTGBID2 };
        testProvider provider = registerProvider(targetGbids, joynrMqttClient1, joynrMqttClient2);
        unregisterProvider(provider, expectedGbid, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForRemove_providerInMultipleBackendsReversed() throws InterruptedException,
                                                                                              EncodingException,
                                                                                              UnsuppportedVersionException {
        final String expectedGbid = TESTGBID2;
        final String[] targetGbids = new String[]{ expectedGbid, TESTGBID1 };
        testProvider provider = registerProvider(targetGbids, joynrMqttClient2, joynrMqttClient1);
        unregisterProvider(provider, expectedGbid, joynrMqttClient2, joynrMqttClient1);
    }

}
