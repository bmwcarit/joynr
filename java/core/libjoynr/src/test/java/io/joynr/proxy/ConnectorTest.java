/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2021 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isA;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import com.fasterxml.jackson.databind.JsonMappingException;

import io.joynr.Async;
import io.joynr.StatelessAsync;
import io.joynr.Sync;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.dispatcher.rpc.annotation.FireAndForget;
import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcBroadcast;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcSubscription;
import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.SubscriptionException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.proxy.ConnectorTest.TestBroadcastInterface.TestBroadcastListener;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import joynr.BroadcastFilterParameters;
import joynr.OnChangeSubscriptionQos;
import joynr.OneWayRequest;
import joynr.PeriodicSubscriptionQos;
import joynr.Reply;
import joynr.Request;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.Localisation.GpsPosition;
import joynr.vehicle.LocalisationSubscriptionInterface;

public class ConnectorTest {

    @Mock
    private ReplyCallerDirectory replyCallerDirectory;
    @Mock
    private SubscriptionManager subscriptionManager;
    @Mock
    private RequestReplyManager requestReplyManager;
    @Mock
    private StatelessAsyncIdCalculator statelessAsyncIdCalculator;
    @Mock
    private MessageRouter messageRouter;
    @Mock
    private Address libJoynrMessagingAddress;
    @Mock
    private Callback<Void> voidCallback;

    private String fromParticipantId;
    private String statelessAsyncParticipantId;
    private String toParticipantId;
    private DiscoveryEntryWithMetaInfo toDiscoveryEntry;
    private Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries;
    private MessagingQos qosSettings;
    private Object proxy;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        fromParticipantId = "fromParticipantId";
        toParticipantId = "toParticipantId";
        toDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        toDiscoveryEntry.setParticipantId(toParticipantId);
        toDiscoveryEntries = new HashSet<>(Arrays.asList(toDiscoveryEntry));
        qosSettings = new MessagingQos();
        proxy = new Object();
    }

    interface TestProxyInterface extends TestSyncInterface, TestAsyncInterface {

    }

    interface TestSubscriptionInterface {

        @JoynrRpcSubscription(attributeName = "testAttribute", attributeType = String.class)
        public Future<String> subscribeToTestAttribute(AttributeSubscriptionListener<String> listener,
                                                       SubscriptionQos subscriptionQos);

        public void unsubscribeFromTestAttribute(String subscriptionId);
    }

    interface TestBroadcastInterface {

        public interface TestBroadcastListener extends BroadcastSubscriptionListener {
            public void onReceive(String testString);
        }

        public class TestBroadcastAdapter implements TestBroadcastListener {
            @Override
            public void onReceive(String testString) {
                // empty implementation
            }

            @Override
            public void onError(SubscriptionException error) {
                // empty implementation
            }

            @Override
            public void onSubscribed(String subscriptionId) {
                // empty implementation
            }
        }

        @JoynrRpcBroadcast(broadcastName = "testBroadcast")
        abstract Future<String> subscribeToTestBroadcast(TestBroadcastListener subscriptionListener,
                                                         OnChangeSubscriptionQos subscriptionQos,
                                                         BroadcastFilterParameters filterParameters);

        @JoynrMulticast(name = "testMulticast")
        abstract Future<String> subscribeToTestMulticast(TestBroadcastListener subscriptionListener,
                                                         OnChangeSubscriptionQos subscriptionQos,
                                                         String... partitions);

        abstract void unsubscribeFromTestBroadcast(String subscriptionId);

    }

    @Sync
    interface TestSyncInterface {
        void methodWithoutParameters();
    }

    @FireAndForget
    interface TestFireAndForgetInterface {
        void methodWithoutParameters();
    }

    @Async
    interface TestAsyncInterface {
        void someMethodwithoutAnnotations(Integer a, String b) throws JsonMappingException;

        Future<Void> methodWithoutParameters(@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback);
    }

    @StatelessAsync
    interface TestStatelessAsyncInterface {
        @StatelessCallbackCorrelation("correlationId")
        void testMethod(MessageIdCallback messageIdCallback);
    }

    @Test
    public void asyncMethodCallWithoutAnnotationThrowsException() throws JoynrIllegalStateException {

        ConnectorInvocationHandler connector = createConnector();
        assertNotNull(connector);
        try {
            Future<String> future = new Future<String>();
            connector.executeAsyncMethod(proxy,
                                         TestAsyncInterface.class.getDeclaredMethod("someMethodwithoutAnnotations",
                                                                                    Integer.class,
                                                                                    String.class),
                                         new Object[]{ 1, "test" },
                                         future);
            fail("Calling a method with missing callback annotation did not throw an exception.");
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            assertEquals(JoynrIllegalStateException.class, e.getClass());
        }

    }

    @Test
    public void subscriptionMethodCallWithNoExpiryDate() throws JoynrIllegalStateException {

        ConnectorInvocationHandler connector = createConnector();
        assertNotNull(connector);
        try {
            Future<String> future = new Future<String>();
            String subscriptionId = "subscriptionId";
            PeriodicSubscriptionQos subscriptionQos = new PeriodicSubscriptionQos();
            subscriptionQos.setPeriodMs(1000).setExpiryDateMs(0).setAlertAfterIntervalMs(1000);
            AttributeSubscriptionListener<GpsPosition> listener = new AttributeSubscriptionAdapter<GpsPosition>();
            Object[] args = new Object[]{ listener, subscriptionQos, subscriptionId };
            Method method = LocalisationSubscriptionInterface.class.getDeclaredMethod("subscribeToGPSPosition",
                                                                                      String.class,
                                                                                      AttributeSubscriptionListener.class,
                                                                                      SubscriptionQos.class);
            AttributeSubscribeInvocation attributeSubscription = new AttributeSubscribeInvocation(method,
                                                                                                  args,
                                                                                                  future,
                                                                                                  proxy);
            connector.executeSubscriptionMethod(attributeSubscription);
            verify(subscriptionManager,
                   times(1)).registerAttributeSubscription(eq(fromParticipantId),
                                                           eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                           eq(attributeSubscription));
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            fail("Calling a subscription method with no expiry date throws an exception.");
        }

    }

    @Test
    public void unsubscriptionMethodCall() throws JoynrIllegalStateException {

        ConnectorInvocationHandler connector = createConnector();
        assertNotNull(connector);
        try {
            Future<String> future = new Future<String>();
            String subscriptionId = "subscriptionId";
            Object[] args = new Object[]{ subscriptionId };
            Method method = LocalisationSubscriptionInterface.class.getDeclaredMethod("unsubscribeFromGPSPosition",
                                                                                      String.class);
            UnsubscribeInvocation unsubscribeInvocation = new UnsubscribeInvocation(method, args, future);
            connector.executeSubscriptionMethod(unsubscribeInvocation);
            verify(subscriptionManager,
                   times(1)).unregisterSubscription(eq(fromParticipantId),
                                                    eq(new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(toDiscoveryEntry))),
                                                    eq(subscriptionId),
                                                    any(MessagingQos.class));
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            fail("Calling a subscription method with no expiry date throws an exception.");
        }
    }

    @Test
    public void asyncMethodCallCallsRequestReplyManagerWithCorrectArguments() {
        ConnectorInvocationHandler connector = createConnector();
        assertNotNull(connector);

        ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);
        Future<Void> future = new Future<Void>();
        try {
            Method method = TestAsyncInterface.class.getDeclaredMethod("methodWithoutParameters", Callback.class);
            connector.executeAsyncMethod(proxy, method, new Object[]{ voidCallback }, future);
            verify(requestReplyManager, times(1)).sendRequest(eq(fromParticipantId),
                                                              eq(toDiscoveryEntry),
                                                              requestCaptor.capture(),
                                                              eq(qosSettings));

            Request actualRequest = requestCaptor.getValue();
            Request expectedRequest = new Request(method.getName(),
                                                  new Object[]{},
                                                  new String[]{},
                                                  actualRequest.getRequestReplyId());
            assertEquals(expectedRequest, actualRequest);
        } catch (Exception e) {
            fail("Unexpected exception from async method call: " + e);
        }
    }

    @Test
    public void syncMethodCallCallsRequestReplyManagerWithCorrectArguments() {
        ConnectorInvocationHandler connector = createConnector();
        assertNotNull(connector);

        ArgumentCaptor<Request> requestCaptor = ArgumentCaptor.forClass(Request.class);
        try {
            Method method = TestSyncInterface.class.getDeclaredMethod("methodWithoutParameters");
            when(requestReplyManager.sendSyncRequest(any(String.class),
                                                     any(DiscoveryEntryWithMetaInfo.class),
                                                     any(Request.class),
                                                     any(SynchronizedReplyCaller.class),
                                                     any(MessagingQos.class))).thenReturn(new Reply());
            connector.executeSyncMethod(method, new Object[]{});
            verify(requestReplyManager, times(1)).sendSyncRequest(eq(fromParticipantId),
                                                                  eq(toDiscoveryEntry),
                                                                  requestCaptor.capture(),
                                                                  isA(SynchronizedReplyCaller.class),
                                                                  eq(qosSettings));

            Request actualRequest = requestCaptor.getValue();
            Request expectedRequest = new Request(method.getName(),
                                                  new Object[]{},
                                                  new String[]{},
                                                  actualRequest.getRequestReplyId());
            assertEquals(expectedRequest, actualRequest);
        } catch (Exception e) {
            fail("Unexpected exception from sync method call: " + e);
        }
    }

    @Test
    public void oneWayMethodCallCallsRequestReplyManagerWithCorrectArguments() {
        ConnectorInvocationHandler connector = createConnector();
        assertNotNull(connector);

        ArgumentCaptor<OneWayRequest> requestCaptor = ArgumentCaptor.forClass(OneWayRequest.class);
        try {
            Method method = TestFireAndForgetInterface.class.getDeclaredMethod("methodWithoutParameters");
            connector.executeOneWayMethod(method, new Object[]{});
            verify(requestReplyManager, times(1)).sendOneWayRequest(eq(fromParticipantId),
                                                                    eq(toDiscoveryEntries),
                                                                    requestCaptor.capture(),
                                                                    eq(qosSettings));

            OneWayRequest actualRequest = requestCaptor.getValue();
            OneWayRequest expectedRequest = new OneWayRequest(method.getName(), new Object[]{}, new String[]{});
            assertEquals(expectedRequest, actualRequest);
        } catch (Exception e) {
            fail("Unexpected exception from one way method call: " + e);
        }
    }

    @Test
    public void unsubscriptionMethodCallCallsSubscriptionManagerWithCorrectArguments() {
        String subscriptionId = "subscriptionId";
        ConnectorInvocationHandler connector = createConnector();
        assertNotNull(connector);

        try {
            Method method = TestSubscriptionInterface.class.getDeclaredMethod("unsubscribeFromTestAttribute",
                                                                              String.class);
            UnsubscribeInvocation invocation = new UnsubscribeInvocation(method, new Object[]{ subscriptionId }, null);
            connector.executeSubscriptionMethod(invocation);
            verify(subscriptionManager, times(1)).unregisterSubscription(fromParticipantId,
                                                                         toDiscoveryEntries,
                                                                         subscriptionId,
                                                                         qosSettings);
        } catch (Exception e) {
            fail("Unexpected exception from unsubscribe call: " + e);
        }
    }

    @Test
    public void subscribeToAttributeCallCallsSubscriptionManagerWithCorrectArguments() {
        AttributeSubscriptionListener<String> listener = new AttributeSubscriptionAdapter<>();
        SubscriptionQos subscriptionQos = new OnChangeSubscriptionQos();
        ConnectorInvocationHandler connector = createConnector();
        assertNotNull(connector);

        try {
            Method method = TestSubscriptionInterface.class.getDeclaredMethod("subscribeToTestAttribute",
                                                                              AttributeSubscriptionListener.class,
                                                                              SubscriptionQos.class);
            AttributeSubscribeInvocation invocation = new AttributeSubscribeInvocation(method,
                                                                                       new Object[]{ listener,
                                                                                               subscriptionQos },
                                                                                       null,
                                                                                       proxy);
            connector.executeSubscriptionMethod(invocation);
            verify(subscriptionManager, times(1)).registerAttributeSubscription(fromParticipantId,
                                                                                toDiscoveryEntries,
                                                                                invocation);
        } catch (Exception e) {
            fail("Unexpected exception from attribute subscribe call: " + e);
        }
    }

    @Test
    public void subscribeToBroadcastCallCallsSubscriptionManagerWithCorrectArguments() {
        TestBroadcastListener listener = new TestBroadcastInterface.TestBroadcastAdapter();
        OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos();
        ConnectorInvocationHandler connector = createConnector();
        assertNotNull(connector);

        try {
            Method method = TestBroadcastInterface.class.getDeclaredMethod("subscribeToTestBroadcast",
                                                                           TestBroadcastListener.class,
                                                                           OnChangeSubscriptionQos.class,
                                                                           BroadcastFilterParameters.class);
            BroadcastSubscribeInvocation invocation = new BroadcastSubscribeInvocation(method,
                                                                                       new Object[]{ listener,
                                                                                               subscriptionQos,
                                                                                               new BroadcastFilterParameters() },
                                                                                       null,
                                                                                       proxy);
            connector.executeSubscriptionMethod(invocation);
            verify(subscriptionManager, times(1)).registerBroadcastSubscription(fromParticipantId,
                                                                                toDiscoveryEntries,
                                                                                invocation);
        } catch (Exception e) {
            fail("Unexpected exception from broadcast subscribe call: " + e);
        }
    }

    @Test
    public void subscribeToMulticastCallCallsSubscriptionManagerWithCorrectArguments() { // TODO
        TestBroadcastListener listener = new TestBroadcastInterface.TestBroadcastAdapter();
        OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos();
        String[] partitions = new String[]{ "partition1", "partition2", "partition3" };
        ConnectorInvocationHandler connector = createConnector();
        assertNotNull(connector);

        try {
            Method method = TestBroadcastInterface.class.getDeclaredMethod("subscribeToTestMulticast",
                                                                           TestBroadcastListener.class,
                                                                           OnChangeSubscriptionQos.class,
                                                                           String[].class);
            MulticastSubscribeInvocation invocation = new MulticastSubscribeInvocation(method,
                                                                                       new Object[]{ listener,
                                                                                               subscriptionQos,
                                                                                               partitions },
                                                                                       null,
                                                                                       proxy);
            connector.executeSubscriptionMethod(invocation);
            verify(subscriptionManager, times(1)).registerMulticastSubscription(fromParticipantId,
                                                                                toDiscoveryEntries,
                                                                                invocation);
        } catch (Exception e) {
            fail("Unexpected exception from multicast subscribe call: " + e);
        }
    }

    @Test
    public void executeStatelessAsyncCallsRequestReplyManagerCorrectly() throws Exception {
        statelessAsyncParticipantId = "statelessAsyncParticipantId";
        ConnectorInvocationHandler connector = createConnector();
        Method method = TestStatelessAsyncInterface.class.getMethod("testMethod",
                                                                    new Class[]{ MessageIdCallback.class });
        StatelessAsyncCallback statelessAsyncCallback = mock(StatelessAsyncCallback.class);
        when(statelessAsyncCallback.getUseCase()).thenReturn("useCase");
        MessageIdCallback messageIdCallback = mock(MessageIdCallback.class);
        when(statelessAsyncIdCalculator.calculateStatelessCallbackRequestReplyId(eq(method))).thenReturn("requestReplyId");
        when(statelessAsyncIdCalculator.calculateStatelessCallbackMethodId(eq(method))).thenReturn("correlationId");
        connector.executeStatelessAsyncMethod(method, new Object[]{ messageIdCallback });
        ArgumentCaptor<Request> captor = ArgumentCaptor.forClass(Request.class);
        verify(requestReplyManager).sendRequest(eq(statelessAsyncParticipantId),
                                                eq(toDiscoveryEntry),
                                                captor.capture(),
                                                any());
        Request request = captor.getValue();
        assertEquals("correlationId", request.getStatelessAsyncCallbackMethodId());
        assertEquals("requestReplyId", request.getRequestReplyId());
    }

    private ConnectorInvocationHandler createConnector() {
        ArbitrationResult arbitrationResult = new ArbitrationResult();
        arbitrationResult.setDiscoveryEntries(toDiscoveryEntries);
        JoynrMessagingConnectorFactory joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(requestReplyManager,
                                                                                                           replyCallerDirectory,
                                                                                                           subscriptionManager,
                                                                                                           statelessAsyncIdCalculator);
        ConnectorFactory connectorFactory = new ConnectorFactory(joynrMessagingConnectorFactory,
                                                                 messageRouter,
                                                                 libJoynrMessagingAddress);
        ConnectorInvocationHandler connector = connectorFactory.create(fromParticipantId,
                                                                       arbitrationResult,
                                                                       qosSettings,
                                                                       statelessAsyncParticipantId)
                                                               .get();
        return connector;
    }
}
