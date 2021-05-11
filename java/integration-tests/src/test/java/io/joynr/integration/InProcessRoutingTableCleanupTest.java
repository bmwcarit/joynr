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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.arbitration.ArbitrationStrategyFunction;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingStub;
import io.joynr.proxy.GuidedProxyBuilder;
import io.joynr.proxy.ProxyBuilder;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.Message.MessageType;
import joynr.MutableMessage;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProxy;
import joynr.types.DiscoveryError;

/**
 * Test RoutingTable reference count handling in CC with proxies and providers within the CC runtime.
 *
 */
@RunWith(MockitoJUnitRunner.class)
public class InProcessRoutingTableCleanupTest extends AbstractRoutingTableCleanupTest {

    @Test
    public void createAndDestroyProxy_routingEntryAddedAndRemoved() throws InterruptedException {
        // register provider
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        Thread.sleep(1l);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        // Save proxy's participant id
        String proxyParticipantId = proxyBuilder.getParticipantId();

        // create proxy
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQosLocal);
        assertNotNull(proxy);

        // Check if proxy is contained in routing table
        checkRefCnt(proxyParticipantId, 1l);
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);

        // set proxy and proxyBuilder to null
        proxy = null;
        proxyBuilder = null;

        waitForGarbageCollection(proxyParticipantId);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void unregisterGlobalProvider_success_decreaseRefCountOnlyOnce() throws InterruptedException {
        // register provider globally
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerGlobal(testProvider, TESTCUSTOMDOMAIN1, providerQosGlobal);

        // build a proxy to increase the refCount of the provider routing entry so that unregisterProvider does not remove the entry
        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQosGlobal);
        assertNotNull(proxy);

        checkRefCnt(FIXEDPARTICIPANTID1, 2);

        CountDownLatch rqCdl = new CountDownLatch(1);
        ArgumentCaptor<ImmutableMessage> messageCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        doAnswer(createVoidCountDownAnswer(rqCdl)).when(mqttMessagingStubMock)
                                                  .transmit(messageCaptor.capture(),
                                                            any(SuccessAction.class),
                                                            any(FailureAction.class));

        // unregister the provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);

        try {
            // Wait for a while until providers are unregistered (locally)
            Thread.sleep(200);
            assertTrue(rqCdl.await(1500, TimeUnit.MILLISECONDS));
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
        } catch (InterruptedException e) {
            fail(e.toString());
        }

        verify(mqttMessagingStubMock).transmit(any(ImmutableMessage.class),
                                               any(SuccessAction.class),
                                               any(FailureAction.class));

        ImmutableMessage rqMsg = messageCaptor.getValue();
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST, rqMsg.getType());
        assertEquals(getGcdParticipantId(), rqMsg.getRecipient());

        // fake success reply and wait for its delivery to LCD
        String gcdProxyParticipantId = rqMsg.getSender();
        CountDownLatch rpCdl = new CountDownLatch(1);
        doAnswer(new Answer<IMessagingStub>() {
            @Override
            public IMessagingStub answer(InvocationOnMock invocation) throws Throwable {
                InProcessMessagingStub inProcessMessagingStubSpy = spy((InProcessMessagingStub) invocation.callRealMethod());
                doAnswer(new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
                        assertEquals(getGcdParticipantId(), msg.getSender());
                        assertEquals(gcdProxyParticipantId, msg.getRecipient());
                        assertEquals(MessageType.VALUE_MESSAGE_TYPE_REPLY, msg.getType());
                        invocation.callRealMethod();
                        SuccessAction action = (SuccessAction) invocation.getArguments()[1];
                        action.execute();
                        checkRefCnt(FIXEDPARTICIPANTID1, 1);
                        rpCdl.countDown();
                        return null;
                    }
                }).when(inProcessMessagingStubSpy)
                  .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));
                return inProcessMessagingStubSpy;
            }
        }).when(inProcessMessagingStubFactorySpy).create(any(InProcessAddress.class));

        try {
            MutableMessage rpMsg = createVoidReply(rqMsg);
            fakeIncomingMqttMessage(gbids[0], rpMsg);
            assertTrue(rpCdl.await(10, TimeUnit.SECONDS));
        } catch (Exception e) {
            fail("Provider unregistration failed: " + e.toString());
        }

        assertTrue(routingTable.containsKey(FIXEDPARTICIPANTID1));
        verifyNoMoreInteractions(mqttMessagingStubMock);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        reset(inProcessMessagingStubFactorySpy);
    }

    @Test
    public void unregisterGlobalProvider_exception_timeoutThenExceptionFromGcd_decreaseRefCountOnlyOnce() throws InterruptedException {
        // register provider globally
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerGlobal(testProvider, TESTCUSTOMDOMAIN1, providerQosGlobal);

        // build a proxy to increase the refCount of the provider routing entry so that unregisterProvider does not remove the entry
        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQosGlobal);
        assertNotNull(proxy);

        checkRefCnt(FIXEDPARTICIPANTID1, 2);

        CountDownLatch rqCdl1 = new CountDownLatch(1);
        ArgumentCaptor<ImmutableMessage> messageCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        doAnswer(createVoidCountDownAnswer(rqCdl1)).when(mqttMessagingStubMock)
                                                   .transmit(messageCaptor.capture(),
                                                             any(SuccessAction.class),
                                                             any(FailureAction.class));

        // unregister the provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);

        try {
            // Wait for a while until providers are unregistered (locally)
            Thread.sleep(200);
            assertTrue(rqCdl1.await(1500, TimeUnit.MILLISECONDS));
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
        } catch (InterruptedException e) {
            fail(e.toString());
        }

        verify(mqttMessagingStubMock).transmit(any(ImmutableMessage.class),
                                               any(SuccessAction.class),
                                               any(FailureAction.class));

        ImmutableMessage rq1 = messageCaptor.getValue();
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST, rq1.getType());
        assertEquals(getGcdParticipantId(), rq1.getRecipient());

        // fake failed reply because of an exception contained in the message
        Semaphore rpSemaphore = new Semaphore(0);
        doAnswer(new Answer<IMessagingStub>() {
            String gcdProxyParticipantId = rq1.getSender();

            @Override
            public IMessagingStub answer(InvocationOnMock invocation) throws Throwable {
                InProcessMessagingStub inProcessMessagingStubSpy = spy((InProcessMessagingStub) invocation.callRealMethod());
                doAnswer(new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
                        assertEquals(getGcdParticipantId(), msg.getSender());
                        assertEquals(gcdProxyParticipantId, msg.getRecipient());
                        invocation.callRealMethod();
                        SuccessAction action = (SuccessAction) invocation.getArguments()[1];
                        action.execute();
                        checkRefCnt(FIXEDPARTICIPANTID1, 1);
                        rpSemaphore.release();
                        return null;
                    }
                }).when(inProcessMessagingStubSpy)
                  .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));
                return inProcessMessagingStubSpy;
            }
        }).when(inProcessMessagingStubFactorySpy).create(any(InProcessAddress.class));

        // for the second try
        CountDownLatch rqCdl2 = new CountDownLatch(1);
        doAnswer(createVoidCountDownAnswer(rqCdl2)).when(mqttMessagingStubMock)
                                                   .transmit(messageCaptor.capture(),
                                                             any(SuccessAction.class),
                                                             any(FailureAction.class));

        try {
            MutableMessage rpMsg1 = createReplyWithException(rq1,
                                                             new JoynrTimeoutException(System.currentTimeMillis()));
            fakeIncomingMqttMessage(gbids[0], rpMsg1);

            assertTrue(rpSemaphore.tryAcquire(10000, TimeUnit.MILLISECONDS));

            // second request
            assertTrue(rqCdl2.await(1500, TimeUnit.MILLISECONDS));

            verify(mqttMessagingStubMock, times(2)).transmit(any(ImmutableMessage.class),
                                                             any(SuccessAction.class),
                                                             any(FailureAction.class));

            // second reply
            ImmutableMessage rq2 = messageCaptor.getValue();
            MutableMessage rpMsg2 = createReplyWithException(rq2, new ProviderRuntimeException("failed"));
            fakeIncomingMqttMessage(gbids[0], rpMsg2);

            assertTrue(rpSemaphore.tryAcquire(10000, TimeUnit.MILLISECONDS));
        } catch (Exception e) {
            fail("Provider unregistration failed: " + e.toString());
        }

        verifyNoMoreInteractions(mqttMessagingStubMock);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        reset(inProcessMessagingStubFactorySpy);
    }

    private void unregisterGlobalProvider_DiscoveryError_decreaseRefCountOnlyOnce(DiscoveryError error) {
        // register provider globally
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerGlobal(testProvider, TESTCUSTOMDOMAIN1, providerQosGlobal);

        // increment the refCount of the provider routing entry so that unregisterProvider does not remove the entry
        routingTable.incrementReferenceCount(FIXEDPARTICIPANTID1);
        checkRefCnt(FIXEDPARTICIPANTID1, 2);

        CountDownLatch rqCdl1 = new CountDownLatch(1);
        ArgumentCaptor<ImmutableMessage> messageCaptor = ArgumentCaptor.forClass(ImmutableMessage.class);
        doAnswer(createVoidCountDownAnswer(rqCdl1)).when(mqttMessagingStubMock)
                                                   .transmit(messageCaptor.capture(),
                                                             any(SuccessAction.class),
                                                             any(FailureAction.class));

        // unregister the provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);

        try {
            // Wait for a while until providers are unregistered (locally)
            Thread.sleep(200);
            assertTrue(rqCdl1.await(1500, TimeUnit.MILLISECONDS));
            checkRefCnt(FIXEDPARTICIPANTID1, 1);
        } catch (InterruptedException e) {
            fail(e.toString());
        }

        verify(mqttMessagingStubMock).transmit(any(ImmutableMessage.class),
                                               any(SuccessAction.class),
                                               any(FailureAction.class));

        ImmutableMessage rq1 = messageCaptor.getValue();
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST, rq1.getType());
        assertEquals(getGcdParticipantId(), rq1.getRecipient());

        // fake failed reply because of an exception contained in the message
        Semaphore rpSemaphore = new Semaphore(0);
        doAnswer(new Answer<IMessagingStub>() {
            String gcdProxyParticipantId = rq1.getSender();

            @Override
            public IMessagingStub answer(InvocationOnMock invocation) throws Throwable {
                InProcessMessagingStub inProcessMessagingStubSpy = spy((InProcessMessagingStub) invocation.callRealMethod());
                doAnswer(new Answer<Void>() {
                    @Override
                    public Void answer(InvocationOnMock invocation) throws Throwable {
                        ImmutableMessage msg = (ImmutableMessage) invocation.getArguments()[0];
                        assertEquals(getGcdParticipantId(), msg.getSender());
                        assertEquals(gcdProxyParticipantId, msg.getRecipient());
                        invocation.callRealMethod();
                        SuccessAction action = (SuccessAction) invocation.getArguments()[1];
                        action.execute();
                        checkRefCnt(FIXEDPARTICIPANTID1, 1);
                        rpSemaphore.release();
                        return null;
                    }
                }).when(inProcessMessagingStubSpy)
                  .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));
                return inProcessMessagingStubSpy;
            }
        }).when(inProcessMessagingStubFactorySpy).create(any(InProcessAddress.class));

        // for the second try
        CountDownLatch rqCdl2 = new CountDownLatch(1);
        doAnswer(createVoidCountDownAnswer(rqCdl2)).when(mqttMessagingStubMock)
                                                   .transmit(messageCaptor.capture(),
                                                             any(SuccessAction.class),
                                                             any(FailureAction.class));

        try {
            MutableMessage rpMsg1 = createReplyWithException(rq1,
                                                             new JoynrTimeoutException(System.currentTimeMillis()));
            fakeIncomingMqttMessage(gbids[0], rpMsg1);

            assertTrue(rpSemaphore.tryAcquire(10000, TimeUnit.MILLISECONDS));

            // second request
            assertTrue(rqCdl2.await(1500, TimeUnit.MILLISECONDS));

            verify(mqttMessagingStubMock, times(2)).transmit(any(ImmutableMessage.class),
                                                             any(SuccessAction.class),
                                                             any(FailureAction.class));

            // second reply
            ImmutableMessage rq2 = messageCaptor.getValue();
            MutableMessage rpMsg2 = createReplyWithException(rq2, new ApplicationException(error));
            fakeIncomingMqttMessage(gbids[0], rpMsg2);

            assertTrue(rpSemaphore.tryAcquire(10000, TimeUnit.MILLISECONDS));
        } catch (Exception e) {
            fail("Provider unregistration failed: " + e.toString());
        }

        verifyNoMoreInteractions(mqttMessagingStubMock);
        checkRefCnt(FIXEDPARTICIPANTID1, 1);
        routingTable.remove(FIXEDPARTICIPANTID1);
        reset(inProcessMessagingStubFactorySpy);
    }

    @Test
    public void unregisterGlobalProvider_error_decreaseRefCountOnlyOnce() throws InterruptedException {
        unregisterGlobalProvider_DiscoveryError_decreaseRefCountOnlyOnce(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        unregisterGlobalProvider_DiscoveryError_decreaseRefCountOnlyOnce(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
        unregisterGlobalProvider_DiscoveryError_decreaseRefCountOnlyOnce(DiscoveryError.INVALID_GBID);
        unregisterGlobalProvider_DiscoveryError_decreaseRefCountOnlyOnce(DiscoveryError.UNKNOWN_GBID);
        unregisterGlobalProvider_DiscoveryError_decreaseRefCountOnlyOnce(DiscoveryError.INTERNAL_ERROR);
    }

    @Test
    public void createAndDestroyMultiProxy_routingEntryAddedAndRemoved() throws InterruptedException {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID3));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN3, providerQosLocal);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // Define custom arbitration strategy function to get multiple providers
        ArbitrationStrategyFunction arbitrationStrategyFunction = customArbitrationStrategyFor(new HashSet<>(Arrays.asList(FIXEDPARTICIPANTID1,
                                                                                                                           FIXEDPARTICIPANTID2)));

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2, TESTCUSTOMDOMAIN3));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        // Save proxy's participant id
        String proxyParticipantId = proxyBuilder.getParticipantId();

        // create proxy
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     arbitrationStrategyFunction,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_ONLY);
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQos);
        assertNotNull(proxy);

        // Check if proxy is contained in routing table
        checkRefCnt(proxyParticipantId, 1l);

        // Check refCount values of routing entries of fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // set proxy and proxyBuilder to null
        proxy = null;
        proxyBuilder = null;

        waitForGarbageCollection(proxyParticipantId);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void registerAndUnregisterProviders_local() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);

        // Check reference count values of providers
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);

        // Wait for a while until providers are unregistered
        try {
            Thread.sleep(200);
        } catch (Exception e) {
            fail("Sleeping failed: " + e.toString());
        }

        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));
    }

    @Test
    public void registerAndUnregisterProviders_global() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerGlobal(testProvider, TESTCUSTOMDOMAIN1, providerQosGlobal);
        registerGlobal(testProvider, TESTCUSTOMDOMAIN2, providerQosGlobal);

        // Check reference count values of providers
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        waitForGlobalRemove();
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
        waitForGlobalRemove();

        // Wait for a while until global remove has finished (reply processed at LCD)
        try {
            Thread.sleep(200);
        } catch (Exception e) {
            fail("Sleeping failed: " + e.toString());
        }

        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));
    }

    @Test
    public void registerProviders_createProxy_refCountForSelectedProviderIncremented() throws InterruptedException {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        Thread.sleep(1l);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        // create proxy
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQosLocal);
        assertNotNull(proxy);

        // Check refCount values of routing entries for fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void registerProviders_createMultiProxy_refCountForSelectedProviderIncremented() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID3));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN3, providerQosLocal);

        // Check refCount values of routing entries of fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // Define custom arbitration strategy function to get multiple providers
        ArbitrationStrategyFunction arbitrationStrategyFunction = customArbitrationStrategyFor(new HashSet<>(Arrays.asList(FIXEDPARTICIPANTID1,
                                                                                                                           FIXEDPARTICIPANTID2)));

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2, TESTCUSTOMDOMAIN3));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        // create proxy
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     arbitrationStrategyFunction,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_ONLY);
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQos);
        assertNotNull(proxy);

        // Check refCount values of routing entries of fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void callProxyOperation_refCountOfProviderAndProxyIsTheSame() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);

        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(TESTCUSTOMDOMAIN1, testProxy.class);

        // Save proxy's participant id
        String proxyParticipantId = proxyBuilder.getParticipantId();

        // create proxy
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQosLocal);

        // Check if proxy is contained in routing table
        checkRefCnt(proxyParticipantId, 1l);
        // Check refCount value of routing entries for fixed provider
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);

        // Perform any proxy operation
        proxy.addNumbers(10, 20, 30);

        // Check if the refCount values of proxy and provider are the same
        checkRefCnt(proxyParticipantId, 1l);
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
    }

    @Test
    public void callMultiProxyOperation_refCountOfProviderAndProxyIsTheSame() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID3));

        // register providers
        DefaulttestProvider testProvider = spy(new DefaulttestProvider());

        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN3, providerQosLocal);

        // Check refCount values of routing entries of fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // Define custom arbitration strategy function to get multiple providers
        ArbitrationStrategyFunction arbitrationStrategyFunction = customArbitrationStrategyFor(new HashSet<>(Arrays.asList(FIXEDPARTICIPANTID1,
                                                                                                                           FIXEDPARTICIPANTID2)));

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2, TESTCUSTOMDOMAIN3));
        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(domains, testProxy.class);

        // Save proxy's participant id
        String proxyParticipantId = proxyBuilder.getParticipantId();

        // create proxy
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     arbitrationStrategyFunction,
                                                     Long.MAX_VALUE,
                                                     DiscoveryScope.LOCAL_ONLY);
        testProxy proxy = createProxy(proxyBuilder, defaultMessagingQos, discoveryQos);

        // Check if proxy is contained in routing table
        checkRefCnt(proxyParticipantId, 1l);
        // Check refCount values of routing entries of fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        CountDownLatch cdl = new CountDownLatch(1);
        doAnswer((invocation) -> {
            checkRefCnt(proxyParticipantId, 1l);
            checkRefCnt(FIXEDPARTICIPANTID1, 2l);
            checkRefCnt(FIXEDPARTICIPANTID2, 2l);
            checkRefCnt(FIXEDPARTICIPANTID3, 1l);
            Void result = (Void) invocation.callRealMethod();
            cdl.countDown();
            return result;
        }).when(testProvider).methodFireAndForgetWithoutParams();

        // Perform any proxy operation
        proxy.methodFireAndForgetWithoutParams();
        try {
            assertTrue(cdl.await(10000, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail(e.toString());
        }

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void useGuidedProxyBuilder_createAndDestroyProxy_routingEntryAddedAndRemoved() throws InterruptedException {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        Set<String> domains = new HashSet<String>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2));
        GuidedProxyBuilder guidedProxyBuilder = joynrRuntime.getGuidedProxyBuilder(domains, testProxy.class);

        // create proxy
        guidedProxyBuilder.setDiscoveryQos(discoveryQosLocal).setMessagingQos(defaultMessagingQos).discover();

        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);

        testProxy proxy = guidedProxyBuilder.buildProxy(testProxy.class, FIXEDPARTICIPANTID1);
        assertNotNull(proxy);

        // Check whether routing entry for built proxy has been created
        assertFalse(sProxyParticipantId.isEmpty());
        checkRefCnt(sProxyParticipantId, 1l);
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        // set proxy and proxyBuilder to null
        proxy = null;
        guidedProxyBuilder = null;

        waitForGarbageCollection(sProxyParticipantId);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void useGuidedProxyBuilder_registerProviders_createProxy_refCountForSelectedProviderIncremented() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);

        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2));
        GuidedProxyBuilder guidedProxyBuilder = joynrRuntime.getGuidedProxyBuilder(domains, testProxy.class);

        // create proxy
        guidedProxyBuilder.setDiscoveryQos(discoveryQosLocal).setMessagingQos(defaultMessagingQos).discover();

        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);

        testProxy proxy = guidedProxyBuilder.buildProxy(testProxy.class, FIXEDPARTICIPANTID1);
        assertNotNull(proxy);

        // Check refCount values of routing entries for fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
    }

    @Test
    public void useGuidedProxyBuilder_callProxyOperation_refCountOfProviderAndProxyIsTheSame() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1));
        GuidedProxyBuilder guidedProxyBuilder = joynrRuntime.getGuidedProxyBuilder(domains, testProxy.class);

        // create proxy
        guidedProxyBuilder.setDiscoveryQos(discoveryQosLocal).setMessagingQos(defaultMessagingQos).discover();
        testProxy proxy = guidedProxyBuilder.buildProxy(testProxy.class, FIXEDPARTICIPANTID1);

        // Check whether routing entry for built proxy has been created
        assertFalse(sProxyParticipantId.isEmpty());
        checkRefCnt(sProxyParticipantId, 1l);

        // Get refCount values of routing entries for fixed providers
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);

        // Perform any proxy operation
        proxy.addNumbers(10, 20, 30);

        // Check if the refCount values of proxy and provider are the same
        checkRefCnt(sProxyParticipantId, 1l);
        checkRefCnt(FIXEDPARTICIPANTID1, 2l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
    }

    @Test
    public void useGuidedProxyBuilder_discoverAndBuildNone_routingEntryOfProxyNotCreated() {
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID1));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID2));
        assertFalse(routingTable.containsKey(FIXEDPARTICIPANTID3));

        // register providers
        DefaulttestProvider testProvider = new DefaulttestProvider();
        registerProvider(testProvider, TESTCUSTOMDOMAIN1, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN2, providerQosLocal);
        registerProvider(testProvider, TESTCUSTOMDOMAIN3, providerQosLocal);

        // Get refCount values of routing entries for fixed provider
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        Set<String> domains = new HashSet<>(Arrays.asList(TESTCUSTOMDOMAIN1, TESTCUSTOMDOMAIN2, TESTCUSTOMDOMAIN3));
        GuidedProxyBuilder guidedProxyBuilder = joynrRuntime.getGuidedProxyBuilder(domains, testProxy.class);

        // perform discovery
        guidedProxyBuilder.setDiscoveryQos(discoveryQosLocal).setMessagingQos(defaultMessagingQos).discover();

        // Check whether routing entry for proxy has not been created
        assertTrue(sProxyParticipantId.isEmpty());

        checkRefCnt(FIXEDPARTICIPANTID1, 2l);
        checkRefCnt(FIXEDPARTICIPANTID2, 2l);
        checkRefCnt(FIXEDPARTICIPANTID3, 2l);

        // build none
        guidedProxyBuilder.buildNone();

        // Check whether number of routing entries has not been changed
        assertTrue(sProxyParticipantId.isEmpty());
        checkRefCnt(FIXEDPARTICIPANTID1, 1l);
        checkRefCnt(FIXEDPARTICIPANTID2, 1l);
        checkRefCnt(FIXEDPARTICIPANTID3, 1l);

        // unregister provider
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN1, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN2, testProvider);
        joynrRuntime.unregisterProvider(TESTCUSTOMDOMAIN3, testProvider);
    }
}
