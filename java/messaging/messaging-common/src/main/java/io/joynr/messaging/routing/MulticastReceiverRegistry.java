package io.joynr.messaging.routing;

/*
 * #%L
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

import java.util.Map;
import java.util.Set;

/**
 * The multicast receiver registry is responsible for maintaining a mapping between
 * multicast IDs and the participant IDs of interested receivers.
 */
public interface MulticastReceiverRegistry {
    /**
     * Add an interested receiver's participant ID to the set of those interested for the given
     * multicast ID. Calling this method repeatedly after the first invocation with the same
     * values has no effect.
     *
     * @param multicastId the ID of the multicast the receiver is interested in.
     * @param participantId the participant ID of the receiver interested in the multicast.
     */
    void registerMulticastReceiver(String multicastId, String participantId);

    /**
     * This method is the reverse operation of {@link #registerMulticastReceiver(String, String)} and will
     * remove an interested participant from the set of those interested in the given multicast.
     *
     * @param multicastId the ID of the multicast the participant is no longer interested in.
     * @param participantId the ID of the participant wanting to remove themselves from the set of interested receiers.
     */
    void unregisterMulticastReceiver(String multicastId, String participantId);

    /**
     * Obtains the current set of participant IDs of the those receivers interested in the given multicast.
     *
     * @param multicastId the ID of the multicast for which to obtain the set of interested receivers.
     *
     * @return the set of participant IDs interested in receiving the given multicast. Never null - if there are no
     * interested receivers, an emtpy set is returned.
     */
    Set<String> getReceivers(String multicastId);

    /**
     * Obtain a map representing all currently registered receivers.
     *
     * @return a map keyed by multicast ID and valued with sets of participant IDs interested in the relevant multicast.
     */
    Map<String, Set<String>> getReceivers();
}
