/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.argThat;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentMatcher;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.dispatching.subscription.MulticastIdUtil;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingStub;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.ImmutableMessage;
import joynr.Message.MessageType;
import joynr.MulticastSubscriptionRequest;
import joynr.MutableMessage;
import joynr.OnChangeSubscriptionQos;
import joynr.OneWayRequest;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testTypes.TestEnum;

/**
 * Test RoutingTable reference count handling in CC runtime for incoming messages from proxies in separate runtimes.
 */
@RunWith(MockitoJUnitRunner.class)
public class CcRoutingTableCleanupTest extends AbstractRoutingTableCleanupTest {
    private static final Logger logger = LoggerFactory.getLogger(CcRoutingTableCleanupTest.class);

    private static final long DEFAULT_WAIT_TIME = 10000;
    private String proxyParticipantId;
    private DefaulttestProvider testProvider;

    @Before
    public void setUp() throws InterruptedException {
        super.setUp();
        proxyParticipantId = "proxy-" + createUuidString();
        testProvider = setupProvider();
    }

    @After
    public void tearDown() {
        reset(inProcessMessagingStubFactorySpy);
        verifyNoMoreInteractions(mqttMessagingStubMock);

        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        unregisterGlobal(TESTCUSTOMDOMAIN1, testProvider);
        checkRefCnt(FIXEDPARTICIPANTID1, 0);

        checkRefCnt(proxyParticipantId, 1);
        routingTable.remove(proxyParticipantId);

        verifyNoMoreInteractions(mqttMessagingStubMock);
        super.tearDown();
    }

    private DefaulttestProvider setupProvider() {
        checkRefCnt(FIXEDPARTICIPANTID1, 0);
        DefaulttestProvider testProvider = spy(new DefaulttestProvider());
        registerGlobal(testProvider, TESTCUSTOMDOMAIN1, providerQosGlobal);
        reset(testProvider);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        return testProvider;
    }

    private void createProxyRoutingEntry(Address address) {
        checkRefCnt(proxyParticipantId, 0);
        // create proxy routing entry to make sure that decrement/remove is not called too often
        routingTable.put(proxyParticipantId, address, true, Long.MAX_VALUE);
        checkRefCnt(proxyParticipantId, 1);
    }

    private CountDownLatch handleAndCheckOutgoing(IMessagingStub stub, MessageType msgType) {
        CountDownLatch rpCdl = new CountDownLatch(1);
        doAnswer(invocation -> {
            ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
            assertEquals(proxyParticipantId, msg.getRecipient());
            assertEquals(FIXEDPARTICIPANTID1, msg.getSender());
            assertEquals(msgType, msg.getType());
            checkRefCnt(proxyParticipantId, 2);
            SuccessAction successAction = (SuccessAction) invocation.getArguments()[1];
            successAction.execute();
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 1);
            rpCdl.countDown();
            return null;
        }).when(stub).transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));
        return rpCdl;
    }

    private void verifyOutgoing(IMessagingStub stub, MessageType type, int times) {
        verify(stub, times(times)).transmit(argThat(new ArgumentMatcher<ImmutableMessage>() {
            @Override
            public boolean matches(Object argument) {
                ImmutableMessage msg = (ImmutableMessage) argument;
                return type.equals(msg.getType());
            }
        }), any(SuccessAction.class), any(FailureAction.class));
    }

    private void checkIncominMsgAndRefCounts(boolean increment, InvocationOnMock invocation, MessageType type) {
        ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
        try {
            assertEquals(type, msg.getType());
            assertEquals(FIXEDPARTICIPANTID1, msg.getRecipient());
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, increment ? 2 : 1);
        } catch (AssertionError e) {
            logger.error("TEST FAILED in checkIncominMsgAndRefCounts", e);
            throw e;
        }
    }

    private CountDownLatch delayProviderVoidOperation() {
        CountDownLatch replyCountDownLatch = new CountDownLatch(1);
        doAnswer(invocation -> {
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 2);
            // wait until reply shall be returned
            replyCountDownLatch.await();
            @SuppressWarnings("unchecked")
            Promise<DeferredVoid> result = (Promise<DeferredVoid>) invocation.callRealMethod();
            return result;
        }).when(testProvider).voidOperation();
        return replyCountDownLatch;
    }

    @FunctionalInterface
    private interface ThrowingBiConsumer<T, U> {
        public void accept(T t, U u) throws Exception;
    }

    private void fakeIncomingRequest() {
        MutableMessage requestMsg = createRequestMsg(proxyParticipantId, FIXEDPARTICIPANTID1);
        try {
            fakeIncomingMqttMessage(gbids[1], requestMsg);
        } catch (Exception e) {
            fail("fake incoming request failed: " + e);
        }
    }

    private void fakeIncomingSrq(String subscriptionId, final long validityMs) {
        MutableMessage requestMsg = createSrqMsg(proxyParticipantId, FIXEDPARTICIPANTID1, subscriptionId, validityMs);
        try {
            fakeIncomingMqttMessage(gbids[1], requestMsg);
        } catch (Exception e) {
            fail("fake incoming subscription request failed: " + e);
        }
    }

    private CountDownLatch fakeIncomingSst(String subscriptionId) {
        CountDownLatch sstCdl = new CountDownLatch(1);
        doAnswer(factory -> {
            InProcessMessagingStub inProcessMessagingStubSpy = spy((InProcessMessagingStub) factory.callRealMethod());

            doAnswer(invocation -> {
                ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
                assertEquals(MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_STOP, msg.getType());
                assertEquals(FIXEDPARTICIPANTID1, msg.getRecipient());
                checkRefCnt(FIXEDPARTICIPANTID1, 1);
                checkRefCnt(proxyParticipantId, 2);

                SuccessAction onSuccess = (SuccessAction) invocation.getArguments()[1];
                invocation.callRealMethod();
                onSuccess.execute();

                sstCdl.countDown();
                return null;
            }).when(inProcessMessagingStubSpy)
              .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

            return inProcessMessagingStubSpy;
        }).when(inProcessMessagingStubFactorySpy).create(any(InProcessAddress.class));
        SubscriptionStop sst = new SubscriptionStop(subscriptionId);
        MutableMessage sstMsg = messageFactory.createSubscriptionStop(proxyParticipantId,
                                                                      FIXEDPARTICIPANTID1,
                                                                      sst,
                                                                      defaultMessagingQos);
        try {
            fakeIncomingMqttMessage(gbids[1], sstMsg);
        } catch (Exception e) {
            fail("fake incoming subscription stop failed: " + e);
        }
        return sstCdl;
    }

    private InProcessMessagingStub mockInProcessStub() {
        InProcessMessagingStub inProcessMessagingStubMock = mock(InProcessMessagingStub.class);
        doReturn(inProcessMessagingStubMock).when(inProcessMessagingStubFactorySpy).create(any(InProcessAddress.class));
        return inProcessMessagingStubMock;
    }

    @Test
    public void mqttRqRp_success() {
        createProxyRoutingEntry(replyToAddress);
        CountDownLatch replyCountDownLatch = delayProviderVoidOperation();
        // fake incoming request and check refCounts
        fakeIncomingRequest();
        CountDownLatch outgoingCdl = handleAndCheckOutgoing(mqttMessagingStubMock,
                                                            MessageType.VALUE_MESSAGE_TYPE_REPLY);
        replyCountDownLatch.countDown();

        waitFor(outgoingCdl, DEFAULT_WAIT_TIME);

        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_REPLY, 1);
    }

    @Test
    public void mqttRqRp_error_rpExpired() {
        createProxyRoutingEntry(replyToAddress);
        CountDownLatch replyCountDownLatch = delayProviderVoidOperation();

        defaultMessagingQos.setTtl_ms(512);
        fakeIncomingRequest();
        // delay reply until it is expired
        sleep(defaultMessagingQos.getRoundTripTtl_ms());
        replyCountDownLatch.countDown();

        sleep(500);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
    }

    @Test
    public void mqttRqRp_error_rqExpired() {
        createProxyRoutingEntry(replyToAddress);
        // fake incoming expired request and check refCounts
        defaultMessagingQos.setTtl_ms(0);
        MutableMessage requestMsg = createRequestMsg(proxyParticipantId, FIXEDPARTICIPANTID1);
        CountDownLatch cdl = new CountDownLatch(1);
        // make sure that the message is expired
        sleep(1);
        FailureAction onFailure = error -> {
            assertTrue(JoynrMessageNotSentException.class.isInstance(error));
            assertTrue(error.getMessage().contains("expired"));
            cdl.countDown();
        };
        try {
            IMqttMessagingSkeleton skeleton = (IMqttMessagingSkeleton) mqttSkeletonFactory.getSkeleton(replyToAddress);
            skeleton.transmit(requestMsg.getImmutableMessage().getSerializedMessage(), onFailure);
        } catch (Exception e) {
            fail("fake incoming request failed: " + e);
        }

        // check refCounts and no provider invocation
        waitFor(cdl, DEFAULT_WAIT_TIME);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
        verifyNoMoreInteractions(testProvider);
    }

    @Test
    public void mqttRqRp_error_rpExpiredInMessageWorker() {
        createProxyRoutingEntry(replyToAddress);
        // let the reply message expire in the MessageWorker
        CountDownLatch rpCdl = new CountDownLatch(1);
        defaultMessagingQos.setTtl_ms(512);
        doAnswer(invocation -> {
            ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
            assertEquals(MessageType.VALUE_MESSAGE_TYPE_REPLY, msg.getType());
            assertEquals(proxyParticipantId, msg.getRecipient());
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 2);
            // The message will be rescheduled with the delay from the exception
            // The expiration check in MessageWorker will fail then
            long remainingTtl = msg.getTtlMs() - System.currentTimeMillis();
            FailureAction onFailure = (FailureAction) invocation.getArguments()[2];
            onFailure.execute(new JoynrDelayMessageException(remainingTtl + 1, "test expired reply in MessageWorker"));
            rpCdl.countDown();
            return null;
        }).when(mqttMessagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        // fake incoming request and check refCounts
        fakeIncomingRequest();

        // wait for the reply message
        waitFor(rpCdl, DEFAULT_WAIT_TIME);
        sleep(defaultMessagingQos.getRoundTripTtl_ms() + 100);

        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);

        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_REPLY, 1);
    }

    @Test
    public void mqttRqRp_error_rqExpiredInMessageWorker_noFakeReply() {
        createProxyRoutingEntry(replyToAddress);
        // let the request message expire in the MessageWorker
        CountDownLatch cdl = new CountDownLatch(1);
        defaultMessagingQos.setTtl_ms(512);
        InProcessMessagingStub inProcessMessagingStubMock = mockInProcessStub();
        doAnswer(invocation -> {
            ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
            checkIncominMsgAndRefCounts(true, invocation, MessageType.VALUE_MESSAGE_TYPE_REQUEST);
            // The message will be rescheduled with the delay from the exception
            // The expiration check in MessageWorker will fail then
            long remainingTtl = msg.getTtlMs() - System.currentTimeMillis();
            FailureAction onFailure = (FailureAction) invocation.getArguments()[2];
            onFailure.execute(new JoynrDelayMessageException(remainingTtl + 1,
                                                             "test expired request in MessageWorker"));
            cdl.countDown();
            return null;
        }).when(inProcessMessagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        // fake incoming request and check refCounts
        fakeIncomingRequest();

        // wait for the request message
        waitFor(cdl, DEFAULT_WAIT_TIME);
        sleep(defaultMessagingQos.getRoundTripTtl_ms() + 100);

        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
        verifyNoMoreInteractions(testProvider);
    }

    @Test
    public void mqttRqRp_error_rqErrorFromStub_fakeReplyCreationFails() {
        createProxyRoutingEntry(replyToAddress);
        // exception from stub
        CountDownLatch cdl = new CountDownLatch(1);
        InProcessMessagingStub inProcessMessagingStubMock = mockInProcessStub();
        doAnswer(invocation -> {
            checkIncominMsgAndRefCounts(true, invocation, MessageType.VALUE_MESSAGE_TYPE_REQUEST);
            FailureAction onFailure = (FailureAction) invocation.getArguments()[2];
            // JoynrMessageNotSentException for a request will trigger fake reply creation
            onFailure.execute(new JoynrMessageNotSentException("test fake reply creation fails"));
            cdl.countDown();
            return null;
        }).when(inProcessMessagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        // fake incoming request and check refCounts
        MutableMessage requestMsg = createRequestMsg(proxyParticipantId, FIXEDPARTICIPANTID1);
        // let fake reply creation fail
        doAnswer(invocation -> {
            ImmutableMessage immutableMsg = spy((ImmutableMessage) invocation.getArguments()[0]);
            doThrow(new RuntimeException("force fake reply creation error")).when(immutableMsg).getEffort();
            return immutableMsg;
        }).when(messageProcessorMock).processIncoming(any(ImmutableMessage.class));
        try {
            fakeIncomingMqttMessage(gbids[1], requestMsg);
        } catch (Exception e) {
            fail("fake incoming request failed: " + e);
        }

        // wait for the request message
        waitFor(cdl, DEFAULT_WAIT_TIME);

        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
        verifyNoMoreInteractions(testProvider);
    }

    @Test
    public void mqttRqRp_error_rqErrorFromStub_fakeReply() {
        createProxyRoutingEntry(replyToAddress);
        // exception from stub
        CountDownLatch rqCdl = new CountDownLatch(1);
        InProcessMessagingStub inProcessMessagingStubMock = mockInProcessStub();
        doAnswer(invocation -> {
            checkIncominMsgAndRefCounts(true, invocation, MessageType.VALUE_MESSAGE_TYPE_REQUEST);
            FailureAction onFailure = (FailureAction) invocation.getArguments()[2];
            // JoynrMessageNotSentException for a request will trigger fake reply creation
            onFailure.execute(new JoynrMessageNotSentException("test fake reply creation"));
            rqCdl.countDown();
            return null;
        }).when(inProcessMessagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        CountDownLatch rpCdl = handleAndCheckOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_REPLY);

        // fake incoming request and check refCounts
        fakeIncomingRequest();

        // wait for request and fake reply
        waitFor(rqCdl, DEFAULT_WAIT_TIME);
        waitFor(rpCdl, DEFAULT_WAIT_TIME);

        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
        verifyNoMoreInteractions(testProvider);

        // fake reply message
        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_REPLY, 1);
    }

    @Test
    public void mqttRqRp_error_rqWithRelativeTtl() {
        createProxyRoutingEntry(replyToAddress);
        // fake incoming request with relative ttl and check refCounts
        MutableMessage requestMsg = createRequestMsg(proxyParticipantId, FIXEDPARTICIPANTID1);
        requestMsg.setTtlAbsolute(false);
        CountDownLatch cdl = new CountDownLatch(1);
        FailureAction onFailure = error -> {
            assertTrue(JoynrRuntimeException.class.isInstance(error));
            assertTrue(error.getMessage().contains("Relative ttl not supported"));
            cdl.countDown();
        };
        try {
            IMqttMessagingSkeleton skeleton = (IMqttMessagingSkeleton) mqttSkeletonFactory.getSkeleton(replyToAddress);
            skeleton.transmit(requestMsg.getImmutableMessage().getSerializedMessage(), onFailure);
        } catch (Exception e) {
            fail("fake incoming request failed: " + e);
        }

        waitFor(cdl, DEFAULT_WAIT_TIME);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
        verifyNoMoreInteractions(testProvider);
    }

    @Test
    public void mqttRqRp_error_rqMaxRetryReached() {
        createProxyRoutingEntry(replyToAddress);
        // trigger retries
        CountDownLatch cdl = new CountDownLatch(3);
        InProcessMessagingStub inProcessMessagingStubMock = mockInProcessStub();
        doAnswer(invocation -> {
            checkIncominMsgAndRefCounts(true, invocation, MessageType.VALUE_MESSAGE_TYPE_REQUEST);
            FailureAction onFailure = (FailureAction) invocation.getArguments()[2];
            // The message will be rescheduled with the delay from the exception
            onFailure.execute(new JoynrDelayMessageException(0, "test max retry count"));
            cdl.countDown();
            return null;
        }).when(inProcessMessagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        fakeIncomingRequest();

        // wait for the request message
        waitFor(cdl, DEFAULT_WAIT_TIME);

        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
        verifyNoMoreInteractions(testProvider);
        verify(inProcessMessagingStubMock, times(3)).transmit(any(ImmutableMessage.class),
                                                              any(SuccessAction.class),
                                                              any(FailureAction.class));

        // no reply message
        verifyNoMoreInteractions(mqttMessagingStubMock, inProcessMessagingStubMock);
    }

    @Test
    public void mqttSrqSrp_success_stoppedBySst() {
        createProxyRoutingEntry(replyToAddress);
        // fake incoming subscription request and check refCounts
        CountDownLatch rqCdl = new CountDownLatch(1);
        // check refCounts before subscription request execution in PublicationManager
        doAnswer(factory -> {
            InProcessMessagingStub inProcessMessagingStubSpy = spy((InProcessMessagingStub) factory.callRealMethod());

            doAnswer(invocation -> {
                checkIncominMsgAndRefCounts(true,
                                            invocation,
                                            MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST);

                SuccessAction onSuccess = (SuccessAction) invocation.getArguments()[1];
                invocation.callRealMethod();
                onSuccess.execute();
                checkRefCnt(FIXEDPARTICIPANTID1, 1);
                checkRefCnt(proxyParticipantId, 3);

                rqCdl.countDown();
                return null;
            }).when(inProcessMessagingStubSpy)
              .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

            return inProcessMessagingStubSpy;
        }).when(inProcessMessagingStubFactorySpy).create(any(InProcessAddress.class));

        // Check refCounts after subscription reply or publication
        CountDownLatch rpCdl = new CountDownLatch(1);
        CountDownLatch pubCdl = new CountDownLatch(2);
        doAnswer(invocation -> {
            ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
            assertEquals(proxyParticipantId, msg.getRecipient());
            if (MessageType.VALUE_MESSAGE_TYPE_PUBLICATION.equals(msg.getType())) {
                checkRefCnt(FIXEDPARTICIPANTID1, 1);
                checkRefCnt(proxyParticipantId, 2);
                pubCdl.countDown();
                return null;
            }
            assertEquals(MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY, msg.getType());
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 3);

            SuccessAction action = (SuccessAction) invocation.getArguments()[1];
            action.execute();
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 2);

            rpCdl.countDown();
            return null;
        }).when(mqttMessagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        // fake incoming subscription request and wait for subscription reply
        String subscriptionId = createUuidString();
        long validityMs = 5000;
        fakeIncomingSrq(subscriptionId, validityMs);
        waitFor(rqCdl, DEFAULT_WAIT_TIME);
        waitFor(rpCdl, DEFAULT_WAIT_TIME);

        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY, 1);

        // trigger publications
        testProvider.fireIntBroadcast(42);
        testProvider.fireIntBroadcast(43);
        // wait for publications
        try {
            assertTrue(pubCdl.await(10000, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail("Wait for publication failed: " + e);
        }
        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_PUBLICATION, 2);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 2);

        // fake incoming subscription stop
        CountDownLatch sstCdl = fakeIncomingSst(subscriptionId);
        waitFor(sstCdl, DEFAULT_WAIT_TIME);
        // expect no more publication
        testProvider.fireIntBroadcast(44);
        sleep(200);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
        verifyNoMoreInteractions(mqttMessagingStubMock);
    }

    @Test
    public void mqttSrqSrp_success_stoppedByExpiration() {
        createProxyRoutingEntry(replyToAddress);
        // fake incoming subscription request and check refCounts
        CountDownLatch rqCdl = new CountDownLatch(1);
        // check refCounts before subscription request execution in PublicationManager
        doAnswer(factory -> {
            InProcessMessagingStub inProcessMessagingStubSpy = spy((InProcessMessagingStub) factory.callRealMethod());

            doAnswer(invocation -> {
                checkIncominMsgAndRefCounts(true,
                                            invocation,
                                            MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST);

                SuccessAction onSuccess = (SuccessAction) invocation.getArguments()[1];
                invocation.callRealMethod();
                onSuccess.execute();
                checkRefCnt(FIXEDPARTICIPANTID1, 1);
                checkRefCnt(proxyParticipantId, 3);

                rqCdl.countDown();
                return null;
            }).when(inProcessMessagingStubSpy)
              .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

            return inProcessMessagingStubSpy;
        }).when(inProcessMessagingStubFactorySpy).create(any(InProcessAddress.class));

        // Check refCounts after subscription reply or publication
        CountDownLatch rpCdl = new CountDownLatch(1);
        CountDownLatch pubCdl = new CountDownLatch(2);
        doAnswer(invocation -> {
            ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
            assertEquals(proxyParticipantId, msg.getRecipient());
            if (MessageType.VALUE_MESSAGE_TYPE_PUBLICATION.equals(msg.getType())) {
                checkRefCnt(FIXEDPARTICIPANTID1, 1);
                checkRefCnt(proxyParticipantId, 2);
                pubCdl.countDown();
                return null;
            }
            assertEquals(MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY, msg.getType());
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 3);

            SuccessAction action = (SuccessAction) invocation.getArguments()[1];
            action.execute();
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 2);

            rpCdl.countDown();
            return null;
        }).when(mqttMessagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        // fake incoming subscription request and wait for subscription reply
        String subscriptionId = createUuidString();
        final long validityMs = 753;
        fakeIncomingSrq(subscriptionId, validityMs);
        waitFor(rqCdl, DEFAULT_WAIT_TIME);
        waitFor(rpCdl, DEFAULT_WAIT_TIME);

        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY, 1);

        // trigger publications
        testProvider.fireIntBroadcast(42);
        testProvider.fireIntBroadcast(43);
        // wait for publications
        try {
            assertTrue(pubCdl.await(10000, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail("Sleep/wait for publications failed: " + e);
        }
        sleep(validityMs + 100);
        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_PUBLICATION, 2);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);

        // expect no more publication
        testProvider.fireIntBroadcast(44);
        sleep(200);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
        verifyNoMoreInteractions(mqttMessagingStubMock);
    }

    @Test
    public void mqttSrqSrp_error_srpExpired() {
        createProxyRoutingEntry(replyToAddress);
        // fake incoming subscription request and check refCounts
        CountDownLatch rqCdl = new CountDownLatch(1);
        // check refCounts before subscription request execution in PublicationManager
        doAnswer(factory -> {
            InProcessMessagingStub inProcessMessagingStubSpy = spy((InProcessMessagingStub) factory.callRealMethod());

            doAnswer(invocation -> {
                checkIncominMsgAndRefCounts(true,
                                            invocation,
                                            MessageType.VALUE_MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST);

                SuccessAction onSuccess = (SuccessAction) invocation.getArguments()[1];
                invocation.callRealMethod();
                onSuccess.execute();
                checkRefCnt(FIXEDPARTICIPANTID1, 1);
                checkRefCnt(proxyParticipantId, 3);

                rqCdl.countDown();
                return null;
            }).when(inProcessMessagingStubSpy)
              .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

            return inProcessMessagingStubSpy;
        }).when(inProcessMessagingStubFactorySpy).create(any(InProcessAddress.class));

        // Let subscription reply expire and check refCounts
        CountDownLatch rpCdl = new CountDownLatch(1);
        doAnswer(invocation -> {
            ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
            assertEquals(proxyParticipantId, msg.getRecipient());
            assertEquals(MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY, msg.getType());
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 3);

            FailureAction onFailure = (FailureAction) invocation.getArguments()[2];
            // The message will be rescheduled with the delay from the exception
            // The expiration check in MessageWorker will fail then
            long remainingTtl = msg.getTtlMs() - System.currentTimeMillis();
            onFailure.execute(new JoynrDelayMessageException(remainingTtl + 1, "test expired reply in MessageWorker"));
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 3);

            rpCdl.countDown();
            return null;
        }).when(mqttMessagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        // fake incoming subscription request and wait for subscription reply
        String subscriptionId = createUuidString();
        final long validityMs = 512;
        fakeIncomingSrq(subscriptionId, validityMs);
        waitFor(rqCdl, DEFAULT_WAIT_TIME);
        waitFor(rpCdl, DEFAULT_WAIT_TIME);
        sleep(validityMs + 100);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);

        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY, 1);

        // expect no more publication
        testProvider.fireIntBroadcast(44);
        sleep(200);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
        verifyNoMoreInteractions(mqttMessagingStubMock);
    }

    @Test
    public void mqttOneWay_success() {
        createProxyRoutingEntry(replyToAddress);
        // fake incoming one way request and check refCounts
        CountDownLatch rqCdl = new CountDownLatch(1);
        doAnswer(factory -> {
            InProcessMessagingStub inProcessMessagingStubSpy = spy((InProcessMessagingStub) factory.callRealMethod());

            doAnswer(invocation -> {
                checkIncominMsgAndRefCounts(false, invocation, MessageType.VALUE_MESSAGE_TYPE_ONE_WAY);

                SuccessAction onSuccess = (SuccessAction) invocation.getArguments()[1];
                invocation.callRealMethod();
                onSuccess.execute();
                checkRefCnt(FIXEDPARTICIPANTID1, 1);
                checkRefCnt(proxyParticipantId, 1);

                rqCdl.countDown();
                return null;
            }).when(inProcessMessagingStubSpy)
              .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

            return inProcessMessagingStubSpy;
        }).when(inProcessMessagingStubFactorySpy).create(any(InProcessAddress.class));

        OneWayRequest request = new OneWayRequest("methodFireAndForgetWithoutParams", new Object[0], new Class[0]);
        MutableMessage requestMsg = messageFactory.createOneWayRequest(proxyParticipantId,
                                                                       FIXEDPARTICIPANTID1,
                                                                       request,
                                                                       defaultMessagingQos);
        String replyTo = RoutingTypesUtil.toAddressString(replyToAddress);
        requestMsg.setReplyTo(replyTo);
        try {
            fakeIncomingMqttMessage(gbids[1], requestMsg);
        } catch (Exception e) {
            fail("fake incoming one way request failed: " + e);
        }
        waitFor(rqCdl, DEFAULT_WAIT_TIME);

        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
    }

    @Test
    public void mqttMrqSrq_success() {
        createProxyRoutingEntry(replyToAddress);
        String multicastName = "emptyBroadcast";
        String multicastId = MulticastIdUtil.createMulticastId(FIXEDPARTICIPANTID1, multicastName, new String[0]);

        // fake incoming subscription request and check refCounts
        CountDownLatch rqCdl = new CountDownLatch(1);
        // check refCounts before subscription request execution in PublicationManager
        doAnswer(factory -> {
            InProcessMessagingStub inProcessMessagingStubSpy = spy((InProcessMessagingStub) factory.callRealMethod());

            doAnswer(invocation -> {
                checkIncominMsgAndRefCounts(true,
                                            invocation,
                                            MessageType.VALUE_MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST);

                SuccessAction onSuccess = (SuccessAction) invocation.getArguments()[1];
                invocation.callRealMethod();
                onSuccess.execute();
                checkRefCnt(FIXEDPARTICIPANTID1, 1);
                checkRefCnt(proxyParticipantId, 3);

                rqCdl.countDown();
                return null;
            }).when(inProcessMessagingStubSpy)
              .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

            return inProcessMessagingStubSpy;
        }).when(inProcessMessagingStubFactorySpy).create(any(InProcessAddress.class));

        // Check refCounts after subscription reply or publication
        CountDownLatch rpCdl = new CountDownLatch(1);
        // multicast is published to all GBIDs
        CountDownLatch pubCdl = new CountDownLatch(4);
        doAnswer(invocation -> {
            ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
            if (MessageType.VALUE_MESSAGE_TYPE_MULTICAST.equals(msg.getType())) {
                assertEquals(multicastId, msg.getRecipient());
                checkRefCnt(FIXEDPARTICIPANTID1, 1);
                checkRefCnt(proxyParticipantId, 2);
                pubCdl.countDown();
                return null;
            }
            assertEquals(proxyParticipantId, msg.getRecipient());
            assertEquals(MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY, msg.getType());
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 3);

            SuccessAction action = (SuccessAction) invocation.getArguments()[1];
            action.execute();
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 2);

            rpCdl.countDown();
            return null;
        }).when(mqttMessagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        // fake incoming subscription request and wait for subscription reply
        String subscriptionId = createUuidString();
        long validityMs = 5000;
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos().setMinIntervalMs(0).setValidityMs(validityMs);
        MulticastSubscriptionRequest request = new MulticastSubscriptionRequest(multicastId,
                                                                                subscriptionId,
                                                                                multicastName,
                                                                                qos);
        MutableMessage requestMsg = messageFactory.createSubscriptionRequest(proxyParticipantId,
                                                                             FIXEDPARTICIPANTID1,
                                                                             request,
                                                                             defaultMessagingQos);
        String replyTo = RoutingTypesUtil.toAddressString(replyToAddress);
        requestMsg.setReplyTo(replyTo);
        try {
            fakeIncomingMqttMessage(gbids[1], requestMsg);
        } catch (Exception e) {
            fail("fake incoming subscription request failed: " + e);
        }
        waitFor(rqCdl, DEFAULT_WAIT_TIME);
        waitFor(rpCdl, DEFAULT_WAIT_TIME);

        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY, 1);

        // trigger publications
        testProvider.fireEmptyBroadcast();
        testProvider.fireEmptyBroadcast();
        // wait for publications
        try {
            assertTrue(pubCdl.await(10000, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail("Wait for publication failed: " + e);
        }
        // multicast is published to all GBIDs
        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_MULTICAST, 4);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 2);

        // fake incoming subscription stop
        CountDownLatch sstCdl = fakeIncomingSst(subscriptionId);
        waitFor(sstCdl, DEFAULT_WAIT_TIME);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
        verifyNoMoreInteractions(mqttMessagingStubMock);
    }

    @Test
    public void mqttAttributeSrqSrp_success() {
        createProxyRoutingEntry(replyToAddress);
        testProvider.setEnumAttribute(TestEnum.ONE);

        // fake incoming subscription request and check refCounts
        CountDownLatch rqCdl = new CountDownLatch(1);
        // check refCounts before subscription request execution in PublicationManager
        doAnswer(factory -> {
            InProcessMessagingStub inProcessMessagingStubSpy = spy((InProcessMessagingStub) factory.callRealMethod());

            doAnswer(invocation -> {
                checkIncominMsgAndRefCounts(true, invocation, MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST);

                SuccessAction onSuccess = (SuccessAction) invocation.getArguments()[1];
                invocation.callRealMethod();
                onSuccess.execute();
                checkRefCnt(FIXEDPARTICIPANTID1, 1);
                checkRefCnt(proxyParticipantId, 3);

                rqCdl.countDown();
                return null;
            }).when(inProcessMessagingStubSpy)
              .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

            return inProcessMessagingStubSpy;
        }).when(inProcessMessagingStubFactorySpy).create(any(InProcessAddress.class));

        // Check refCounts after subscription reply or publication
        CountDownLatch rpCdl = new CountDownLatch(1);
        CountDownLatch pubCdl = new CountDownLatch(2);
        doAnswer(invocation -> {
            ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
            assertEquals(proxyParticipantId, msg.getRecipient());
            if (MessageType.VALUE_MESSAGE_TYPE_PUBLICATION.equals(msg.getType())) {
                checkRefCnt(FIXEDPARTICIPANTID1, 1);
                pubCdl.countDown();
                return null;
            }
            assertEquals(MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY, msg.getType());
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 3);

            SuccessAction action = (SuccessAction) invocation.getArguments()[1];
            action.execute();
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
            checkRefCnt(proxyParticipantId, 2);

            rpCdl.countDown();
            return null;
        }).when(mqttMessagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));

        // fake incoming subscription request and wait for subscription reply
        String subscriptionId = createUuidString();
        long validityMs = 5000;
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos().setMinIntervalMs(0).setValidityMs(validityMs);
        SubscriptionRequest request = new SubscriptionRequest(subscriptionId, "enumAttribute", qos);
        MutableMessage requestMsg = messageFactory.createSubscriptionRequest(proxyParticipantId,
                                                                             FIXEDPARTICIPANTID1,
                                                                             request,
                                                                             defaultMessagingQos);
        String replyTo = RoutingTypesUtil.toAddressString(replyToAddress);
        requestMsg.setReplyTo(replyTo);
        try {
            fakeIncomingMqttMessage(gbids[1], requestMsg);
        } catch (Exception e) {
            fail("fake incoming subscription request failed: " + e);
        }
        waitFor(rqCdl, DEFAULT_WAIT_TIME);
        waitFor(rpCdl, DEFAULT_WAIT_TIME);

        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REPLY, 1);

        // trigger publications
        testProvider.enumAttributeChanged(TestEnum.TWO);
        // wait for publications
        try {
            assertTrue(pubCdl.await(10000, TimeUnit.MILLISECONDS));
        } catch (Throwable e) {
            fail("Wait for publication failed: " + e);
        }
        verifyOutgoing(mqttMessagingStubMock, MessageType.VALUE_MESSAGE_TYPE_PUBLICATION, 2);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 2);

        // fake incoming subscription stop
        CountDownLatch sstCdl = fakeIncomingSst(subscriptionId);
        waitFor(sstCdl, DEFAULT_WAIT_TIME);
        // expect no more publication
        testProvider.enumAttributeChanged(TestEnum.ZERO);
        sleep(200);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        checkRefCnt(proxyParticipantId, 1);
        verifyNoMoreInteractions(mqttMessagingStubMock);
    }

}
