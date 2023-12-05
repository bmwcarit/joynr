/*
 * #%L
 * %%
 * Copyright (C) 2020-2023 BMW Car IT GmbH
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

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.io.IOException;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.Semaphore;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatchers;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;

import io.joynr.common.ExpiryDate;
import io.joynr.context.JoynrMessageScopeModule;
import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.RequestInterpreter;
import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.sender.MessageSender;
import io.joynr.provider.AbstractSubscriptionPublisher;
import io.joynr.provider.ProviderCallback;
import io.joynr.provider.ProviderContainer;
import io.joynr.proxy.DefaultStatelessAsyncIdCalculatorImpl;
import io.joynr.proxy.JoynrMessagingConnectorFactory;
import io.joynr.proxy.StatelessAsyncIdCalculator;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.JoynrThreadFactory;
import io.joynr.util.ObjectMapper;
import joynr.MutableMessage;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.exceptions.MethodInvocationException;
import joynr.types.DiscoveryEntryWithMetaInfo;

/**
 * This test mocks the Http Communication Manager out and tests only the functionality contained in the Dispatcher.
 */
@RunWith(MockitoJUnitRunner.class)
public class RequestReplyManagerTest {
    private static final long TIME_TO_LIVE = 10000L;
    private final static long REPLY_DIRECTORY_CLEANUP_TASK_INTERVAL_MS = 5000L;
    private RequestReplyManager requestReplyManager;
    private ReplyCallerDirectory replyCallerDirectory;
    private ProviderDirectory providerDirectory;
    private String testSenderParticipantId;
    private String testOneWayRecipientParticipantId;
    private DiscoveryEntryWithMetaInfo testOneWayRecipientDiscoveryEntry;
    private Set<DiscoveryEntryWithMetaInfo> testOneWayRecipientDiscoveryEntries;
    private String testMessageResponderParticipantId;
    private DiscoveryEntryWithMetaInfo testMessageResponderDiscoveryEntry;
    private String testResponderUnregisteredParticipantId;

    private final String payload1 = "testPayload 1";
    private final String payload2 = "testPayload 2";

    private Request request1;
    private Request request2;
    private Request request3;
    private Request request4;
    private OneWayRequest oneWay1;

    private ObjectMapper objectMapper;

    @Mock
    private MessageRouter messageRouterMock;

    @Mock
    private MessageSender messageSenderMock;

    @Mock
    private AbstractSubscriptionPublisher subscriptionPublisherMock;

    @Mock
    private ProviderContainer providerContainer;
    private RequestCallerFactory requestCallerFactory;

    @Mock
    private RequestInterpreter mockRequestInterpreter;
    @Mock
    private ShutdownNotifier mockShutdownNotifier;

    @Before
    public void setUp() throws NoSuchMethodException, SecurityException, JsonGenerationException, IOException {
        requestCallerFactory = new RequestCallerFactory();
        testOneWayRecipientParticipantId = "testOneWayRecipientParticipantId";
        testOneWayRecipientDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        testOneWayRecipientDiscoveryEntry.setParticipantId(testOneWayRecipientParticipantId);
        testOneWayRecipientDiscoveryEntries = new HashSet<>(Arrays.asList(testOneWayRecipientDiscoveryEntry));
        testMessageResponderParticipantId = "testMessageResponderParticipantId";
        testMessageResponderDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        testMessageResponderDiscoveryEntry.setParticipantId(testMessageResponderParticipantId);
        testSenderParticipantId = "testSenderParticipantId";
        testResponderUnregisteredParticipantId = "testResponderUnregisteredParticipantId";

        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                install(new JoynrMessageScopeModule());
                bind(MessageSender.class).toInstance(messageSenderMock);
                bind(MessageRouter.class).toInstance(messageRouterMock);
                bind(RequestReplyManager.class).to(RequestReplyManagerImpl.class);
                requestStaticInjection(RpcUtils.class, Request.class, JoynrMessagingConnectorFactory.class);

                ThreadFactory namedThreadFactory = new JoynrThreadFactory("joynr.Cleanup");
                ScheduledExecutorService cleanupExecutor = Executors.newSingleThreadScheduledExecutor(namedThreadFactory);
                bind(ScheduledExecutorService.class).annotatedWith(Names.named(JOYNR_SCHEDULER_CLEANUP))
                                                    .toInstance(cleanupExecutor);
                bind(StatelessAsyncIdCalculator.class).to(DefaultStatelessAsyncIdCalculatorImpl.class);
                bind(StatelessAsyncRequestReplyIdManager.class).to(DefaultStatelessAsyncRequestReplyIdManagerImpl.class);
                bind(String.class).annotatedWith(Names.named(MessagingPropertyKeys.CHANNELID)).toInstance("channelId");
                Multibinder.newSetBinder(binder(), new TypeLiteral<JoynrMessageProcessor>() {
                });
            }
        });

        objectMapper = injector.getInstance(ObjectMapper.class);
        objectMapper.registerSubtypes(Request.class, OneWayRequest.class);

        requestReplyManager = injector.getInstance(RequestReplyManager.class);
        providerDirectory = injector.getInstance(ProviderDirectory.class);
        replyCallerDirectory = injector.getInstance(ReplyCallerDirectory.class);
        requestReplyManager = injector.getInstance(RequestReplyManager.class);

        // dispatcher.addListener(testOneWayRecipientParticipantId, testListener);

        // jsonRequestString1 = "{\"_typeName\":\"Request\", \"methodName\":\"respond\",\"params\":{\"payload\": \""
        // + payload1 + "\"}}";
        // jsonRequestString2 = "{\"_typeName\":\"Request\", \"methodName\":\"respond\",\"params\":{\"payload\": \""
        // + payload2 + "\"}}";

        Object[] params1 = new Object[]{ payload1 };
        Object[] params2 = new Object[]{ payload2 };

        // MethodMetaInformation methodMetaInformation = new
        // MethodMetaInformation(TestRequestCaller.class.getMethod("respond", new Class[]{ Object.class }));
        Method method = TestProvider.class.getMethod("methodWithStrings", new Class[]{ String.class });
        request1 = new Request(method.getName(), params1, method.getParameterTypes());
        request2 = new Request(method.getName(), params2, method.getParameterTypes());
        request3 = new Request("unknownMethodName", params2, method.getParameterTypes());
        request4 = new Request(method.getName(),
                               params1,
                               method.getParameterTypes(),
                               "requestReplyId",
                               "statelessAsyncCallbackMethodId");

        Method fireAndForgetMethod = TestOneWayRecipient.class.getMethod("fireAndForgetMethod",
                                                                         new Class[]{ String.class });
        oneWay1 = new OneWayRequest(fireAndForgetMethod.getName(),
                                    new Object[]{ payload1 },
                                    fireAndForgetMethod.getParameterTypes());
    }

    @After
    public void tearDown() {
        providerDirectory.remove(testMessageResponderParticipantId);
        providerDirectory.remove(testOneWayRecipientParticipantId);
    }

    @Test
    public void oneWayMessagesAreSentToTheCommunicationManager() throws Exception {
        requestReplyManager.sendOneWayRequest(testSenderParticipantId,
                                              testOneWayRecipientDiscoveryEntries,
                                              oneWay1,
                                              new MessagingQos(TIME_TO_LIVE));

        ArgumentCaptor<MutableMessage> messageCapture = ArgumentCaptor.forClass(MutableMessage.class);
        verify(messageSenderMock, times(1)).sendMessage(messageCapture.capture());
        assertEquals(messageCapture.getValue().getSender(), testSenderParticipantId);
        assertEquals(messageCapture.getValue().getRecipient(), testOneWayRecipientParticipantId);

        assertEquals(oneWay1, objectMapper.readValue(messageCapture.getValue().getPayload(), OneWayRequest.class));
    }

    @Test
    public void requestMessagesSentToTheCommunicationManager() throws Exception {
        requestReplyManager.sendRequest(testSenderParticipantId,
                                        testMessageResponderDiscoveryEntry,
                                        request1,
                                        new MessagingQos(TIME_TO_LIVE));

        ArgumentCaptor<MutableMessage> messageCapture = ArgumentCaptor.forClass(MutableMessage.class);
        verify(messageSenderMock, times(1)).sendMessage(messageCapture.capture());
        assertEquals(messageCapture.getValue().getSender(), testSenderParticipantId);
        assertEquals(messageCapture.getValue().getRecipient(), testMessageResponderParticipantId);

        assertEquals(new String(messageCapture.getValue().getPayload(), StandardCharsets.UTF_8),
                     objectMapper.writeValueAsString(request1));
        assertFalse(messageCapture.getValue().isStatelessAsync());
    }

    @Test
    public void statelessAsyncFlagSetCorrectly() throws Exception {
        requestReplyManager.sendRequest(testSenderParticipantId,
                                        testMessageResponderDiscoveryEntry,
                                        request4,
                                        new MessagingQos(TIME_TO_LIVE),
                                        true);

        ArgumentCaptor<MutableMessage> messageCaptor = ArgumentCaptor.forClass(MutableMessage.class);
        verify(messageSenderMock).sendMessage(messageCaptor.capture());
        MutableMessage message = messageCaptor.getValue();
        assertNotNull(message);
        assertTrue(message.isStatelessAsync());
    }

    private abstract class ReplyCallback extends ProviderCallback<Reply> {
    }

    @Test
    public void requestCallerInvokedForIncomingRequest() throws Exception {
        TestProvider testRequestCallerSpy = spy(new TestProvider(1));

        when(providerContainer.getRequestCaller()).thenReturn(requestCallerFactory.create(testRequestCallerSpy));
        providerDirectory.add(testMessageResponderParticipantId, providerContainer);
        ReplyCallback replyCallbackMock = mock(ReplyCallback.class);
        requestReplyManager.handleRequest(replyCallbackMock, testMessageResponderParticipantId, request1, TIME_TO_LIVE);

        String reply = (String) testRequestCallerSpy.getSentPayloadFor(request1);

        ArgumentCaptor<Reply> replyCapture = ArgumentCaptor.forClass(Reply.class);
        verify(testRequestCallerSpy).methodWithStrings(eq(payload1));
        verify(replyCallbackMock).onSuccess(replyCapture.capture());
        assertEquals(reply, replyCapture.getValue().getResponse()[0]);
    }

    @Test
    public void requestCallerRejectsForIncomingRequest() throws Exception {
        TestProvider testRequestCallerSpy = spy(new TestProvider(1));

        when(providerContainer.getRequestCaller()).thenReturn(requestCallerFactory.create(testRequestCallerSpy));
        providerDirectory.add(testMessageResponderParticipantId, providerContainer);
        ReplyCallback replyCallbackMock = mock(ReplyCallback.class);
        requestReplyManager.handleRequest(replyCallbackMock, testMessageResponderParticipantId, request3, TIME_TO_LIVE);

        ArgumentCaptor<JoynrException> exceptionCapture = ArgumentCaptor.forClass(JoynrException.class);
        verify(replyCallbackMock).onFailure(exceptionCapture.capture());
        assertTrue(exceptionCapture.getValue() instanceof MethodInvocationException);
        assertEquals(((MethodInvocationException) (exceptionCapture.getValue())).getProviderVersion()
                                                                                .getMajorVersion()
                                                                                .intValue(),
                     47);
        assertEquals(((MethodInvocationException) (exceptionCapture.getValue())).getProviderVersion()
                                                                                .getMinorVersion()
                                                                                .intValue(),
                     11);
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
        TestProvider testResponderUnregistered = new TestProvider(1);

        testResponderUnregistered.waitForMessage((int) (TIME_TO_LIVE * 0.05));
        when(providerContainer.getRequestCaller()).thenReturn(requestCallerFactory.create(testResponderUnregistered));
        providerDirectory.add(testResponderUnregisteredParticipantId, providerContainer);

        testResponderUnregistered.assertAllPayloadsReceived((int) (TIME_TO_LIVE));
        testResponderUnregistered.assertReceivedPayloadsContainsNot(payload1);
        testResponderUnregistered.assertReceivedPayloadsContains(payload2);
    }

    @Test
    public void removesExpiredRequestReply() throws Exception {
        TestProvider testResponder = new TestProvider(1);
        // To expire within 1000 ms from now
        ExpiryDate ttlReplyCaller1 = ExpiryDate.fromRelativeTtl(1000L);
        final ReplyCaller replyCaller1 = mock(ReplyCaller.class);
        replyCallerDirectory.addReplyCaller(request1.getRequestReplyId(), replyCaller1, ttlReplyCaller1);
        // To expire within REPLY_DIRECTORY_CLEANUP_TASK_INTERVAL_MS + 1000L ms (~6s) from now
        ExpiryDate ttlReplyCaller2 = ExpiryDate.fromRelativeTtl(REPLY_DIRECTORY_CLEANUP_TASK_INTERVAL_MS + 1000L);
        final ReplyCaller replyCaller2 = mock(ReplyCaller.class);
        replyCallerDirectory.addReplyCaller(request2.getRequestReplyId(), replyCaller2, ttlReplyCaller2);

        Thread.sleep(REPLY_DIRECTORY_CLEANUP_TASK_INTERVAL_MS + 100L);
        requestReplyManager.handleReply(new Reply(request1.getRequestReplyId(),
                                                  testResponder.getSentPayloadFor(request1)));

        verify(replyCaller1, never()).messageCallBack(any(Reply.class));
        verify(replyCaller1, times(1)).error(any(Throwable.class));
        verify(replyCaller2, never()).messageCallBack(any(Reply.class));
        verify(replyCaller2, never()).error(any(Throwable.class));
    }

    @Test
    public void sendOneWayTtl() throws JoynrMessageNotSentException, JoynrSendBufferFullException,
                                JsonGenerationException, JsonMappingException, IOException {
        testOneWay(1, TIME_TO_LIVE);
    }

    @Test
    public void sendExpiredOneWay() {
        testOneWay(0, -1L);
    }

    private void testOneWay(int expectedCalls, long forTtl) {
        TestOneWayRecipient oneWayRecipient = spy(new TestOneWayRecipient(expectedCalls));
        when(providerContainer.getRequestCaller()).thenReturn(requestCallerFactory.create(oneWayRecipient));
        providerDirectory.add(testOneWayRecipientParticipantId, providerContainer);

        requestReplyManager.handleOneWayRequest(testOneWayRecipientParticipantId,
                                                oneWay1,
                                                ExpiryDate.fromRelativeTtl(forTtl).getValue());

        oneWayRecipient.assertAllPayloadsReceived(TIME_TO_LIVE);
    }

    private Set<Thread> prepareParallelRequests(int numRequests,
                                                CountDownLatch callCount,
                                                CountDownLatch replyCount,
                                                Semaphore semaphore,
                                                RequestCaller requestCallerMock,
                                                ReplyCallback replyCallbackMock,
                                                Request request) {
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                callCount.countDown();
                if (!semaphore.tryAcquire(5000, TimeUnit.MILLISECONDS)) {
                    fail("wait time expired for semaphore");
                }
                @SuppressWarnings("unchecked")
                ProviderCallback<Reply> cb = (ProviderCallback<Reply>) invocation.getArguments()[0];
                cb.onSuccess(new Reply());
                replyCount.countDown();
                return null;
            }
        }).when(mockRequestInterpreter)
          .execute(ArgumentMatchers.<ProviderCallback<Reply>> any(), any(RequestCaller.class), any(Request.class));

        when(providerContainer.getRequestCaller()).thenReturn(requestCallerMock);
        providerDirectory.add(testMessageResponderParticipantId, providerContainer);

        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                requestReplyManager.handleRequest(replyCallbackMock,
                                                  testMessageResponderParticipantId,
                                                  request,
                                                  Long.MAX_VALUE);
            }
        };
        Set<Thread> threads = new HashSet<Thread>();
        for (int i = 0; i < numRequests; i++) {
            threads.add(new Thread(runnable));
        }
        return threads;
    }

    @Test
    public void parallelRequestHandling() throws NoSuchMethodException, SecurityException, InterruptedException {
        int numRequests = 42;
        CountDownLatch callCount = new CountDownLatch(numRequests);
        CountDownLatch replyCount = new CountDownLatch(numRequests);
        Semaphore semaphore = new Semaphore(0);
        requestReplyManager = new RequestReplyManagerImpl(null,
                                                          null,
                                                          providerDirectory,
                                                          null,
                                                          mockRequestInterpreter,
                                                          null,
                                                          mockShutdownNotifier,
                                                          null);
        RequestCaller requestCallerMock = mock(RequestCaller.class);
        ReplyCallback replyCallbackMock = mock(ReplyCallback.class);

        Method method = TestProvider.class.getMethod("voidOperation", new Class[0]);
        Request request = new Request(method.getName(), new Object[0], method.getParameterTypes());

        Set<Thread> threads = prepareParallelRequests(numRequests,
                                                      callCount,
                                                      replyCount,
                                                      semaphore,
                                                      requestCallerMock,
                                                      replyCallbackMock,
                                                      request);
        threads.stream().forEach(t -> t.start());

        assertTrue(callCount.await(5000, TimeUnit.MILLISECONDS));
        verify(mockRequestInterpreter, times(numRequests)).execute(replyCallbackMock, requestCallerMock, request);

        verify(replyCallbackMock, never()).onSuccess(any(Reply.class));
        assertEquals(numRequests, replyCount.getCount());
        semaphore.release(numRequests);
        assertTrue(replyCount.await(5000, TimeUnit.MILLISECONDS));
        verify(replyCallbackMock, times(numRequests)).onSuccess(any(Reply.class));

        threads.stream().forEach(t -> {
            try {
                t.join();
            } catch (InterruptedException e) {
                fail("t.join() interrupted: " + e);
            }
        });
    }

    private Set<Thread> prepareParallelOneWayRequests(int numRequests,
                                                      CountDownLatch callCount,
                                                      CountDownLatch replyCount,
                                                      Semaphore semaphore,
                                                      RequestCaller requestCallerMock,
                                                      OneWayRequest request) {
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                callCount.countDown();
                if (!semaphore.tryAcquire(5000, TimeUnit.MILLISECONDS)) {
                    fail("wait time expired for semaphore");
                }
                replyCount.countDown();
                return null;
            }
        }).when(mockRequestInterpreter).invokeMethod(any(RequestCaller.class), any(OneWayRequest.class));

        when(providerContainer.getRequestCaller()).thenReturn(requestCallerMock);
        providerDirectory.add(testMessageResponderParticipantId, providerContainer);

        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                requestReplyManager.handleOneWayRequest(testMessageResponderParticipantId, request, Long.MAX_VALUE);
            }
        };
        Set<Thread> threads = new HashSet<Thread>();
        for (int i = 0; i < numRequests; i++) {
            threads.add(new Thread(runnable));
        }
        return threads;
    }

    @Test
    public void parallelOnyWayRequestHandling() throws NoSuchMethodException, SecurityException, InterruptedException {
        int numRequests = 42;
        CountDownLatch callCount = new CountDownLatch(numRequests);
        CountDownLatch replyCount = new CountDownLatch(numRequests);
        Semaphore semaphore = new Semaphore(0);
        requestReplyManager = new RequestReplyManagerImpl(null,
                                                          null,
                                                          providerDirectory,
                                                          null,
                                                          mockRequestInterpreter,
                                                          null,
                                                          mockShutdownNotifier,
                                                          null);
        RequestCaller requestCallerMock = mock(RequestCaller.class);

        Method method = TestProvider.class.getMethod("voidOperation", new Class[0]);
        OneWayRequest oneWayRequest = new OneWayRequest(method.getName(), new Object[0], method.getParameterTypes());

        Set<Thread> threads = prepareParallelOneWayRequests(numRequests,
                                                            callCount,
                                                            replyCount,
                                                            semaphore,
                                                            requestCallerMock,
                                                            oneWayRequest);
        threads.stream().forEach(t -> t.start());

        assertTrue(callCount.await(5000, TimeUnit.MILLISECONDS));
        verify(mockRequestInterpreter, times(numRequests)).invokeMethod(requestCallerMock, oneWayRequest);

        assertEquals(numRequests, replyCount.getCount());
        semaphore.release(numRequests);
        assertTrue(replyCount.await(5000, TimeUnit.MILLISECONDS));

        threads.stream().forEach(t -> {
            try {
                t.join();
            } catch (InterruptedException e) {
                fail("t.join() interrupted: " + e);
            }
        });
    }

    @Test
    public void parallelRequestAndOneWayRequestHandling() throws NoSuchMethodException, SecurityException,
                                                          InterruptedException {
        int numRequests = 31;
        int numOneWayRequests = 32;
        CountDownLatch callCount = new CountDownLatch(numRequests + numOneWayRequests);
        CountDownLatch replyCount = new CountDownLatch(numRequests + numOneWayRequests);
        Semaphore semaphore = new Semaphore(0);
        requestReplyManager = new RequestReplyManagerImpl(null,
                                                          null,
                                                          providerDirectory,
                                                          null,
                                                          mockRequestInterpreter,
                                                          null,
                                                          mockShutdownNotifier,
                                                          null);
        RequestCaller requestCallerMock = mock(RequestCaller.class);
        ReplyCallback replyCallbackMock = mock(ReplyCallback.class);

        Method method = TestProvider.class.getMethod("voidOperation", new Class[0]);
        Request request = new Request(method.getName(), new Object[0], method.getParameterTypes());
        Set<Thread> requestThreads = prepareParallelRequests(numRequests,
                                                             callCount,
                                                             replyCount,
                                                             semaphore,
                                                             requestCallerMock,
                                                             replyCallbackMock,
                                                             request);

        OneWayRequest oneWayRequest = new OneWayRequest(method.getName(), new Object[0], method.getParameterTypes());
        Set<Thread> oneWayRequestThreads = prepareParallelOneWayRequests(numOneWayRequests,
                                                                         callCount,
                                                                         replyCount,
                                                                         semaphore,
                                                                         requestCallerMock,
                                                                         oneWayRequest);

        Set<Thread> threads = new HashSet<Thread>(requestThreads);
        threads.addAll(oneWayRequestThreads);

        threads.stream().forEach(t -> t.start());

        assertTrue(callCount.await(5000, TimeUnit.MILLISECONDS));
        verify(mockRequestInterpreter, times(numRequests)).execute(replyCallbackMock, requestCallerMock, request);
        verify(mockRequestInterpreter, times(numOneWayRequests)).invokeMethod(requestCallerMock, oneWayRequest);

        verify(replyCallbackMock, never()).onSuccess(any(Reply.class));
        assertEquals(numRequests + numOneWayRequests, replyCount.getCount());
        semaphore.release(numRequests + numOneWayRequests);
        assertTrue(replyCount.await(5000, TimeUnit.MILLISECONDS));
        verify(replyCallbackMock, times(numRequests)).onSuccess(any(Reply.class));

        threads.stream().forEach(t -> {
            try {
                t.join();
            } catch (InterruptedException e) {
                fail("t.join() interrupted: " + e);
            }
        });
    }

}
