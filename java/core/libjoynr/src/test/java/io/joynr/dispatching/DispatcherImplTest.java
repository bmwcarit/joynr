/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import static io.joynr.proxy.StatelessAsyncIdCalculator.REQUEST_REPLY_ID_SEPARATOR;
import static io.joynr.proxy.StatelessAsyncIdCalculator.USE_CASE_SEPARATOR;
import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;
import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyObject;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.argThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.Spy;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;

import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.dispatching.subscription.PublicationManager;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingQosEffort;
import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.sender.MessageSender;
import io.joynr.provider.AbstractSubscriptionPublisher;
import io.joynr.provider.ProviderCallback;
import io.joynr.provider.ProviderContainer;
import io.joynr.proxy.JoynrMessagingConnectorFactory;
import io.joynr.proxy.StatelessAsyncIdCalculator;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import io.joynr.util.JoynrThreadFactory;
import joynr.ImmutableMessage;
import joynr.MulticastPublication;
import joynr.MulticastSubscriptionRequest;
import joynr.MutableMessage;
import joynr.OnChangeSubscriptionQos;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionRequest;

@RunWith(MockitoJUnitRunner.class)
public class DispatcherImplTest {

    private static final String TEST_CUSTOM_HEADER_KEY = "testCustHeaderKey";
    private static final String TEST_CUSTOM_HEADER_VALUE = "testCustHeaderValue";

    @Mock
    private RequestReplyManager requestReplyManagerMock;
    @Mock
    private SubscriptionManager subscriptionManagerMock;
    @Mock
    private PublicationManager publicationManagerMock;
    @Mock
    private MulticastReceiverRegistrar mockMulticastReceiverRegistrar;
    @Mock
    private MessageSender messageSenderMock;
    @Mock
    private StatelessAsyncIdCalculator statelessAsyncIdCalculator;
    @Spy
    private MessageReceiverMock messageReceiverMock = new MessageReceiverMock();

    @Captor
    private ArgumentCaptor<ProviderCallback<Reply>> providerCallbackReply;

    private Dispatcher fixture;
    private ProviderDirectory requestCallerDirectory;
    private MutableMessageFactory messageFactory;
    final boolean compress = false;

    @Before
    public void setUp() throws NoSuchMethodException, SecurityException {
        Injector injector = Guice.createInjector(new JsonMessageSerializerModule(), new AbstractModule() {

            @Override
            protected void configure() {
                bind(Dispatcher.class).to(DispatcherImpl.class);
                bind(Boolean.class).annotatedWith(Names.named(MessagingPropertyKeys.PROPERTY_MESSAGING_COMPRESS_REPLIES))
                                   .toInstance(false);
                bind(RequestReplyManager.class).toInstance(requestReplyManagerMock);
                bind(SubscriptionManager.class).toInstance(subscriptionManagerMock);
                bind(PublicationManager.class).toInstance(publicationManagerMock);
                bind(MessageSender.class).toInstance(messageSenderMock);
                bind(MulticastReceiverRegistrar.class).toInstance(mockMulticastReceiverRegistrar);
                bind(MessageReceiver.class).toInstance(messageReceiverMock);
                Multibinder.newSetBinder(binder(), new TypeLiteral<JoynrMessageProcessor>() {
                });

                requestStaticInjection(RpcUtils.class, Request.class, JoynrMessagingConnectorFactory.class);

                ThreadFactory namedThreadFactory = new JoynrThreadFactory("joynr.Cleanup");
                ScheduledExecutorService cleanupExecutor = Executors.newSingleThreadScheduledExecutor(namedThreadFactory);
                bind(ScheduledExecutorService.class).annotatedWith(Names.named(JOYNR_SCHEDULER_CLEANUP))
                                                    .toInstance(cleanupExecutor);
                bind(StatelessAsyncIdCalculator.class).toInstance(statelessAsyncIdCalculator);
                bind(String.class).annotatedWith(Names.named(MessagingPropertyKeys.CHANNELID)).toInstance("channelid");
            }

        });
        fixture = injector.getInstance(Dispatcher.class);
        messageReceiverMock.start(fixture, new ReceiverStatusListener() {
            @Override
            public void receiverStarted() {
            }

            @Override
            public void receiverException(Throwable e) {
            }
        });
        requestCallerDirectory = injector.getInstance(ProviderDirectory.class);
        messageFactory = injector.getInstance(MutableMessageFactory.class);
    }

    @Test
    public void addRequestCallerDoesNotBlock() throws InterruptedException, ExecutionException, TimeoutException {

        final Callable<Boolean> stuffToDo = new Callable<Boolean>() {

            @Override
            public Boolean call() throws Exception {

                try {
                    String requestReplyId = createUuidString();
                    RequestCaller requestCaller = mock(RequestCaller.class);
                    AbstractSubscriptionPublisher subscriptionPublisher = mock(AbstractSubscriptionPublisher.class);
                    int majorVersion = 42;
                    /* setBlockInitialisation to true causes the messageReceiver to block
                     * during startup
                     * The MessageReceiver is invoked by the dispatcher once a request caller
                     * is registered
                     *
                     */
                    messageReceiverMock.setBlockOnInitialisation(true);
                    requestCallerDirectory.add(requestReplyId,
                                               new ProviderContainer("interfaceName",
                                                                     DispatcherImplTest.class,
                                                                     majorVersion,
                                                                     requestCaller,
                                                                     subscriptionPublisher));
                } finally {
                    messageReceiverMock.setBlockOnInitialisation(false);
                }
                return true;
            }
        };

        final ExecutorService executor = Executors.newSingleThreadExecutor();
        final Future<Boolean> future = executor.submit(stuffToDo);

        // should not throw a timeout exception
        future.get(1000, TimeUnit.MILLISECONDS);
        verify(messageReceiverMock).start(eq(fixture), any(ReceiverStatusListener.class));
    }

    @Test
    public void testHandleOneWayRequest() throws Exception {
        OneWayRequest request = new OneWayRequest("method", new Object[0], new Class<?>[0]);
        String toParticipantId = "toParticipantId";
        MessagingQos messagingQos = new MessagingQos(1000L);
        MutableMessage joynrMessage = messageFactory.createOneWayRequest("fromParticipantId",
                                                                         toParticipantId,
                                                                         request,
                                                                         messagingQos);

        fixture.messageArrived(joynrMessage.getImmutableMessage());

        verify(requestReplyManagerMock).handleOneWayRequest(toParticipantId, request, joynrMessage.getTtlMs());
        verify(messageSenderMock, never()).sendMessage(any(MutableMessage.class));
    }

    @Test
    public void testCustomHeadersAreSetForOneWayRequest() throws Exception {
        // Given an incoming message representing a OneWayRequest...
        OneWayRequest request = new OneWayRequest("method", new Object[0], new Class<?>[0]);
        String toParticipantId = "toParticipantId";
        MessagingQos messagingQos = new MessagingQos(1000L);
        MutableMessage joynrMessage = messageFactory.createOneWayRequest("fromParticipantId",
                                                                         toParticipantId,
                                                                         request,
                                                                         messagingQos);
        // ...and containing a custom header
        Map<String, String> customHeaders = new HashMap<>();
        customHeaders.put(TEST_CUSTOM_HEADER_KEY, TEST_CUSTOM_HEADER_VALUE);
        joynrMessage.setCustomHeaders(customHeaders);

        // When it is processed by the dispatcher
        fixture.messageArrived(joynrMessage.getImmutableMessage());

        // The deserialized OneWayRequest forwarded to the RequestReplyManager contains the custom header
        ArgumentCaptor<OneWayRequest> argument = ArgumentCaptor.forClass(OneWayRequest.class);
        verify(requestReplyManagerMock).handleOneWayRequest(any(String.class), argument.capture(), anyLong());
        assertTrue(argument.getValue().getContext().containsKey(TEST_CUSTOM_HEADER_KEY));
        assertEquals(TEST_CUSTOM_HEADER_VALUE, argument.getValue().getContext().get(TEST_CUSTOM_HEADER_KEY));
    }

    @Test
    public void testCustomHeadersAreSetForRequest() throws Exception {
        // Given an incoming message representing a Request...
        Request request = new Request("method", new Object[0], new Class<?>[0]);
        String toParticipantId = "toParticipantId";
        MessagingQos messagingQos = new MessagingQos(1000L);
        MutableMessage joynrMessage = messageFactory.createRequest("fromParticipantId",
                                                                   toParticipantId,
                                                                   request,
                                                                   messagingQos);
        // ...and containing a custom header
        Map<String, String> customHeaders = new HashMap<>();
        customHeaders.put(TEST_CUSTOM_HEADER_KEY, TEST_CUSTOM_HEADER_VALUE);
        joynrMessage.setCustomHeaders(customHeaders);

        // When it is processed by the dispatcher
        fixture.messageArrived(joynrMessage.getImmutableMessage());

        // The deserialized Request forwarded to the RequestReplyManager contains the custom header
        ArgumentCaptor<Request> argument = ArgumentCaptor.forClass(Request.class);
        verify(requestReplyManagerMock).handleRequest(anyObject(), anyString(), argument.capture(), anyLong());
        assertTrue(argument.getValue().getContext().containsKey(TEST_CUSTOM_HEADER_KEY));
        assertEquals(TEST_CUSTOM_HEADER_VALUE, argument.getValue().getContext().get(TEST_CUSTOM_HEADER_KEY));
    }

    @Test
    public void testSendMulticastMessage() {
        MutableMessageFactory messageFactoryMock = mock(MutableMessageFactory.class);
        ObjectMapper objectMapperMock = mock(ObjectMapper.class);
        fixture = new DispatcherImpl(requestReplyManagerMock,
                                     subscriptionManagerMock,
                                     publicationManagerMock,
                                     mockMulticastReceiverRegistrar,
                                     messageSenderMock,
                                     messageFactoryMock,
                                     objectMapperMock,
                                     compress,
                                     statelessAsyncIdCalculator);

        String fromParticipantId = "fromParticipantId";
        MulticastPublication multicastPublication = mock(MulticastPublication.class);
        MessagingQos messagingQos = mock(MessagingQos.class);

        fixture.sendMulticast(fromParticipantId, multicastPublication, messagingQos);

        verify(messageFactoryMock).createMulticast(eq(fromParticipantId), eq(multicastPublication), eq(messagingQos));
    }

    @Test
    public void testReceiveMulticastSubscription() throws Exception {
        String from = "from";
        String to = "to";
        MulticastSubscriptionRequest subscriptionRequest = new MulticastSubscriptionRequest("multicastId",
                                                                                            "subscriptionId",
                                                                                            "multicastName",
                                                                                            new OnChangeSubscriptionQos());
        MutableMessage joynrMessage = messageFactory.createSubscriptionRequest(from,
                                                                               to,
                                                                               subscriptionRequest,
                                                                               new MessagingQos(1000L));

        MutableMessageFactory messageFactoryMock = mock(MutableMessageFactory.class);
        ObjectMapper objectMapperMock = mock(ObjectMapper.class);
        when(objectMapperMock.readValue(anyString(), eq(SubscriptionRequest.class))).thenReturn(subscriptionRequest);

        fixture = new DispatcherImpl(requestReplyManagerMock,
                                     subscriptionManagerMock,
                                     publicationManagerMock,
                                     mockMulticastReceiverRegistrar,
                                     messageSenderMock,
                                     messageFactoryMock,
                                     objectMapperMock,
                                     compress,
                                     statelessAsyncIdCalculator);

        fixture.messageArrived(joynrMessage.getImmutableMessage());

        verify(publicationManagerMock).addSubscriptionRequest(eq(from), eq(to), eq(subscriptionRequest));
    }

    private void testPropagateCompressFlagFromRequestToRepliesImpl(final boolean compress,
                                                                   final boolean compressAllOutgoingReplies) throws Exception {
        MessagingQos messagingQos = new MessagingQos(1000L);
        messagingQos.setCompress(compress);

        String requestReplyId = createUuidString();
        Request request = new Request("methodName", new Object[]{}, new String[]{}, requestReplyId);
        final String providerParticipantId = "toParticipantId";

        MutableMessage joynrMessage = messageFactory.createRequest("fromParticipantId",
                                                                   providerParticipantId,
                                                                   request,
                                                                   messagingQos);

        ImmutableMessage outgoingMessage = joynrMessage.getImmutableMessage();

        if (!compressAllOutgoingReplies) {
            fixture.messageArrived(outgoingMessage);
            verify(requestReplyManagerMock).handleRequest(providerCallbackReply.capture(),
                                                          eq(providerParticipantId),
                                                          eq(request),
                                                          eq(joynrMessage.getTtlMs()));
            providerCallbackReply.getValue().onSuccess(new Reply(requestReplyId));
            verify(messageSenderMock).sendMessage(argThat(new MessageIsCompressedMatcher(compress)));
        } else {
            MutableMessageFactory messageFactoryMock = mock(MutableMessageFactory.class);
            ObjectMapper objectMapperMock = mock(ObjectMapper.class);

            when(objectMapperMock.readValue(any(String.class), eq(Request.class))).thenReturn(request);

            fixture = new DispatcherImpl(requestReplyManagerMock,
                                         subscriptionManagerMock,
                                         publicationManagerMock,
                                         mockMulticastReceiverRegistrar,
                                         messageSenderMock,
                                         messageFactoryMock,
                                         objectMapperMock,
                                         compressAllOutgoingReplies,
                                         statelessAsyncIdCalculator);

            fixture.messageArrived(outgoingMessage);
            verify(requestReplyManagerMock).handleRequest(providerCallbackReply.capture(),
                                                          eq(providerParticipantId),
                                                          eq(request),
                                                          eq(joynrMessage.getTtlMs()));

            providerCallbackReply.getValue().onSuccess(new Reply(requestReplyId));

            ArgumentCaptor<MessagingQos> qosCaptor = ArgumentCaptor.forClass(MessagingQos.class);
            verify(messageFactoryMock).createReply(eq(providerParticipantId),
                                                   anyString(),
                                                   any(Reply.class),
                                                   qosCaptor.capture());
            assertEquals(true, qosCaptor.getValue().getCompress());
        }
    }

    @Test
    public void testPropagateCompressFlagFromRequestToReplies() throws Exception {
        boolean compressAllOutgoingReplies = false;
        boolean compress = true;
        testPropagateCompressFlagFromRequestToRepliesImpl(compress, compressAllOutgoingReplies);
        compress = false;
        testPropagateCompressFlagFromRequestToRepliesImpl(compress, compressAllOutgoingReplies);
    }

    @Test
    public void testCompressReplies() throws Exception {
        boolean compressAllOutgoingReplies = true;
        boolean compress = true;
        testPropagateCompressFlagFromRequestToRepliesImpl(compress, compressAllOutgoingReplies);
        compress = false;
        testPropagateCompressFlagFromRequestToRepliesImpl(compress, compressAllOutgoingReplies);
    }

    @Test
    public void testPropagateExpirationFromRequestToReplies() throws Exception {
        MessagingQos messagingQos = new MessagingQos(60000L);

        String rrId = createUuidString();
        Request request = new Request("methodName", new Object[]{}, new String[]{}, rrId);
        final String providerId = "toParticipantId";

        MutableMessage joynrMsg = messageFactory.createRequest("fromParticipantId", providerId, request, messagingQos);
        ImmutableMessage requestMsg = joynrMsg.getImmutableMessage();

        fixture.messageArrived(requestMsg);
        verify(requestReplyManagerMock).handleRequest(providerCallbackReply.capture(),
                                                      eq(providerId),
                                                      eq(request),
                                                      eq(requestMsg.getTtlMs()));
        providerCallbackReply.getValue().onSuccess(new Reply(rrId));
        ArgumentCaptor<MutableMessage> captor = ArgumentCaptor.forClass(MutableMessage.class);
        verify(messageSenderMock).sendMessage(captor.capture());
        ImmutableMessage replyMsg = captor.getValue().getImmutableMessage();

        // Now finally perform our actual test
        // We want to make sure that the TTL of the reply matches the TTL of the
        // request. We allow it to be up to 1 ms less or 1000 ms more than the
        // request's TTL.
        // We are using 1000 ms just to make sure that we do not have a toggling
        // test in CI.
        assertTrue(((requestMsg.getTtlMs() - 1) <= replyMsg.getTtlMs())
                && ((requestMsg.getTtlMs() + 1000) >= replyMsg.getTtlMs()));
    }

    private void testPropagateEffortFromRequestToRepliesImpl(final MessagingQosEffort effort) throws Exception {
        Mockito.reset(messageSenderMock);
        MessagingQos messagingQos = new MessagingQos(1000L, effort);

        String requestReplyId = createUuidString();
        Request request = new Request("methodName", new Object[]{}, new String[]{}, requestReplyId);
        final String providerParticipantId = "toParticipantId";

        MutableMessage joynrMessage = messageFactory.createRequest("fromParticipantId",
                                                                   providerParticipantId,
                                                                   request,
                                                                   messagingQos);

        ImmutableMessage outgoingMessage = joynrMessage.getImmutableMessage();

        fixture.messageArrived(outgoingMessage);
        verify(requestReplyManagerMock).handleRequest(providerCallbackReply.capture(),
                                                      eq(providerParticipantId),
                                                      eq(request),
                                                      eq(joynrMessage.getTtlMs()));
        providerCallbackReply.getValue().onSuccess(new Reply(requestReplyId));
        ArgumentCaptor<MutableMessage> captor = ArgumentCaptor.forClass(MutableMessage.class);
        verify(messageSenderMock).sendMessage(captor.capture());
        ImmutableMessage immutableMessage = captor.getValue().getImmutableMessage();
        if (effort == MessagingQosEffort.NORMAL || effort == null) {
            assertEquals(null, immutableMessage.getEffort());
        } else {
            assertEquals(effort.name(), immutableMessage.getEffort());
        }
    }

    @Test
    public void testPropagateEffortFromRequestToReplies() throws Exception {
        MessagingQosEffort effort = MessagingQosEffort.NORMAL;
        testPropagateEffortFromRequestToRepliesImpl(effort);
        effort = MessagingQosEffort.BEST_EFFORT;
        testPropagateEffortFromRequestToRepliesImpl(effort);
        effort = null;
        testPropagateEffortFromRequestToRepliesImpl(effort);
    }

    @Test
    public void testRequestWithInvalidEffort_replyUsesDefaultEffort() throws Exception {
        MessagingQos messagingQos = new MessagingQos(1000L);

        String requestReplyId = createUuidString();
        Request request = new Request("methodName", new Object[]{}, new String[]{}, requestReplyId);
        final String providerParticipantId = "toParticipantId";

        MutableMessage joynrMessage = messageFactory.createRequest("fromParticipantId",
                                                                   providerParticipantId,
                                                                   request,
                                                                   messagingQos);

        joynrMessage.setEffort("INVALID_EFFORT");
        ImmutableMessage outgoingMessage = joynrMessage.getImmutableMessage();

        fixture.messageArrived(outgoingMessage);
        verify(requestReplyManagerMock).handleRequest(providerCallbackReply.capture(),
                                                      eq(providerParticipantId),
                                                      eq(request),
                                                      eq(joynrMessage.getTtlMs()));
        providerCallbackReply.getValue().onSuccess(new Reply(requestReplyId));
        ArgumentCaptor<MutableMessage> captor = ArgumentCaptor.forClass(MutableMessage.class);
        verify(messageSenderMock).sendMessage(captor.capture());
        ImmutableMessage immutableMessage = captor.getValue().getImmutableMessage();
        assertEquals(null, immutableMessage.getEffort());
    }

    @Test
    public void testStatelessAsyncReplyInformationExtracted() throws Exception {
        String methodId = "456";
        String requestReplyId = String.format("123%s%s", REQUEST_REPLY_ID_SEPARATOR, methodId);
        when(statelessAsyncIdCalculator.extractMethodIdFromRequestReplyId(eq(requestReplyId))).thenReturn(methodId);
        Reply reply = new Reply(requestReplyId);
        String statelessAsyncParticipantId = createUuidString();
        String statelessAsyncCallbackId = String.format("interface%suseCase", USE_CASE_SEPARATOR);
        when(statelessAsyncIdCalculator.fromParticipantUuid(eq(statelessAsyncParticipantId))).thenReturn(statelessAsyncCallbackId);
        MutableMessage mutableMessage = messageFactory.createReply(createUuidString(),
                                                                   statelessAsyncParticipantId,
                                                                   reply,
                                                                   new MessagingQos(1000L));

        fixture.messageArrived(mutableMessage.getImmutableMessage());

        verify(statelessAsyncIdCalculator).extractMethodIdFromRequestReplyId(eq(requestReplyId));
        verify(statelessAsyncIdCalculator).fromParticipantUuid(eq(statelessAsyncParticipantId));
        ArgumentCaptor<Reply> captor = ArgumentCaptor.forClass(Reply.class);
        verify(requestReplyManagerMock).handleReply(captor.capture());
        Reply capturedReply = captor.getValue();
        assertNotNull(capturedReply);
        assertEquals(methodId, capturedReply.getStatelessAsyncCallbackMethodId());
        assertEquals(statelessAsyncCallbackId, capturedReply.getStatelessAsyncCallbackId());
    }

    private static class MessageIsCompressedMatcher extends ArgumentMatcher<MutableMessage> {
        private final boolean shouldMessageBeCompressed;

        public MessageIsCompressedMatcher(final boolean compressed) {
            this.shouldMessageBeCompressed = compressed;
        }

        @Override
        public boolean matches(Object argument) {
            if (!(argument instanceof MutableMessage)) {
                return false;
            }

            try {
                return ((MutableMessage) argument).getImmutableMessage().isCompressed() == shouldMessageBeCompressed;
            } catch (SecurityException | EncodingException | UnsuppportedVersionException e) {
                e.printStackTrace();
                return false;
            }
        }
    }

}
