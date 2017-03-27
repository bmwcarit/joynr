package io.joynr.dispatching.subscription;

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

import java.io.IOException;
import java.util.List;

import io.joynr.dispatching.subscription.PublicationManagerImpl.PublicationInformation;
import io.joynr.pubsub.publication.BroadcastFilter;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;

public interface PublicationManager {

    /**
     * Adds SubscriptionRequest when the Provider is not yet registered and there is no RequestCaller as yet.
     *
     * @param fromParticipantId origin participant id
     * @param toParticipantId destination participant id
     * @param subscriptionRequest request to be added
     */
    void addSubscriptionRequest(String fromParticipantId,
                                String toParticipantId,
                                SubscriptionRequest subscriptionRequest);

    /**
     * Stops the sending of publications
     *
     * @param subscriptionId id of subscription ot be stopped
     */
    void stopPublication(final String subscriptionId);

    /**
     * Call passed through from ProviderListener when an attribute on the provider is changed.
     * @param subscriptionId subscription id
     * @param value value
     */
    void attributeValueChanged(String subscriptionId, Object value);

    void broadcastOccurred(String subscriptionId, List<BroadcastFilter> filters, Object... values);

    void multicastOccurred(String providerParticipantId, String multicastName, String[] partitions, Object... values);

    void sendSubscriptionPublication(SubscriptionPublication publication, PublicationInformation information)
                                                                                                             throws IOException;

    void shutdown();

}
