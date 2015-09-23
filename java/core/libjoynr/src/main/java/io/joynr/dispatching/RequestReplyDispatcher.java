package io.joynr.dispatching;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import io.joynr.proxy.Callback;
import joynr.OneWay;
import joynr.Reply;
import joynr.Request;

/**
 * The dispatcher sits on top of the HTTP Communication Manager and offers its users to send one-way messages or
 * request-reply messages. The dispatcher manages the construction of the message along with its appropriate headers
 * before calling the HTTP Communications Manager. It is responsible for managing the logic behind call-backs,
 * retrieving responses, sending replies, and delivering one-way messages to the appropriate listener.
 */
public interface RequestReplyDispatcher {

    /**
     * Removes the listener registered for the interface address.
     * @param participantId participant id
     */
    public void removeListener(final String participantId);

    public void shutdown();

    void addOneWayRecipient(String participantId, PayloadListener<?> listener);

    public void handleReply(Reply reply);

    public void handleRequest(Callback<Reply> replyCallback,
                              String providerParticipant,
                              Request request,
                              long expiryDate);

    public void handleOneWayRequest(String providerParticipantId, OneWay request, long expiryDate);

    public void handleError(Request request, Throwable error);
}
