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
package io.joynr.dispatching.subscription;

import io.joynr.util.MultiMap;
import joynr.SubscriptionRequest;

/**
 * Stores SubscriptionRequests and related participantIds so that subscriptionRequests
 * can be persisted and reactivated on provider restart.
 */
public interface SubscriptionRequestStorage {

    /**
     * Maps providerParticipantId to all its active subscriptionRequests
     * @return saved subscriptionRequests
     */
    MultiMap<String, PersistedSubscriptionRequest> getSavedSubscriptionRequests();

    /**
     * Removes the subscriptionRequest from persistence
     * @param providerId provider participant id
     * @param subscriptionRequest subscriptionRequest to be removed
     */
    void removeSubscriptionRequest(String providerId, PersistedSubscriptionRequest subscriptionRequest);

    /**
     * Persists the subscriptionRequest so that the subscription can be reactivated on provider restart
     * @param proxyParticipantId proxy participant id
     * @param providerParticipantId provider participant id
     * @param subscriptionRequest subscriptionRequest to persist
     */
    void persistSubscriptionRequest(String proxyParticipantId,
                                    String providerParticipantId,
                                    SubscriptionRequest subscriptionRequest);
}
