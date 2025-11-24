/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2023 BMW Car IT GmbH
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
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;
import static org.junit.Assume.assumeFalse;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;

import io.joynr.proxy.invocation.UnsubscribeInvocation;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameter;
import org.junit.runners.Parameterized.Parameters;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
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
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import io.joynr.runtime.PrepareForShutdownListener;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.exceptions.ApplicationException;
import joynr.types.DiscoveryEntryWithMetaInfo;

@RunWith(Parameterized.class)
public class ProxyInvocationHandlerTest {
    private MessagingQos messagingQos = new MessagingQos();
    private DiscoveryQos discoveryQos = new DiscoveryQos();
    private String proxyParticipantId = "proxyParticipantId";
    private String interfaceName = "interfaceName";
    private String domain = "domain";
    private Object proxy;

    @Parameters
    public static Object[] data() {
        return new Object[]{ Boolean.FALSE, Boolean.TRUE };
    }

    @Rule
    public MockitoRule rule = MockitoJUnit.rule();
    @Parameter
    public boolean separateReplyReceiver;

    @Mock
    private ConnectorFactory mockConnectorFactory;

    @Mock
    private MessageRouter mockMessageRouter;
    @Mock
    private GarbageCollectionHandler mockGcHandler;

    @Mock
    private ShutdownNotifier mockShutdownNotifier;
    private ShutdownListener shutdownListener;
    private PrepareForShutdownListener prepareForShutdownListener;

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
        Future<Void> testAsyncMethod(String inputData);
    }

    @Before
    public void setup() {
        proxy = new Object();
        StatelessAsyncCallback mockStatelessAsyncCallback = Mockito.mock(StatelessAsyncCallback.class);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                prepareForShutdownListener = (PrepareForShutdownListener) invocation.getArguments()[0];
                return null;
            }
        }).when(mockShutdownNotifier).registerProxyInvocationHandlerPrepareForShutdownListener(any());
        proxyInvocationHandler = new ProxyInvocationHandlerImpl(new HashSet<String>(Arrays.asList(domain)),
                                                                interfaceName,
                                                                proxyParticipantId,
                                                                discoveryQos,
                                                                messagingQos,
                                                                Optional.of(mockStatelessAsyncCallback),
                                                                separateReplyReceiver,
                                                                mockConnectorFactory,
                                                                mockMessageRouter,
                                                                mockGcHandler,
                                                                mockShutdownNotifier,
                                                                mockStatelessAsyncIdCalculator);
    }

    @Test(timeout = 3000)
    public void callProxyInvocationHandlerSyncFromMultipleThreadsTest() throws Throwable {

        java.util.concurrent.Future<?> call1 = threadPool.submit(new Callable<Object>() {
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

        java.util.concurrent.Future<?> call2 = threadPool.submit(new Callable<Object>() {
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

        void unsubscribeFromMyMulticast(String participantId);

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
    public void testExecuteSyncFailsAfterPrepareForShutdown_NoSeparateReplyReceiver() throws Exception {
        assumeFalse(separateReplyReceiver);
        Method method = TestServiceSync.class.getMethod("testSyncMethod", String.class);
        testExecutionFailsAfterPrepareForShutdown(method);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testExecuteAsyncFailsAfterPrepareForShutdown_NoSeparateReplyReceiver() throws Exception {
        assumeFalse(separateReplyReceiver);
        Method method = TestServiceAsync.class.getMethod("testAsyncMethod", String.class);
        testExecutionFailsAfterPrepareForShutdown(method);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testExecuteSubscriptionMethodFailsAfterPrepareForShutdown_NoSeparateReplyReceiver() throws Exception {
        assumeFalse(separateReplyReceiver);
        Method method = MyBroadcastInterface.class.getMethod("subscribeToMyMulticast",
                                                             MyBroadcastSubscriptionListener.class,
                                                             SubscriptionQos.class,
                                                             String[].class);
        testExecutionFailsAfterPrepareForShutdown(method);
    }

    private void testExecutionFailsAfterPrepareForShutdown(Method method) throws Exception {
        prepareForShutdownListener.prepareForShutdown();
        proxyInvocationHandler.invokeInternal(proxy, method, new Object[]{ "inputData" });
        fail("Should not get this far.");
    }

    @Test
    public void testExecuteSyncAfterPrepareForShutdownAllowed_SeparateReplyReceiver() throws Exception {
        assumeTrue(separateReplyReceiver);
        Method method = TestServiceSync.class.getMethod("testSyncMethod", String.class);
        ConnectorInvocationHandler connectorInvocationHandler = prepareObjectsAndInvoke(method,
                                                                                        new Object[]{ "inputData" });
        verify(connectorInvocationHandler).executeSyncMethod(eq(method), any());
    }

    @Test
    public void testExecuteAsyncAfterPrepareForShutdownAllowed_SeparateReplyReceiver() throws Exception {
        assumeTrue(separateReplyReceiver);
        Method method = TestServiceAsync.class.getMethod("testAsyncMethod", String.class);
        ConnectorInvocationHandler connectorInvocationHandler = prepareObjectsAndInvoke(method,
                                                                                        new Object[]{ "inputData" });
        verify(connectorInvocationHandler).executeAsyncMethod(eq(proxy), eq(method), any(), any());
    }

    @Test
    public void testExecuteSubscriptionMethodAfterPrepareForShutdownAllowed_SeparateReplyReceiver() throws Exception {
        assumeTrue(separateReplyReceiver);
        Method method = MyBroadcastInterface.class.getMethod("subscribeToMyMulticast",
                                                             MyBroadcastSubscriptionListener.class,
                                                             SubscriptionQos.class,
                                                             String[].class);
        MyBroadcastSubscriptionListener broadcastSubscriptionListener = mock(MyBroadcastSubscriptionListener.class);
        SubscriptionQos subscriptionQos = mock(SubscriptionQos.class);
        Object[] args = new Object[]{ broadcastSubscriptionListener, subscriptionQos,
                new String[]{ "one", "two", "three" } };

        ConnectorInvocationHandler connectorInvocationHandler = prepareObjectsAndInvoke(method, args);
        verify(connectorInvocationHandler).executeSubscriptionMethod(Mockito.<MulticastSubscribeInvocation> any());
    }

    @Test
    public void testExecuteUnsubscribeMethodAfterPrepareForShutdownAllowed() throws Exception {
        Method method = MyBroadcastInterface.class.getMethod("unsubscribeFromMyMulticast", String.class);
        ConnectorInvocationHandler connectorInvocationHandler = prepareObjectsAndInvoke(method,
                                                                                        new Object[]{ "inputData" });
        verify(connectorInvocationHandler).executeSubscriptionMethod(Mockito.<UnsubscribeInvocation> any());
    }

    @Test
    public void testExecuteFireAndForgetAfterPrepareForShutdownAllowed() throws Exception {
        Method method = TestServiceSync.class.getMethod("callMe", String.class);
        ConnectorInvocationHandler connectorInvocationHandler = prepareObjectsAndInvoke(method,
                                                                                        new Object[]{ "inputData" });
        verify(connectorInvocationHandler).executeOneWayMethod(eq(method), any());
    }

    private ConnectorInvocationHandler prepareObjectsAndInvoke(Method method,
                                                               Object[] args) throws ApplicationException {
        ConnectorInvocationHandler connectorInvocationHandler = mock(ConnectorInvocationHandler.class);
        when(mockConnectorFactory.create(anyString(),
                                         any(),
                                         any(),
                                         any())).thenReturn(Optional.of(connectorInvocationHandler));
        prepareForShutdownListener.prepareForShutdown();
        proxyInvocationHandler.createConnector(mock(ArbitrationResult.class));
        proxyInvocationHandler.invokeInternal(proxy, method, args);
        return connectorInvocationHandler;
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
        verify(mockGcHandler).registerProxy(proxy, proxyParticipantId, prepareForShutdownListener, shutdownListener);
    }

    @Test
    public void testRegisterShutdownListener() {
        verify(mockShutdownNotifier).registerProxyInvocationHandlerShutdownListener(proxyInvocationHandler.shutdownListener);
    }

    @Test
    public void testRegisterPrepareForShutdownListener() {
        verify(mockShutdownNotifier).registerProxyInvocationHandlerPrepareForShutdownListener(proxyInvocationHandler.prepareForShutdownListener);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testRegisterForShutdown_throws() {
        ShutdownNotifier shutdownNotifier = new ShutdownNotifier();
        shutdownNotifier.registerForShutdown(proxyInvocationHandler.shutdownListener);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testRegisterToBeShutdownAsLast_throws() {
        ShutdownNotifier shutdownNotifier = new ShutdownNotifier();
        shutdownNotifier.registerToBeShutdownAsLast(proxyInvocationHandler.shutdownListener);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testRegisterPrepareForShutdownListener_throws() {
        ShutdownNotifier shutdownNotifier = new ShutdownNotifier();
        shutdownNotifier.registerPrepareForShutdownListener(proxyInvocationHandler.prepareForShutdownListener);
    }

    @Test
    public void testUnregisterPrepareForShutdownListener() throws NoSuchFieldException, IllegalAccessException {
        ShutdownNotifier shutdownNotifier = new ShutdownNotifier();

        List<PrepareForShutdownListener> spyProxyListenerList = injectSpyList(shutdownNotifier,
                                                                              "proxyInvocationHandlerPrepareForShutdownListenerList");
        List<PrepareForShutdownListener> spyOtherListenerList = injectSpyList(shutdownNotifier,
                                                                              "prepareForShutdownListenerList");

        shutdownNotifier.registerProxyInvocationHandlerPrepareForShutdownListener(proxyInvocationHandler.prepareForShutdownListener);

        Mockito.verify(spyProxyListenerList, Mockito.times(1)).add(proxyInvocationHandler.prepareForShutdownListener);
        Mockito.verify(spyOtherListenerList, Mockito.never()).add(proxyInvocationHandler.prepareForShutdownListener);

        shutdownNotifier.unregister(proxyInvocationHandler.prepareForShutdownListener);

        Mockito.verify(spyProxyListenerList, Mockito.times(1))
               .remove(proxyInvocationHandler.prepareForShutdownListener);
        Mockito.verify(spyOtherListenerList, Mockito.never()).remove(proxyInvocationHandler.prepareForShutdownListener);
    }

    @Test
    public void testUnregisterShutdownListener() throws NoSuchFieldException, IllegalAccessException {
        ShutdownNotifier shutdownNotifier = new ShutdownNotifier();

        List<ShutdownListener> spyProxyListenerList = injectSpyList(shutdownNotifier,
                                                                    "proxyInvocationHandlerShutdownListenerList");
        List<ShutdownListener> spyOtherListenerList = injectSpyList(shutdownNotifier, "shutdownListenerList");

        shutdownNotifier.registerProxyInvocationHandlerShutdownListener(proxyInvocationHandler.shutdownListener);

        Mockito.verify(spyProxyListenerList, Mockito.times(1)).add(proxyInvocationHandler.shutdownListener);
        Mockito.verify(spyOtherListenerList, Mockito.never()).add(proxyInvocationHandler.shutdownListener);

        shutdownNotifier.unregister(proxyInvocationHandler.shutdownListener);

        Mockito.verify(spyProxyListenerList, Mockito.times(1)).remove(proxyInvocationHandler.shutdownListener);
        Mockito.verify(spyOtherListenerList, Mockito.never()).remove(proxyInvocationHandler.shutdownListener);
    }

    @SuppressWarnings("unchecked")
    private <T> List<T> injectSpyList(ShutdownNotifier notifier, String fieldName) throws NoSuchFieldException,
                                                                                   IllegalAccessException {

        Field field = ShutdownNotifier.class.getDeclaredField(fieldName);
        field.setAccessible(true);

        List<T> spyList = Mockito.spy(new ArrayList<>());
        field.set(notifier, spyList);

        return spyList;
    }
}
