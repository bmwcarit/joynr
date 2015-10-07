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

import static org.mockito.Mockito.times;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.dispatcher.rpc.JoynrAsyncInterface;
import io.joynr.dispatcher.rpc.JoynrSyncInterface;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;

import java.lang.reflect.Method;
import java.util.ArrayList;

import joynr.PeriodicSubscriptionQos;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.types.Localisation.GpsPosition;
import joynr.vehicle.LocalisationSubscriptionInterface;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import com.fasterxml.jackson.databind.JsonMappingException;

public class ConnectorTests {

    @Mock
    private ReplyCallerDirectory replyCallerDirectory;
    @Mock
    private SubscriptionManager subscriptionManager;
    @Mock
    private RequestReplyManager requestReplyManager;

    @Mock
    private MessageRouter messageRouter;

    private String fromParticipantId;
    private String toParticipantId;
    private String channelId;
    private MessagingQos qosSettings;
    private ChannelAddress address;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        fromParticipantId = "fromParticipantId";
        toParticipantId = "toParticipantId";
        channelId = "testChannelId";
        address = new ChannelAddress(channelId);
        qosSettings = new MessagingQos();

    }

    interface TestProxyInterface extends TestSyncInterface, TestAsyncInterface {

    }

    interface TestSyncInterface extends JoynrSyncInterface {

    }

    interface TestAsyncInterface extends JoynrAsyncInterface {
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
            Assert.assertEquals(JsonMappingException.class, e.getClass());
        }

    }

    @Test
    public void subscriptionMethodCallWithNoExpiryDate() throws JoynrIllegalStateException {

        ConnectorInvocationHandler connector = createConnector();
        Assert.assertNotNull(connector);
        try {
            Future<String> future = new Future<String>();
            String subscriptionId = "subscriptionId";
            PeriodicSubscriptionQos subscriptionQos = new PeriodicSubscriptionQos(1000, 0, 1000);
            AttributeSubscriptionListener<GpsPosition> listener = new AttributeSubscriptionListener<GpsPosition>() {
                @Override
                public void onReceive(GpsPosition value) {
                }

                @Override
                public void onError() {
                }
            };
            Object[] args = new Object[]{ listener, subscriptionQos, subscriptionId };
            Method method = LocalisationSubscriptionInterface.class.getDeclaredMethod("subscribeToGPSPosition",
                                                                                      AttributeSubscriptionListener.class,
                                                                                      SubscriptionQos.class,
                                                                                      String.class);
            AttributeSubscribeInvocation attributeSubscription = new AttributeSubscribeInvocation(method, args, future);
            connector.executeSubscriptionMethod(attributeSubscription);
            Mockito.verify(subscriptionManager, times(1))
                   .registerAttributeSubscription(Mockito.eq(fromParticipantId),
                                                  Mockito.eq(toParticipantId),
                                                  Mockito.eq(attributeSubscription));
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            Assert.fail("Calling a subscription method with no expiry date throws an exception.");
        }

    }

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
            Mockito.verify(subscriptionManager, times(1)).unregisterSubscription(Mockito.eq(fromParticipantId),
                                                                                 Mockito.eq(toParticipantId),
                                                                                 Mockito.eq(subscriptionId),
                                                                                 Mockito.any(MessagingQos.class));
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            Assert.fail("Calling a subscription method with no expiry date throws an exception.");
        }
    }

    private ConnectorInvocationHandler createConnector() {
        ArbitrationResult arbitrationResult = new ArbitrationResult();
        ArrayList<Address> addresses = new ArrayList<Address>();
        addresses.add(address);
        arbitrationResult.setAddress(addresses);
        arbitrationResult.setParticipantId(toParticipantId);
        JoynrMessagingConnectorFactory joynrMessagingConnectorFactory = new JoynrMessagingConnectorFactory(requestReplyManager,
                                                                                                           replyCallerDirectory,
                                                                                                           subscriptionManager);
        ConnectorFactory connectorFactory = new ConnectorFactory(joynrMessagingConnectorFactory, messageRouter);
        ConnectorInvocationHandler connector = connectorFactory.create(fromParticipantId,
                                                                       arbitrationResult,
                                                                       qosSettings);
        return connector;
    }
}
