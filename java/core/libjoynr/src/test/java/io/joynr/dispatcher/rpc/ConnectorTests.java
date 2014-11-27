package io.joynr.dispatcher.rpc;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.RequestReplySender;
import io.joynr.endpoints.EndpointAddressBase;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ConnectorFactory;
import io.joynr.proxy.ConnectorInvocationHandler;
import io.joynr.proxy.Future;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import io.joynr.pubsub.subscription.SubscriptionManager;

import java.lang.reflect.Method;
import java.util.ArrayList;

import joynr.PeriodicSubscriptionQos;
import joynr.SubscriptionRequest;
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
    private RequestReplyDispatcher dispatcher;
    @Mock
    private SubscriptionManager subscriptionManager;
    @Mock
    private RequestReplySender messageSender;

    private String fromParticipantId;
    private String toParticipantId;
    private String channelId;
    private EndpointAddressBase endpointAddress;
    private MessagingQos qosSettings;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        fromParticipantId = "fromParticipantId";
        toParticipantId = "toParticipantId";
        channelId = "testChannelId";
        endpointAddress = new JoynrMessagingEndpointAddress(channelId);
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
            Object[] args = new Object[]{ null, subscriptionQos };
            Method method = LocalisationSubscriptionInterface.class.getDeclaredMethod("subscribeToGPSPosition",
                                                                                      AttributeSubscriptionListener.class,
                                                                                      SubscriptionQos.class,
                                                                                      String.class);
            connector.executeSubscriptionMethod(method, args, future, subscriptionId);
            Mockito.verify(messageSender, times(1)).sendSubscriptionRequest(Mockito.eq(fromParticipantId),
                                                                            Mockito.eq(toParticipantId),
                                                                            Mockito.eq(endpointAddress),
                                                                            Mockito.any(SubscriptionRequest.class),
                                                                            Mockito.any(MessagingQos.class));
        } catch (Exception e) {
            // This is what is supposed to happen -> no error handling
            Assert.fail("Calling a subscription method with no expiry date throws an exception.");
        }

    }

    private ConnectorInvocationHandler createConnector() {
        ArbitrationResult arbitrationResult = new ArbitrationResult();
        ArrayList<EndpointAddressBase> endpointAddresses = new ArrayList<EndpointAddressBase>();
        endpointAddresses.add(endpointAddress);
        arbitrationResult.setEndpointAddress(endpointAddresses);
        arbitrationResult.setParticipantId(toParticipantId);
        ConnectorInvocationHandler connector = ConnectorFactory.create(dispatcher,
                                                                       subscriptionManager,
                                                                       messageSender,
                                                                       fromParticipantId,
                                                                       arbitrationResult,
                                                                       qosSettings);
        return connector;
    }
}
