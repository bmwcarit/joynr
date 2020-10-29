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
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Matchers;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.JoynrVersion;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.dispatching.MutableMessageFactory;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttMessagingSkeletonFactory;
import io.joynr.messaging.mqtt.MqttMessagingSkeletonProvider;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.BroadcastSubscriptionRequest;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;
import joynr.OnChangeSubscriptionQos;
import joynr.Request;
import joynr.SubscriptionRequest;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.Version;

/**
 * Test that the correct backend connection is used for (global) proxy calls and provider replies and publications.
 */
@RunWith(MockitoJUnitRunner.class)
public class MqttMultipleBackendProviderProxyTest extends AbstractMqttMultipleBackendTest {

    private GlobalDiscoveryEntry globalDiscoveryEntry1;
    private GlobalDiscoveryEntry globalDiscoveryEntry2;

    @Before
    public void setUp() throws InterruptedException {
        super.setUp();
        createJoynrRuntimeWithMockedGcdClient();

        JoynrVersion joynrVersion = testProxy.class.getAnnotation(JoynrVersion.class);

        globalDiscoveryEntry1 = new GlobalDiscoveryEntry();
        globalDiscoveryEntry1.setProviderVersion(new Version(joynrVersion.major(), joynrVersion.minor()));
        globalDiscoveryEntry1.setParticipantId("participantId1");
        globalDiscoveryEntry1.setDomain(TESTDOMAIN);
        MqttAddress mqttAddress1 = new MqttAddress(TESTGBID1, TESTTOPIC);
        globalDiscoveryEntry1.setAddress(CapabilityUtils.serializeAddress(mqttAddress1));

        globalDiscoveryEntry2 = new GlobalDiscoveryEntry();
        globalDiscoveryEntry2.setProviderVersion(new Version(joynrVersion.major(), joynrVersion.minor()));
        globalDiscoveryEntry2.setParticipantId("participantId2");
        globalDiscoveryEntry2.setDomain(TESTDOMAIN);
        MqttAddress mqttAddress2 = new MqttAddress(TESTGBID2, TESTTOPIC);
        globalDiscoveryEntry2.setAddress(CapabilityUtils.serializeAddress(mqttAddress2));
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForProxyMethodCall() throws InterruptedException {
        ArgumentCaptor<String> topicCaptor = ArgumentCaptor.forClass(String.class);
        testProxy proxy1 = buildProxyForGlobalDiscoveryEntry(globalDiscoveryEntry1);
        testProxy proxy2 = buildProxyForGlobalDiscoveryEntry(globalDiscoveryEntry2);
        verify(joynrMqttClient1, times(0)).publishMessage(anyString(),
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

        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        doAnswer(createVoidCountDownAnswer(publishCountDownLatch)).when(joynrMqttClient1)
                                                                  .publishMessage(anyString(),
                                                                                  any(byte[].class),
                                                                                  anyInt(),
                                                                                  anyLong(),
                                                                                  any(SuccessAction.class),
                                                                                  any(FailureAction.class));
        proxy1.methodFireAndForgetWithoutParams();
        assertTrue(publishCountDownLatch.await(100, TimeUnit.MILLISECONDS));
        verify(joynrMqttClient1).publishMessage(topicCaptor.capture(),
                                                any(byte[].class),
                                                anyInt(),
                                                anyLong(),
                                                any(SuccessAction.class),
                                                any(FailureAction.class));
        assertTrue(topicCaptor.getValue().startsWith(TESTTOPIC));
        verify(joynrMqttClient2, times(0)).publishMessage(anyString(),
                                                          any(byte[].class),
                                                          anyInt(),
                                                          anyLong(),
                                                          any(SuccessAction.class),
                                                          any(FailureAction.class));

        reset(joynrMqttClient1);
        reset(joynrMqttClient2);

        publishCountDownLatch = new CountDownLatch(1);
        doAnswer(createVoidCountDownAnswer(publishCountDownLatch)).when(joynrMqttClient2)
                                                                  .publishMessage(anyString(),
                                                                                  any(byte[].class),
                                                                                  anyInt(),
                                                                                  anyLong(),
                                                                                  any(SuccessAction.class),
                                                                                  any(FailureAction.class));
        proxy2.methodFireAndForgetWithoutParams();
        assertTrue(publishCountDownLatch.await(100, TimeUnit.MILLISECONDS));
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
        assertTrue(topicCaptor.getValue().startsWith(TESTTOPIC));
    }

    private testProxy buildProxyForGlobalDiscoveryEntry(GlobalDiscoveryEntry globalDiscoveryEntry) throws InterruptedException {
        Semaphore semaphore = new Semaphore(0);

        doAnswer(new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError> callback = (CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>) invocation.getArguments()[0];
                List<GlobalDiscoveryEntry> globalDiscoveryEntryList = new ArrayList<>();
                globalDiscoveryEntryList.add(globalDiscoveryEntry);
                callback.onSuccess(globalDiscoveryEntryList);
                return null;
            }
        }).when(gcdClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                  any(String[].class),
                                  anyString(),
                                  anyLong(),
                                  any(String[].class));

        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(TESTDOMAIN, testProxy.class);
        proxyBuilder.setDiscoveryQos(discoveryQos);
        testProxy proxy = proxyBuilder.build(new ProxyCreatedCallback<testProxy>() {

            @Override
            public void onProxyCreationFinished(testProxy result) {
                semaphore.release();

            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                fail("Proxy creation failed: " + error.toString());

            }
        });
        assertTrue(semaphore.tryAcquire(10, TimeUnit.SECONDS));
        reset(gcdClient);
        return proxy;
    }

    private String registerProvider(JoynrProvider provider) {
        ArgumentCaptor<GlobalDiscoveryEntry> gdeCaptor = ArgumentCaptor.forClass(GlobalDiscoveryEntry.class);

        doAnswer(new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) invocation.getArguments()[0];
                callback.onSuccess(null);
                return null;
            }
        }).when(gcdClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                               gdeCaptor.capture(),
                               any(String[].class));

        Future<Void> future = joynrRuntime.getProviderRegistrar(TESTDOMAIN, provider)
                                          .withProviderQos(providerQos)
                                          .withGbids(gbids)
                                          .awaitGlobalRegistration()
                                          .register();

        try {
            future.get(10000);
        } catch (Exception e) {
            fail("registerProvider failed: " + e);
        }

        assertEquals(1, gdeCaptor.getAllValues().size());
        return gdeCaptor.getValue().getParticipantId();
    }

    private void fakeIncomingMessage(String targetGbid, MutableMessage incomingMessage) throws EncodingException,
                                                                                        UnsuppportedVersionException {

        MqttMessagingSkeletonProvider skeletonProvider = injector.getInstance(MqttMessagingSkeletonProvider.class);
        MqttMessagingSkeletonFactory skeletonFactory = (MqttMessagingSkeletonFactory) skeletonProvider.get();
        IMqttMessagingSkeleton skeleton = (IMqttMessagingSkeleton) skeletonFactory.getSkeleton(new MqttAddress(targetGbid,
                                                                                                               ""));
        // the GBID of the replyTo address will be replaced in skeleton.transmit before adding the routing entry
        incomingMessage.setReplyTo(CapabilityUtils.serializeAddress(new MqttAddress("anyString", "")));
        skeleton.transmit(incomingMessage.getImmutableMessage().getSerializedMessage(), new FailureAction() {
            @Override
            public void execute(Throwable error) {
                fail("fake request failed in skeleton.transmit: " + error);
            }
        });
    }

    private void unregisterProvider(Object provider) throws InterruptedException, EncodingException,
                                                     UnsuppportedVersionException {
        CountDownLatch removeCountDownLatch = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) invocation.getArguments()[0];
                callback.onSuccess(null);
                removeCountDownLatch.countDown();
                return null;
            }
        }).when(gcdClient)
          .remove(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(), anyString(), any(String[].class));

        joynrRuntime.unregisterProvider(TESTDOMAIN, provider);

        assertTrue(removeCountDownLatch.await(1500, TimeUnit.MILLISECONDS));
    }

    private static interface MessageCreator {
        MutableMessage create(String proxyParticipantId, String providerParticipantId, String requestReplyId);
    }

    private void checkReplyMessage(byte[] serializedMessage,
                                   String senderParticipantId,
                                   String recipientParticipantId,
                                   Message.MessageType replyMessageType,
                                   String requestReplyId) throws EncodingException, UnsuppportedVersionException {
        assertNotNull(serializedMessage);

        ImmutableMessage replyMessage = new ImmutableMessage(serializedMessage);
        assertEquals(recipientParticipantId, replyMessage.getSender());
        assertEquals(senderParticipantId, replyMessage.getRecipient());
        assertEquals(replyMessageType, replyMessage.getType());
        assertEquals(requestReplyId, replyMessage.getCustomHeaders().get(Message.CUSTOM_HEADER_REQUEST_REPLY_ID));
    }

    private void testCorrectMqttConnectionIsUsedForProviderReply(String targetGbid,
                                                                 MessageCreator messageCreator,
                                                                 Message.MessageType replyMessageType,
                                                                 JoynrMqttClient expectedClient,
                                                                 JoynrMqttClient otherClient) throws InterruptedException,
                                                                                              EncodingException,
                                                                                              UnsuppportedVersionException {
        final String proxyParticipantId = "senderParticipantId";
        final String requestReplyId = UUID.randomUUID().toString();
        DefaulttestProvider testProvider = new DefaulttestProvider();

        String providerParticipantId = registerProvider(testProvider);

        fakeIncomingMessage(targetGbid,
                            messageCreator.create(proxyParticipantId, providerParticipantId, requestReplyId));

        CountDownLatch replyCountDownLatch = new CountDownLatch(1);
        doAnswer(createVoidCountDownAnswer(replyCountDownLatch)).when(expectedClient)
                                                                .publishMessage(anyString(),
                                                                                any(byte[].class),
                                                                                anyInt(),
                                                                                anyLong(),
                                                                                any(SuccessAction.class),
                                                                                any(FailureAction.class));
        assertTrue(replyCountDownLatch.await(1000, TimeUnit.MILLISECONDS));

        ArgumentCaptor<byte[]> messageCaptor = ArgumentCaptor.forClass(byte[].class);
        verify(expectedClient).publishMessage(anyString(),
                                              messageCaptor.capture(),
                                              anyInt(),
                                              anyLong(),
                                              any(SuccessAction.class),
                                              any(FailureAction.class));
        verify(otherClient, times(0)).publishMessage(anyString(),
                                                     any(byte[].class),
                                                     anyInt(),
                                                     anyLong(),
                                                     any(SuccessAction.class),
                                                     any(FailureAction.class));
        byte[] serializedMessage = messageCaptor.getValue();
        checkReplyMessage(serializedMessage,
                          proxyParticipantId,
                          providerParticipantId,
                          replyMessageType,
                          requestReplyId);

        unregisterProvider(testProvider);
    }

    private void testCorrectMqttConnectionIsUsedForProviderRequestReply(String targetGbid,
                                                                        JoynrMqttClient expectedClient,
                                                                        JoynrMqttClient otherClient) throws InterruptedException,
                                                                                                     EncodingException,
                                                                                                     UnsuppportedVersionException {
        MessageCreator messageCreator = new MessageCreator() {
            @Override
            public MutableMessage create(String proxyParticipantId,
                                         String providerParticipantId,
                                         String requestReplyId) {
                MutableMessageFactory messageFactory = injector.getInstance(MutableMessageFactory.class);
                MutableMessage requestMessage = messageFactory.createRequest(proxyParticipantId,
                                                                             providerParticipantId,
                                                                             new Request("voidOperation",
                                                                                         new Object[0],
                                                                                         new String[0],
                                                                                         requestReplyId),
                                                                             new MessagingQos());
                return requestMessage;
            }
        };
        Message.MessageType replyType = Message.MessageType.VALUE_MESSAGE_TYPE_REPLY;
        testCorrectMqttConnectionIsUsedForProviderReply(targetGbid,
                                                        messageCreator,
                                                        replyType,
                                                        expectedClient,
                                                        otherClient);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForProviderRequestReply_defaultGbid() throws InterruptedException,
                                                                                     EncodingException,
                                                                                     UnsuppportedVersionException {
        testCorrectMqttConnectionIsUsedForProviderRequestReply(TESTGBID1, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForProviderRequestReply_nonDefaultGbid() throws InterruptedException,
                                                                                        EncodingException,
                                                                                        UnsuppportedVersionException {
        testCorrectMqttConnectionIsUsedForProviderRequestReply(TESTGBID2, joynrMqttClient2, joynrMqttClient1);
    }

    private void testCorrectMqttConnectionIsUsedForProviderAttributeSubscriptionReply(String targetGbid,
                                                                                      JoynrMqttClient expectedClient,
                                                                                      JoynrMqttClient otherClient) throws InterruptedException,
                                                                                                                   EncodingException,
                                                                                                                   UnsuppportedVersionException {
        MessageCreator messageCreator = new MessageCreator() {
            @Override
            public MutableMessage create(String proxyParticipantId,
                                         String providerParticipantId,
                                         String requestReplyId) {
                MutableMessageFactory messageFactory = injector.getInstance(MutableMessageFactory.class);
                MutableMessage subscriptionRequestMessage = messageFactory.createSubscriptionRequest(proxyParticipantId,
                                                                                                     providerParticipantId,
                                                                                                     new SubscriptionRequest(requestReplyId,
                                                                                                                             "testAttribute",
                                                                                                                             new OnChangeSubscriptionQos()),
                                                                                                     new MessagingQos());
                return subscriptionRequestMessage;
            }
        };

        Message.MessageType replyType1 = Message.MessageType.VALUE_MESSAGE_TYPE_PUBLICATION;
        Message.MessageType replyType2 = Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY;

        final String proxyParticipantId = "senderParticipantId";
        final String requestReplyId = UUID.randomUUID().toString();
        DefaulttestProvider testProvider = new DefaulttestProvider();

        String providerParticipantId = registerProvider(testProvider);

        fakeIncomingMessage(targetGbid,
                            messageCreator.create(proxyParticipantId, providerParticipantId, requestReplyId));

        // expect initial publication and SubscriptionReply
        CountDownLatch replyCountDownLatch = new CountDownLatch(2);
        doAnswer(createVoidCountDownAnswer(replyCountDownLatch)).when(expectedClient)
                                                                .publishMessage(anyString(),
                                                                                any(byte[].class),
                                                                                anyInt(),
                                                                                anyLong(),
                                                                                any(SuccessAction.class),
                                                                                any(FailureAction.class));
        assertTrue(replyCountDownLatch.await(1000, TimeUnit.MILLISECONDS));

        ArgumentCaptor<byte[]> messageCaptor = ArgumentCaptor.forClass(byte[].class);
        verify(expectedClient, times(2)).publishMessage(anyString(),
                                                        messageCaptor.capture(),
                                                        anyInt(),
                                                        anyLong(),
                                                        any(SuccessAction.class),
                                                        any(FailureAction.class));
        verify(otherClient, times(0)).publishMessage(anyString(),
                                                     any(byte[].class),
                                                     anyInt(),
                                                     anyLong(),
                                                     any(SuccessAction.class),
                                                     any(FailureAction.class));
        List<byte[]> serializedMessages = messageCaptor.getAllValues();
        checkReplyMessage(serializedMessages.get(0),
                          proxyParticipantId,
                          providerParticipantId,
                          replyType1,
                          requestReplyId);
        checkReplyMessage(serializedMessages.get(1),
                          proxyParticipantId,
                          providerParticipantId,
                          replyType2,
                          requestReplyId);

        unregisterProvider(testProvider);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForProviderAttributeSubscriptionReply_defaultGbid() throws InterruptedException,
                                                                                                   EncodingException,
                                                                                                   UnsuppportedVersionException {
        testCorrectMqttConnectionIsUsedForProviderAttributeSubscriptionReply(TESTGBID1,
                                                                             joynrMqttClient1,
                                                                             joynrMqttClient2);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForProviderAttributeSubscriptionReply_nonDefaultGbid() throws InterruptedException,
                                                                                                      EncodingException,
                                                                                                      UnsuppportedVersionException {
        testCorrectMqttConnectionIsUsedForProviderAttributeSubscriptionReply(TESTGBID2,
                                                                             joynrMqttClient2,
                                                                             joynrMqttClient1);
    }

    private void testCorrectMqttConnectionIsUsedForProviderBroadcastSubscriptionReply(String targetGbid,
                                                                                      JoynrMqttClient expectedClient,
                                                                                      JoynrMqttClient otherClient) throws InterruptedException,
                                                                                                                   EncodingException,
                                                                                                                   UnsuppportedVersionException {
        MessageCreator messageCreator = new MessageCreator() {
            @Override
            public MutableMessage create(String proxyParticipantId,
                                         String providerParticipantId,
                                         String requestReplyId) {
                MutableMessageFactory messageFactory = injector.getInstance(MutableMessageFactory.class);
                MutableMessage subscriptionRequestMessage = messageFactory.createSubscriptionRequest(proxyParticipantId,
                                                                                                     providerParticipantId,
                                                                                                     new BroadcastSubscriptionRequest(requestReplyId,
                                                                                                                                      "booleanBroadcast",
                                                                                                                                      null,
                                                                                                                                      new OnChangeSubscriptionQos()),
                                                                                                     new MessagingQos());
                return subscriptionRequestMessage;
            }
        };
        Message.MessageType replyType = Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY;
        testCorrectMqttConnectionIsUsedForProviderReply(targetGbid,
                                                        messageCreator,
                                                        replyType,
                                                        expectedClient,
                                                        otherClient);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForProviderMulticastSubscriptionReply_defaultGbid() throws InterruptedException,
                                                                                                   EncodingException,
                                                                                                   UnsuppportedVersionException {
        testCorrectMqttConnectionIsUsedForProviderBroadcastSubscriptionReply(TESTGBID1,
                                                                             joynrMqttClient1,
                                                                             joynrMqttClient2);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForProviderMulticastSubscriptionReply_nonDefaultGbid() throws InterruptedException,
                                                                                                      EncodingException,
                                                                                                      UnsuppportedVersionException {
        testCorrectMqttConnectionIsUsedForProviderBroadcastSubscriptionReply(TESTGBID2,
                                                                             joynrMqttClient2,
                                                                             joynrMqttClient1);
    }

    @Test
    public void testMulticastIsPublishedToAllKnownBackends() throws InterruptedException, EncodingException,
                                                             UnsuppportedVersionException {
        DefaulttestProvider testProvider = new DefaulttestProvider();

        String providerParticipantId = registerProvider(testProvider);
        String multicastId = providerParticipantId + "/emptyBroadcast";

        CountDownLatch countDownLatch = new CountDownLatch(2);
        doAnswer(createVoidCountDownAnswer(countDownLatch)).when(joynrMqttClient1)
                                                           .publishMessage(anyString(),
                                                                           any(byte[].class),
                                                                           anyInt(),
                                                                           anyLong(),
                                                                           any(SuccessAction.class),
                                                                           any(FailureAction.class));
        doAnswer(createVoidCountDownAnswer(countDownLatch)).when(joynrMqttClient2)
                                                           .publishMessage(anyString(),
                                                                           any(byte[].class),
                                                                           anyInt(),
                                                                           anyLong(),
                                                                           any(SuccessAction.class),
                                                                           any(FailureAction.class));
        verify(joynrMqttClient1, times(0)).publishMessage(anyString(),
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

        testProvider.fireEmptyBroadcast();

        assertTrue(countDownLatch.await(1000, TimeUnit.MILLISECONDS));
        ArgumentCaptor<byte[]> messageCaptor1 = ArgumentCaptor.forClass(byte[].class);
        ArgumentCaptor<byte[]> messageCaptor2 = ArgumentCaptor.forClass(byte[].class);
        verify(joynrMqttClient1, times(1)).publishMessage(anyString(),
                                                          messageCaptor1.capture(),
                                                          anyInt(),
                                                          anyLong(),
                                                          any(SuccessAction.class),
                                                          any(FailureAction.class));
        verify(joynrMqttClient2, times(1)).publishMessage(anyString(),
                                                          messageCaptor2.capture(),
                                                          anyInt(),
                                                          anyLong(),
                                                          any(SuccessAction.class),
                                                          any(FailureAction.class));

        checkReplyMessage(messageCaptor1.getValue(),
                          multicastId,
                          providerParticipantId,
                          Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST,
                          null);
        checkReplyMessage(messageCaptor2.getValue(),
                          multicastId,
                          providerParticipantId,
                          Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST,
                          null);

        unregisterProvider(testProvider);
    }

}
