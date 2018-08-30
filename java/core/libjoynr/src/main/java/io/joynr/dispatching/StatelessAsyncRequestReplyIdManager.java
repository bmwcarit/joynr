/*-
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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

import joynr.Reply;
import joynr.Request;

public interface StatelessAsyncRequestReplyIdManager {

    /**
     * This method is called just before sending out a stateless async request in order to give the request reply ID
     * manager a chance to store any relevant data in a distributed database, for example.
     *
     * Implementations should make sure to harvest and persist any information from the request necessary to later be
     * able to correlate the reply passed into {@link #getCallbackId(Reply)} to the request that went out. For
     * example, it could store the stateless async callback ID matched to the request reply ID in a distributed
     * key/value store, so that the getCallbackId method can then query that store using the requestReplyId from
     * the incoming {@link Reply} in order to get the stateless async callback ID from any of the nodes in a cluster.
     *
     * @param request the request from which to harvest the necessary information
     */
    void register(Request request);

    /**
     * Called when a {@link Reply} has arrived and the {@link RequestReplyManager} needs to know which callback to
     * route it to.
     *
     * See also {@link #register(Request)}.
     *
     * @param reply the incoming reply for which the {@link RequestReplyManager} needs to know the callback ID to use.
     * @return the callback ID to use for the incoming reply. If not a stateless async reply, this must be the
     * {@link Reply#getRequestReplyId()}, otherwise the relevant stateless async callback ID.
     */
    String getCallbackId(Reply reply);

}
