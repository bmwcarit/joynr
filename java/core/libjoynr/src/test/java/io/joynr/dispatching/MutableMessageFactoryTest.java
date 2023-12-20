/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.dispatching;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import com.fasterxml.jackson.databind.JsonNode;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;

import io.joynr.common.ExpiryDate;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingQosEffort;
import io.joynr.util.ObjectMapper;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MulticastPublication;
import joynr.MutableMessage;
import joynr.PeriodicSubscriptionQos;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;

@RunWith(MockitoJUnitRunner.class)
public class MutableMessageFactoryTest {
    private static final long TTL = 1000;
    private static final long MAX_ALLOWED_EXPIRY_DATE_DIFF_MS = 500;
    private static final long NO_TTL_UPLIFT = 0;
    private MutableMessageFactory mutableMessageFactory;
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

        Injector injector = Guice.createInjector(new JoynrPropertiesModule(new Properties()),
                                                 new JsonMessageSerializerModule(),
                                                 new AbstractModule() {

                                                     @Override
                                                     protected void configure() {
                                                         bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_TTL_UPLIFT_MS))
                                                                         .toInstance(NO_TTL_UPLIFT);
                                                         requestStaticInjection(Request.class);
                                                         Multibinder<JoynrMessageProcessor> joynrMessageProcessorMultibinder = Multibinder.newSetBinder(binder(),
                                                                                                                                                        new TypeLiteral<JoynrMessageProcessor>() {
                                                                                                                                                        });
                                                         joynrMessageProcessorMultibinder.addBinding()
                                                                                         .toInstance(new JoynrMessageProcessor() {
                                                                                             @Override
                                                                                             public MutableMessage processOutgoing(MutableMessage joynrMessage) {
                                                                                                 joynrMessage.getCustomHeaders()
                                                                                                             .put("test",
                                                                                                                  "test");
                                                                                                 return joynrMessage;
                                                                                             }

                                                                                             @Override
                                                                                             public ImmutableMessage processIncoming(ImmutableMessage joynrMessage) {
                                                                                                 return joynrMessage;
                                                                                             }
                                                                                         });
                                                     }
                                                 });

        objectMapper = injector.getInstance(ObjectMapper.class);

        payload = "payload";
        Method method = TestProvider.class.getMethod("methodWithStrings", new Class[]{ String.class });
        request = new Request(method.getName(), new String[]{ payload }, method.getParameterTypes());
        String requestReplyId = request.getRequestReplyId();
        reply = new Reply(requestReplyId, objectMapper.<JsonNode> valueToTree(payload));
        messagingQos = new MessagingQos(TTL);
        expiryDate = ExpiryDate.fromRelativeTtl(messagingQos.getRoundTripTtl_ms());

        String subscriptionId = "subscription";
        String attributeName = "attribute";
        PeriodicSubscriptionQos subscriptionqos = new PeriodicSubscriptionQos();
        subscriptionqos.setPeriodMs(1000).setValidityMs(10).setAlertAfterIntervalMs(1500).setPublicationTtlMs(1000);
        subscriptionRequest = new SubscriptionRequest(subscriptionId, attributeName, subscriptionqos);
        String response = "response";
        publication = new SubscriptionPublication(Arrays.asList(response), subscriptionId);

        mutableMessageFactory = injector.getInstance(MutableMessageFactory.class);
    }

    public static void assertExpiryDateEquals(long expectedValue, MutableMessage message) {
        long diff = Math.abs(expectedValue - message.getTtlMs());

        assertTrue(message.isTtlAbsolute());
        assertTrue("ExpiryDate=" + message.getTtlMs() + " differs " + diff + "ms (more than "
                + MAX_ALLOWED_EXPIRY_DATE_DIFF_MS + "ms) from expected value=" + expectedValue,
                   diff <= MAX_ALLOWED_EXPIRY_DATE_DIFF_MS);
    }

    @Test
    public void createRequest() {
        MutableMessage message = mutableMessageFactory.createRequest(fromParticipantId,
                                                                     toParticipantId,
                                                                     request,
                                                                     messagingQos);
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST, message.getType());

        assertEquals(fromParticipantId, message.getSender());
        assertEquals(toParticipantId, message.getRecipient());

        assertExpiryDateEquals(expiryDate.getValue(), message);
        assertTrue(message.getPayload() != null);
    }

    @Test
    public void createRequestWithCustomEffort() {
        MessagingQos customMessagingQos = new MessagingQos();
        customMessagingQos.setEffort(MessagingQosEffort.BEST_EFFORT);
        MutableMessage message = mutableMessageFactory.createRequest(fromParticipantId,
                                                                     toParticipantId,
                                                                     request,
                                                                     customMessagingQos);
        expiryDate = ExpiryDate.fromRelativeTtl(customMessagingQos.getRoundTripTtl_ms());
        assertExpiryDateEquals(expiryDate.getValue(), message);
        assertEquals(String.valueOf(MessagingQosEffort.BEST_EFFORT), message.getEffort());
    }

    @Test
    public void createRequestWithCustomHeaders() throws Exception {
        final Map<String, String> myCustomHeaders = new HashMap<>();
        final String headerName = "header";
        final String headerValue = "value";
        myCustomHeaders.put(headerName, headerValue);
        messagingQos.getCustomMessageHeaders().putAll(myCustomHeaders);
        MutableMessage message = mutableMessageFactory.createRequest(fromParticipantId,
                                                                     toParticipantId,
                                                                     request,
                                                                     messagingQos);
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST, message.getType());
        assertExpiryDateEquals(expiryDate.getValue(), message);

        final String expectedCustomHeaderName = Message.CUSTOM_HEADER_PREFIX + headerName;
        assertTrue(message.getImmutableMessage().getHeaders().containsKey(expectedCustomHeaderName));

        Map<String, String> customHeaders = message.getCustomHeaders();
        assertTrue(customHeaders.size() == 3);
        assertTrue(customHeaders.containsKey(headerName));
    }

    @Test
    public void testCreateOneWayRequest() {
        MutableMessage message = mutableMessageFactory.createOneWayRequest(fromParticipantId,
                                                                           toParticipantId,
                                                                           request,
                                                                           messagingQos);
        assertNotNull(message);
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY, message.getType());
        assertEquals(fromParticipantId, message.getSender());
        assertEquals(toParticipantId, message.getRecipient());
        assertExpiryDateEquals(expiryDate.getValue(), message);
        assertNotNull(message.getPayload());
    }

    @Test
    public void testCreateOneWayRequestWithCustomHeader() throws Exception {
        final Map<String, String> myCustomHeaders = new HashMap<>();
        final String headerName = "header";
        final String headerValue = "value";
        myCustomHeaders.put(headerName, headerValue);
        messagingQos.getCustomMessageHeaders().putAll(myCustomHeaders);
        MutableMessage message = mutableMessageFactory.createOneWayRequest(fromParticipantId,
                                                                           toParticipantId,
                                                                           request,
                                                                           messagingQos);
        assertNotNull(message);
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_ONE_WAY, message.getType());
        assertExpiryDateEquals(expiryDate.getValue(), message);

        final String expectedCustomHeaderName = Message.CUSTOM_HEADER_PREFIX + headerName;
        assertTrue(message.getImmutableMessage().getHeaders().containsKey(expectedCustomHeaderName));

        Map<String, String> customHeaders = message.getCustomHeaders();
        assertTrue(customHeaders.size() == 2);
        assertTrue(customHeaders.containsKey(headerName));
    }

    @Test
    public void createReply() {
        MutableMessage message = mutableMessageFactory.createReply(fromParticipantId,
                                                                   toParticipantId,
                                                                   reply,
                                                                   messagingQos);

        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_REPLY, message.getType());
        assertEquals(fromParticipantId, message.getSender());
        assertEquals(toParticipantId, message.getRecipient());
        assertExpiryDateEquals(expiryDate.getValue(), message);

        assertTrue(message.getPayload() != null);
    }

    @Test
    public void createSubscriptionRequest() {
        MutableMessage message = mutableMessageFactory.createSubscriptionRequest(fromParticipantId,
                                                                                 toParticipantId,
                                                                                 subscriptionRequest,
                                                                                 messagingQos);
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_SUBSCRIPTION_REQUEST, message.getType());
        assertEquals(fromParticipantId, message.getSender());
        assertEquals(toParticipantId, message.getRecipient());
        assertExpiryDateEquals(expiryDate.getValue(), message);

        assertTrue(message.getPayload() != null);
    }

    @Test
    public void createPublication() {
        MutableMessage message = mutableMessageFactory.createPublication(fromParticipantId,
                                                                         toParticipantId,
                                                                         publication,
                                                                         messagingQos);
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_PUBLICATION, message.getType());
        assertEquals(fromParticipantId, message.getSender());
        assertEquals(toParticipantId, message.getRecipient());
        assertExpiryDateEquals(expiryDate.getValue(), message);

        assertTrue(message.getPayload() != null);
    }

    @Test
    public void testMessageProcessorUsed() {
        MutableMessage message = mutableMessageFactory.createRequest("from",
                                                                     "to",
                                                                     new Request("name", new Object[0], new Class[0]),
                                                                     new MessagingQos());
        assertNotNull(message.getCustomHeaders().get("test"));
        assertEquals("test", message.getCustomHeaders().get("test"));
    }

    @Test
    public void testCreateMulticastMessage() {
        String multicastId = "multicastId";
        MulticastPublication multicastPublication = new MulticastPublication(Collections.emptyList(), multicastId);

        MutableMessage joynrMessage = mutableMessageFactory.createMulticast(fromParticipantId,
                                                                            multicastPublication,
                                                                            messagingQos);

        assertNotNull(joynrMessage);
        assertExpiryDateEquals(expiryDate.getValue(), joynrMessage);
        assertEquals(fromParticipantId, joynrMessage.getSender());
        assertEquals(multicastId, joynrMessage.getRecipient());
        assertEquals(Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST, joynrMessage.getType());
        assertTrue(new String(joynrMessage.getPayload(),
                              StandardCharsets.UTF_8).contains(MulticastPublication.class.getName()));
    }

    @Test
    public void testCompressedFlagIsSet() {
        MutableMessage message;
        MessagingQos messagingQos = new MessagingQos();

        messagingQos.setCompress(true);
        message = mutableMessageFactory.createRequest("from", "to", request, messagingQos);
        assertEquals(true, message.getCompressed());

        messagingQos.setCompress(false);
        message = mutableMessageFactory.createRequest("from", "to", request, messagingQos);
        assertEquals(false, message.getCompressed());
    }
}
