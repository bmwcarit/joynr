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

import io.joynr.arbitration.ArbitrationResult;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.proxy.TestBroadcastInterface.TestBroadcastListener;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import joynr.BroadcastFilterParameters;
import joynr.MulticastSubscriptionQos;
import joynr.OnChangeSubscriptionQos;
import joynr.OneWayRequest;
import joynr.PeriodicSubscriptionQos;
import joynr.Reply;
import joynr.Request;
import joynr.exceptions.ApplicationException;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.Localisation.GpsPosition;
import joynr.vehicle.LocalisationSubscriptionInterface;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;

import java.util.HashSet;
import java.util.Set;

import static java.util.Collections.singletonList;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isA;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.openMocks;

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
    private DiscoveryEntryWithMetaInfo toDiscoveryEntry;
    private Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries;
    private MessagingQos qosSettings;
    private Object proxy;
    private ConnectorInvocationHandler connector;

    @Before
    public void setUp() {
        openMocks(this);
        fromParticipantId = "fromParticipantId";
        statelessAsyncParticipantId = "statelessAsyncParticipantId";
        final var toParticipantId = "toParticipantId";
        toDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        toDiscoveryEntry.setParticipantId(toParticipantId);
        toDiscoveryEntries = new HashSet<>(singletonList(toDiscoveryEntry));
        qosSettings = new MessagingQos();
        proxy = new Object();
        connector = createConnector();
    }

    @Test
    public void asyncMethodCallWithoutAnnotationThrowsException() throws JoynrIllegalStateException {
        try {
            final var future = new Future<String>();
            connector.executeAsyncMethod(proxy,
                                         TestAsyncInterface.class.getDeclaredMethod("someMethodWithoutAnnotations",
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
        try {
            final var attributeSubscription = createSubscribeToGpsInvocation();
            connector.executeSubscriptionMethod(attributeSubscription);
            verify(subscriptionManager).registerAttributeSubscription(eq(fromParticipantId),
                                                                      eq(toDiscoveryEntries),
                                                                      eq(attributeSubscription));
        } catch (final Exception e) {
            // This is what is supposed to happen -> no error handling
            fail("Calling a subscription method with no expiry date throws an exception.");
        }
    }

    @Test
    public void unsubscriptionMethodCall() throws JoynrIllegalStateException {
        try {
            final var future = new Future<String>();
            final var subscriptionId = "subscriptionId";
            final var args = new Object[]{ subscriptionId };
            final var method = LocalisationSubscriptionInterface.class.getDeclaredMethod("unsubscribeFromGPSPosition",
                                                                                         String.class);
            final var unsubscribeInvocation = new UnsubscribeInvocation(method, args, future);
            connector.executeSubscriptionMethod(unsubscribeInvocation);
            verify(subscriptionManager).unregisterSubscription(eq(fromParticipantId),
                                                               eq(toDiscoveryEntries),
                                                               eq(subscriptionId),
                                                               any(MessagingQos.class));
        } catch (final Exception e) {
            // This is what is supposed to happen -> no error handling
            fail("Calling a subscription method with no expiry date throws an exception.");
        }
    }

    @Test
    public void asyncMethodCallCallsRequestReplyManagerWithCorrectArguments() {
        final var requestCaptor = ArgumentCaptor.forClass(Request.class);
        final var future = new Future<Void>();
        try {
            final var method = TestAsyncInterface.class.getDeclaredMethod("methodWithoutParameters", Callback.class);
            connector.executeAsyncMethod(proxy, method, new Object[]{ voidCallback }, future);
            verify(requestReplyManager).sendRequest(eq(fromParticipantId),
                                                    eq(toDiscoveryEntry),
                                                    requestCaptor.capture(),
                                                    eq(qosSettings),
                                                    any(ExpiryDate.class));

            final var actualRequest = requestCaptor.getValue();
            final var expectedRequest = new Request(method.getName(),
                                                    new Object[]{},
                                                    new String[]{},
                                                    actualRequest.getRequestReplyId());
            assertEquals(expectedRequest, actualRequest);
        } catch (final Exception e) {
            fail("Unexpected exception from async method call: " + e);
        }
    }

    @Test
    public void syncMethodCallWithModelledError_throwsApplicationException() throws NoSuchMethodException {
        final var method = TestSyncInterface.class.getDeclaredMethod("methodWithoutParametersWithModelledErrors");
        final var errorValue = ApplicationErrors.ERROR_VALUE_1;
        final var expected = new ApplicationException(errorValue);

        when(requestReplyManager.sendSyncRequest(any(String.class),
                                                 any(DiscoveryEntryWithMetaInfo.class),
                                                 any(Request.class),
                                                 any(SynchronizedReplyCaller.class),
                                                 any(MessagingQos.class),
                                                 any(ExpiryDate.class))).thenReturn(new Reply("rrid", expected));
        try {
            connector.executeSyncMethod(method, new Object[]{});
            fail("Unexpected success from sync method call.");
        } catch (final ApplicationException ex) {
            assertEquals(ex, expected);
        }
    }

    @Test
    public void syncMethodCallWithoutModelledError_doesNotThrowApplicationException() throws NoSuchMethodException,
                                                                                      ApplicationException {
        final var method = TestSyncInterface.class.getDeclaredMethod("methodWithoutParameters");
        final var errorValue = ApplicationErrors.ERROR_VALUE_1;
        final var expected = new ApplicationException(errorValue);
        when(requestReplyManager.sendSyncRequest(any(String.class),
                                                 any(DiscoveryEntryWithMetaInfo.class),
                                                 any(Request.class),
                                                 any(SynchronizedReplyCaller.class),
                                                 any(MessagingQos.class),
                                                 any(ExpiryDate.class))).thenReturn(new Reply("rrid", expected));
        try {
            connector.executeSyncMethod(method, new Object[]{});
            fail("Unexpected success from sync method call.");
        } catch (final JoynrRuntimeException ex) {
            String message = "An ApplicationException was received, but none was expected. Is the provider version incompatible with the consumer? "
                    + expected;
            assertEquals(message, ex.getMessage());
        }
    }

    @Test
    public void syncMethodCallCallsRequestReplyManagerWithCorrectArguments() {
        final var requestCaptor = ArgumentCaptor.forClass(Request.class);
        try {
            final var method = TestSyncInterface.class.getDeclaredMethod("methodWithoutParameters");
            when(requestReplyManager.sendSyncRequest(any(String.class),
                                                     any(DiscoveryEntryWithMetaInfo.class),
                                                     any(Request.class),
                                                     any(SynchronizedReplyCaller.class),
                                                     any(MessagingQos.class),
                                                     any(ExpiryDate.class))).thenReturn(new Reply());
            connector.executeSyncMethod(method, new Object[]{});
            verify(requestReplyManager).sendSyncRequest(eq(fromParticipantId),
                                                        eq(toDiscoveryEntry),
                                                        requestCaptor.capture(),
                                                        isA(SynchronizedReplyCaller.class),
                                                        eq(qosSettings),
                                                        any(ExpiryDate.class));

            final var actualRequest = requestCaptor.getValue();
            final var expectedRequest = new Request(method.getName(),
                                                    new Object[]{},
                                                    new String[]{},
                                                    actualRequest.getRequestReplyId());
            assertEquals(expectedRequest, actualRequest);
        } catch (final Exception e) {
            fail("Unexpected exception from sync method call: " + e);
        }
    }

    @Test
    public void oneWayMethodCallCallsRequestReplyManagerWithCorrectArguments() {
        final var requestCaptor = ArgumentCaptor.forClass(OneWayRequest.class);
        try {
            final var method = TestFireAndForgetInterface.class.getDeclaredMethod("methodWithoutParameters");
            connector.executeOneWayMethod(method, new Object[]{});
            verify(requestReplyManager).sendOneWayRequest(eq(fromParticipantId),
                                                          eq(toDiscoveryEntries),
                                                          requestCaptor.capture(),
                                                          eq(qosSettings));

            final var actualRequest = requestCaptor.getValue();
            final var expectedRequest = new OneWayRequest(method.getName(), new Object[]{}, new String[]{});
            assertEquals(expectedRequest, actualRequest);
        } catch (final Exception e) {
            fail("Unexpected exception from one way method call: " + e);
        }
    }

    @Test
    public void unsubscriptionMethodCallCallsSubscriptionManagerWithCorrectArguments() {
        final var subscriptionId = "subscriptionId";
        try {
            final var method = TestSubscriptionInterface.class.getDeclaredMethod("unsubscribeFromTestAttribute",
                                                                                 String.class);
            final var invocation = new UnsubscribeInvocation(method, new Object[]{ subscriptionId }, null);
            connector.executeSubscriptionMethod(invocation);
            verify(subscriptionManager).unregisterSubscription(fromParticipantId,
                                                               toDiscoveryEntries,
                                                               subscriptionId,
                                                               qosSettings);
        } catch (final Exception e) {
            fail("Unexpected exception from unsubscribe call: " + e);
        }
    }

    @Test
    public void subscribeToAttributeCallCallsSubscriptionManagerWithCorrectArguments() {
        try {
            final var invocation = createSubscribeToAttributeInvocation();
            connector.executeSubscriptionMethod(invocation);
            verify(subscriptionManager).registerAttributeSubscription(fromParticipantId,
                                                                      toDiscoveryEntries,
                                                                      invocation);
        } catch (final Exception e) {
            fail("Unexpected exception from attribute subscribe call: " + e);
        }
    }

    @Test
    public void subscribeToBroadcastCallCallsSubscriptionManagerWithCorrectArguments() {
        try {
            final var invocation = createBroadcastInvocation();
            connector.executeSubscriptionMethod(invocation);
            verify(subscriptionManager).registerBroadcastSubscription(fromParticipantId,
                                                                      toDiscoveryEntries,
                                                                      invocation);
        } catch (final Exception e) {
            fail("Unexpected exception from broadcast subscribe call: " + e);
        }
    }

    @Test
    public void subscribeToMulticastCallCallsSubscriptionManagerWithCorrectArguments() {
        try {
            final var invocation = createMulticastInvocation();
            connector.executeSubscriptionMethod(invocation);
            verify(subscriptionManager).registerMulticastSubscription(fromParticipantId,
                                                                      toDiscoveryEntries,
                                                                      invocation);
        } catch (final Exception e) {
            fail("Unexpected exception from multicast subscribe call: " + e);
        }
    }

    @Test
    public void executeStatelessAsyncCallsRequestReplyManagerCorrectly() throws Exception {
        final var method = TestStatelessAsyncInterface.class.getMethod("testMethod", MessageIdCallback.class);
        final var statelessAsyncCallback = mock(StatelessAsyncCallback.class);
        when(statelessAsyncCallback.getUseCase()).thenReturn("useCase");
        final var messageIdCallback = mock(MessageIdCallback.class);
        when(statelessAsyncIdCalculator.calculateStatelessCallbackRequestReplyId(eq(method))).thenReturn("requestReplyId");
        when(statelessAsyncIdCalculator.calculateStatelessCallbackMethodId(eq(method))).thenReturn("correlationId");
        connector.executeStatelessAsyncMethod(method, new Object[]{ messageIdCallback });
        final var captor = ArgumentCaptor.forClass(Request.class);
        verify(requestReplyManager).sendRequest(eq(statelessAsyncParticipantId),
                                                eq(toDiscoveryEntry),
                                                captor.capture(),
                                                any(),
                                                eq(true));
        final var request = captor.getValue();
        assertEquals("correlationId", request.getStatelessAsyncCallbackMethodId());
        assertEquals("requestReplyId", request.getRequestReplyId());
    }

    private ConnectorInvocationHandler createConnector() {
        final var arbitrationResult = new ArbitrationResult();
        arbitrationResult.setDiscoveryEntries(toDiscoveryEntries);
        final var optional = getConnectorFactory().create(fromParticipantId,
                                                          arbitrationResult,
                                                          qosSettings,
                                                          statelessAsyncParticipantId);
        assertTrue(optional.isPresent());
        return optional.get();
    }

    private ConnectorFactory getConnectorFactory() {
        final var joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(requestReplyManager,
                                                                                      replyCallerDirectory,
                                                                                      subscriptionManager,
                                                                                      statelessAsyncIdCalculator);
        return new ConnectorFactory(joynrMessagingConnectorFactory, messageRouter, libJoynrMessagingAddress);
    }

    private MulticastSubscribeInvocation createMulticastInvocation() throws NoSuchMethodException {
        final var partitions = new String[]{ "partition1", "partition2", "partition3" };
        final var listener = new TestBroadcastInterface.TestBroadcastAdapter();
        final var subscriptionQos = new MulticastSubscriptionQos();
        final var method = TestBroadcastInterface.class.getDeclaredMethod("subscribeToTestMulticast",
                                                                          TestBroadcastListener.class,
                                                                          MulticastSubscriptionQos.class,
                                                                          String[].class);
        return new MulticastSubscribeInvocation(method,
                                                new Object[]{ listener, subscriptionQos, partitions },
                                                null,
                                                proxy);
    }

    private AttributeSubscribeInvocation createSubscribeToGpsInvocation() throws NoSuchMethodException {
        final var future = new Future<String>();
        final var subscriptionQos = new PeriodicSubscriptionQos();
        subscriptionQos.setPeriodMs(1000).setExpiryDateMs(0).setAlertAfterIntervalMs(1000);
        final var listener = new AttributeSubscriptionAdapter<GpsPosition>();
        final var args = new Object[]{ listener, subscriptionQos, "subscriptionId" };
        final var method = LocalisationSubscriptionInterface.class.getDeclaredMethod("subscribeToGPSPosition",
                                                                                     String.class,
                                                                                     AttributeSubscriptionListener.class,
                                                                                     SubscriptionQos.class);
        return new AttributeSubscribeInvocation(method, args, future, proxy);
    }

    private AttributeSubscribeInvocation createSubscribeToAttributeInvocation() throws NoSuchMethodException {
        final var listener = new AttributeSubscriptionAdapter<String>();
        final var subscriptionQos = new OnChangeSubscriptionQos();
        final var method = TestSubscriptionInterface.class.getDeclaredMethod("subscribeToTestAttribute",
                                                                             AttributeSubscriptionListener.class,
                                                                             SubscriptionQos.class);
        return new AttributeSubscribeInvocation(method, new Object[]{ listener, subscriptionQos }, null, proxy);
    }

    private BroadcastSubscribeInvocation createBroadcastInvocation() throws NoSuchMethodException {
        final var listener = new TestBroadcastInterface.TestBroadcastAdapter();
        final var subscriptionQos = new OnChangeSubscriptionQos();
        final var method = TestBroadcastInterface.class.getDeclaredMethod("subscribeToTestBroadcast",
                                                                          TestBroadcastListener.class,
                                                                          OnChangeSubscriptionQos.class,
                                                                          BroadcastFilterParameters.class);
        return new BroadcastSubscribeInvocation(method,
                                                new Object[]{ listener, subscriptionQos,
                                                        new BroadcastFilterParameters() },
                                                null,
                                                proxy);
    }
}
