/*
 * #%L
 * joynr::java::messaging::messaging-service
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
package io.joynr.messaging.service;

/**
 * Interface for the implementation for messaging service.
 * 
 * @author christina.strobel
 * 
 */
public interface MessagingService {

    /**
     * Checks whether message receivers are registered for this channel.
     * 
     * @param ccid the channel id
     * @return <code>true</code> if there are message receivers on this channel,
     *         <code>false</code> if not.
     */
    boolean hasMessageReceiver(String ccid);

    /**
     * Passes the message to the registered message receiver.
     * 
     * @param ccid the channel to pass the message on
     * @param serializedMessage the message to send (serialized SMRF message)
     */
    void passMessageToReceiver(String ccid, byte[] serializedMessage);

    /**
     * Check whether the messaging component is responsible to send messages on
     * this channel.<br>
     * 
     * In scenarios with only one messaging component, this will always return
     * <code>true</code>. In scenarios in which channels are assigned to several
     * messaging components, only the component that the channel was assigned
     * to, returns <code>true</code>.
     * 
     * @param ccid
     *            the channel ID or cluster controller ID, respectively.
     * @return <code>true</code> if the messaging component is responsible for
     *         forwarding messages on this channel, <code>false</code>
     *         otherwise.
     */
    boolean isAssignedForChannel(String ccid);

    /**
     * Check whether the messaging component was responsible before but the
     * channel has been assigned to a new messaging component in the mean time.
     * 
     * @param ccid the channel id
     * @return <code>true</code> if the messaging component was responsible
     *         before but isn't any more, <code>false</code> if it either never
     *         was responsible or still is responsible.
     */
    boolean hasChannelAssignmentMoved(String ccid);
}
