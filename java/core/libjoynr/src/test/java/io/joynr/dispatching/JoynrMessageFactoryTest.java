package io.joynr.dispatching;

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

import static org.junit.Assert.assertNotNull;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.JoynrMessageFactory;
import io.joynr.messaging.MessagingModule;
import io.joynr.pubsub.SubscriptionQos;

import java.lang.reflect.Method;
import java.util.Arrays;

import joynr.JoynrMessage;
import joynr.PeriodicSubscriptionQos;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

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
    private SubscriptionRequest subscriptionRequest;

    private SubscriptionPublication publication;
    private ObjectMapper objectMapper;

    @Before
    public void setUp() throws NoSuchMethodException, SecurityException {

        fromParticipantId = "sender";
        toParticipantId = "receiver";
        Injector injector = Guice.createInjector(new MessagingModule(), new AbstractModule() {

            @Override
            protected void configure() {
                requestStaticInjection(Request.class);

            }

        });
        objectMapper = injector.getInstance(ObjectMapper.class);

        payload = "payload";
        Method method = TestRequestCaller.class.getMethod("respond", new Class[]{ String.class });
        request = new Request(method.getName(), new String[]{ payload }, method.getParameterTypes());
        String requestReplyId = request.getRequestReplyId();
        reply = new Reply(requestReplyId, objectMapper.<JsonNode> valueToTree(payload));
        expiryDate = ExpiryDate.fromRelativeTtl(1000);

        String subscriptionId = "subscription";
        String attributeName = "attribute";
        SubscriptionQos subscriptionqos = new PeriodicSubscriptionQos(1000, 1, 1500, 1000);
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
                                                                 expiryDate);
        Assert.assertEquals(JoynrMessage.MESSAGE_TYPE_REQUEST, message.getType());

        Assert.assertEquals(fromParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID));
        Assert.assertEquals(toParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID));

        Assert.assertEquals(String.valueOf(expiryDate.getValue()),
                            message.getHeaderValue(JoynrMessage.HEADER_NAME_EXPIRY_DATE));
        Assert.assertTrue(message.getPayload() != null);
        assertNotNull(message.getCreatorUserId());
    }

    @Test
    public void createReply() {
        JoynrMessage message = joynrMessageFactory.createReply(fromParticipantId,
                                                               toParticipantId,
                                                               reply,
                                                               ExpiryDate.fromRelativeTtl(TTL));

        Assert.assertEquals(JoynrMessage.MESSAGE_TYPE_REPLY, message.getType());
        Assert.assertEquals(fromParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID));
        Assert.assertEquals(toParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID));
        Assert.assertEquals(JoynrMessage.CONTENT_TYPE_APPLICATION_JSON,
                            message.getHeaderValue(JoynrMessage.HEADER_NAME_CONTENT_TYPE));

        Assert.assertTrue(message.getPayload() != null);
        assertNotNull(message.getCreatorUserId());
    }

    @Test
    public void createSubscriptionRequest() {
        JoynrMessage message = joynrMessageFactory.createSubscriptionRequest(fromParticipantId,
                                                                             toParticipantId,
                                                                             subscriptionRequest,
                                                                             ExpiryDate.fromRelativeTtl(TTL),
                                                                             false);
        Assert.assertEquals(JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST, message.getType());
        Assert.assertEquals(fromParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID));
        Assert.assertEquals(toParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID));

        Assert.assertTrue(message.getPayload() != null);
        assertNotNull(message.getCreatorUserId());
    }

    @Test
    public void createPublication() {
        JoynrMessage message = joynrMessageFactory.createPublication(fromParticipantId,
                                                                     toParticipantId,
                                                                     publication,
                                                                     ExpiryDate.fromRelativeTtl(TTL));
        Assert.assertEquals(JoynrMessage.MESSAGE_TYPE_PUBLICATION, message.getType());
        Assert.assertEquals(fromParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID));
        Assert.assertEquals(toParticipantId, message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID));

        Assert.assertTrue(message.getPayload() != null);
        assertNotNull(message.getCreatorUserId());
    }
}
