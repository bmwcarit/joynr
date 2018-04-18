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

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;
import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.argThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.UUID;
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
import org.mockito.Spy;
import org.mockito.runners.MockitoJUnitRunner;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
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
import io.joynr.messaging.ReceiverStatusListener;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.sender.MessageSender;
import io.joynr.provider.AbstractSubscriptionPublisher;
import io.joynr.provider.ProviderCallback;
import io.joynr.provider.ProviderContainer;
import io.joynr.proxy.JoynrMessagingConnectorFactory;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
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

    @Mock
    private RequestReplyManager requestReplyManagerMock;
    @Mock
    private SubscriptionManager subscriptionManagerMock;
    @Mock
    private PublicationManager publicationManagerMock;
    @Mock
    private MessageRouter messageRouterMock;
    @Mock
    private MessageSender messageSenderMock;
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
                bind(MessageRouter.class).toInstance(messageRouterMock);
                bind(MessageReceiver.class).toInstance(messageReceiverMock);
                Multibinder.newSetBinder(binder(), new TypeLiteral<JoynrMessageProcessor>() {
                });

                requestStaticInjection(RpcUtils.class, Request.class, JoynrMessagingConnectorFactory.class);

                ThreadFactory namedThreadFactory = new ThreadFactoryBuilder().setNameFormat("joynr.Cleanup-%d").build();
                ScheduledExecutorService cleanupExecutor = Executors.newSingleThreadScheduledExecutor(namedThreadFactory);
                bind(ScheduledExecutorService.class).annotatedWith(Names.named(JOYNR_SCHEDULER_CLEANUP))
                                                    .toInstance(cleanupExecutor);
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
                    String requestReplyId = UUID.randomUUID().toString();
                    RequestCaller requestCaller = mock(RequestCaller.class);
                    AbstractSubscriptionPublisher subscriptionPublisher = mock(AbstractSubscriptionPublisher.class);
                    /* setBlockInitialisation to true causes the messageReceiver to block
                     * during startup
                     * The MessageReceiver is invoked by the dispatcher once a request caller
                     * is registered
                     *
                     */
                    messageReceiverMock.setBlockOnInitialisation(true);
                    requestCallerDirectory.add(requestReplyId, new ProviderContainer("interfaceName",
                                                                                     DispatcherImplTest.class,
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
    public void testSendMulticastMessage() {
        MutableMessageFactory messageFactoryMock = mock(MutableMessageFactory.class);
        ObjectMapper objectMapperMock = mock(ObjectMapper.class);
        fixture = new DispatcherImpl(requestReplyManagerMock,
                                     subscriptionManagerMock,
                                     publicationManagerMock,
                                     messageRouterMock,
                                     messageSenderMock,
                                     messageFactoryMock,
                                     objectMapperMock,
                                     compress);

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
                                     messageRouterMock,
                                     messageSenderMock,
                                     messageFactoryMock,
                                     objectMapperMock,
                                     compress);

        fixture.messageArrived(joynrMessage.getImmutableMessage());

        verify(publicationManagerMock).addSubscriptionRequest(eq(from), eq(to), eq(subscriptionRequest));
    }

    private void testPropagateCompressFlagFromRequestToRepliesImpl(final boolean compress,
                                                                   final boolean compressAllOutgoingReplies)
                                                                                                            throws Exception {
        MessagingQos messagingQos = new MessagingQos(1000L);
        messagingQos.setCompress(compress);

        String requestReplyId = UUID.randomUUID().toString();
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
                                         messageRouterMock,
                                         messageSenderMock,
                                         messageFactoryMock,
                                         objectMapperMock,
                                         compressAllOutgoingReplies);

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
