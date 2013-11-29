package io.joynr.pubsub.subscription;

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

import io.joynr.pubsub.SubscriptionQos;

import javax.annotation.CheckForNull;

import com.fasterxml.jackson.core.type.TypeReference;

public interface SubscriptionManager {

    String registerAttributeSubscription(final String attributeName,
                                         Class<? extends TypeReference<?>> attributeTypeReference,
                                         SubscriptionListener<?> attributeSubscriptionCallback,
                                         final SubscriptionQos qos);

    void unregisterAttributeSubscription(final String subscriptionId);

    void touchSubscriptionState(final String subscriptionId);

    @CheckForNull
    SubscriptionListener<?> getSubscriptionListener(final String subscriptionId);

    Class<? extends TypeReference<?>> getAttributeTypeReference(String subscriptionId);

    void shutdown();
}
