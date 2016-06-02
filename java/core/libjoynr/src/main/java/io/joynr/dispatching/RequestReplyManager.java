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

import java.io.IOException;
import java.util.Set;

import io.joynr.common.ExpiryDate;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.provider.ProviderCallback;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;

public interface RequestReplyManager {

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

    public void sendRequest(final String fromParticipantId, final String toParticipantId, Request request, long ttl_ms)
                                                                                                                       throws IOException;

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

    public Object sendSyncRequest(final String fromParticipantId,
                                  final String toParticipantId,
                                  Request request,
                                  SynchronizedReplyCaller synchronizedReplyCaller,
                                  long ttl_ms) throws IOException;

    /**
     * Send a one way message.
     *
     * @param fromParticipantId
     *            ParticipantId of the endpoint to send to
     * @param toParticipantIds
     *            ParticipantIds of the endpoints to send to
     * @param oneWayRequest
     *            The request data tto send to the endpoints
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

    public void sendOneWayRequest(final String fromParticipantId,
                                  final Set<String> toParticipantIds,
                                  OneWayRequest oneWayRequest,
                                  long ttl_ms) throws IOException;

    public void sendReply(final String fromParticipantId, final String toParticipantId, Reply payload, ExpiryDate ttl_ms)
                                                                                                                         throws IOException;

    public void handleReply(Reply reply);

    public void handleRequest(ProviderCallback<Reply> replyCallback,
                              String providerParticipant,
                              Request request,
                              long expiryDate);

    public void handleOneWayRequest(String providerParticipantId, OneWayRequest request, long expiryDate);

    public void handleError(Request request, Throwable error);

    public void shutdown();
}
