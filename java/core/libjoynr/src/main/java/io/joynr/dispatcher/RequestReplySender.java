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

import io.joynr.common.ExpiryDate;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;

import java.io.IOException;

import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.system.routingtypes.Address;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;

public interface RequestReplySender {

    /**
     * Sends a request, the reply message is passed to the specified callBack in a different thread.
     *
     * @param fromParticipantId
     *            ParticipantId of the sending endpoint.
     * @param toParticipantId
     *            ParticipantId of the endpoint to send to
     * @param request
     *            Request to be send
     * @param ttl_ms
     *            Time to live in milliseconds.
     * @throws IOException
     *            in case of I/O error
     * @throws JsonMappingException
     *            in case JSON could not be mapped
     * @throws JsonGenerationException
     *            in case JSON could not be generated
     * @throws JoynrMessageNotSentException
     *            in case message could not be sent
     * @throws JoynrSendBufferFullException
     *            in case send buffer is full
     */

    public abstract void sendRequest(final String fromParticipantId,
                                     final String toParticipantId,
                                     Request request,
                                     long ttl_ms) throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                                 JsonGenerationException, JsonMappingException, IOException;

    /**
     * Sends a request and blocks the current thread until the response is received or the roundTripTtl is reached. If
     * an error occures or no response arrives in time an JoynCommunicationException is thrown.
     *
     * @param fromParticipantId
     *            ParticipantId of the sending endpoint.
     * @param toParticipantId
     *            ParticipantId of the endpoint to send to
     * @param request
     *            Request to be send
     * @param synchronizedReplyCaller
     *            Synchronized reply caller
     * @param ttl_ms
     *            Time to live in milliseconds.
     * @return response object
     * @throws IOException
     *            in case of I/O error
     * @throws JsonMappingException
     *            in case JSON could not be mapped
     * @throws JsonGenerationException
     *            in case JSON could not be generated
     * @throws JoynrMessageNotSentException
     *            in case message could not be sent
     * @throws JoynrSendBufferFullException
     *            in case send buffer is full
     */

    public abstract Object sendSyncRequest(final String fromParticipantId,
                                           final String toParticipantId,
                                           Request request,
                                           SynchronizedReplyCaller synchronizedReplyCaller,
                                           long ttl_ms) throws JoynrCommunicationException,
                                                       JoynrSendBufferFullException, JoynrMessageNotSentException,
                                                       JsonGenerationException, JsonMappingException, IOException;

    /**
     * Send a one way message.
     *
     * @param fromParticipantId
     *            ParticipantId of the endpoint to send to
     * @param toParticipantId
     *            ParticipantId of the sending endpoint.
     * @param payload
     *            Payload Object to send.
     * @param ttl_ms
     *            Time to live in milliseconds.
     * @throws IOException
     *            in case of I/O error
     * @throws JsonMappingException
     *            in case of mapping error
     * @throws JsonGenerationException
     *            in case JSON could not be generated
     * @throws JoynrMessageNotSentException
     *            in case message could not be sent
     * @throws JoynrSendBufferFullException
     *            in case send buffer is full
     */

    public abstract void sendOneWay(final String fromParticipantId,
                                    final String toParticipantId,
                                    Object payload,
                                    long ttl_ms) throws JoynrSendBufferFullException, JoynrMessageNotSentException,
                                                JsonGenerationException, JsonMappingException, IOException;

    public void sendReply(final String fromParticipantId, final String toParticipantId, Reply payload, ExpiryDate ttl_ms)
                                                                                                                         throws JoynrSendBufferFullException,
                                                                                                                         JoynrMessageNotSentException,
                                                                                                                         JsonGenerationException,
                                                                                                                         JsonMappingException,
                                                                                                                         IOException;

    public abstract void registerAddress(String participantId, Address address);

    public abstract void sendSubscriptionRequest(String fromParticipantId,
                                                 String toParticipantId,
                                                 SubscriptionRequest subscriptionRequest,
                                                 MessagingQos qosSettings,
                                                 boolean broadcast) throws JoynrSendBufferFullException,
                                                                   JoynrMessageNotSentException,
                                                                   JsonGenerationException, JsonMappingException,
                                                                   IOException;

    public abstract void sendSubscriptionPublication(String fromParticipantId,
                                                     String toParticipantId,
                                                     SubscriptionPublication publication,
                                                     MessagingQos messagingQos) throws JoynrSendBufferFullException,
                                                                               JoynrMessageNotSentException,
                                                                               JsonGenerationException,
                                                                               JsonMappingException, IOException;

    public abstract void sendSubscriptionStop(String fromParticipantId,
                                              String toParticipantId,
                                              SubscriptionStop subscriptionStop,
                                              MessagingQos qosSettings) throws JoynrSendBufferFullException,
                                                                       JoynrMessageNotSentException,
                                                                       JsonGenerationException, JsonMappingException,
                                                                       IOException;

    public void shutdown();
}
