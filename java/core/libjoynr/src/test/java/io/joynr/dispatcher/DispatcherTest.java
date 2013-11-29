package io.joynr.dispatcher;

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

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import io.joynr.common.ExpiryDate;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.util.PreconfiguredEndpointDirectoryModule;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Map;
import java.util.Properties;
import java.util.UUID;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import joynr.JoynrMessage;
import joynr.OneWay;
import joynr.Reply;
import joynr.Request;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Maps;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.util.Modules;

/**
 * This test mocks the Http Communication Manager out and tests only the functionality contained in the Dispatcher.
 */
public class DispatcherTest {
    @SuppressWarnings("unused")
    private static final Logger logger = LoggerFactory.getLogger(DispatcherTest.class);
    private static final int TIME_OUT_MS = 10 * 1000;
    private static final long TIME_TO_LIVE = 10000L;
    MessageSenderReceiverMock messageSenderReceiverMock;
    RequestReplyDispatcher dispatcher;
    private String channelId;
    private String testSenderParticipantId;
    private String testMessageListenerParticipantId;
    private String testMessageResponderParticipantId;
    private String testListenerUnregisteredParticipantId;
    private String testResponderUnregisteredParticipantId;

    private JoynrMessage messageToResponder;
    private JoynrMessage messageToUnregisteredListener1;
    private JoynrMessage messageToUnregisteredListener2;
    private JoynrMessage messageToUnregisteredResponder1;
    private JoynrMessage messageToUnregisteredResponder2;

    private TestRequestCaller testResponderUnregistered;
    private final String payload1 = "testPayload 1";
    private final String payload2 = "testPayload 2";
    private final String payload3 = "testPayload 3";

    private Request jsonRequest1;
    private Request jsonRequest2;

    private MessagingEndpointDirectory messagingEndpointDirectory = new MessagingEndpointDirectory("channelurldirectory_participantid",
                                                                                                   "discoverydirectory_channelid",
                                                                                                   "capabilitiesdirectory_participantid",
                                                                                                   "discoverydirectory_channelid");
    private RequestReplySender requestReplySender;
    private JoynrMessagingEndpointAddress dummyEndpointAddress;
    private ObjectMapper objectMapper;

    @Before
    public void setUp() throws NoSuchMethodException, SecurityException, JsonGenerationException, IOException {

        testMessageListenerParticipantId = "testMessageListenerParticipantId";
        testMessageResponderParticipantId = "testMessageResponderParticipantId";
        testSenderParticipantId = "testSenderParticipantId";
        testListenerUnregisteredParticipantId = "testListenerUnregisteredParticipantId";
        testResponderUnregisteredParticipantId = "testResponderUnregisteredParticipantId";
        dummyEndpointAddress = new JoynrMessagingEndpointAddress("dummyChannelId");
        messagingEndpointDirectory.put(testMessageListenerParticipantId, dummyEndpointAddress);
        messagingEndpointDirectory.put(testMessageResponderParticipantId, dummyEndpointAddress);
        messagingEndpointDirectory.put(testListenerUnregisteredParticipantId, dummyEndpointAddress);
        messagingEndpointDirectory.put(testResponderUnregisteredParticipantId, dummyEndpointAddress);
        messagingEndpointDirectory.put(testSenderParticipantId, dummyEndpointAddress);

        channelId = "disTest-" + UUID.randomUUID().toString();
        Properties properties = new Properties();
        properties.put(MessagingPropertyKeys.CHANNELID, channelId);
        Injector injector = Guice.createInjector(Modules.override(new JoynrPropertiesModule(properties),
                                                                  new MessagingModule(),
                                                                  new DispatcherTestModule())
                                                        .with(new PreconfiguredEndpointDirectoryModule(messagingEndpointDirectory)));

        objectMapper = injector.getInstance(ObjectMapper.class);
        objectMapper.registerSubtypes(Request.class, OneWay.class);

        dispatcher = injector.getInstance(RequestReplyDispatcher.class);
        requestReplySender = injector.getInstance(RequestReplySender.class);
        messageSenderReceiverMock = injector.getInstance(MessageSenderReceiverMock.class);
        messageSenderReceiverMock.registerMessageListener(dispatcher);
        messageSenderReceiverMock.startReceiver();

        testResponderUnregistered = new TestRequestCaller(1);

        // dispatcher.addListener(testMessageListenerParticipantId, testListener);

        // jsonRequestString1 = "{\"_typeName\":\"Request\", \"methodName\":\"respond\",\"params\":{\"payload\": \""
        // + payload1 + "\"}}";
        // jsonRequestString2 = "{\"_typeName\":\"Request\", \"methodName\":\"respond\",\"params\":{\"payload\": \""
        // + payload2 + "\"}}";

        Object[] params1 = new Object[]{ payload1 };
        Object[] params2 = new Object[]{ payload2 };

        // MethodMetaInformation methodMetaInformation = new
        // MethodMetaInformation(TestRequestCaller.class.getMethod("respond", new Class[]{ Object.class }));
        Method method = TestRequestCaller.class.getMethod("respond", new Class[]{ String.class });
        jsonRequest1 = new Request(method.getName(), params1, method.getParameterTypes());
        jsonRequest2 = new Request(method.getName(), params2, method.getParameterTypes());

        Map<String, String> headerToResponder = Maps.newHashMap();
        headerToResponder.put(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID, testSenderParticipantId);
        headerToResponder.put(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID, testMessageResponderParticipantId);
        headerToResponder.put(JoynrMessage.HEADER_NAME_CONTENT_TYPE, JoynrMessage.CONTENT_TYPE_TEXT_PLAIN);
        messageToResponder = new JoynrMessage();
        messageToResponder.setHeader(headerToResponder);
        messageToResponder.setType(JoynrMessage.MESSAGE_TYPE_REQUEST);
        messageToResponder.setPayload(payload1);

        Map<String, String> headerToUnregisteredListener = Maps.newHashMap();
        headerToUnregisteredListener.put(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID, testSenderParticipantId);
        headerToUnregisteredListener.put(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID,
                                         testListenerUnregisteredParticipantId);

        messageToUnregisteredListener1 = new JoynrMessage();
        messageToUnregisteredListener1.setHeader(headerToUnregisteredListener);
        messageToUnregisteredListener1.setType(JoynrMessage.MESSAGE_TYPE_ONE_WAY);

        messageToUnregisteredListener1.setPayload(payload2);

        messageToUnregisteredListener2 = new JoynrMessage();
        messageToUnregisteredListener2.setHeader(headerToUnregisteredListener);
        messageToUnregisteredListener2.setType(JoynrMessage.MESSAGE_TYPE_ONE_WAY);

        messageToUnregisteredListener2.setPayload(payload3);

        Map<String, String> requestHeader = Maps.newHashMap();
        requestHeader.put(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID, testSenderParticipantId);
        requestHeader.put(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID, testResponderUnregisteredParticipantId);
        requestHeader.put(JoynrMessage.HEADER_NAME_EXPIRY_DATE, String.valueOf(System.currentTimeMillis()
                + TIME_TO_LIVE));
        requestHeader.put(JoynrMessage.HEADER_NAME_CONTENT_TYPE, JoynrMessage.CONTENT_TYPE_APPLICATION_JSON);
        requestHeader.put(JoynrMessage.HEADER_NAME_REPLY_CHANNELID, channelId);

        messageToUnregisteredResponder1 = new JoynrMessage();
        messageToUnregisteredResponder1.setHeader(requestHeader);
        messageToUnregisteredResponder1.setType(JoynrMessage.MESSAGE_TYPE_REQUEST);
        messageToUnregisteredResponder1.setPayload(objectMapper.writeValueAsString(jsonRequest1));

        messageToUnregisteredResponder2 = new JoynrMessage();
        messageToUnregisteredResponder2.setHeader(requestHeader);
        messageToUnregisteredResponder2.setType(JoynrMessage.MESSAGE_TYPE_REQUEST);
        messageToUnregisteredResponder2.setPayload(objectMapper.writeValueAsString(jsonRequest2));

    }

    @After
    public void tearDown() {
        dispatcher.removeListener(testMessageListenerParticipantId);
        dispatcher.removeRequestCaller(testMessageResponderParticipantId);
    }

    @Test
    public void oneWayMessagesAreSentToTheCommunicationManager() throws Exception {
        requestReplySender.sendOneWay(testSenderParticipantId, testMessageListenerParticipantId, payload1, TIME_TO_LIVE);

        assertEquals(1, messageSenderReceiverMock.getSentMessages().size());

        JoynrMessage messageSent = messageSenderReceiverMock.getSentMessages().get(0);
        assertEquals(messageSent.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID), testSenderParticipantId);
        assertEquals(messageSent.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID),
                     testMessageListenerParticipantId);

        assertEquals(messageSent.getPayload(), payload1);
    }

    @Test
    public void requestReplyMessagesActivateACallBack() throws Exception {
        TestRequestCaller testRequestCaller = new TestRequestCaller(1);
        dispatcher.addRequestCaller(testMessageResponderParticipantId, testRequestCaller);

        ReplyCaller replyCaller = mock(ReplyCaller.class);
        dispatcher.addReplyCaller(jsonRequest1.getRequestReplyId(), replyCaller, TIME_TO_LIVE * 2);

        requestReplySender.sendRequest(testSenderParticipantId,
                                       testMessageResponderParticipantId,
                                       dummyEndpointAddress,
                                       jsonRequest1,
                                       TIME_TO_LIVE);
        // sendRequest is async -> we have to wait a bit

        testRequestCaller.assertAllPayloadsReceived(TIME_OUT_MS);
        String reply = (String) testRequestCaller.getSentPayloadFor(jsonRequest1);
        ArgumentCaptor<Reply> jsonReply = ArgumentCaptor.forClass(Reply.class);
        verify(replyCaller, timeout(TIME_OUT_MS).times(1)).messageCallBack(jsonReply.capture());
        assertEquals(reply, jsonReply.getValue().getResponse());
        assertEquals(2, messageSenderReceiverMock.getSentMessages().size());
    }

    @Test
    public void addReplyCallerDoesNotBlock() throws InterruptedException, ExecutionException, TimeoutException {

        final String requestReplyId = UUID.randomUUID().toString();
        final ReplyCaller replyCaller = mock(ReplyCaller.class);

        final Callable<Boolean> stuffToDo = new Callable<Boolean>() {

            @Override
            public Boolean call() throws Exception {

                try {
                    messageSenderReceiverMock.setBlockOnInitialisation(true);
                    dispatcher.addReplyCaller(requestReplyId, replyCaller, TIME_TO_LIVE * 2);
                } finally {
                    messageSenderReceiverMock.setBlockOnInitialisation(false);
                }
                return true;
            }
        };

        final ExecutorService executor = Executors.newSingleThreadExecutor();
        final Future<Boolean> future = executor.submit(stuffToDo);

        // should not throw a timeout exception
        future.get(1000, TimeUnit.MILLISECONDS);

    }

    // @Test
    // public void queueMessagesForUnregisteredListener() throws InterruptedException {
    // messageSenderReceiverMock.receiveMessage(messageToUnregisteredListener1,
    // convertToAbsoluteTtl((int) (TIME_TO_LIVE * 0.1)));
    // messageSenderReceiverMock.receiveMessage(messageToUnregisteredListener2, convertToAbsoluteTtl(TIME_TO_LIVE));
    // testListenerUnregistered.waitForMessage((int) (TIME_TO_LIVE * 0.5));
    // dispatcher.addListener(testListenerUnregisteredParticipantId, testListenerUnregistered);
    // testListenerUnregistered.assertAllPayloadsReceived((int) (TIME_TO_LIVE));
    // testListenerUnregistered.assertReceivedPayloadsContains(payload3);
    // testListenerUnregistered.assertReceivedPayloadsContainsNot(payload2);
    // }

    @Test
    public void queueMessagesForUnregisteredResponder() throws InterruptedException {
        messageToUnregisteredResponder1.setExpirationDate(ExpiryDate.fromRelativeTtl((int) (TIME_TO_LIVE * 0.1)));
        messageSenderReceiverMock.receiveMessage(messageToUnregisteredResponder1);

        messageToUnregisteredResponder2.setExpirationDate(ExpiryDate.fromRelativeTtl((int) (TIME_TO_LIVE * 2)));
        messageSenderReceiverMock.receiveMessage(messageToUnregisteredResponder2);

        Thread.sleep((long) (TIME_TO_LIVE * 0.1 + 20));

        testResponderUnregistered.waitForMessage((int) (TIME_TO_LIVE * 0.5));
        dispatcher.addRequestCaller(testResponderUnregisteredParticipantId, testResponderUnregistered);

        testResponderUnregistered.assertAllPayloadsReceived((int) (TIME_TO_LIVE));
        testResponderUnregistered.assertReceivedPayloadsContainsNot(payload1);
        testResponderUnregistered.assertReceivedPayloadsContains(payload2);
    }

    @Test
    public void requestReplyMessagesRemoveCallBackByTtl() throws Exception {
        TestRequestCaller testResponder = new TestRequestCaller(1);
        long ttlReplyCaller = 1000L;

        final ReplyCaller replyCaller = mock(ReplyCaller.class);
        dispatcher.addReplyCaller(jsonRequest1.getRequestReplyId(), replyCaller, ttlReplyCaller);
        requestReplySender.sendRequest(testSenderParticipantId,
                                       testMessageResponderParticipantId,
                                       dummyEndpointAddress,
                                       jsonRequest1,
                                       ttlReplyCaller);

        Thread.sleep(ttlReplyCaller);
        dispatcher.addRequestCaller(testMessageResponderParticipantId, testResponder);

        assertEquals(1, messageSenderReceiverMock.getSentMessages().size());
        JoynrMessage sentMessage = messageSenderReceiverMock.getSentMessages().get(0);

        JoynrMessage reply = new JoynrMessage();
        reply.setType(JoynrMessage.MESSAGE_TYPE_REPLY);

        Map<String, String> header = Maps.newHashMap();
        // header.put(JoynrMessage.HEADER_NAME_REQUEST_REPLY_ID, sentMessage.getHeader()
        // .get(JoynrMessage.HEADER_NAME_REQUEST_REPLY_ID));
        reply.setHeader(header);
        reply.setPayload("The reply to " + sentMessage.getPayload());
        dispatcher.messageArrived(reply);

        verify(replyCaller, never()).messageCallBack(any(Reply.class));
    }

    @Test
    public void sendOneWayTtl() throws JoynrMessageNotSentException, JoynrSendBufferFullException,
                               JsonGenerationException, JsonMappingException, IOException {

        TestOneWayRecipient oneWayRecipient = new TestOneWayRecipient(1);
        dispatcher.addOneWayRecipient(testMessageListenerParticipantId, oneWayRecipient);

        OneWay payload = new OneWay(payload1);
        requestReplySender.sendOneWay(testSenderParticipantId, testMessageListenerParticipantId, payload, TIME_TO_LIVE);

        oneWayRecipient.assertAllPayloadsReceived(TIME_OUT_MS);
    }

    @Test
    public void requestReplyTtl() throws JoynrMessageNotSentException, JoynrSendBufferFullException,
                                 JsonGenerationException, JsonMappingException, IOException {
        TestRequestCaller testResponder = new TestRequestCaller(1);
        dispatcher.addRequestCaller(testMessageResponderParticipantId, testResponder);
        ReplyCaller replyCaller = mock(ReplyCaller.class);
        dispatcher.addReplyCaller(jsonRequest1.getRequestReplyId(), replyCaller, TIME_TO_LIVE * 2);

        requestReplySender.sendRequest(testSenderParticipantId,
                                       testMessageResponderParticipantId,
                                       dummyEndpointAddress,
                                       jsonRequest1,
                                       TIME_TO_LIVE);

        assertEquals(1, messageSenderReceiverMock.getSentMessages().size());
        testResponder.assertAllPayloadsReceived(TIME_OUT_MS);

    }
}
