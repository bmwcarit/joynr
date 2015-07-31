package io.joynr.pubsub.subscription;

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

import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;

import javax.annotation.CheckForNull;

public interface SubscriptionManager {

    void registerAttributeSubscription(AttributeSubscribeInvocation subscriptionRequest);

    void registerBroadcastSubscription(BroadcastSubscribeInvocation subscriptionRequest);

    void unregisterSubscription(final String subscriptionId);

    void touchSubscriptionState(final String subscriptionId);

    Class<?> getAttributeType(String subscriptionId);

    Class<?>[] getBroadcastOutParameterTypes(String subscriptionId);

    boolean isBroadcast(String subscriptionId);

    BroadcastSubscriptionListener getBroadcastSubscriptionListener(String subscriptionId);

    @CheckForNull
    <T> AttributeSubscriptionListener<T> getSubscriptionListener(String subscriptionId);
}
