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
package io.joynr.proxy;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledThreadPoolExecutor;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.Async;
import io.joynr.Sync;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.dispatcher.rpc.JoynrBroadcastSubscriptionInterface;
import io.joynr.dispatcher.rpc.annotation.FireAndForget;
import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.GarbageCollectionHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.exceptions.ApplicationException;
import joynr.types.DiscoveryEntryWithMetaInfo;

@RunWith(MockitoJUnitRunner.class)
public class ProxyInvocationHandlerTest {
    private MessagingQos messagingQos = new MessagingQos();
    private DiscoveryQos discoveryQos = new DiscoveryQos();
    private String proxyParticipantId = "proxyParticipantId";
    private String interfaceName = "interfaceName";
    private String domain = "domain";
    private Object proxy;

    @Mock
    private ConnectorFactory mockConnectorFactory;

    @Mock
    private MessageRouter mockMessageRouter;
    @Mock
    private GarbageCollectionHandler mockGcHandler;

    @Mock
    private ShutdownNotifier mockShutdownNotifier;
    private ShutdownListener shutdownListener;

    @Mock
    private StatelessAsyncIdCalculator mockStatelessAsyncIdCalculator;

    private ProxyInvocationHandlerImpl proxyInvocationHandler;

    private final ExecutorService threadPool = new ScheduledThreadPoolExecutor(2);

    public static interface TestSyncInterface {
        public void testMethod();
    }

    @FireAndForget
    private static interface TestServiceFireAndForget {
        void callMe(String message);
    }

    @Sync
    private static interface TestServiceSync extends TestServiceFireAndForget {
        String testSyncMethod(String inputData);
    }

    @Async
    private interface TestServiceAsync extends TestServiceFireAndForget {
        Promise<DeferredVoid> testAsyncMethod(String inputData);
    }

    @Before
    public void setup() {
        proxy = new Object();
        StatelessAsyncCallback mockStatelessAsyncCallback = Mockito.mock(StatelessAsyncCallback.class);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                shutdownListener = (ShutdownListener) invocation.getArguments()[0];
                return null;
            }
        }).when(mockShutdownNotifier).registerForShutdown(any());
        proxyInvocationHandler = new ProxyInvocationHandlerImpl(new HashSet<String>(Arrays.asList(domain)),
                                                                interfaceName,
                                                                proxyParticipantId,
                                                                discoveryQos,
                                                                messagingQos,
                                                                Optional.of(mockStatelessAsyncCallback),
                                                                mockConnectorFactory,
                                                                mockMessageRouter,
                                                                mockGcHandler,
                                                                mockShutdownNotifier,
                                                                mockStatelessAsyncIdCalculator);
    }

    @Test(timeout = 3000)
    public void callProxyInvocationHandlerSyncFromMultipleThreadsTest() throws Throwable {

        Future<?> call1 = threadPool.submit(new Callable<Object>() {
            @Override
            public Object call() throws Exception {
                Object result = null;
                try {
                    result = proxyInvocationHandler.invokeInternal(proxy,
                                                                   TestSyncInterface.class.getDeclaredMethod("testMethod",
                                                                                                             new Class<?>[]{}),
                                                                   new Object[]{});
                } catch (Exception e) {
                }

                return result;
            }
        });

        Future<?> call2 = threadPool.submit(new Callable<Object>() {
            @Override
            public Object call() throws Exception {
                Object result = null;
                try {
                    result = proxyInvocationHandler.invokeInternal(proxy,
                                                                   TestSyncInterface.class.getDeclaredMethod("testMethod",
                                                                                                             new Class<?>[]{}),
                                                                   new Object[]{});
                } catch (Exception e) {
                }
                return result;
            }
        });

        ArbitrationResult arbitrationResult = new ArbitrationResult();
        DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo();
        discoveryEntry.setParticipantId("participantId");
        arbitrationResult.setDiscoveryEntries(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(discoveryEntry)));
        when(mockConnectorFactory.create(Mockito.anyString(),
                                         Mockito.<ArbitrationResult> any(),
                                         any(),
                                         any())).thenReturn(Optional.empty());
        proxyInvocationHandler.createConnector(arbitrationResult);

        // if the bug that causes one thread to hang in arbitration exists, one
        // of these calls will never return, causing the test to timeout and fail
        call1.get();
        call2.get();

    }

    @Test
    public void testCallFireAndForgetMethod() throws Throwable {
        ConnectorInvocationHandler connectorInvocationHandler = mock(ConnectorInvocationHandler.class);
        when(mockConnectorFactory.create(Mockito.anyString(),
                                         Mockito.<ArbitrationResult> any(),
                                         Mockito.eq(messagingQos),
                                         Mockito.eq(null))).thenReturn(Optional.of(connectorInvocationHandler));
        Method fireAndForgetMethod = TestServiceSync.class.getMethod("callMe", new Class<?>[]{ String.class });
        Object[] args = new Object[]{ "test" };

        ArbitrationResult arbitrationResult = new ArbitrationResult();
        DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo();
        discoveryEntry.setParticipantId("participantId");
        arbitrationResult.setDiscoveryEntries(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(discoveryEntry)));
        proxyInvocationHandler.createConnector(arbitrationResult);
        proxyInvocationHandler.invokeInternal(proxy, fireAndForgetMethod, args);

        verify(connectorInvocationHandler).executeOneWayMethod(fireAndForgetMethod, args);
    }

    @SuppressWarnings("serial")
    private static class MyException extends Exception {
    }

    @Test
    public void testThrowableOnInvoke() {
        ProxyInvocationHandler subject = mock(ProxyInvocationHandler.class, CALLS_REAL_METHODS);
        subject.setThrowableForInvoke(new MyException());
        try {
            subject.invoke(this, getClass().getMethod("testThrowableOnInvoke", new Class<?>[0]), new Object[0]);
        } catch (MyException e) {
            // Expected
        } catch (Throwable t) {
            fail("Wrong exception " + t);
        }
    }

    private static interface MyBroadcastSubscriptionListener extends BroadcastSubscriptionListener {
        void onReceive();
    }

    private static interface MyBroadcastInterface extends JoynrBroadcastSubscriptionInterface {
        @JoynrMulticast(name = "myMulticast")
        void subscribeToMyMulticast(MyBroadcastSubscriptionListener broadcastSubscriptionListener,
                                    SubscriptionQos subscriptionQos,
                                    String... partitions);
    }

    @Test
    public void testPartitionsPassedToMulticastSubscription() throws NoSuchMethodException, ApplicationException {
        ConnectorInvocationHandler connectorInvocationHandler = mock(ConnectorInvocationHandler.class);
        when(mockConnectorFactory.create(Mockito.anyString(),
                                         Mockito.<ArbitrationResult> any(),
                                         Mockito.eq(messagingQos),
                                         Mockito.eq(null))).thenReturn(Optional.of(connectorInvocationHandler));
        MyBroadcastSubscriptionListener broadcastSubscriptionListener = mock(MyBroadcastSubscriptionListener.class);
        SubscriptionQos subscriptionQos = mock(SubscriptionQos.class);

        Method subscribeMethod = MyBroadcastInterface.class.getMethod("subscribeToMyMulticast",
                                                                      MyBroadcastSubscriptionListener.class,
                                                                      SubscriptionQos.class,
                                                                      String[].class);
        Object[] args = new Object[]{ broadcastSubscriptionListener, subscriptionQos,
                new String[]{ "one", "two", "three" } };

        ArbitrationResult arbitrationResult = new ArbitrationResult();
        DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo();
        discoveryEntry.setParticipantId("participantId");
        arbitrationResult.setDiscoveryEntries(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(discoveryEntry)));
        proxyInvocationHandler.createConnector(arbitrationResult);
        proxyInvocationHandler.invokeInternal(proxy, subscribeMethod, args);

        ArgumentCaptor<MulticastSubscribeInvocation> captor = ArgumentCaptor.forClass(MulticastSubscribeInvocation.class);
        verify(connectorInvocationHandler).executeSubscriptionMethod(captor.capture());
        MulticastSubscribeInvocation multicastSubscribeInvocation = captor.getValue();
        assertNotNull(multicastSubscribeInvocation);
        assertArrayEquals(new String[]{ "one", "two", "three" }, multicastSubscribeInvocation.getPartitions());
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testExecuteSyncFailsAfterPrepareForShutdown() throws Exception {
        Method method = TestServiceSync.class.getMethod("testSyncMethod", String.class);
        testExecutionFailsAfterPrepareForShutdown(method);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testExecuteAsyncFailsAfterPrepareForShutdown() throws Exception {
        Method method = TestServiceAsync.class.getMethod("testAsyncMethod", String.class);
        testExecutionFailsAfterPrepareForShutdown(method);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testExecuteSubscriptionMethodFailsAfterPrepareForShutdown() throws Exception {
        Method method = MyBroadcastSubscriptionListener.class.getMethod("onSubscribed", String.class);
        testExecutionFailsAfterPrepareForShutdown(method);
    }

    private void testExecutionFailsAfterPrepareForShutdown(Method method) throws Exception {
        shutdownListener.prepareForShutdown();
        proxyInvocationHandler.invokeInternal(proxy, method, new Object[]{ "inputData" });
        fail("Should not get this far.");
    }

    @Test
    public void testExecuteFireAndForgetAfterPrepareForShutdownAllowed() throws Exception {
        ConnectorInvocationHandler connectorInvocationHandler = mock(ConnectorInvocationHandler.class);
        when(mockConnectorFactory.create(anyString(),
                                         any(),
                                         any(),
                                         any())).thenReturn(Optional.of(connectorInvocationHandler));
        shutdownListener.prepareForShutdown();
        proxyInvocationHandler.createConnector(mock(ArbitrationResult.class));
        Method method = TestServiceSync.class.getMethod("callMe", String.class);
        proxyInvocationHandler.invokeInternal(proxy, method, new Object[]{ "inputData" });
        verify(connectorInvocationHandler).executeOneWayMethod(eq(method), any());
    }

    @Test
    public void testHandlingReceivedSelectedAndNonSelectedProviderParticipantIds() throws Throwable {
        ConnectorInvocationHandler connectorInvocationHandler = mock(ConnectorInvocationHandler.class);
        final String proxyParticipantId = "proxyParticipantId";
        final String expectedProxyParticipantId = proxyParticipantId;
        when(mockConnectorFactory.create(eq(proxyParticipantId),
                                         Mockito.<ArbitrationResult> any(),
                                         Mockito.eq(messagingQos),
                                         Mockito.eq(null))).thenReturn(Optional.of(connectorInvocationHandler));

        DiscoveryEntryWithMetaInfo selectedDiscoveryEntry1 = new DiscoveryEntryWithMetaInfo();
        selectedDiscoveryEntry1.setParticipantId("participantId1");

        DiscoveryEntryWithMetaInfo selectedDiscoveryEntry2 = new DiscoveryEntryWithMetaInfo();
        selectedDiscoveryEntry2.setParticipantId("participantId2");

        Set<String> expectedProviderParticipantIds = new HashSet<>();
        expectedProviderParticipantIds.add("participantId1");
        expectedProviderParticipantIds.add("participantId2");

        DiscoveryEntryWithMetaInfo nonSelectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        final String nonSelectedParticipantId = "nonSelectedParticipantId";
        final String expectedNonSelectedParticipantId = nonSelectedParticipantId;
        nonSelectedDiscoveryEntry.setParticipantId(nonSelectedParticipantId);

        ArbitrationResult arbitrationResult = new ArbitrationResult();
        arbitrationResult.setDiscoveryEntries(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(selectedDiscoveryEntry1,
                                                                                                    selectedDiscoveryEntry2)));
        arbitrationResult.setOtherDiscoveryEntries(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(nonSelectedDiscoveryEntry)));

        proxyInvocationHandler.createConnector(arbitrationResult);

        verify(mockGcHandler).registerProxyProviderParticipantIds(eq(expectedProxyParticipantId),
                                                                  eq(expectedProviderParticipantIds));
        verify(mockMessageRouter).removeNextHop(expectedNonSelectedParticipantId);
    }

    @Test
    public void testRegisterProxy() {
        Object proxy = new Object();
        proxyInvocationHandler.registerProxy(proxy);
        verify(mockGcHandler).registerProxy(proxy, proxyParticipantId, shutdownListener);
    }
}
