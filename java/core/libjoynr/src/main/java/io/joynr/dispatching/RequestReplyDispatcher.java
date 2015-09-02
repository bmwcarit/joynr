package io.joynr.dispatching;

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

import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.messaging.MessageArrivedListener;

/**
 * The dispatcher sits on top of the HTTP Communication Manager and offers its users to send one-way messages or
 * request-reply messages. The dispatcher manages the construction of the message along with its appropriate headers
 * before calling the HTTP Communications Manager. It is responsible for managing the logic behind call-backs,
 * retrieving responses, sending replies, and delivering one-way messages to the appropriate listener.
 */
public interface RequestReplyDispatcher extends MessageArrivedListener {

    public void addReplyCaller(final String requestReplyId, ReplyCaller replyCaller, long roundTripTtl_ms);

    public void removeReplyCaller(final String requestReplyId);

    public void addRequestCaller(final String participantId, RequestCaller requestCaller);

    public void removeRequestCaller(String participantId);

    /**
     * Removes the listener registered for the interface address.
     * @param participantId participant id
     */
    public void removeListener(final String participantId);

    /**
     * 
     * @param clear
     *            indicates whether the channel should be closed
     */
    public void shutdown(boolean clear);

    void addOneWayRecipient(String participantId, PayloadListener<?> listener);

}
