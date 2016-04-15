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

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;
import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import io.joynr.provider.ProviderContainer;

import java.io.IOException;
import java.lang.reflect.Method;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadFactory;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Maps;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.ProviderCallback;
import io.joynr.proxy.JoynrMessagingConnectorFactory;
import joynr.JoynrMessage;
import joynr.OneWay;
import joynr.Reply;
import joynr.Request;

/**
 * This test mocks the Http Communication Manager out and tests only the functionality contained in the Dispatcher.
 */
@RunWith(MockitoJUnitRunner.class)
public class RequestReplyManagerTest {
    private static final long TIME_TO_LIVE = 10000L;
    private RequestReplyManager requestReplyManager;
    private ReplyCallerDirectory replyCallerDirectory;
    private ProviderDirectory providerDirectory;
    private String testSenderParticipantId;
    private String testMessageListenerParticipantId;
    private String testMessageResponderParticipantId;
    private String testResponderUnregisteredParticipantId;

    private final String payload1 = "testPayload 1";
    private final String payload2 = "testPayload 2";

    private Request request1;
    private Request request2;
    private OneWay oneWay1;

    private ObjectMapper objectMapper;

    @Mock
    private MessageRouter messageRouterMock;

    @Before
    public void setUp() throws NoSuchMethodException, SecurityException, JsonGenerationException, IOException {

        testMessageListenerParticipantId = "testMessageListenerParticipantId";
        testMessageResponderParticipantId = "testMessageResponderParticipantId";
        testSenderParticipantId = "testSenderParticipantId";
        testResponderUnregisteredParticipantId = "testResponderUnregisteredParticipantId";

        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bind(MessageRouter.class).toInstance(messageRouterMock);
                bind(RequestReplyManager.class).to(RequestReplyManagerImpl.class);
                bind(RequestReplyManager.class).to(RequestReplyManagerImpl.class);
                requestStaticInjection(RpcUtils.class, Request.class, JoynrMessagingConnectorFactory.class);

                ThreadFactory namedThreadFactory = new ThreadFactoryBuilder().setNameFormat("joynr.Cleanup-%d").build();
                ScheduledExecutorService cleanupExecutor = Executors.newSingleThreadScheduledExecutor(namedThreadFactory);
                bind(ScheduledExecutorService.class).annotatedWith(Names.named(JOYNR_SCHEDULER_CLEANUP))
                                                    .toInstance(cleanupExecutor);
            }
        });

        objectMapper = injector.getInstance(ObjectMapper.class);
        objectMapper.registerSubtypes(Request.class, OneWay.class);

        requestReplyManager = injector.getInstance(RequestReplyManager.class);
        providerDirectory = injector.getInstance(ProviderDirectory.class);
        replyCallerDirectory = injector.getInstance(ReplyCallerDirectory.class);
        requestReplyManager = injector.getInstance(RequestReplyManager.class);

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
        request1 = new Request(method.getName(), params1, method.getParameterTypes());
        request2 = new Request(method.getName(), params2, method.getParameterTypes());

        oneWay1 = new OneWay(payload1);
        Map<String, String> headerToResponder = Maps.newHashMap();
        headerToResponder.put(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID, testSenderParticipantId);
        headerToResponder.put(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID, testMessageResponderParticipantId);
        headerToResponder.put(JoynrMessage.HEADER_NAME_CONTENT_TYPE, JoynrMessage.CONTENT_TYPE_TEXT_PLAIN);

        Map<String, String> requestHeader = Maps.newHashMap();
        requestHeader.put(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID, testSenderParticipantId);
        requestHeader.put(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID, testResponderUnregisteredParticipantId);
        requestHeader.put(JoynrMessage.HEADER_NAME_EXPIRY_DATE, String.valueOf(System.currentTimeMillis()
                + TIME_TO_LIVE));
        requestHeader.put(JoynrMessage.HEADER_NAME_CONTENT_TYPE, JoynrMessage.CONTENT_TYPE_APPLICATION_JSON);
    }

    @After
    public void tearDown() {
        requestReplyManager.removeListener(testMessageListenerParticipantId);
        providerDirectory.remove(testMessageResponderParticipantId);
    }

    @Test
    public void oneWayMessagesAreSentToTheCommunicationManager() throws Exception {
        requestReplyManager.sendOneWay(testSenderParticipantId,
                                       testMessageListenerParticipantId,
                                       payload1,
                                       TIME_TO_LIVE);

        ArgumentCaptor<JoynrMessage> messageCapture = ArgumentCaptor.forClass(JoynrMessage.class);
        verify(messageRouterMock, Mockito.times(1)).route(messageCapture.capture());
        assertEquals(messageCapture.getValue().getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                     testSenderParticipantId);
        assertEquals(messageCapture.getValue().getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID),
                     testMessageListenerParticipantId);

        assertEquals(messageCapture.getValue().getPayload(), payload1);
    }

    @Test
    public void requestMessagesSentToTheCommunicationManager() throws Exception {
        requestReplyManager.sendRequest(testSenderParticipantId,
                                        testMessageResponderParticipantId,
                                        request1,
                                        TIME_TO_LIVE);

        ArgumentCaptor<JoynrMessage> messageCapture = ArgumentCaptor.forClass(JoynrMessage.class);
        verify(messageRouterMock, Mockito.times(1)).route(messageCapture.capture());
        assertEquals(messageCapture.getValue().getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                     testSenderParticipantId);
        assertEquals(messageCapture.getValue().getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID),
                     testMessageResponderParticipantId);

        assertEquals(messageCapture.getValue().getPayload(), objectMapper.writeValueAsString(request1));
    }

    private abstract class ReplyCallback extends ProviderCallback<Reply> {
    }

    @Test
    public void requestCallerInvokedForIncomingRequest() throws Exception {
        TestRequestCaller testRequestCallerSpy = Mockito.spy(new TestRequestCaller(1));

        providerDirectory.add(testMessageResponderParticipantId, new ProviderContainer(testRequestCallerSpy));
        ReplyCallback replyCallbackMock = mock(ReplyCallback.class);
        requestReplyManager.handleRequest(replyCallbackMock, testMessageResponderParticipantId, request1, TIME_TO_LIVE);

        String reply = (String) testRequestCallerSpy.getSentPayloadFor(request1);

        ArgumentCaptor<Reply> replyCapture = ArgumentCaptor.forClass(Reply.class);
        verify(testRequestCallerSpy).respond(Mockito.eq(payload1));
        verify(replyCallbackMock).onSuccess(replyCapture.capture());
        assertEquals(reply, replyCapture.getValue().getResponse()[0]);
    }

    @Test
    public void replyCallerInvokedForIncomingReply() throws Exception {
        ReplyCaller replyCaller = mock(ReplyCaller.class);
        replyCallerDirectory.addReplyCaller(request1.getRequestReplyId(),
                                            replyCaller,
                                            ExpiryDate.fromRelativeTtl(TIME_TO_LIVE * 2));

        Reply reply = new Reply(request1.getRequestReplyId(), payload1);
        requestReplyManager.handleReply(reply);

        verify(replyCaller).messageCallBack(reply);
    }

    @Test
    public void queueMessagesForUnregisteredResponder() throws InterruptedException {
        ReplyCallback replyCallbackMock = mock(ReplyCallback.class);

        requestReplyManager.handleRequest(replyCallbackMock,
                                          testResponderUnregisteredParticipantId,
                                          request1,
                                          ExpiryDate.fromRelativeTtl((int) (TIME_TO_LIVE * 0.03)).getValue());
        requestReplyManager.handleRequest(replyCallbackMock,
                                          testResponderUnregisteredParticipantId,
                                          request2,
                                          ExpiryDate.fromRelativeTtl((int) (TIME_TO_LIVE * 5)).getValue());

        Thread.sleep((long) (TIME_TO_LIVE * 0.03 + 20));
        TestRequestCaller testResponderUnregistered = new TestRequestCaller(1);

        testResponderUnregistered.waitForMessage((int) (TIME_TO_LIVE * 0.05));
        providerDirectory.add(testResponderUnregisteredParticipantId, new ProviderContainer(testResponderUnregistered));

        testResponderUnregistered.assertAllPayloadsReceived((int) (TIME_TO_LIVE));
        testResponderUnregistered.assertReceivedPayloadsContainsNot(payload1);
        testResponderUnregistered.assertReceivedPayloadsContains(payload2);
    }

    @Test
    public void requestReplyMessagesRemoveCallBackByTtl() throws Exception {
        TestRequestCaller testResponder = new TestRequestCaller(1);
        ExpiryDate ttlReplyCaller = ExpiryDate.fromRelativeTtl(1000L);

        final ReplyCaller replyCaller = mock(ReplyCaller.class);
        replyCallerDirectory.addReplyCaller(request1.getRequestReplyId(), replyCaller, ttlReplyCaller);

        Thread.sleep(ttlReplyCaller.getRelativeTtl() + 100);
        requestReplyManager.handleReply(new Reply(request1.getRequestReplyId(),
                                                  testResponder.getSentPayloadFor(request1)));

        verify(replyCaller, never()).messageCallBack(any(Reply.class));
    }

    @Test
    public void sendOneWayTtl() throws JoynrMessageNotSentException, JoynrSendBufferFullException,
                               JsonGenerationException, JsonMappingException, IOException {

        TestOneWayRecipient oneWayRecipient = new TestOneWayRecipient(1);
        requestReplyManager.addOneWayRecipient(testMessageListenerParticipantId, oneWayRecipient);

        requestReplyManager.handleOneWayRequest(testMessageListenerParticipantId, oneWay1, TIME_TO_LIVE);

        oneWayRecipient.assertAllPayloadsReceived(TIME_TO_LIVE);
    }

    @Test
    @Ignore
    public void requestReplyRoundtrip() throws JoynrMessageNotSentException, JoynrSendBufferFullException,
                                       JsonGenerationException, JsonMappingException, IOException {
        /*
         * This test is not a unit test, but an integration test. We already have such integration tests, so this test is obsolete
        TestRequestCaller testResponder = new TestRequestCaller(1);
        requestCallerDirectory.addCaller(testMessageResponderParticipantId, testResponder);
        ReplyCaller replyCaller = mock(ReplyCaller.class);
        requestReplyManager.addReplyCaller(request1.getRequestReplyId(), replyCaller, TIME_TO_LIVE * 2);

        requestReplyManager.sendRequest(testSenderParticipantId,
                                        testMessageResponderParticipantId,
                                        request1,
                                        TIME_TO_LIVE);

        testResponder.assertAllPayloadsReceived(20);
        assertEquals(2, messageSenderReceiverMock.getSentMessages().size());
         */
    }
}
