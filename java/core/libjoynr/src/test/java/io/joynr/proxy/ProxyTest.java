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

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;

import java.io.IOException;
import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.Semaphore;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.node.TextNode;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.assistedinject.FactoryModuleBuilder;
import com.google.inject.name.Named;

import io.joynr.Async;
import io.joynr.JoynrVersion;
import io.joynr.Sync;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.ArbitratorFactory;
import io.joynr.arbitration.DiscoveryEntryVersionFilter;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.common.ExpiryDate;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatcher.rpc.RequestStatusCode;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.routing.GarbageCollectionHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.runtime.SystemServicesSettings;
import io.joynr.util.JoynrThreadFactory;
import joynr.MulticastSubscriptionQos;
import joynr.OnChangeSubscriptionQos;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.exceptions.ApplicationException;
import joynr.system.RoutingTypes.Address;
import joynr.test.JoynrTestLoggingRule;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.ProviderQos;
import joynr.types.Version;
import joynr.vehicle.NavigationBroadcastInterface.LocationUpdateBroadcastListener;
import joynr.vehicle.NavigationBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters;
import joynr.vehicle.NavigationBroadcastInterface.LocationUpdateSelectiveBroadcastListener;
import joynr.vehicle.NavigationProxy;

public class ProxyTest {
    private static final Logger logger = LoggerFactory.getLogger(ProxyTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    private static final int ONE_MINUTE_IN_MS = 60 * 1000;
    private static final long MAX_TTL_MS = 2592000000L;
    private static final long DEFAULT_DISCOVERY_TIMEOUT_MS = 30000L;
    private static final long DEFAULT_RETRY_INTERVAL_MS = 2000L;
    private static final long ARBITRATION_MINIMUMRETRYDELAY = 2000L;

    private Semaphore testInterfaceProxyCreatedSemaphore;
    private Semaphore navigationProxyCreatedSemaphore;
    private ProxyCreatedCallback<NavigationProxy> navigationProxyCreatedCallback;
    private ProxyCreatedCallback<TestInterface> testInterfaceProxyCreatedCallback;

    private DiscoveryQos discoveryQos;
    private MessagingQos messagingQos;

    private String domain;
    private String fromParticipantId;
    private String toParticipantId;
    private String asyncReplyText = "replyText";

    private DiscoveryEntryWithMetaInfo toDiscoveryEntry;
    private Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries;

    private ProxyBuilderFactory proxyBuilderFactory;

    private ScheduledExecutorService scheduler;

    @Mock
    private ReplyCallerDirectory replyCallerDirectory;
    @Mock
    private RequestReplyManager requestReplyManager;
    @Mock
    SubscriptionManager subscriptionManager;
    @Mock
    MessageRouter messageRouter;
    @Mock
    private GarbageCollectionHandler gcdHandler;
    @Mock
    Dispatcher dispatcher;
    @Mock
    RoutingTable routingTable;
    @Mock
    private StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectory;

    @Mock
    private LocalDiscoveryAggregator localDiscoveryAggregator;

    @Mock
    private ShutdownNotifier shutdownNotifier;

    @Mock
    private Callback<String> callback;

    @Mock
    private DiscoveryEntryVersionFilter discoveryEntryVersionFilter;

    private enum ApplicationErrors {
        ERROR_VALUE_1, ERROR_VALUE_2, ERROR_VALUE_3
    }

    @Sync
    public interface SyncTestInterface {
        String method1();

        String method1(MessagingQos messagingQos);

        String methodWithApplicationError() throws ApplicationException;
    }

    @Async
    public interface AsyncTestInterface {
        Future<String> asyncMethod(@JoynrRpcCallback(deserializationType = String.class) Callback<String> callback);

        Future<String> asyncMethod(@JoynrRpcCallback(deserializationType = String.class) Callback<String> callback,
                                   MessagingQos messagingQos);

        Future<String> asyncMethodWithApplicationError(@JoynrRpcCallback(deserializationType = String.class) Callback<String> callback);
    }

    @io.joynr.dispatcher.rpc.annotation.FireAndForget
    public interface FireAndForgetTestInterface {
        void methodFireAndForget();

        void methodFireAndForget(MessagingQos messagingQos);
    }

    @JoynrVersion(major = 0, minor = 0)
    public interface TestInterface extends SyncTestInterface, AsyncTestInterface, FireAndForgetTestInterface {
        public static final String INTERFACE_NAME = "TestInterface";
    }

    // CHECKSTYLE IGNORE MethodLength FOR NEXT 1 LINES
    @SuppressWarnings({ "unchecked", "rawtypes" })
    @Before
    public void setUp() throws Exception {
        navigationProxyCreatedSemaphore = new Semaphore(0);
        navigationProxyCreatedCallback = new ProxyCreatedCallback<NavigationProxy>() {
            @Override
            public void onProxyCreationFinished(NavigationProxy result) {
                navigationProxyCreatedSemaphore.release();
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                fail("Navigation proxy creation failed: " + error);
            }
        };
        testInterfaceProxyCreatedSemaphore = new Semaphore(0);
        testInterfaceProxyCreatedCallback = new ProxyCreatedCallback<TestInterface>() {
            @Override
            public void onProxyCreationFinished(TestInterface result) {
                testInterfaceProxyCreatedSemaphore.release();
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                fail("TestInterface proxy creation failed: " + error);
            }
        };

        domain = "TestDomain";
        fromParticipantId = "ProxyTestFromParticipantId";
        toParticipantId = "ProxyTestToParticipantId";
        toDiscoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(47, 11),
                                                          domain,
                                                          TestInterface.INTERFACE_NAME,
                                                          toParticipantId,
                                                          new ProviderQos(),
                                                          System.currentTimeMillis(),
                                                          System.currentTimeMillis() + ONE_MINUTE_IN_MS,
                                                          "publicKeyId",
                                                          true);
        toDiscoveryEntries = new HashSet<DiscoveryEntryWithMetaInfo>();
        toDiscoveryEntries.add(toDiscoveryEntry);

        MockitoAnnotations.initMocks(this);
        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                requestStaticInjection(RpcUtils.class);
                bind(ReplyCallerDirectory.class).toInstance(replyCallerDirectory);
                bind(RequestReplyManager.class).toInstance(requestReplyManager);
                bind(SubscriptionManager.class).toInstance(subscriptionManager);
                bind(MessageRouter.class).toInstance(messageRouter);
                bind(GarbageCollectionHandler.class).toInstance(gcdHandler);
                bind(RoutingTable.class).toInstance(routingTable);
                bind(StatelessAsyncCallbackDirectory.class).toInstance(statelessAsyncCallbackDirectory);
                bind(StatelessAsyncIdCalculator.class).toInstance(new DefaultStatelessAsyncIdCalculatorImpl("channel"));
                install(new FactoryModuleBuilder().implement(ProxyInvocationHandler.class,
                                                             ProxyInvocationHandlerImpl.class)
                                                  .build(ProxyInvocationHandlerFactory.class));

            }

            @Provides
            @Singleton
            @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS)
            Address getDispatcherAddress() {
                return new InProcessAddress();
            }

        });

        proxyBuilderFactory = new ProxyBuilderFactoryImpl(localDiscoveryAggregator,
                                                          injector.getInstance(ProxyInvocationHandlerFactory.class),
                                                          shutdownNotifier,
                                                          injector.getInstance(StatelessAsyncCallbackDirectory.class),
                                                          MAX_TTL_MS,
                                                          DEFAULT_DISCOVERY_TIMEOUT_MS,
                                                          DEFAULT_RETRY_INTERVAL_MS,
                                                          ARBITRATION_MINIMUMRETRYDELAY);

        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();

                DiscoveryEntryWithMetaInfo[] fakeCapabilitiesResult = { toDiscoveryEntry };
                ((Callback) args[0]).resolve((Object) fakeCapabilitiesResult);
                return null;
            }
        }).when(localDiscoveryAggregator).lookup(Mockito.<CallbackWithModeledError> any(),
                                                 Mockito.<String[]> any(),
                                                 Mockito.<String> any(),
                                                 Mockito.<joynr.types.DiscoveryQos> any(),
                                                 Mockito.<String[]> any());

        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();
                AttributeSubscribeInvocation request = (AttributeSubscribeInvocation) args[2];
                if (request.getSubscriptionId() == null) {
                    request.setSubscriptionId(createUuidString());
                }
                request.getFuture().resolve(request.getSubscriptionId());
                return null;
            }
        }).when(subscriptionManager).registerAttributeSubscription(any(String.class),
                                                                   eq(new HashSet(Arrays.asList(toDiscoveryEntry))),
                                                                   Mockito.any(AttributeSubscribeInvocation.class));

        Mockito.doAnswer(new Answer<Object>() { //TODO simulate resolve here ! subscription reply bastern ... handle subscriptionreply ausführen.. 
            @Override
            public Object answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();
                BroadcastSubscribeInvocation request = (BroadcastSubscribeInvocation) args[2];
                if (request.getSubscriptionId() == null) {
                    request.setSubscriptionId(createUuidString());

                }
                request.getFuture().resolve(request.getSubscriptionId());
                return null;
            }
        }).when(subscriptionManager).registerBroadcastSubscription(any(String.class),
                                                                   eq(new HashSet(Arrays.asList(toDiscoveryEntry))),
                                                                   Mockito.any(BroadcastSubscribeInvocation.class));

        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();
                MulticastSubscribeInvocation request = (MulticastSubscribeInvocation) args[2];
                if (request.getSubscriptionId() == null) {
                    request.setSubscriptionId(createUuidString());

                }
                request.getFuture().resolve(request.getSubscriptionId());
                return null;
            }
        }).when(subscriptionManager).registerMulticastSubscription(any(String.class),
                                                                   eq(new HashSet(Arrays.asList(toDiscoveryEntry))),
                                                                   Mockito.any(MulticastSubscribeInvocation.class));

        discoveryQos = new DiscoveryQos(10000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
        messagingQos = new MessagingQos();

        doAnswer(new Answer<Set<DiscoveryEntryWithMetaInfo>>() {
            @Override
            public Set<DiscoveryEntryWithMetaInfo> answer(InvocationOnMock invocation) throws Throwable {
                return (Set<DiscoveryEntryWithMetaInfo>) invocation.getArguments()[1];
            }
        }).when(discoveryEntryVersionFilter).filter(Mockito.<Version> any(),
                                                    Mockito.<Set<DiscoveryEntryWithMetaInfo>> any(),
                                                    Mockito.<Map<String, Set<Version>>> any());

        Field discoveryEntryVersionFilterField = ArbitratorFactory.class.getDeclaredField("discoveryEntryVersionFilter");
        discoveryEntryVersionFilterField.setAccessible(true);
        discoveryEntryVersionFilterField.set(ArbitratorFactory.class, discoveryEntryVersionFilter);

        doAnswer(new Answer<Set<DiscoveryEntryWithMetaInfo>>() {
            @Override
            public Set<DiscoveryEntryWithMetaInfo> answer(InvocationOnMock invocation) throws Throwable {
                return (Set<DiscoveryEntryWithMetaInfo>) invocation.getArguments()[1];
            }
        }).when(discoveryEntryVersionFilter).filter(Mockito.<Version> any(),
                                                    Mockito.<Set<DiscoveryEntryWithMetaInfo>> any(),
                                                    Mockito.<Map<String, Set<Version>>> any());

        Field schedulerField = ArbitratorFactory.class.getDeclaredField("scheduler");
        schedulerField.setAccessible(true);
        String name = "TEST.joynr.scheduler.arbitration.arbitratorRunnable";
        ThreadFactory joynrThreadFactory = new JoynrThreadFactory(name, true);
        scheduler = Executors.newSingleThreadScheduledExecutor(joynrThreadFactory);
        schedulerField.set(ArbitratorFactory.class, scheduler);

        Field shutdownNotifierField = ArbitratorFactory.class.getDeclaredField("shutdownNotifier");
        shutdownNotifierField.setAccessible(true);
        shutdownNotifierField.set(ArbitratorFactory.class, shutdownNotifier);

        ArbitratorFactory.start();
        verify(shutdownNotifier).registerForShutdown(any(ShutdownListener.class));
    }

    @After
    public void tearDown() {
        ArbitratorFactory.shutdown();
        scheduler.shutdown();
        try {
            assertTrue(scheduler.awaitTermination(2000, TimeUnit.MILLISECONDS));
        } catch (InterruptedException e) {
            fail("InterruptedException in scheduler.awaitTermination: " + e);
        }
    }

    private <T> ProxyBuilderDefaultImpl<T> getProxyBuilder(final Class<T> interfaceClass) {
        return (ProxyBuilderDefaultImpl<T>) proxyBuilderFactory.get(domain, interfaceClass);
    }

    private TestInterface getTestInterfaceProxy() throws InterruptedException {
        ProxyBuilder<TestInterface> proxyBuilder = getProxyBuilder(TestInterface.class);
        proxyBuilder.setParticipantId(fromParticipantId);
        TestInterface proxy = proxyBuilder.setMessagingQos(messagingQos)
                                          .setDiscoveryQos(discoveryQos)
                                          .build(testInterfaceProxyCreatedCallback);
        assertTrue(testInterfaceProxyCreatedSemaphore.tryAcquire(1000, TimeUnit.MILLISECONDS));
        return proxy;
    }

    private NavigationProxy getNavigationProxy() throws InterruptedException {
        ProxyBuilder<NavigationProxy> proxyBuilder = getProxyBuilder(NavigationProxy.class);
        proxyBuilder.setParticipantId(fromParticipantId);
        NavigationProxy proxy = proxyBuilder.setMessagingQos(messagingQos)
                                            .setDiscoveryQos(discoveryQos)
                                            .build(navigationProxyCreatedCallback);
        assertTrue(navigationProxyCreatedSemaphore.tryAcquire(1000, TimeUnit.MILLISECONDS));
        return proxy;
    }

    @Test
    public void createProxyWithMessageQosLargerThanMaxTtlUsesMax() throws Exception {
        ProxyBuilderDefaultImpl<TestInterface> proxyBuilder = getProxyBuilder(TestInterface.class);
        MessagingQos messagingQosTtlTooLarge = new MessagingQos(MAX_TTL_MS + 1);
        proxyBuilder.setMessagingQos(messagingQosTtlTooLarge).setDiscoveryQos(discoveryQos).build();
        assertTrue(proxyBuilder.messagingQos.getRoundTripTtl_ms() == MAX_TTL_MS);
    }

    @Test
    public void createProxyWithMessageQosTtlEqualMaxTtlIsOk() throws Exception {
        ProxyBuilderDefaultImpl<TestInterface> proxyBuilder = getProxyBuilder(TestInterface.class);
        MessagingQos messagingQosTtlTooLarge = new MessagingQos(MAX_TTL_MS);
        proxyBuilder.setMessagingQos(messagingQosTtlTooLarge).setDiscoveryQos(discoveryQos).build();
        assertTrue(proxyBuilder.messagingQos.getRoundTripTtl_ms() == MAX_TTL_MS);
    }

    @Test
    public void createProxyWithMessageQosTtlSmallerThanMaxTtlIsNotModified() throws Exception {
        ProxyBuilderDefaultImpl<TestInterface> proxyBuilder = getProxyBuilder(TestInterface.class);
        long messageTtl = 5000;
        MessagingQos messagingQosTtlTooLarge = new MessagingQos(messageTtl);
        proxyBuilder.setMessagingQos(messagingQosTtlTooLarge).setDiscoveryQos(discoveryQos).build();
        assertTrue(proxyBuilder.messagingQos.getRoundTripTtl_ms() == messageTtl);
    }

    public void createProxyAndCallSyncMethodSuccess(MessagingQos privateMessagingQos) {
        MessagingQos expectedMessagingQos;

        if (privateMessagingQos != null) {
            expectedMessagingQos = privateMessagingQos;
        } else {
            expectedMessagingQos = messagingQos;
        }

        String requestReplyId = "createProxyAndCallSyncMethod_requestReplyId";
        Mockito.when(requestReplyManager.sendSyncRequest(Mockito.<String> any(),
                                                         Mockito.<DiscoveryEntryWithMetaInfo> any(),
                                                         Mockito.<Request> any(),
                                                         Mockito.<SynchronizedReplyCaller> any(),
                                                         Mockito.<MessagingQos> any()))
               .thenReturn(new Reply(requestReplyId, "Answer"));

        ProxyBuilder<TestInterface> proxyBuilder = getProxyBuilder(TestInterface.class);
        TestInterface proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        String result;
        if (privateMessagingQos != null) {
            result = proxy.method1(privateMessagingQos);
        } else {
            result = proxy.method1();
        }
        ArgumentCaptor<MessagingQos> messagingQosCaptor = ArgumentCaptor.forClass(MessagingQos.class);
        ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);
        verify(requestReplyManager).sendSyncRequest(Mockito.<String> any(),
                                                    Mockito.<DiscoveryEntryWithMetaInfo> any(),
                                                    requestCaptor.capture(),
                                                    Mockito.<SynchronizedReplyCaller> any(),
                                                    messagingQosCaptor.capture());
        Assert.assertEquals(expectedMessagingQos.getRoundTripTtl_ms(),
                            messagingQosCaptor.getValue().getRoundTripTtl_ms());
        Assert.assertFalse(requestCaptor.getValue().hasParams());
        Assert.assertEquals("Answer", result);
    }

    @Test
    public void createProxyAndCallSyncMethodSuccessWithDefaultTtl() throws Exception {
        MessagingQos privateMessagingQos = null;
        createProxyAndCallSyncMethodSuccess(privateMessagingQos);
    }

    @Test
    public void createProxyAndCallSyncMethodSuccessWithSpecialTtl() throws Exception {
        MessagingQos privateMessagingQos = new MessagingQos(120000);
        Assert.assertTrue(messagingQos.getRoundTripTtl_ms() != privateMessagingQos.getRoundTripTtl_ms());
        createProxyAndCallSyncMethodSuccess(privateMessagingQos);
    }

    @Test
    public void createProxyAndCallSyncMethodFailWithApplicationError() throws Exception {
        String requestReplyId = "createProxyAndCallSyncMethod_requestReplyId";
        Mockito.when(requestReplyManager.sendSyncRequest(Mockito.<String> any(),
                                                         Mockito.<DiscoveryEntryWithMetaInfo> any(),
                                                         Mockito.<Request> any(),
                                                         Mockito.<SynchronizedReplyCaller> any(),
                                                         Mockito.<MessagingQos> any()))
               .thenReturn(new Reply(requestReplyId,
                                     new ApplicationException(ApplicationErrors.ERROR_VALUE_2,
                                                              "syncMethodCallApplicationException")));

        ProxyBuilder<TestInterface> proxyBuilder = getProxyBuilder(TestInterface.class);
        TestInterface proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        ApplicationException exception = null;
        try {
            proxy.methodWithApplicationError();
            Assert.fail("Should throw ApplicationException");
        } catch (ApplicationException e) {
            exception = e;
        }
        Assert.assertEquals(new ApplicationException(ApplicationErrors.ERROR_VALUE_2,
                                                     "syncMethodCallApplicationException"),
                            exception);
    }

    public void createProxyAndCallAsyncMethodSuccess(MessagingQos privateMessagingQos) throws Exception {
        MessagingQos expectedMessagingQos;

        if (privateMessagingQos != null) {
            expectedMessagingQos = privateMessagingQos;
        } else {
            expectedMessagingQos = messagingQos;
        }

        TestInterface proxy = getTestInterfaceProxy();

        // when joynrMessageSender1.sendRequest is called, get the replyCaller from the mock dispatcher and call
        // messageCallback on it.
        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws JsonParseException, JsonMappingException,
                                                              IOException {
                // capture the replyCaller passed into the dispatcher for calling later
                ArgumentCaptor<ReplyCaller> replyCallerCaptor = ArgumentCaptor.forClass(ReplyCaller.class);
                verify(replyCallerDirectory).addReplyCaller(anyString(),
                                                            replyCallerCaptor.capture(),
                                                            Mockito.any(ExpiryDate.class));

                String requestReplyId = "createProxyAndCallAsyncMethodSuccess_requestReplyId";
                // pass the response to the replyCaller
                replyCallerCaptor.getValue().messageCallBack(new Reply(requestReplyId, new TextNode(asyncReplyText)));
                return null;
            }
        }).when(requestReplyManager).sendRequest(Mockito.<String> any(),
                                                 Mockito.<DiscoveryEntryWithMetaInfo> any(),
                                                 Mockito.<Request> any(),
                                                 Mockito.<MessagingQos> any());

        final Future<String> future;
        if (privateMessagingQos != null) {
            future = proxy.asyncMethod(callback, privateMessagingQos);
        } else {
            future = proxy.asyncMethod(callback);
        }

        // the test usually takes only 200 ms, so if we wait 1 sec, something has gone wrong
        String reply = future.get(1000);

        ArgumentCaptor<MessagingQos> messagingQosCaptor = ArgumentCaptor.forClass(MessagingQos.class);
        ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);
        verify(requestReplyManager).sendRequest(Mockito.<String> any(),
                                                Mockito.<DiscoveryEntryWithMetaInfo> any(),
                                                requestCaptor.capture(),
                                                messagingQosCaptor.capture());
        verify(callback).resolve(asyncReplyText);
        Assert.assertEquals(expectedMessagingQos.getRoundTripTtl_ms(),
                            messagingQosCaptor.getValue().getRoundTripTtl_ms());
        Assert.assertFalse(requestCaptor.getValue().hasParams());
        Assert.assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        Assert.assertEquals(asyncReplyText, reply);
    }

    @Test
    public void createProxyAndCallAsyncMethodWithDefaultTtl() throws Exception {
        MessagingQos privateMessagingQos = null;
        createProxyAndCallAsyncMethodSuccess(privateMessagingQos);
    }

    @Test
    public void createProxyAndCallAsyncMethodWithSpecialTtl() throws Exception {
        MessagingQos privateMessagingQos = new MessagingQos(120000);
        Assert.assertTrue(messagingQos.getRoundTripTtl_ms() != privateMessagingQos.getRoundTripTtl_ms());
        createProxyAndCallAsyncMethodSuccess(privateMessagingQos);
    }

    @SuppressWarnings({ "unchecked" })
    @Test
    public void createProxyAndCallAsyncMethodFailWithApplicationError() throws Exception {
        final ApplicationException expected = new ApplicationException(ApplicationErrors.ERROR_VALUE_3,
                                                                       "TEST: createProxyAndCallAsyncMethodFailWithApplicationError");
        TestInterface proxy = getTestInterfaceProxy();

        // when joynrMessageSender1.sendRequest is called, get the replyCaller from the mock dispatcher and call
        // messageCallback on it.
        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws JsonParseException, JsonMappingException,
                                                              IOException {
                // capture the replyCaller passed into the dispatcher for calling later
                ArgumentCaptor<ReplyCaller> replyCallerCaptor = ArgumentCaptor.forClass(ReplyCaller.class);
                verify(replyCallerDirectory).addReplyCaller(anyString(),
                                                            replyCallerCaptor.capture(),
                                                            any(ExpiryDate.class));

                String requestReplyId = "createProxyAndCallAsyncMethodSuccess_requestReplyId";
                // pass the response to the replyCaller
                replyCallerCaptor.getValue().messageCallBack(new Reply(requestReplyId, expected));
                return null;
            }
        }).when(requestReplyManager).sendRequest(Mockito.<String> any(),
                                                 Mockito.<DiscoveryEntryWithMetaInfo> any(),
                                                 Mockito.<Request> any(),
                                                 Mockito.<MessagingQos> any());

        CallbackWithModeledError<String, Enum<?>> callbackWithApplicationException = Mockito.mock(CallbackWithModeledError.class);
        final Future<String> future = proxy.asyncMethodWithApplicationError(callbackWithApplicationException);

        // the test usually takes only 200 ms, so if we wait 1 sec, something has gone wrong
        try {
            future.get(1000);
            Assert.fail("Should throw ApplicationException");
        } catch (ApplicationException e) {
            Assert.assertEquals(expected, e);
        }

        verify(callbackWithApplicationException).onFailure(expected.getError());
        Assert.assertEquals(RequestStatusCode.ERROR, future.getStatus().getCode());
    }

    @Test
    public void createProxyAndCallAsyncMethodFail() throws Exception {

        // Expect this exception to be passed back to the callback onFailure and thrown in the future
        final JoynrCommunicationException expectedException = new JoynrCommunicationException();
        // final JoynCommunicationException expectedException = null;

        TestInterface proxy = getTestInterfaceProxy();

        // when joynrMessageSender1.sendRequest is called, get the replyCaller from the mock dispatcher and call
        // messageCallback on it.
        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws JsonParseException, JsonMappingException,
                                                              IOException {
                // capture the replyCaller passed into the dispatcher for calling later
                ArgumentCaptor<ReplyCaller> replyCallerCaptor = ArgumentCaptor.forClass(ReplyCaller.class);
                verify(replyCallerDirectory).addReplyCaller(anyString(),
                                                            replyCallerCaptor.capture(),
                                                            any(ExpiryDate.class));

                // pass the exception to the replyCaller
                replyCallerCaptor.getValue().error(expectedException);
                return null;
            }
        }).when(requestReplyManager).sendRequest(Mockito.<String> any(),
                                                 Mockito.<DiscoveryEntryWithMetaInfo> any(),
                                                 Mockito.<Request> any(),
                                                 Mockito.<MessagingQos> any());

        boolean exceptionThrown = false;
        String reply = "";
        final Future<String> future = proxy.asyncMethod(callback);
        try {
            // the test usually takes only 200 ms, so if we wait 1 sec, something has gone wrong
            reply = future.get(1000);
        } catch (JoynrCommunicationException e) {
            exceptionThrown = true;
        }
        Assert.assertTrue("exception must be thrown from get", exceptionThrown);
        verify(callback).onFailure(expectedException);
        verifyNoMoreInteractions(callback);
        Assert.assertEquals(RequestStatusCode.ERROR, future.getStatus().getCode());
        Assert.assertEquals("", reply);
    }

    @Test
    public void createProxySubscribeToBroadcast() throws Exception {
        NavigationProxy proxy = getNavigationProxy();

        long expiryDate = System.currentTimeMillis() + 30000;
        MulticastSubscriptionQos subscriptionQos = new MulticastSubscriptionQos().setExpiryDateMs(expiryDate);

        proxy.subscribeToLocationUpdateBroadcast(mock(LocationUpdateBroadcastListener.class), subscriptionQos);

        ArgumentCaptor<MulticastSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(MulticastSubscribeInvocation.class);

        verify(subscriptionManager,
               times(1)).registerMulticastSubscription(eq(fromParticipantId),
                                                       eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                       subscriptionRequest.capture());
        assertEquals("locationUpdate", subscriptionRequest.getValue().getSubscriptionName());
    }

    @Test
    public void createProxySubscribeAndUnsubscribeFromSelectiveBroadcast() throws Exception {
        NavigationProxy proxy = getNavigationProxy();

        long minInterval_ms = 0;
        long expiryDate = System.currentTimeMillis() + 30000;
        long publicationTtl_ms = 5000;
        OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos().setMinIntervalMs(minInterval_ms)
                                                                               .setExpiryDateMs(expiryDate)
                                                                               .setPublicationTtlMs(publicationTtl_ms);

        LocationUpdateSelectiveBroadcastFilterParameters filterParameter = new LocationUpdateSelectiveBroadcastFilterParameters();
        Future<String> subscriptionId = proxy.subscribeToLocationUpdateSelectiveBroadcast(mock(LocationUpdateSelectiveBroadcastListener.class),
                                                                                          subscriptionQos,
                                                                                          filterParameter);

        ArgumentCaptor<BroadcastSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(BroadcastSubscribeInvocation.class);

        verify(subscriptionManager,
               times(1)).registerBroadcastSubscription(eq(fromParticipantId),
                                                       eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                       subscriptionRequest.capture());

        assertEquals("locationUpdateSelective", subscriptionRequest.getValue().getBroadcastName());

        // now, let's remove the previous subscriptionRequest
        proxy.unsubscribeFromGuidanceActive(subscriptionId.get(100L));
        verify(subscriptionManager,
               times(1)).unregisterSubscription(eq(fromParticipantId),
                                                eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                eq(subscriptionId.get()),
                                                any(MessagingQos.class));
    }

    @Test
    public void createProxySubscribeAndUnsubscribeFromBroadcast() throws Exception {
        NavigationProxy proxy = getNavigationProxy();

        long expiryDate = System.currentTimeMillis() + 30000;
        MulticastSubscriptionQos subscriptionQos = new MulticastSubscriptionQos().setExpiryDateMs(expiryDate);

        Future<String> subscriptionId = proxy.subscribeToLocationUpdateBroadcast(mock(LocationUpdateBroadcastListener.class),
                                                                                 subscriptionQos);

        ArgumentCaptor<MulticastSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(MulticastSubscribeInvocation.class);

        verify(subscriptionManager,
               times(1)).registerMulticastSubscription(eq(fromParticipantId),
                                                       eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                       subscriptionRequest.capture());

        assertEquals("locationUpdate", subscriptionRequest.getValue().getSubscriptionName());

        // now, let's remove the previous subscriptionRequest
        proxy.unsubscribeFromGuidanceActive(subscriptionId.get(100L));
        verify(subscriptionManager,
               times(1)).unregisterSubscription(eq(fromParticipantId),
                                                eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                eq(subscriptionId.get()),
                                                any(MessagingQos.class));
    }

    @Test
    public void createProxySubscribeToBroadcastWithSubscriptionId() throws Exception {
        NavigationProxy proxy = getNavigationProxy();
        long expiryDate = System.currentTimeMillis() + 30000;
        MulticastSubscriptionQos subscriptionQos = new MulticastSubscriptionQos().setExpiryDateMs(expiryDate);

        String subscriptionId = createUuidString();
        Future<String> subscriptionId2 = proxy.subscribeToLocationUpdateBroadcast(subscriptionId,
                                                                                  mock(LocationUpdateBroadcastListener.class),
                                                                                  subscriptionQos);

        assertEquals(subscriptionId, subscriptionId2.get(500));

        ArgumentCaptor<MulticastSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(MulticastSubscribeInvocation.class);

        verify(subscriptionManager,
               times(1)).registerMulticastSubscription(eq(fromParticipantId),
                                                       eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                       subscriptionRequest.capture());

        assertEquals("locationUpdate", subscriptionRequest.getValue().getSubscriptionName());
        assertEquals(subscriptionId, subscriptionRequest.getValue().getSubscriptionId());
    }

    @Test
    public void createProxySubscribeAndUnsubscribeFromAttribute() throws Exception {
        NavigationProxy proxy = getNavigationProxy();

        long minInterval_ms = 0;
        long expiryDate = System.currentTimeMillis() + 30000;
        long publicationTtl_ms = 5000;
        SubscriptionQos subscriptionQos = new OnChangeSubscriptionQos().setMinIntervalMs(minInterval_ms)
                                                                       .setExpiryDateMs(expiryDate)
                                                                       .setPublicationTtlMs(publicationTtl_ms);

        abstract class BooleanSubscriptionListener implements AttributeSubscriptionListener<Boolean> {
        }
        Future<String> subscriptionId = proxy.subscribeToGuidanceActive(mock(BooleanSubscriptionListener.class),
                                                                        subscriptionQos);

        ArgumentCaptor<AttributeSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(AttributeSubscribeInvocation.class);

        verify(subscriptionManager,
               times(1)).registerAttributeSubscription(eq(fromParticipantId),
                                                       eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                       subscriptionRequest.capture());

        assertEquals("guidanceActive", subscriptionRequest.getValue().getAttributeName());
        assertEquals(subscriptionId.get(), subscriptionRequest.getValue().getSubscriptionId());
        // now, let's remove the previous subscriptionRequest
        proxy.unsubscribeFromGuidanceActive(subscriptionId.get());
        verify(subscriptionManager,
               times(1)).unregisterSubscription(eq(fromParticipantId),
                                                eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                eq(subscriptionId.get()),
                                                any(MessagingQos.class));
    }

    @Test
    public void createProxySubscribeToAttributeWithSubscriptionId() throws Exception {
        NavigationProxy proxy = getNavigationProxy();
        long minInterval_ms = 0;
        long expiryDate = System.currentTimeMillis() + 30000;
        long publicationTtl_ms = 5000;
        SubscriptionQos subscriptionQos = new OnChangeSubscriptionQos().setMinIntervalMs(minInterval_ms)
                                                                       .setExpiryDateMs(expiryDate)
                                                                       .setPublicationTtlMs(publicationTtl_ms);

        abstract class BooleanSubscriptionListener implements AttributeSubscriptionListener<Boolean> {
        }
        String subscriptionId = createUuidString();
        Future<String> subscriptionId2 = proxy.subscribeToGuidanceActive(subscriptionId,
                                                                         mock(BooleanSubscriptionListener.class),
                                                                         subscriptionQos);

        assertEquals(subscriptionId, subscriptionId2.get());

        ArgumentCaptor<AttributeSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(AttributeSubscribeInvocation.class);

        verify(subscriptionManager,
               times(1)).registerAttributeSubscription(eq(fromParticipantId),
                                                       eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                       subscriptionRequest.capture());

        assertEquals("guidanceActive", subscriptionRequest.getValue().getAttributeName());
        assertEquals(subscriptionId, subscriptionRequest.getValue().getSubscriptionId());
    }

    @Test
    public void createProxyUnSubscribeFromBroadcast() throws Exception {
        NavigationProxy proxy = getNavigationProxy();

        String subscriptionId = createUuidString();
        proxy.unsubscribeFromLocationUpdateBroadcast(subscriptionId);

        verify(subscriptionManager,
               times(1)).unregisterSubscription(eq(fromParticipantId),
                                                eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                eq(subscriptionId),
                                                any(MessagingQos.class));
    }

    public void createProxyAndCallFireAndForgetMethod(MessagingQos privateMessagingQos) {
        MessagingQos expectedMessagingQos;

        if (privateMessagingQos != null) {
            expectedMessagingQos = privateMessagingQos;
        } else {
            expectedMessagingQos = messagingQos;
        }

        ProxyBuilder<TestInterface> proxyBuilder = getProxyBuilder(TestInterface.class);
        TestInterface proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        if (privateMessagingQos != null) {
            proxy.methodFireAndForget(privateMessagingQos);
        } else {
            proxy.methodFireAndForget();
        }
        ArgumentCaptor<MessagingQos> messagingQosCaptor = ArgumentCaptor.forClass(MessagingQos.class);
        ArgumentCaptor<OneWayRequest> oneWayRequestCaptor = ArgumentCaptor.forClass(OneWayRequest.class);
        verify(requestReplyManager).sendOneWayRequest(Mockito.<String> any(),
                                                      Mockito.<Set<DiscoveryEntryWithMetaInfo>> any(),
                                                      oneWayRequestCaptor.capture(),
                                                      messagingQosCaptor.capture());
        Assert.assertEquals(expectedMessagingQos.getRoundTripTtl_ms(),
                            messagingQosCaptor.getValue().getRoundTripTtl_ms());
        Assert.assertFalse(oneWayRequestCaptor.getValue().hasParams());
    }

    @Test
    public void createProxyAndCallFireAndForgetMethodWithDefaultTtl() throws Exception {
        MessagingQos privateMessagingQos = null;
        createProxyAndCallFireAndForgetMethod(privateMessagingQos);
    }

    @Test
    public void createProxyAndCallFireAndForgetMethodWithSpecialTtl() throws Exception {
        MessagingQos privateMessagingQos = new MessagingQos(120000);
        Assert.assertTrue(messagingQos.getRoundTripTtl_ms() != privateMessagingQos.getRoundTripTtl_ms());
        createProxyAndCallFireAndForgetMethod(privateMessagingQos);
    }
}
