package io.joynr.proxy;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static org.hamcrest.Matchers.contains;
import static org.mockito.Matchers.argThat;

import static org.mockito.Mockito.*;
import static org.junit.Assert.*;

import java.lang.reflect.Method;
import java.util.Collections;
import java.util.Set;

//import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock; //import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.common.collect.Sets;

import io.joynr.Async;
import io.joynr.Sync;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.dispatcher.rpc.annotation.FireAndForget;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcBroadcast;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import joynr.OnChangeSubscriptionQos;
import joynr.OneWayRequest;
import joynr.PeriodicSubscriptionQos;
import joynr.Reply;
import joynr.Request;
import joynr.system.RoutingTypes.Address;
import joynr.types.Localisation.GpsPosition;
import joynr.vehicle.LocalisationSubscriptionInterface;

@RunWith(MockitoJUnitRunner.class)
public class ConnectorTest {

    @Mock
    private ReplyCallerDirectory replyCallerDirectory;
    @Mock
    private SubscriptionManager subscriptionManager;
    @Mock
    private RequestReplyManager requestReplyManager;
    @Mock
    private MessageRouter messageRouter;
    @Mock
    private Address libJoynrMessagingAddress;
    private final String fromParticipantId = "fromParticipantId";;
    private final String toParticipantId = "toParticipantId";
    private final MessagingQos qosSettings = new MessagingQos();;

    @Before
    public void setUp() {
    }

    static interface TestProxyInterface extends TestSyncInterface, TestAsyncInterface {
        static interface TestBroadcastListener extends BroadcastSubscriptionListener {
            void onReceive(String broadcastString);

            void onError();
        }

        @JoynrRpcBroadcast(broadcastName = "testBroadcast")
        String subscribeToTestBroadcast(TestBroadcastListener subscriptionListener,
                                        OnChangeSubscriptionQos subscriptionQos);
    }

    @Sync
    static interface TestSyncInterface {
        void someSyncMethod(Integer a, String b);
    }

    @Async
    static interface TestAsyncInterface {
        void someMethodwithoutAnnotations(Integer a, String b) throws JsonMappingException;

        void someAsyncMethod(@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback,
                             Integer a,
                             String b);
    }

    @FireAndForget
    static interface TestFireAndForgetInterface {
        void someOneWayMethod(Integer a, String b);
    }

    @Test
    public void asyncMethodCallWithoutAnnotationThrowsException() throws JoynrIllegalStateException {
        ConnectorInvocationHandler connector = createConnector(1);
        assertNotNull(connector);
        try {
            Future<String> future = new Future<String>();
            connector.executeAsyncMethod(TestAsyncInterface.class.getDeclaredMethod("someMethodwithoutAnnotations",
                                                                                    Integer.class,
                                                                                    String.class), new Object[]{ 1,
                    "test" }, future);
            fail("Calling a method with missing callback annotation did not throw an exception.");
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            assertEquals(JoynrIllegalStateException.class, e.getClass());
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void subscriptionMethodCallWithNoExpiryDate() throws JoynrIllegalStateException {
        ConnectorInvocationHandler connector = createConnector(1);
        assertNotNull(connector);
        try {
            Future<String> future = new Future<String>();
            String subscriptionId = "subscriptionId";
            @SuppressWarnings("deprecation")
            PeriodicSubscriptionQos subscriptionQos = new PeriodicSubscriptionQos(1000, 0, 1000);
            AttributeSubscriptionListener<GpsPosition> listener = new AttributeSubscriptionListener<GpsPosition>() {
                @Override
                public void onReceive(GpsPosition value) {
                }

                @Override
                public void onError(JoynrRuntimeException error) {
                }
            };
            Object[] args = new Object[]{ listener, subscriptionQos, subscriptionId };
            Method method = LocalisationSubscriptionInterface.class.getDeclaredMethod("subscribeToGPSPosition",
                                                                                      AttributeSubscriptionListener.class,
                                                                                      SubscriptionQos.class,
                                                                                      String.class);
            AttributeSubscribeInvocation attributeSubscription = new AttributeSubscribeInvocation(method, args, future);
            connector.executeSubscriptionMethod(attributeSubscription);
            verify(subscriptionManager).registerAttributeSubscription(eq(fromParticipantId),
                                                                      (Set<String>) argThat(contains(toParticipantId)),
                                                                      eq(attributeSubscription));
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            fail("Calling a subscription method with no expiry date throws an exception.");
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void unsubscriptionMethodCall() throws JoynrIllegalStateException {
        ConnectorInvocationHandler connector = createConnector(1);
        assertNotNull(connector);
        try {
            Future<String> future = new Future<String>();
            String subscriptionId = "subscriptionId";
            Object[] args = new Object[]{ subscriptionId };
            Method method = LocalisationSubscriptionInterface.class.getDeclaredMethod("unsubscribeFromGPSPosition",
                                                                                      String.class);
            UnsubscribeInvocation unsubscribeInvocation = new UnsubscribeInvocation(method, args, future);
            connector.executeSubscriptionMethod(unsubscribeInvocation);
            verify(subscriptionManager).unregisterSubscription(eq(fromParticipantId),
                                                               (Set<String>) argThat(contains(toParticipantId)),
                                                               eq(subscriptionId),
                                                               any(MessagingQos.class));
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            fail("Calling a subscription method with no expiry date throws an exception.");
        }
    }

    @Test
    public void executeSyncMethods() {
        MethodType methodType = MethodType.SYNC;
        String methodName = "someSyncMethod";
        checkSuccessCase(methodType, methodName, 1);
        checkForIllegalStateException(methodType,
                                      methodName,
                                      0,
                                      "You must have exactly one participant to be able to execute a sync method.");
        checkForIllegalStateException(methodType,
                                      methodName,
                                      2,
                                      "You can't execute sync methods for multiple participants.");
        checkForIllegalArgumentException(methodType);
    }

    @Test
    public void executeAsyncMethods() {
        MethodType methodType = MethodType.ASYNC;
        String methodName = "someAsyncMethod";
        checkSuccessCase(methodType, methodName, 1);
        checkForIllegalStateException(methodType,
                                      methodName,
                                      0,
                                      "You must have exactly one participant to be able to execute an async method.");
        checkForIllegalStateException(methodType,
                                      methodName,
                                      2,
                                      "You can't execute async methods for multiple participants.");
        checkForIllegalArgumentException(methodType);
    }

    @Test
    public void executeOneWayMethods() {
        MethodType methodType = MethodType.ONEWAY;
        String methodName = "someOneWayMethod";
        checkSuccessCase(methodType, methodName, 1);
        checkSuccessCase(methodType, methodName, 2);
        checkForIllegalStateException(methodType,
                                      methodName,
                                      0,
                                      "You must have at least one participant to be able to execute an oneWayMethod.");
        checkForIllegalArgumentException(methodType);
    }

    @Test
    public void executeSubscriptionMethods() {
        MethodType methodType = MethodType.SUBSCRIPTION;
        String methodName = "subscribeToTestBroadcast";
        checkSuccessCase(methodType, methodName, 1);
        checkSuccessCase(methodType, methodName, 2);
        checkForIllegalStateException(methodType,
                                      methodName,
                                      0,
                                      "You must have at least one participant to be able to execute a subscription method.");
    }

    private void checkSuccessCase(MethodType methodType, String methodName, int numberOfParticipants) {
        ConnectorInvocationHandler connector = createConnector(numberOfParticipants);
        assertNotNull(connector);
        reset(requestReplyManager);
        try {
            switch (methodType) {
            case SYNC:
                when(requestReplyManager.sendSyncRequest(any(String.class),
                                                         any(String.class),
                                                         any(Request.class),
                                                         any(SynchronizedReplyCaller.class),
                                                         anyLong())).thenReturn(new Reply());
                connector.executeSyncMethod(TestSyncInterface.class.getDeclaredMethod(methodName,
                                                                                      Integer.class,
                                                                                      String.class), new Object[]{ 1,
                        "test" });
                verify(requestReplyManager).sendSyncRequest(eq(fromParticipantId),
                                                            eq(toParticipantId),
                                                            any(Request.class),
                                                            any(SynchronizedReplyCaller.class),
                                                            eq(qosSettings.getRoundTripTtl_ms()));
                break;
            case ASYNC:
                Future<String> future = new Future<String>();
                connector.executeAsyncMethod(TestAsyncInterface.class.getDeclaredMethod(methodName,
                                                                                        Callback.class,
                                                                                        Integer.class,
                                                                                        String.class), new Object[]{
                        mock(Callback.class), 1, "test" }, future);
                verify(requestReplyManager).sendRequest(eq(fromParticipantId),
                                                        eq(toParticipantId),
                                                        any(Request.class),
                                                        eq(qosSettings.getRoundTripTtl_ms()));
                break;
            case ONEWAY:
                connector.executeOneWayMethod(TestFireAndForgetInterface.class.getDeclaredMethod(methodName,
                                                                                                 Integer.class,
                                                                                                 String.class),
                                              new Object[]{ 1, "test" });
                if (numberOfParticipants == 1) {
                    verify(requestReplyManager).sendOneWayRequest(eq(fromParticipantId),
                                                                  eq(Sets.newHashSet(toParticipantId)),
                                                                  any(OneWayRequest.class),
                                                                  eq(qosSettings.getRoundTripTtl_ms()));
                } else {
                    verify(requestReplyManager).sendOneWayRequest(eq(fromParticipantId),
                                                                  eq(getParticipantIds(numberOfParticipants)),
                                                                  any(OneWayRequest.class),
                                                                  eq(qosSettings.getRoundTripTtl_ms()));
                }
                break;
            case SUBSCRIPTION:
                BroadcastSubscribeInvocation broadcastSubscribeInvocation = mock(BroadcastSubscribeInvocation.class);
                connector.executeSubscriptionMethod(broadcastSubscribeInvocation);
                verify(subscriptionManager).registerBroadcastSubscription(eq(fromParticipantId),
                                                                          eq(getParticipantIds(numberOfParticipants)),
                                                                          eq(broadcastSubscribeInvocation));
                UnsubscribeInvocation unsubscribeInvocation = mock(UnsubscribeInvocation.class);
                String testSubscriptionId = "testSubscriptionId";
                when(unsubscribeInvocation.getSubscriptionId()).thenReturn(testSubscriptionId);
                connector.executeSubscriptionMethod(unsubscribeInvocation);
                verify(subscriptionManager).unregisterSubscription(eq(fromParticipantId),
                                                                   eq(getParticipantIds(numberOfParticipants)),
                                                                   eq(testSubscriptionId),
                                                                   eq(qosSettings));
                AttributeSubscribeInvocation attributeSubscribeInvocation = mock(AttributeSubscribeInvocation.class);
                connector.executeSubscriptionMethod(attributeSubscribeInvocation);
                verify(subscriptionManager).registerAttributeSubscription(eq(fromParticipantId),
                                                                          eq(getParticipantIds(numberOfParticipants)),
                                                                          eq(attributeSubscribeInvocation));
                break;
            }
        } catch (Exception e) {
            fail("Calling " + methodType + " method with " + numberOfParticipants
                    + " ParticipantIds must not throw an exception. " + e.toString());
        }
    }

    private void checkForIllegalStateException(MethodType methodType,
                                               String methodName,
                                               int numberOfParticipantIds,
                                               String errorMessage) {
        ConnectorInvocationHandler connector = createConnector(numberOfParticipantIds);
        assertNotNull(connector);
        try {
            switch (methodType) {
            case SYNC:
                connector.executeSyncMethod(TestSyncInterface.class.getDeclaredMethod(methodName,
                                                                                      Integer.class,
                                                                                      String.class), new Object[]{ 1,
                        "test" });
                break;
            case ASYNC:
                Future<String> future = new Future<String>();
                connector.executeAsyncMethod(TestAsyncInterface.class.getDeclaredMethod(methodName,
                                                                                        Callback.class,
                                                                                        Integer.class,
                                                                                        String.class), new Object[]{ 1,
                        "test" }, future);
                break;
            case ONEWAY:
                connector.executeOneWayMethod(TestFireAndForgetInterface.class.getDeclaredMethod(methodName,
                                                                                                 Integer.class,
                                                                                                 String.class),
                                              new Object[]{ 1, "test" });
                break;
            case SUBSCRIPTION:
                connector.executeSubscriptionMethod(mock(BroadcastSubscribeInvocation.class));
                connector.executeSubscriptionMethod(mock(UnsubscribeInvocation.class));
                connector.executeSubscriptionMethod(mock(AttributeSubscribeInvocation.class));
                break;
            }
            fail("Calling " + methodType + " method with " + numberOfParticipantIds
                    + " ParticipantIds did not throw an exception.");
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            assertEquals(JoynrIllegalStateException.class, e.getClass());
        }
    }

    private void checkForIllegalArgumentException(MethodType methodType) {
        ConnectorInvocationHandler connector = createConnector(1);
        assertNotNull(connector);
        try {
            switch (methodType) {
            case SYNC:
                connector.executeSyncMethod(null, new Object[]{ 1, "test" });
                break;
            case ASYNC:
                Future<String> future = new Future<String>();
                connector.executeAsyncMethod(null, new Object[]{ 1, "test" }, future);
                break;
            case ONEWAY:
                connector.executeOneWayMethod(null, new Object[]{ 1, "test" });
                break;
            case SUBSCRIPTION:
                break;
            }
            fail("Calling a " + methodType + " method passing null as method did not throw an exception.");
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            assertEquals(IllegalArgumentException.class, e.getClass());
        }
    }

    private ConnectorInvocationHandler createConnector(int numberOfParticipants) {
        Set<String> participantIds = getParticipantIds(numberOfParticipants);
        ArbitrationResult arbitrationResult = new ArbitrationResult();
        arbitrationResult.setParticipantIds(participantIds);

        JoynrMessagingConnectorFactory joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(requestReplyManager,
                                                                                                           replyCallerDirectory,
                                                                                                           subscriptionManager);
        ConnectorFactory connectorFactory = new ConnectorFactory(joynrMessagingConnectorFactory,
                                                                 messageRouter,
                                                                 libJoynrMessagingAddress);
        ConnectorInvocationHandler connector = connectorFactory.create(fromParticipantId,
                                                                       arbitrationResult,
                                                                       qosSettings);

        return connector;
    }

    private Set<String> getParticipantIds(int numberOfParticipants) {
        if (numberOfParticipants == 1) {
            return Sets.newHashSet(toParticipantId);
        } else if (numberOfParticipants == 0) {
            Set<String> emptySet = Collections.<String> emptySet();
            return emptySet;
        } else {
            Set<String> participantIds = Sets.newHashSet(toParticipantId);
            for (int i = 1; i < numberOfParticipants; i++) {
                participantIds.add("anotherParticipantId" + i);
            }
            return participantIds;
        }
    }

    private enum MethodType {
        SYNC, ASYNC, ONEWAY, SUBSCRIPTION;
    }
};
