package io.joynr.dispatching;

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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;
import io.joynr.common.ExpiryDate;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingQosEffort;
import io.joynr.pubsub.SubscriptionQos;
import joynr.JoynrMessage;
import joynr.MulticastPublication;
import joynr.MulticastSubscriptionRequest;
import joynr.PeriodicSubscriptionQos;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class JoynrMessageFactoryTest {
    private static final long TTL = 1000;
    JoynrMessageFactory joynrMessageFactory;
    private String fromParticipantId;
    private String toParticipantId;
    private Request request;
    private Reply reply;
    private String payload;
    private ExpiryDate expiryDate;
    private MessagingQos messagingQos;
    private SubscriptionRequest subscriptionRequest;

    private SubscriptionPublication publication;
    private ObjectMapper objectMapper;

    @Before
    public void setUp() throws NoSuchMethodException, SecurityException {

        fromParticipantId = "sender";
        toParticipantId = "receiver";
        Injector injector = Guice.createInjector(new JsonMessageSerializerModule(), new AbstractModule() {

            @Override
            protected void configure() {
                requestStaticInjection(Request.class);
                Multibinder<JoynrMessageProcessor> joynrMessageProcessorMultibinder = Multibinder.newSetBinder(binder(),
                                                                                                               new TypeLiteral<JoynrMessageProcessor>() {
                                                                                                               });
                joynrMessageProcessorMultibinder.addBinding().toInstance(new JoynrMessageProcessor() {
                    @Override
                    public JoynrMessage process(JoynrMessage joynrMessage) {
                        joynrMessage.getHeader().put("test", "test");
                        return joynrMessage;
                    }
                });
            }

        });
        objectMapper = injector.getInstance(ObjectMapper.class);

        payload = "payload";
        Method method = TestRequestCaller.class.getMethod("respond", new Class[]{ String.class });
        request = new Request(method.getName(), new String[]{ payload }, method.getParameterTypes());
        String requestReplyId = request.getRequestReplyId();
        reply = new Reply(requestReplyId, objectMapper.<JsonNode> valueToTree(payload));
        messagingQos = new MessagingQos(TTL);
        expiryDate = DispatcherUtils.convertTtlToExpirationDate(messagingQos.getRoundTripTtl_ms());

        String subscriptionId = "subscription";
        String attributeName = "attribute";
        PeriodicSubscriptionQos subscriptionqos = new PeriodicSubscriptionQos();
        subscriptionqos.setPeriodMs(1000).setValidityMs(10).setAlertAfterIntervalMs(1500).setPublicationTtlMs(1000);
        subscriptionRequest = new SubscriptionRequest(subscriptionId, attributeName, subscriptionqos);
        String response = "response";
        publication = new SubscriptionPublication(Arrays.asList(response), subscriptionId);

        joynrMessageFactory = injector.getInstance(JoynrMessageFactory.class);
    }

    @Test
    public void createRequest() {
        JoynrMessage message = joynrMessageFactory.createRequest(fromParticipantId,
                                                                 toParticipantId,
                                                                 request,
                                                                 messagingQos);
        assertEquals(JoynrMessage.MESSAGE_TYPE_REQUEST, message.getType());

        assertEquals(fromParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID));
        assertEquals(toParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID));

        String expiryDateString = String.valueOf(expiryDate.getValue());
        String headerExpiry = message.getHeaderValue(JoynrMessage.HEADER_NAME_EXPIRY_DATE);
        assertEquals(expiryDateString.substring(0, expiryDateString.length() - 4),
                     headerExpiry.substring(0, headerExpiry.length() - 4));
        assertTrue(message.getPayload() != null);
        assertNotNull(message.getCreatorUserId());
    }

    @Test
    public void createRequestWithCustomEffort() {
        MessagingQos customMessagingQos = new MessagingQos();
        customMessagingQos.setEffort(MessagingQosEffort.BEST_EFFORT);
        JoynrMessage message = joynrMessageFactory.createRequest(fromParticipantId,
                                                                 toParticipantId,
                                                                 request,
                                                                 customMessagingQos);
        assertEquals(String.valueOf(MessagingQosEffort.BEST_EFFORT),
                     message.getHeaderValue(JoynrMessage.HEADER_NAME_EFFORT));
    }

    @Test
    public void createRequestWithCustomHeaders() {
        final Map<String, String> myCustomHeaders = new HashMap<>();
        final String headerName = "header";
        final String headerValue = "value";
        myCustomHeaders.put(headerName, headerValue);
        messagingQos.getCustomMessageHeaders().putAll(myCustomHeaders);
        JoynrMessage message = joynrMessageFactory.createRequest(fromParticipantId,
                                                                 toParticipantId,
                                                                 request,
                                                                 messagingQos);
        assertEquals(JoynrMessage.MESSAGE_TYPE_REQUEST, message.getType());

        final String expectedCustomHeaderName = JoynrMessage.MESSAGE_CUSTOM_HEADER_PREFIX + headerName;
        assertTrue(message.getHeader().containsKey(expectedCustomHeaderName));
        Map<String, String> customHeaders = message.getCustomHeaders();
        assertTrue(customHeaders.size() == 1);
        assertTrue(customHeaders.containsKey(headerName));
    }

    @Test
    public void testCreateOneWayRequest() {
        JoynrMessage joynrMessage = joynrMessageFactory.createOneWayRequest(fromParticipantId,
                                                                            toParticipantId,
                                                                            request,
                                                                            messagingQos);
        assertNotNull(joynrMessage);
        assertEquals(JoynrMessage.MESSAGE_TYPE_ONE_WAY, joynrMessage.getType());
        assertEquals(fromParticipantId, joynrMessage.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID));
        assertEquals(toParticipantId, joynrMessage.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID));
        String expiryDateString = String.valueOf(expiryDate.getValue());
        String headerExpiryString = joynrMessage.getHeaderValue(JoynrMessage.HEADER_NAME_EXPIRY_DATE);
        assertEquals(expiryDateString.substring(0, expiryDateString.length() - 4),
                     headerExpiryString.substring(0, headerExpiryString.length() - 4));
        assertNotNull(joynrMessage.getPayload());
        assertNotNull(joynrMessage.getCreatorUserId());
    }

    @Test
    public void testCreateOneWayRequestWithCustomHeader() {
        final Map<String, String> myCustomHeaders = new HashMap<>();
        final String headerName = "header";
        final String headerValue = "value";
        myCustomHeaders.put(headerName, headerValue);
        messagingQos.getCustomMessageHeaders().putAll(myCustomHeaders);
        JoynrMessage message = joynrMessageFactory.createOneWayRequest(fromParticipantId,
                                                                            toParticipantId,
                                                                            request,
                                                                            messagingQos);
        assertNotNull(message);
        assertEquals(JoynrMessage.MESSAGE_TYPE_ONE_WAY, message.getType());
        final String expectedCustomHeaderName = JoynrMessage.MESSAGE_CUSTOM_HEADER_PREFIX + headerName;
        assertTrue(message.getHeader().containsKey(expectedCustomHeaderName));
        Map<String, String> customHeaders = message.getCustomHeaders();
        assertTrue(customHeaders.size() == 1);
        assertTrue(customHeaders.containsKey(headerName));

    }

    @Test
    public void createReply() {
        JoynrMessage message = joynrMessageFactory.createReply(fromParticipantId, toParticipantId, reply, messagingQos);

        assertEquals(JoynrMessage.MESSAGE_TYPE_REPLY, message.getType());
        assertEquals(fromParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID));
        assertEquals(toParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID));
        assertEquals(JoynrMessage.CONTENT_TYPE_APPLICATION_JSON,
                     message.getHeaderValue(JoynrMessage.HEADER_NAME_CONTENT_TYPE));

        assertTrue(message.getPayload() != null);
        assertNotNull(message.getCreatorUserId());
    }

    @Test
    public void createSubscriptionRequest() {
        JoynrMessage message = joynrMessageFactory.createSubscriptionRequest(fromParticipantId,
                                                                             toParticipantId,
                                                                             subscriptionRequest,
                                                                             messagingQos);
        assertEquals(JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST, message.getType());
        assertEquals(fromParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID));
        assertEquals(toParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID));

        assertTrue(message.getPayload() != null);
        assertNotNull(message.getCreatorUserId());
    }

    @Test
    public void createPublication() {
        JoynrMessage message = joynrMessageFactory.createPublication(fromParticipantId,
                                                                     toParticipantId,
                                                                     publication,
                                                                     messagingQos);
        assertEquals(JoynrMessage.MESSAGE_TYPE_PUBLICATION, message.getType());
        assertEquals(fromParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID));
        assertEquals(toParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID));

        assertTrue(message.getPayload() != null);
        assertNotNull(message.getCreatorUserId());
    }

    @Test
    public void testMessageProcessorUsed() {
        JoynrMessage joynrMessage = joynrMessageFactory.createRequest("from",
                                                                      "to",
                                                                      new Request("name", new Object[0], new Class[0]),
                                                                      new MessagingQos());
        assertNotNull(joynrMessage.getHeader().get("test"));
        assertEquals("test", joynrMessage.getHeader().get("test"));
    }

    @Test
    public void testCreateMulticastMessage() {
        String multicastId = "multicastId";
        MulticastPublication multicastPublication = new MulticastPublication(Collections.emptyList(), multicastId);

        JoynrMessage joynrMessage = joynrMessageFactory.createMulticast(fromParticipantId,
                                                                        multicastPublication,
                                                                        messagingQos);

        assertNotNull(joynrMessage);
        assertEquals(fromParticipantId, joynrMessage.getFrom());
        assertEquals(multicastId, joynrMessage.getTo());
        assertEquals(JoynrMessage.MESSAGE_TYPE_MULTICAST, joynrMessage.getType());
        assertTrue(joynrMessage.getPayload().contains(MulticastPublication.class.getName()));
    }

    @Test
    public void testCreateMulticastSubscriptionRequest() {
        String multicastId = "multicastId";
        String subscriptionId = "subscriptionId";
        String multicastName = "multicastName";
        SubscriptionQos subscriptionQos = mock(SubscriptionQos.class);

        MulticastSubscriptionRequest multicastSubscriptionRequest = new MulticastSubscriptionRequest(multicastId,
                                                                                                     subscriptionId,
                                                                                                     multicastName,
                                                                                                     subscriptionQos);

        JoynrMessage result = joynrMessageFactory.createSubscriptionRequest(fromParticipantId,
                                                                            toParticipantId,
                                                                            multicastSubscriptionRequest,
                                                                            messagingQos);

        assertNotNull(result);
        assertEquals(JoynrMessage.MESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST, result.getType());
    }
}
