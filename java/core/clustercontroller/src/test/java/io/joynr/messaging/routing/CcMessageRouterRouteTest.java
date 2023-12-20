/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.messaging.routing;

import io.joynr.common.ExpiryDate;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessMessagingSkeleton;
import io.joynr.messaging.tracking.MessageTrackerForGracefulShutdown;
import joynr.BroadcastFilterParameters;
import joynr.BroadcastSubscriptionRequest;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MulticastPublication;
import joynr.MutableMessage;
import joynr.OnChangeSubscriptionQos;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionReply;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.system.RoutingTypes.Address;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;
import java.util.function.Supplier;

import static io.joynr.proxy.StatelessAsyncIdCalculator.REQUEST_REPLY_ID_SEPARATOR;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@RunWith(MockitoJUnitRunner.class)
public class CcMessageRouterRouteTest extends AbstractCcMessageRouterTest {

    private Set<String> trackerRegisteredMessages;
    private Semaphore semaphore;
    private MessageProcessedListener mockMessageProcessedListener;

    private static final String REQUEST_REPLY_ID = "requestReplyId";
    private static final String TO_PARTICIPANT_ID = "toInProcess";
    private static final String SUBSCRIPTION_ID = "subscriptionId";
    private static final String ERROR_MESSAGE = "Error message";
    private static final String MULTICAST_ID = "multicast/id/test";
    private static final String STATELESS_ASYNC_REQUEST_REPLY_ID = REQUEST_REPLY_ID + REQUEST_REPLY_ID_SEPARATOR
            + "someMethodId";
    private static final Consumer<InvocationOnMock> MSG_SUCCESS = i -> ((SuccessAction) i.getArgument(1)).execute();
    private static final Consumer<InvocationOnMock> MSG_ERROR = i -> ((FailureAction) i.getArgument(2)).execute(new JoynrMessageNotSentException("Message was not sent."));

    private final Supplier<ImmutableMessage> replySupplier = this::createReply;
    private final Supplier<ImmutableMessage> requestSupplier = this::createRequest;
    private final Supplier<ImmutableMessage> subscriptionPublicationSupplier = this::createSubscriptionPublication;
    private final Supplier<ImmutableMessage> subscriptionReplySupplier = this::createSubscriptionReply;
    private final Supplier<ImmutableMessage> subscriptionStopSupplier = this::createSubscriptionStop;
    private final Supplier<ImmutableMessage> subscriptionRequestSupplier = this::createSubscriptionRequest;
    private final Supplier<ImmutableMessage> statelessAsyncReplySupplier = this::createStatelessAsyncReply;
    private final Supplier<ImmutableMessage> statelessAsyncRequestSupplier = this::createStatelessAsyncRequest;
    private final Supplier<ImmutableMessage> oneWayRequestSupplier = this::createOneWayRequest;
    private final Supplier<ImmutableMessage> multicastPublicationSupplier = this::createMulticastPublication;
    private final Supplier<ImmutableMessage> broadcastSubscriptionRequestSupplier = this::createBroadcastSubscriptionRequest;

    @Test
    public void testReplyProcessedSuccessfullyIsTracked() throws InterruptedException {
        testMessageType(replySupplier, MSG_SUCCESS);
    }

    @Test
    public void testReplyProcessedWithErrorIsTracked() throws InterruptedException {
        testMessageType(replySupplier, MSG_ERROR);
    }

    @Test
    public void testRequestProcessedSuccessfullyIsTracked() throws InterruptedException {
        testMessageType(requestSupplier, MSG_SUCCESS);
    }

    @Test
    public void testRequestProcessedWithErrorIsTracked() throws InterruptedException {
        testMessageType(requestSupplier, MSG_ERROR);
    }

    @Test
    public void testSubscriptionPublicationProcessedSuccessfullyIsTracked() throws InterruptedException {
        testMessageType(subscriptionPublicationSupplier, MSG_SUCCESS);
    }

    @Test
    public void testSubscriptionPublicationProcessedWithErrorIsTracked() throws InterruptedException {
        testMessageType(subscriptionPublicationSupplier, MSG_ERROR);
    }

    @Test
    public void testSubscriptionReplyProcessedSuccessfullyIsNotTracked() throws InterruptedException {
        testMessageType(subscriptionReplySupplier, MSG_SUCCESS, false);
    }

    @Test
    public void testSubscriptionReplyProcessedWithErrorIsMotTracked() throws InterruptedException {
        testMessageType(subscriptionReplySupplier, MSG_ERROR, false);
    }

    @Test
    public void testSubscriptionStopProcessedSuccessfullyIsTracked() throws InterruptedException {
        testMessageType(subscriptionStopSupplier, MSG_SUCCESS);
    }

    @Test
    public void testSubscriptionStopProcessedWithErrorIsTracked() throws InterruptedException {
        testMessageType(subscriptionStopSupplier, MSG_ERROR);
    }

    @Test
    public void testSubscriptionRequestProcessedSuccessfullyIsTracked() throws InterruptedException {
        testMessageType(subscriptionRequestSupplier, MSG_SUCCESS);
    }

    @Test
    public void testSubscriptionRequestProcessedWithErrorIsTracked() throws InterruptedException {
        testMessageType(subscriptionRequestSupplier, MSG_ERROR);
    }

    @Test
    public void testStatelessAsyncReplyProcessedSuccessfullyIsTracked() throws InterruptedException {
        testMessageType(statelessAsyncReplySupplier, MSG_SUCCESS);
    }

    @Test
    public void testStatelessAsyncReplyProcessedWithErrorIsTracked() throws InterruptedException {
        testMessageType(statelessAsyncReplySupplier, MSG_ERROR);
    }

    @Test
    public void testStatelessAsyncRequestProcessedSuccessfullyIsTracked() throws InterruptedException {
        testMessageType(statelessAsyncRequestSupplier, MSG_SUCCESS);
    }

    @Test
    public void testStatelessAsyncRequestProcessedWithErrorIsTracked() throws InterruptedException {
        testMessageType(statelessAsyncRequestSupplier, MSG_ERROR);
    }

    @Test
    public void testOneWayRequestProcessedSuccessfullyIsTracked() throws InterruptedException {
        testMessageType(oneWayRequestSupplier, MSG_SUCCESS);
    }

    @Test
    public void testOneWayRequestProcessedWithErrorIsTracked() throws InterruptedException {
        testMessageType(oneWayRequestSupplier, MSG_ERROR);
    }

    @Test
    public void testMulticastPublicationProcessedSuccessfullyIsTracked() throws InterruptedException {
        testMessageType(multicastPublicationSupplier, MSG_SUCCESS);
    }

    @Test
    public void testMulticastPublicationProcessedWithErrorIsTracked() throws InterruptedException {
        testMessageType(multicastPublicationSupplier, MSG_ERROR);
    }

    @Test
    public void testBroadcastSubscriptionRequestProcessedSuccessfullyIsTracked() throws InterruptedException {
        testMessageType(broadcastSubscriptionRequestSupplier, MSG_SUCCESS);
    }

    @Test
    public void testBroadcastSubscriptionRequestProcessedWithErrorIsTracked() throws InterruptedException {
        testMessageType(broadcastSubscriptionRequestSupplier, MSG_ERROR);
    }

    private void testMessageType(final Supplier<ImmutableMessage> messageSupplier,
                                 final Consumer<InvocationOnMock> invocationConsumer) throws InterruptedException {
        testMessageType(messageSupplier, invocationConsumer, true);
    }

    private void testMessageType(final Supplier<ImmutableMessage> messageSupplier,
                                 final Consumer<InvocationOnMock> invocationConsumer,
                                 final boolean trackable) throws InterruptedException {
        createDefaultMessageRouter();
        assertEquals(0, trackerRegisteredMessages.size());
        final ImmutableMessage immutableMessage = messageSupplier.get();
        final String requestReplyId = getMessageId(immutableMessage);
        mockAddress(immutableMessage.getRecipient());
        mockMessageProcessed(semaphore, invocationConsumer);
        if (Message.MessageType.VALUE_MESSAGE_TYPE_MULTICAST.equals(immutableMessage.getType())) {
            final Map<Address, Set<String>> participantIdSet = new HashMap<>();
            participantIdSet.put(new Address(), Set.of(AddressManager.multicastAddressCalculatorParticipantId));
            doReturn(participantIdSet).when(addressManager).getParticipantIdMap(immutableMessage);
            doReturn(Optional.of(mqttAddress)).when(addressManager).getAddressForDelayableImmutableMessage(any());
        }

        ccMessageRouter.routeIn(immutableMessage);
        if (trackable) {
            assertEquals(1, trackerRegisteredMessages.size());
            assertTrue(trackerRegisteredMessages.contains(requestReplyId));
        } else {
            assertEquals(0, trackerRegisteredMessages.size());
        }

        assertTrue(semaphore.tryAcquire(100000, TimeUnit.MILLISECONDS));
        verify(mockMessageProcessedListener).messageProcessed(eq(immutableMessage.getId()));
        assertEquals(0, trackerRegisteredMessages.size());
    }

    private String getMessageId(final ImmutableMessage message) {
        if (isRequestOrReply(message.getType())) {
            final String requestReplyId = message.getCustomHeaders().get(Message.CUSTOM_HEADER_REQUEST_REPLY_ID);
            if (!requestReplyId.contains(REQUEST_REPLY_ID_SEPARATOR)) {
                return requestReplyId;
            }
        }
        return message.getId();
    }

    private boolean isRequestOrReply(final Message.MessageType type) {
        return Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST.equals(type)
                || Message.MessageType.VALUE_MESSAGE_TYPE_REPLY.equals(type);
    }

    private void mockAddress(final String toParticipantId) {
        when(inProcessMessagingStubFactoryMock.create(any(InProcessAddress.class))).thenReturn(messagingStubMock);
        final InProcessAddress inProcessAddress = new InProcessAddress(mock(InProcessMessagingSkeleton.class));
        routingTable.put(toParticipantId, inProcessAddress, true, Long.MAX_VALUE);
    }

    private void mockMessageProcessed(final Semaphore semaphore, final Consumer<InvocationOnMock> invocationConsumer) {
        doAnswer((Answer<Void>) invocation -> {
            Thread.sleep(100);
            invocationConsumer.accept(invocation);
            semaphore.release();
            return null;
        }).when(messagingStubMock)
          .transmit(any(ImmutableMessage.class), any(SuccessAction.class), any(FailureAction.class));
    }

    private ImmutableMessage getImmutableMessage(final MutableMessage joynrMessage) {
        try {
            return joynrMessage.getImmutableMessage();
        } catch (final Exception e) {
            throw new RuntimeException(e);
        }
    }

    private ImmutableMessage createReply() {
        return createReply(REQUEST_REPLY_ID);
    }

    private ImmutableMessage createReply(final String requestReplyId) {
        final Reply reply = new Reply(requestReplyId);
        final MutableMessage mutableMessage = messageFactory.createReply(fromParticipantId,
                                                                         CcMessageRouterRouteTest.TO_PARTICIPANT_ID,
                                                                         reply,
                                                                         new MessagingQos());
        mutableMessage.setTtlMs(ExpiryDate.fromRelativeTtl(100000000).getValue());
        mutableMessage.setTtlAbsolute(true);
        return getImmutableMessage(mutableMessage);
    }

    private ImmutableMessage createStatelessAsyncReply() {
        return createReply(STATELESS_ASYNC_REQUEST_REPLY_ID);
    }

    private ImmutableMessage createRequest() {
        return createRequest(REQUEST_REPLY_ID);
    }

    private ImmutableMessage createRequest(final String requestReplyId) {
        final Request request = new Request("noMethod", new Object[]{}, new String[]{}, requestReplyId);

        final MutableMessage mutableMessage = messageFactory.createRequest(fromParticipantId,
                                                                           toParticipantId,
                                                                           request,
                                                                           new MessagingQos());
        mutableMessage.setLocalMessage(true);

        return getImmutableMessage(mutableMessage);
    }

    private ImmutableMessage createStatelessAsyncRequest() {
        return createRequest(STATELESS_ASYNC_REQUEST_REPLY_ID);
    }

    private ImmutableMessage createSubscriptionPublication() {
        final SubscriptionPublication subscriptionPublication = new SubscriptionPublication(new JoynrRuntimeException(CcMessageRouterRouteTest.ERROR_MESSAGE),
                                                                                            CcMessageRouterRouteTest.SUBSCRIPTION_ID);
        final MutableMessage mutableMessage = messageFactory.createPublication(fromParticipantId,
                                                                               toParticipantId,
                                                                               subscriptionPublication,
                                                                               new MessagingQos());
        return getImmutableMessage(mutableMessage);
    }

    private ImmutableMessage createSubscriptionReply() {
        final SubscriptionReply subscriptionReply = new SubscriptionReply(CcMessageRouterRouteTest.SUBSCRIPTION_ID);
        final MutableMessage mutableMessage = messageFactory.createSubscriptionReply(fromParticipantId,
                                                                                     toParticipantId,
                                                                                     subscriptionReply,
                                                                                     new MessagingQos());
        return getImmutableMessage(mutableMessage);
    }

    private ImmutableMessage createSubscriptionStop() {
        final SubscriptionStop subscriptionStop = new SubscriptionStop(CcMessageRouterRouteTest.SUBSCRIPTION_ID);
        final MutableMessage mutableMessage = messageFactory.createSubscriptionStop(fromParticipantId,
                                                                                    toParticipantId,
                                                                                    subscriptionStop,
                                                                                    new MessagingQos());
        return getImmutableMessage(mutableMessage);
    }

    private ImmutableMessage createSubscriptionRequest() {
        final SubscriptionRequest subscriptionRequest = new SubscriptionRequest(CcMessageRouterRouteTest.SUBSCRIPTION_ID,
                                                                                "subscribedToName",
                                                                                new OnChangeSubscriptionQos());
        final MutableMessage mutableMessage = messageFactory.createSubscriptionRequest(fromParticipantId,
                                                                                       toParticipantId,
                                                                                       subscriptionRequest,
                                                                                       new MessagingQos());
        return getImmutableMessage(mutableMessage);
    }

    private ImmutableMessage createOneWayRequest() {
        final OneWayRequest oneWayRequest = new OneWayRequest("noMethod", new Object[]{}, new String[]{});
        final MutableMessage mutableMessage = messageFactory.createOneWayRequest(fromParticipantId,
                                                                                 toParticipantId,
                                                                                 oneWayRequest,
                                                                                 new MessagingQos());
        return getImmutableMessage(mutableMessage);
    }

    private ImmutableMessage createMulticastPublication() {
        final MulticastPublication multicastPublication = new MulticastPublication(new ArrayList<>(),
                                                                                   CcMessageRouterRouteTest.MULTICAST_ID);
        final MutableMessage mutableMessage = messageFactory.createMulticast(fromParticipantId,
                                                                             multicastPublication,
                                                                             new MessagingQos());
        return getImmutableMessage(mutableMessage);
    }

    private ImmutableMessage createBroadcastSubscriptionRequest() {
        final BroadcastSubscriptionRequest multicastSubscriptionRequest = new BroadcastSubscriptionRequest(CcMessageRouterRouteTest.SUBSCRIPTION_ID,
                                                                                                           "subscribedToName",
                                                                                                           new BroadcastFilterParameters(),
                                                                                                           new OnChangeSubscriptionQos());
        final MutableMessage mutableMessage = messageFactory.createSubscriptionRequest(fromParticipantId,
                                                                                       toParticipantId,
                                                                                       multicastSubscriptionRequest,
                                                                                       new MessagingQos());
        return getImmutableMessage(mutableMessage);
    }

    @Override
    @SuppressWarnings("unchecked")
    protected void createDefaultMessageRouter() {
        super.createDefaultMessageRouter();

        try {
            final Field trackerField = CcMessageRouter.class.getDeclaredField("messageTracker");
            trackerField.setAccessible(true);
            final MessageTrackerForGracefulShutdown tracker = (MessageTrackerForGracefulShutdown) trackerField.get(ccMessageRouter);
            final Field messageSetField = MessageTrackerForGracefulShutdown.class.getDeclaredField("registeredMessages");
            messageSetField.setAccessible(true);
            trackerRegisteredMessages = (Set<String>) messageSetField.get(tracker);
        } catch (final NoSuchFieldException | IllegalAccessException e) {
            fail("Unable to access CcMessageRouter's messageTracker: " + e.getMessage());
        }

        semaphore = new Semaphore(0);

        mockMessageProcessedListener = mock(MessageProcessedListener.class);
        ccMessageRouter.registerMessageProcessedListener(mockMessageProcessedListener);
    }
}