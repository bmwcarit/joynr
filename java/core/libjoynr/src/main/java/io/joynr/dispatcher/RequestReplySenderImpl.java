package io.joynr.dispatcher;

/*
 * #%L
 * joynr::java::core::libjoynr
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

import io.joynr.common.ExpiryDate;
import io.joynr.endpoints.EndpointAddressBase;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRequestInterruptedException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.MessagingQos;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import joynr.JoynrMessage;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.inject.Inject;
import com.google.inject.Singleton;

@Singleton
public class RequestReplySenderImpl implements RequestReplySender {
    private static final Logger logger = LoggerFactory.getLogger(RequestReplySenderImpl.class);
    JoynrMessageFactory joynrMessageFactory;
    MessageSender messageSender;
    MessagingEndpointDirectory messagingEndpointDirectory;
    private boolean running = true;
    private List<Thread> outstandingRequestThreads = Collections.synchronizedList(new ArrayList<Thread>());

    @Inject
    public RequestReplySenderImpl(JoynrMessageFactory joynrMessageFactory,
                                  MessageSender messageSender,
                                  MessagingEndpointDirectory messagingEndpointDirectory) {
        this.joynrMessageFactory = joynrMessageFactory;
        this.messageSender = messageSender;
        this.messagingEndpointDirectory = messagingEndpointDirectory;
    }

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.dispatcher.MessageSender#sendRequest(java. lang.String, java.lang.String,
     * java.lang.Object, io.joynr.dispatcher.ReplyCaller, long, long)
     */

    @Override
    public void sendRequest(final String fromParticipantId,
                            final String toParticipantId,
                            final EndpointAddressBase endpointAddress,
                            Request request,
                            long ttl_ms) throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                        JsonGenerationException, JsonMappingException, IOException {

        logger.trace("SEND USING RequestReplySenderImpl with Id: " + System.identityHashCode(this));

        ExpiryDate expiryDate = DispatcherUtils.convertTtlToExpirationDate(ttl_ms);

        JoynrMessage message = joynrMessageFactory.createRequest(fromParticipantId,
                                                                 toParticipantId,
                                                                 request,
                                                                 expiryDate,
                                                                 messageSender.getReplyToChannelId());

        routeMessageByEndpointAddress(toParticipantId, message, endpointAddress);

    }

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.dispatcher.MessageSender#sendSyncRequest(java .lang.String, java.lang.String,
     * java.lang.Object, long, long)
     */

    @Override
    public Object sendSyncRequest(final String fromParticipantId,
                                  final String toParticipantId,
                                  final EndpointAddressBase endpointAddress,
                                  Request request,
                                  SynchronizedReplyCaller synchronizedReplyCaller,
                                  long ttl_ms) throws JoynrCommunicationException, JoynrSendBufferFullException,
                                              JoynrMessageNotSentException, JsonGenerationException,
                                              JsonMappingException, IOException {

        if (!running) {
            throw new IllegalStateException("Request: " + request.getRequestReplyId() + " failed. SenderImpl ID: "
                    + System.identityHashCode(this) + ": joynr is shutting down");
        }

        final ArrayList<Object> responsePayloadContainer = new ArrayList<Object>(1);
        // the synchronizedReplyCaller will call notify on the responsePayloadContainer when a message arrives
        synchronizedReplyCaller.setResponseContainer(responsePayloadContainer);

        sendRequest(fromParticipantId, toParticipantId, endpointAddress, request, ttl_ms);

        long entryTime = System.currentTimeMillis();

        // saving all calling threads so that they can be interrupted at shutdown
        outstandingRequestThreads.add(Thread.currentThread());
        synchronized (responsePayloadContainer) {
            while (running && responsePayloadContainer.isEmpty() && entryTime + ttl_ms > System.currentTimeMillis()) {
                try {
                    responsePayloadContainer.wait(ttl_ms);
                } catch (InterruptedException e) {
                    if (running) {
                        throw new JoynrRequestInterruptedException("Request: " + request.getRequestReplyId()
                                + " interrupted.");
                    }
                    throw new JoynrShutdownException("Request: " + request.getRequestReplyId()
                            + " interrupted by shutdown");

                }
            }
        }
        outstandingRequestThreads.remove(Thread.currentThread());

        if (responsePayloadContainer.isEmpty()) {
            throw new JoynrCommunicationException("Request: " + request.getRequestReplyId()
                    + " failed. The response didn't arrive in time");
        }

        Object response = responsePayloadContainer.get(0);
        if (response instanceof Throwable) {
            Throwable error = (Throwable) response;
            throw new JoynrMessageNotSentException("Request: " + request.getRequestReplyId() + " failed: "
                    + error.getMessage(), error);
        }

        return response;
    }

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.dispatcher.MessageSender#sendOneWay(java.lang .String, java.lang.String,
     * java.lang.Object, long)
     */

    @Override
    public void sendOneWay(final String fromParticipantId, final String toParticipantId, Object payload, long ttl_ms)
                                                                                                                     throws JoynrSendBufferFullException,
                                                                                                                     JoynrMessageNotSentException,
                                                                                                                     JsonGenerationException,
                                                                                                                     JsonMappingException,
                                                                                                                     IOException {
        JoynrMessage message = joynrMessageFactory.createOneWay(fromParticipantId,
                                                                toParticipantId,
                                                                payload,
                                                                DispatcherUtils.convertTtlToExpirationDate(ttl_ms));

        routeMessage(toParticipantId, message);

    }

    public void sendReply(final String fromParticipantId,
                          final String toParticipantId,
                          Reply payload,
                          ExpiryDate expiryDate) throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                                JsonGenerationException, JsonMappingException, IOException {
        JoynrMessage message = joynrMessageFactory.createReply(fromParticipantId, toParticipantId, payload, expiryDate);

        routeMessage(toParticipantId, message);
    }

    @Override
    public void sendSubscriptionRequest(String fromParticipantId,
                                        String toParticipantId,
                                        EndpointAddressBase endpointAddress,
                                        SubscriptionRequest subscriptionRequest,
                                        MessagingQos qosSettings) throws JoynrSendBufferFullException,
                                                                 JoynrMessageNotSentException, JsonGenerationException,
                                                                 JsonMappingException, IOException {
        JoynrMessage message = joynrMessageFactory.createSubscriptionRequest(fromParticipantId,
                                                                             toParticipantId,
                                                                             subscriptionRequest,
                                                                             messageSender.getReplyToChannelId(),
                                                                             DispatcherUtils.convertTtlToExpirationDate(qosSettings.getRoundTripTtl_ms()));

        routeMessageByEndpointAddress(toParticipantId, message, endpointAddress);
    }

    @Override
    public void sendSubscriptionPublication(String fromParticipantId,
                                            String toParticipantId,
                                            SubscriptionPublication publication,
                                            MessagingQos messagingQos) throws JoynrSendBufferFullException,
                                                                      JoynrMessageNotSentException,
                                                                      JsonGenerationException, JsonMappingException,
                                                                      IOException {
        JoynrMessage message = joynrMessageFactory.createPublication(fromParticipantId,
                                                                     toParticipantId,
                                                                     publication,
                                                                     DispatcherUtils.convertTtlToExpirationDate(messagingQos.getRoundTripTtl_ms()));
        routeMessage(toParticipantId, message);

    }

    @Override
    public void sendSubscriptionStop(String fromParticipantId,
                                     String toParticipantId,
                                     EndpointAddressBase endpointAddress,
                                     SubscriptionStop subscriptionStop,
                                     MessagingQos messagingQos) throws JoynrSendBufferFullException,
                                                               JoynrMessageNotSentException, JsonGenerationException,
                                                               JsonMappingException, IOException {
        JoynrMessage message = joynrMessageFactory.createSubscriptionStop(fromParticipantId,
                                                                          toParticipantId,
                                                                          subscriptionStop,
                                                                          DispatcherUtils.convertTtlToExpirationDate(messagingQos.getRoundTripTtl_ms()));
        routeMessageByEndpointAddress(toParticipantId, message, endpointAddress);

    }

    private void routeMessage(final String toParticipantId, JoynrMessage message) throws JsonGenerationException,
                                                                                 JsonMappingException,
                                                                                 JoynrCommunicationException,
                                                                                 IOException {
        if (messagingEndpointDirectory.containsKey(toParticipantId)) {
            EndpointAddressBase endpointAddress = messagingEndpointDirectory.get(toParticipantId);
            routeMessageByEndpointAddress(toParticipantId, message, endpointAddress);
        } else {
            throw new JoynrCommunicationException("Failed to send Request: No route for given participantId: "
                    + toParticipantId);
        }
    }

    private void routeMessageByEndpointAddress(final String toParticipantId,
                                               JoynrMessage message,
                                               EndpointAddressBase endpointAddress)
                                                                                   throws JoynrSendBufferFullException,
                                                                                   JoynrMessageNotSentException,
                                                                                   JsonGenerationException,
                                                                                   JsonMappingException, IOException {

        if (endpointAddress instanceof JoynrMessagingEndpointAddress) {
            logger.info("SEND  messageId: {} type: {} from: {} to: {} header: {}",
                        new String[]{ message.getId(), message.getType(),
                                message.getHeaderValue(JoynrMessage.HEADER_NAME_FROM_PARTICIPANT_ID),
                                message.getHeaderValue(JoynrMessage.HEADER_NAME_TO_PARTICIPANT_ID),
                                message.getHeader().toString() });
            logger.debug("\r\n>>>>>>>>>>>>>>>>\r\n:{}", message.toLogMessage());

            String destinationChannelId = ((JoynrMessagingEndpointAddress) endpointAddress).getChannelId();
            messageSender.sendMessage(destinationChannelId, message);
        } else {
            throw new JoynrCommunicationException("Failed to send Request: EndpointAddress type not supported");
        }
    }

    @Override
    public void registerEndpointAddress(String participantId, EndpointAddressBase endpointAddress) {
        messagingEndpointDirectory.put(participantId, endpointAddress);
    }

    @Override
    public void shutdown() {
        running = false;
        synchronized (outstandingRequestThreads) {
            for (Thread thread : outstandingRequestThreads) {
                logger.debug("shutting down. Interrupting thread: " + thread);
                thread.interrupt();
            }
        }
        messageSender.shutdown();
    }
}
