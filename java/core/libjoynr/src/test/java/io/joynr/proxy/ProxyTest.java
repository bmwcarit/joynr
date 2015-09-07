package io.joynr.proxy;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.capabilities.CapabilitiesCallback;
import io.joynr.capabilities.CapabilityEntry;
import io.joynr.capabilities.CapabilityEntryImpl;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.dispatcher.Dispatcher;
import io.joynr.dispatcher.ReplyCaller;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.dispatcher.SynchronizedReplyCaller;
import io.joynr.dispatcher.rpc.JoynrAsyncInterface;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.dispatcher.rpc.JoynrSyncInterface;
import io.joynr.dispatcher.rpc.RequestStatusCode;
import io.joynr.dispatcher.rpc.RpcUtils;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessLibjoynrMessagingSkeleton;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.pubsub.subscription.SubscriptionManager;

import java.io.IOException;
import java.util.ArrayList;
import java.util.UUID;

import joynr.exceptions.ApplicationException;
import joynr.OnChangeSubscriptionQos;
import joynr.Reply;
import joynr.Request;
import joynr.system.routingtypes.ChannelAddress;
import joynr.types.ProviderQos;
import joynr.vehicle.NavigationBroadcastInterface.LocationUpdateBroadcastListener;
import joynr.vehicle.NavigationBroadcastInterface.LocationUpdateSelectiveBroadcastFilterParameters;
import joynr.vehicle.NavigationBroadcastInterface.LocationUpdateSelectiveBroadcastListener;
import joynr.vehicle.NavigationProxy;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.node.TextNode;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.assistedinject.FactoryModuleBuilder;

public class ProxyTest {
    private DiscoveryQos discoveryQos;
    private MessagingQos messagingQos;
    @Mock
    private RequestReplyDispatcher requestReplyDispatcher;
    @Mock
    private RequestReplySender requestReplySender;
    @Mock
    SubscriptionManager subscriptionManager;
    @Mock
    MessageSender messageSender;
    @Mock
    MessageRouter messageRouter;
    @Mock
    Dispatcher dispatcher;
    @Mock
    RoutingTable routingTable;

    @Mock
    private LocalCapabilitiesDirectory capabilitiesClient;

    private String domain;
    private String toParticipantId;
    private String asyncReplyText = "replyText";

    @Mock
    private Callback<String> callback;

    private ProxyBuilderFactory proxyBuilderFactory;

    private enum ApplicationErrors {
        ERROR_VALUE_1, ERROR_VALUE_2, ERROR_VALUE_3
    }

    public interface SyncTestInterface extends JoynrSyncInterface {
        String method1();

        String methodWithApplicationError() throws ApplicationException;
    }

    public static class StringTypeRef extends TypeReference<String> {
    }

    public interface AsyncTestInterface extends JoynrAsyncInterface {
        Future<String> asyncMethod(@JoynrRpcCallback(deserializationType = StringTypeRef.class) Callback<String> callback);

        Future<String> asyncMethodWithApplicationError(@JoynrRpcCallback(deserializationType = StringTypeRef.class) Callback<String> callback);
    }

    public interface TestInterface extends SyncTestInterface, AsyncTestInterface {
        public static final String INTERFACE_NAME = "TestInterface";
    }

    @Before
    public void setUp() throws Exception {
        toParticipantId = "TestParticipantId";
        MockitoAnnotations.initMocks(this);
        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                requestStaticInjection(RpcUtils.class);
                bind(RequestReplyDispatcher.class).toInstance(requestReplyDispatcher);
                bind(RequestReplySender.class).toInstance(requestReplySender);
                bind(SubscriptionManager.class).toInstance(subscriptionManager);
                bind(MessageSender.class).toInstance(messageSender);
                bind(MessageRouter.class).toInstance(messageRouter);
                bind(RoutingTable.class).toInstance(routingTable);
                install(new FactoryModuleBuilder().implement(ProxyInvocationHandler.class,
                                                             ProxyInvocationHandlerImpl.class)
                                                  .build(ProxyInvocationHandlerFactory.class));
            }

        });

        proxyBuilderFactory = new ProxyBuilderFactory(capabilitiesClient,
                                                      injector.getInstance(ProxyInvocationHandlerFactory.class),
                                                      messageRouter,
                                                      new InProcessAddress(new InProcessLibjoynrMessagingSkeleton(dispatcher)));

        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();
                ArrayList<CapabilityEntry> fakeCapabilitiesResult = new ArrayList<CapabilityEntry>();
                fakeCapabilitiesResult.add(new CapabilityEntryImpl(domain,
                                                                   TestInterface.INTERFACE_NAME,
                                                                   new ProviderQos(),
                                                                   toParticipantId,
                                                                   System.currentTimeMillis(),
                                                                   new ChannelAddress("testChannelId")));
                ((CapabilitiesCallback) args[3]).processCapabilitiesReceived(fakeCapabilitiesResult);
                return null;
            }
        }).when(capabilitiesClient).lookup(Mockito.<String> any(),
                                           Mockito.<String> any(),
                                           Mockito.<DiscoveryQos> any(),
                                           Mockito.<CapabilitiesCallback> any());

        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();
                AttributeSubscribeInvocation request = (AttributeSubscribeInvocation) args[2];
                if (request.getSubscriptionId() == null) {
                    request.setSubscriptionId(UUID.randomUUID().toString());
                }
                return null;
            }
        }).when(subscriptionManager).registerAttributeSubscription(any(String.class),
                                                                   any(String.class),
                                                                   Mockito.any(AttributeSubscribeInvocation.class));

        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                Object[] args = invocation.getArguments();
                BroadcastSubscribeInvocation request = (BroadcastSubscribeInvocation) args[2];
                if (request.getSubscriptionId() == null) {
                    request.setSubscriptionId(UUID.randomUUID().toString());
                }
                return null;
            }
        }).when(subscriptionManager).registerBroadcastSubscription(any(String.class),
                                                                   any(String.class),
                                                                   Mockito.any(BroadcastSubscribeInvocation.class));

        domain = "TestDomain";

        discoveryQos = new DiscoveryQos(10000, ArbitrationStrategy.HighestPriority, Long.MAX_VALUE);
        messagingQos = new MessagingQos();
    }

    private <T extends JoynrInterface> ProxyBuilder<T> getProxyBuilder(final Class<T> interfaceClass) {
        return proxyBuilderFactory.get(domain, interfaceClass);
    }

    @Test
    public void createProxyAndCallSyncMethodSuccess() throws Exception {
        String requestReplyId = "createProxyAndCallSyncMethod_requestReplyId";
        Mockito.when(requestReplySender.sendSyncRequest(Mockito.<String> any(),
                                                        Mockito.<String> any(),
                                                        Mockito.<Request> any(),
                                                        Mockito.<SynchronizedReplyCaller> any(),
                                                        Mockito.anyLong())).thenReturn(new Reply(requestReplyId,
                                                                                                 "Answer"));

        ProxyBuilder<TestInterface> proxyBuilder = getProxyBuilder(TestInterface.class);
        TestInterface proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        String result = proxy.method1();
        Assert.assertEquals("Answer", result);

    }

    @Test
    public void createProxyAndCallSyncMethodFailWithApplicationError() throws Exception {
        String requestReplyId = "createProxyAndCallSyncMethod_requestReplyId";
        Mockito.when(requestReplySender.sendSyncRequest(Mockito.<String> any(),
                                                        Mockito.<String> any(),
                                                        Mockito.<Request> any(),
                                                        Mockito.<SynchronizedReplyCaller> any(),
                                                        Mockito.anyLong()))
               .thenReturn(new Reply(requestReplyId, new ApplicationException(ApplicationErrors.ERROR_VALUE_2,
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
                                                     "syncMethodCallApplicationException"), exception);
    }

    @Test
    public void createProxyAndCallAsyncMethodSuccess() throws Exception {
        ProxyBuilder<TestInterface> proxyBuilder = getProxyBuilder(TestInterface.class);
        TestInterface proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        // when joynrMessageSender1.sendRequest is called, get the replyCaller from the mock dispatcher and call
        // messageCallback on it.
        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws JsonParseException, JsonMappingException,
                                                             IOException {
                // capture the replyCaller passed into the dispatcher for calling later
                ArgumentCaptor<ReplyCaller> replyCallerCaptor = ArgumentCaptor.forClass(ReplyCaller.class);
                verify(requestReplyDispatcher).addReplyCaller(anyString(), replyCallerCaptor.capture(), anyLong());

                String requestReplyId = "createProxyAndCallAsyncMethodSuccess_requestReplyId";
                // pass the response to the replyCaller
                replyCallerCaptor.getValue().messageCallBack(new Reply(requestReplyId, new TextNode(asyncReplyText)));
                return null;
            }
        }).when(requestReplySender).sendRequest(Mockito.<String> any(),
                                                Mockito.<String> any(),
                                                Mockito.<Request> any(),
                                                Mockito.anyLong());
        final Future<String> future = proxy.asyncMethod(callback);

        // the test usually takes only 200 ms, so if we wait 1 sec, something has gone wrong
        String reply = future.getReply(1000);

        verify(callback).resolve(asyncReplyText);
        Assert.assertEquals(RequestStatusCode.OK, future.getStatus().getCode());
        Assert.assertEquals(asyncReplyText, reply);
    }

    @Test
    public void createProxyAndCallAsyncMethodFailWithApplicationError() throws Exception {
        final ApplicationException expected = new ApplicationException(ApplicationErrors.ERROR_VALUE_3,
                                                                       "TEST: createProxyAndCallAsyncMethodFailWithApplicationError");
        ProxyBuilder<TestInterface> proxyBuilder = getProxyBuilder(TestInterface.class);
        TestInterface proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        // when joynrMessageSender1.sendRequest is called, get the replyCaller from the mock dispatcher and call
        // messageCallback on it.
        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws JsonParseException, JsonMappingException,
                                                             IOException {
                // capture the replyCaller passed into the dispatcher for calling later
                ArgumentCaptor<ReplyCaller> replyCallerCaptor = ArgumentCaptor.forClass(ReplyCaller.class);
                verify(requestReplyDispatcher).addReplyCaller(anyString(), replyCallerCaptor.capture(), anyLong());

                String requestReplyId = "createProxyAndCallAsyncMethodSuccess_requestReplyId";
                // pass the response to the replyCaller
                replyCallerCaptor.getValue().messageCallBack(new Reply(requestReplyId, expected));
                return null;
            }
        }).when(requestReplySender).sendRequest(Mockito.<String> any(),
                                                Mockito.<String> any(),
                                                Mockito.<Request> any(),
                                                Mockito.anyLong());
        final Future<String> future = proxy.asyncMethodWithApplicationError(callback);

        // the test usually takes only 200 ms, so if we wait 1 sec, something has gone wrong
        try {
            future.getReply(1000);
            Assert.fail("Should throw ApplicationException");
        } catch (ApplicationException e) {
            Assert.assertEquals(expected, e);
        }

        verify(callback).onFailure(expected);
        Assert.assertEquals(RequestStatusCode.ERROR, future.getStatus().getCode());
    }

    @Test
    public void createProxyAndCallAsyncMethodFail() throws Exception {

        // Expect this exception to be passed back to the callback onFailure and thrown in the future
        final JoynrCommunicationException expectedException = new JoynrCommunicationException();
        // final JoynCommunicationException expectedException = null;

        ProxyBuilder<TestInterface> proxyBuilder = getProxyBuilder(TestInterface.class);
        TestInterface proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        // when joynrMessageSender1.sendRequest is called, get the replyCaller from the mock dispatcher and call
        // messageCallback on it.
        Mockito.doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws JsonParseException, JsonMappingException,
                                                             IOException {
                // capture the replyCaller passed into the dispatcher for calling later
                ArgumentCaptor<ReplyCaller> replyCallerCaptor = ArgumentCaptor.forClass(ReplyCaller.class);
                verify(requestReplyDispatcher).addReplyCaller(anyString(), replyCallerCaptor.capture(), anyLong());

                // pass the exception to the replyCaller
                replyCallerCaptor.getValue().error(expectedException);
                return null;
            }
        }).when(requestReplySender).sendRequest(Mockito.<String> any(),
                                                Mockito.<String> any(),
                                                Mockito.<Request> any(),
                                                Mockito.anyLong());

        boolean exceptionThrown = false;
        String reply = "";
        final Future<String> future = proxy.asyncMethod(callback);
        try {
            // the test usually takes only 200 ms, so if we wait 1 sec, something has gone wrong
            reply = future.getReply(1000);
        } catch (JoynrCommunicationException e) {
            exceptionThrown = true;
        }
        Assert.assertTrue("exception must be thrown from getReply", exceptionThrown);
        verify(callback).onFailure(expectedException);
        verifyNoMoreInteractions(callback);
        Assert.assertEquals(RequestStatusCode.ERROR, future.getStatus().getCode());
        Assert.assertEquals("", reply);
    }

    @Test
    public void createProxySubscribeToBroadcast() throws Exception {
        ProxyBuilder<NavigationProxy> proxyBuilder = getProxyBuilder(NavigationProxy.class);
        String fromParticipantId = proxyBuilder.getParticipantId();
        NavigationProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        long minInterval_ms = 0;
        long expiryDate = System.currentTimeMillis() + 30000;
        long publicationTtl_ms = 5000;
        OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms,
                                                                              expiryDate,
                                                                              publicationTtl_ms);

        proxy.subscribeToLocationUpdateBroadcast(mock(LocationUpdateBroadcastListener.class), subscriptionQos);

        ArgumentCaptor<BroadcastSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(BroadcastSubscribeInvocation.class);

        verify(subscriptionManager, times(1)).registerBroadcastSubscription(eq(fromParticipantId),
                                                                            eq(toParticipantId),
                                                                            subscriptionRequest.capture());
        assertEquals("locationUpdate", subscriptionRequest.getValue().getBroadcastName());
    }

    @Test
    public void createProxySubscribeAndUnsubscribeFromSelectiveBroadcast() throws Exception {
        ProxyBuilder<NavigationProxy> proxyBuilder = getProxyBuilder(NavigationProxy.class);
        String fromParticipantId = proxyBuilder.getParticipantId();
        NavigationProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        long minInterval_ms = 0;
        long expiryDate = System.currentTimeMillis() + 30000;
        long publicationTtl_ms = 5000;
        OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms,
                                                                              expiryDate,
                                                                              publicationTtl_ms);

        LocationUpdateSelectiveBroadcastFilterParameters filterParameter = new LocationUpdateSelectiveBroadcastFilterParameters();
        String subscriptionId = proxy.subscribeToLocationUpdateSelectiveBroadcast(mock(LocationUpdateSelectiveBroadcastListener.class),
                                                                                  subscriptionQos,
                                                                                  filterParameter);

        ArgumentCaptor<BroadcastSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(BroadcastSubscribeInvocation.class);

        verify(subscriptionManager, times(1)).registerBroadcastSubscription(eq(fromParticipantId),
                                                                            eq(toParticipantId),
                                                                            subscriptionRequest.capture());

        assertEquals("locationUpdateSelective", subscriptionRequest.getValue().getBroadcastName());

        // now, let's remove the previous subscriptionRequest
        proxy.unsubscribeFromGuidanceActive(subscriptionId);
        verify(subscriptionManager, times(1)).unregisterSubscription(eq(fromParticipantId),
                                                                     eq(toParticipantId),
                                                                     eq(subscriptionId),
                                                                     any(MessagingQos.class));
    }

    @Test
    public void createProxySubscribeAndUnsubscribeFromBroadcast() throws Exception {
        ProxyBuilder<NavigationProxy> proxyBuilder = getProxyBuilder(NavigationProxy.class);
        String fromParticipantId = proxyBuilder.getParticipantId();
        NavigationProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        long minInterval_ms = 0;
        long expiryDate = System.currentTimeMillis() + 30000;
        long publicationTtl_ms = 5000;
        OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms,
                                                                              expiryDate,
                                                                              publicationTtl_ms);

        String subscriptionId = proxy.subscribeToLocationUpdateBroadcast(mock(LocationUpdateBroadcastListener.class),
                                                                         subscriptionQos);

        ArgumentCaptor<BroadcastSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(BroadcastSubscribeInvocation.class);

        verify(subscriptionManager, times(1)).registerBroadcastSubscription(eq(fromParticipantId),
                                                                            eq(toParticipantId),
                                                                            subscriptionRequest.capture());

        assertEquals("locationUpdate", subscriptionRequest.getValue().getBroadcastName());

        // now, let's remove the previous subscriptionRequest
        proxy.unsubscribeFromGuidanceActive(subscriptionId);
        verify(subscriptionManager, times(1)).unregisterSubscription(eq(fromParticipantId),
                                                                     eq(toParticipantId),
                                                                     eq(subscriptionId),
                                                                     any(MessagingQos.class));
    }

    @Test
    public void createProxySubscribeToBroadcastWithSubscriptionId() throws Exception {
        ProxyBuilder<NavigationProxy> proxyBuilder = getProxyBuilder(NavigationProxy.class);
        String fromParticipantId = proxyBuilder.getParticipantId();
        NavigationProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        long minInterval_ms = 0;
        long expiryDate = System.currentTimeMillis() + 30000;
        long publicationTtl_ms = 5000;
        OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms,
                                                                              expiryDate,
                                                                              publicationTtl_ms);

        String subscriptionId = UUID.randomUUID().toString();
        String subscriptionId2 = proxy.subscribeToLocationUpdateBroadcast(mock(LocationUpdateBroadcastListener.class),
                                                                          subscriptionQos,
                                                                          subscriptionId);

        assertEquals(subscriptionId, subscriptionId2);

        ArgumentCaptor<BroadcastSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(BroadcastSubscribeInvocation.class);

        verify(subscriptionManager, times(1)).registerBroadcastSubscription(eq(fromParticipantId),
                                                                            eq(toParticipantId),
                                                                            subscriptionRequest.capture());

        assertEquals("locationUpdate", subscriptionRequest.getValue().getBroadcastName());
        assertEquals(subscriptionId, subscriptionRequest.getValue().getSubscriptionId());
    }

    @Test
    public void createProxySubscribeAndUnsubscribeFromAttribute() throws Exception {
        ProxyBuilder<NavigationProxy> proxyBuilder = getProxyBuilder(NavigationProxy.class);
        String fromParticipantId = proxyBuilder.getParticipantId();
        NavigationProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        long minInterval_ms = 0;
        long expiryDate = System.currentTimeMillis() + 30000;
        long publicationTtl_ms = 5000;
        SubscriptionQos subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms, expiryDate, publicationTtl_ms);

        abstract class BooleanSubscriptionListener implements AttributeSubscriptionListener<Boolean> {
        }
        ;
        String subscriptionId = proxy.subscribeToGuidanceActive(mock(BooleanSubscriptionListener.class),
                                                                subscriptionQos);

        ArgumentCaptor<AttributeSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(AttributeSubscribeInvocation.class);

        verify(subscriptionManager, times(1)).registerAttributeSubscription(eq(fromParticipantId),
                                                                            eq(toParticipantId),
                                                                            subscriptionRequest.capture());

        assertEquals("guidanceActive", subscriptionRequest.getValue().getAttributeName());
        assertEquals(subscriptionId, subscriptionRequest.getValue().getSubscriptionId());

        // now, let's remove the previous subscriptionRequest
        proxy.unsubscribeFromGuidanceActive(subscriptionId);
        verify(subscriptionManager, times(1)).unregisterSubscription(eq(fromParticipantId),
                                                                     eq(toParticipantId),
                                                                     eq(subscriptionId),
                                                                     any(MessagingQos.class));
    }

    @Test
    public void createProxySubscribeToAttributeWithSubscriptionId() throws Exception {
        ProxyBuilder<NavigationProxy> proxyBuilder = getProxyBuilder(NavigationProxy.class);
        String fromParticipantId = proxyBuilder.getParticipantId();
        NavigationProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();
        long minInterval_ms = 0;
        long expiryDate = System.currentTimeMillis() + 30000;
        long publicationTtl_ms = 5000;
        SubscriptionQos subscriptionQos = new OnChangeSubscriptionQos(minInterval_ms, expiryDate, publicationTtl_ms);

        abstract class BooleanSubscriptionListener implements AttributeSubscriptionListener<Boolean> {
        }
        ;
        String subscriptionId = UUID.randomUUID().toString();
        String subscriptionId2 = proxy.subscribeToGuidanceActive(mock(BooleanSubscriptionListener.class),
                                                                 subscriptionQos,
                                                                 subscriptionId);

        assertEquals(subscriptionId, subscriptionId2);

        ArgumentCaptor<AttributeSubscribeInvocation> subscriptionRequest = ArgumentCaptor.forClass(AttributeSubscribeInvocation.class);

        verify(subscriptionManager, times(1)).registerAttributeSubscription(eq(fromParticipantId),
                                                                            eq(toParticipantId),
                                                                            subscriptionRequest.capture());

        assertEquals("guidanceActive", subscriptionRequest.getValue().getAttributeName());
        assertEquals(subscriptionId, subscriptionRequest.getValue().getSubscriptionId());
    }

    @Test
    public void createProxyUnSubscribeFromBroadcast() throws Exception {
        ProxyBuilder<NavigationProxy> proxyBuilder = getProxyBuilder(NavigationProxy.class);
        String fromParticipantId = proxyBuilder.getParticipantId();
        NavigationProxy proxy = proxyBuilder.setMessagingQos(messagingQos).setDiscoveryQos(discoveryQos).build();

        String subscriptionId = UUID.randomUUID().toString();
        proxy.unsubscribeFromLocationUpdateBroadcast(subscriptionId);

        verify(subscriptionManager, times(1)).unregisterSubscription(eq(fromParticipantId),
                                                                     eq(toParticipantId),
                                                                     eq(subscriptionId),
                                                                     any(MessagingQos.class));
    }
}
