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
import static org.mockito.Mockito.times;

import java.lang.reflect.Method;
import java.util.Set;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.common.collect.Sets;

import io.joynr.Async;
import io.joynr.Sync;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import joynr.PeriodicSubscriptionQos;
import joynr.system.RoutingTypes.Address;
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
    private MessageRouter messageRouter;
    @Mock
    private Address libJoynrMessagingAddress;

    private String fromParticipantId;
    private String toParticipantId;
    private MessagingQos qosSettings;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        fromParticipantId = "fromParticipantId";
        toParticipantId = "toParticipantId";
        qosSettings = new MessagingQos();

    }

    interface TestProxyInterface extends TestSyncInterface, TestAsyncInterface {

    }

    @Sync
    interface TestSyncInterface {

    }

    @Async
    interface TestAsyncInterface {
        void someMethodwithoutAnnotations(Integer a, String b) throws JsonMappingException;
    }

    @Test
    public void asyncMethodCallWithoutAnnotationThrowsException() throws JoynrIllegalStateException {

        ConnectorInvocationHandler connector = createConnector();
        Assert.assertNotNull(connector);
        try {
            Future<String> future = new Future<String>();
            connector.executeAsyncMethod(TestAsyncInterface.class.getDeclaredMethod("someMethodwithoutAnnotations",
                                                                                    Integer.class,
                                                                                    String.class), new Object[]{ 1,
                    "test" }, future);
            Assert.fail("Calling a method with missing callback annotation did not throw an exception.");
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            Assert.assertEquals(JoynrIllegalStateException.class, e.getClass());
        }

    }

    @SuppressWarnings("unchecked")
    @Test
    public void subscriptionMethodCallWithNoExpiryDate() throws JoynrIllegalStateException {

        ConnectorInvocationHandler connector = createConnector();
        Assert.assertNotNull(connector);
        try {
            Future<String> future = new Future<String>();
            String subscriptionId = "subscriptionId";
            PeriodicSubscriptionQos subscriptionQos = new PeriodicSubscriptionQos();
            subscriptionQos.setPeriodMs(1000).setExpiryDateMs(0).setAlertAfterIntervalMs(1000);
            AttributeSubscriptionListener<GpsPosition> listener = new AttributeSubscriptionAdapter<GpsPosition>();
            Object[] args = new Object[]{ listener, subscriptionQos, subscriptionId };
            Method method = LocalisationSubscriptionInterface.class.getDeclaredMethod("subscribeToGPSPosition",
                                                                                      AttributeSubscriptionListener.class,
                                                                                      SubscriptionQos.class,
                                                                                      String.class);
            AttributeSubscribeInvocation attributeSubscription = new AttributeSubscribeInvocation(method, args, future);
            connector.executeSubscriptionMethod(attributeSubscription);
            Mockito.verify(subscriptionManager, times(1))
                   .registerAttributeSubscription(Mockito.eq(fromParticipantId),
                                                  (Set<String>) argThat(contains(toParticipantId)),
                                                  Mockito.eq(attributeSubscription));
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            Assert.fail("Calling a subscription method with no expiry date throws an exception.");
        }

    }

    @SuppressWarnings("unchecked")
    @Test
    public void unsubscriptionMethodCall() throws JoynrIllegalStateException {

        ConnectorInvocationHandler connector = createConnector();
        Assert.assertNotNull(connector);
        try {
            Future<String> future = new Future<String>();
            String subscriptionId = "subscriptionId";
            Object[] args = new Object[]{ subscriptionId };
            Method method = LocalisationSubscriptionInterface.class.getDeclaredMethod("unsubscribeFromGPSPosition",
                                                                                      String.class);
            UnsubscribeInvocation unsubscribeInvocation = new UnsubscribeInvocation(method, args, future);
            connector.executeSubscriptionMethod(unsubscribeInvocation);
            Mockito.verify(subscriptionManager, times(1))
                   .unregisterSubscription(Mockito.eq(fromParticipantId),
                                           (Set<String>) argThat(contains(toParticipantId)),
                                           Mockito.eq(subscriptionId),
                                           Mockito.any(MessagingQos.class));
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            Assert.fail("Calling a subscription method with no expiry date throws an exception.");
        }
    }

    private ConnectorInvocationHandler createConnector() {
        ArbitrationResult arbitrationResult = new ArbitrationResult();
        arbitrationResult.setParticipantIds(Sets.newHashSet(toParticipantId));
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
}
