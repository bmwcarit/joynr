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

import io.joynr.common.ExpiryDate;
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
     * In order to send stateless async request, use overload of this method which takes
     * boolean isStatelessAsync as an argument.
     *
     * @param fromParticipantId
     *            ParticipantId of the sending endpoint.
     * @param toDiscoveryEntry
     *            DiscoveryEntry of the endpoint to send to
     * @param request
     *            Request to be sent
     * @param qosSettings MessagingQos for the request
     */
    default void sendRequest(final String fromParticipantId,
                             final DiscoveryEntryWithMetaInfo toDiscoveryEntry,
                             final Request request,
                             final MessagingQos qosSettings) {
        final ExpiryDate expiryDate = ExpiryDate.fromRelativeTtl(qosSettings.getRoundTripTtl_ms());
        sendRequest(fromParticipantId, toDiscoveryEntry, request, qosSettings, expiryDate);
    }

    /**
     * Sends a request, the reply message is passed to the specified callBack in a different thread.
     * In order to send stateless async request, use overload of this method which takes
     * boolean isStatelessAsync as an argument.
     *
     * @param fromParticipantId
     *            ParticipantId of the sending endpoint.
     * @param toDiscoveryEntry
     *            DiscoveryEntry of the endpoint to send to
     * @param request
     *            Request to be sent
     * @param qosSettings MessagingQos for the request
     * @param expiryDate message expiry date
     */
    void sendRequest(final String fromParticipantId,
                     final DiscoveryEntryWithMetaInfo toDiscoveryEntry,
                     final Request request,
                     final MessagingQos qosSettings,
                     final ExpiryDate expiryDate);

    /**
     * Sends a request, the reply message is passed to the specified callBack in a different thread.
     *
     * @param fromParticipantId
     *            ParticipantId of the sending endpoint.
     * @param toDiscoveryEntry
     *            DiscoveryEntry of the endpoint to send to
     * @param request
     *            Request to be sent
     * @param qosSettings
     *            MessagingQos for the request
     * @param isStatelessAsync
     *            Whether a request is stateless async
     */

    default void sendRequest(final String fromParticipantId,
                             final DiscoveryEntryWithMetaInfo toDiscoveryEntry,
                             final Request request,
                             final MessagingQos qosSettings,
                             final boolean isStatelessAsync) {
        final ExpiryDate expiryDate = ExpiryDate.fromRelativeTtl(qosSettings.getRoundTripTtl_ms());
        sendRequest(fromParticipantId, toDiscoveryEntry, request, qosSettings, isStatelessAsync, expiryDate);
    }

    /**
     * Sends a request, the reply message is passed to the specified callBack in a different thread.
     *
     * @param fromParticipantId
     *            ParticipantId of the sending endpoint.
     * @param toDiscoveryEntry
     *            DiscoveryEntry of the endpoint to send to
     * @param request
     *            Request to be sent
     * @param qosSettings
     *            MessagingQos for the request
     * @param isStatelessAsync
     *            Whether a request is stateless async
     * @param expiryDate
     *            message expiry date
     */
    void sendRequest(final String fromParticipantId,
                     final DiscoveryEntryWithMetaInfo toDiscoveryEntry,
                     final Request request,
                     final MessagingQos qosSettings,
                     final boolean isStatelessAsync,
                     final ExpiryDate expiryDate);

    /**
     * Sends a request and blocks the current thread until the response is received or the roundTripTtl is reached. If
     * an error occurs or no response arrives in time an JoynrCommunicationException is thrown.
     *
     * @param fromParticipantId
     *            ParticipantId of the sending endpoint.
     * @param toDiscoveryEntry
     *            DiscoveryEntry of the endpoint to send to
     * @param request
     *            Request to be sent
     * @param synchronizedReplyCaller
     *            Synchronized reply caller
     * @param qosSettings MessagingQos for the request
     * @param expiryDate message expiry date
     * @return response object
     */

    Reply sendSyncRequest(final String fromParticipantId,
                          final DiscoveryEntryWithMetaInfo toDiscoveryEntry,
                          final Request request,
                          final SynchronizedReplyCaller synchronizedReplyCaller,
                          final MessagingQos qosSettings,
                          final ExpiryDate expiryDate);

    /**
     * Send a one way message.
     *
     * @param fromParticipantId
     *            ParticipantId of the endpoint to send to
     * @param toDiscoveryEntries
     *            DiscoveryEntries of the endpoints to send to
     * @param oneWayRequest
     *            The request data tto send to the endpoints
     * @param messagingQos MessagingQos for the request
     */

    void sendOneWayRequest(final String fromParticipantId,
                           final Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                           OneWayRequest oneWayRequest,
                           MessagingQos messagingQos);

    void handleReply(Reply reply);

    void handleRequest(ProviderCallback<Reply> replyCallback,
                       String providerParticipant,
                       Request request,
                       long expiryDate);

    void handleOneWayRequest(String providerParticipantId, OneWayRequest request, long expiryDate);

    void shutdown();
}
