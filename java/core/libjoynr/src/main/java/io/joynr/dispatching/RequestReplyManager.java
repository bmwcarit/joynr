/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2023 BMW Car IT GmbH
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

import java.util.Set;

import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.messaging.MessagingQos;
import io.joynr.provider.ProviderCallback;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.types.DiscoveryEntryWithMetaInfo;

public interface RequestReplyManager {

    /**
     * Sends a request, the reply message is passed to the specified callBack in a different thread.
     *
     * @param fromParticipantId
     *            ParticipantId of the sending endpoint.
     * @param toDiscoveryEntry
     *            DiscoveryEntry of the endpoint to send to
     * @param request
     *            Request to be send
     * @param qosSettings MessagingQos for the request
     */

    public void sendRequest(final String fromParticipantId,
                            final DiscoveryEntryWithMetaInfo toDiscoveryEntry,
                            Request request,
                            MessagingQos qosSettings);

    /**
     * Sends a request and blocks the current thread until the response is received or the roundTripTtl is reached. If
     * an error occures or no response arrives in time an JoynCommunicationException is thrown.
     *
     * @param fromParticipantId
     *            ParticipantId of the sending endpoint.
     * @param toDiscoveryEntry
     *            DiscoveryEntry of the endpoint to send to
     * @param request
     *            Request to be send
     * @param synchronizedReplyCaller
     *            Synchronized reply caller
     * @param qosSettings MessagingQos for the request
     * @return response object
     */

    public Reply sendSyncRequest(final String fromParticipantId,
                                 final DiscoveryEntryWithMetaInfo toDiscoveryEntry,
                                 Request request,
                                 SynchronizedReplyCaller synchronizedReplyCaller,
                                 MessagingQos qosSettings);

    /**
     * Send a one way message.
     *
     * @param fromParticipantId
     *            ParticipantId of the endpoint to send to
     * @param toDiscoveryEntries
     *            DiscveryEntries of the endpoints to send to
     * @param oneWayRequest
     *            The request data tto send to the endpoints
     * @param messagingQos MessagingQos for the request
     */

    public void sendOneWayRequest(final String fromParticipantId,
                                  final Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                                  OneWayRequest oneWayRequest,
                                  MessagingQos messagingQos);

    public void handleReply(Reply reply);

    public void handleRequest(ProviderCallback<Reply> replyCallback,
                              String providerParticipant,
                              Request request,
                              long expiryDate);

    public void handleOneWayRequest(String providerParticipantId, OneWayRequest request, long expiryDate);

    public void shutdown();
}
