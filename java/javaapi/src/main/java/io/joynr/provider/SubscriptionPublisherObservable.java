package io.joynr.provider;

/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

import io.joynr.pubsub.publication.AttributeListener;
import io.joynr.pubsub.publication.BroadcastListener;

public interface SubscriptionPublisherObservable {

    /**
     * Registers an attribute listener that gets notified in case the attribute
     * changes. This is used for on change subscriptions.
     *
     * @param attributeName the attribute name as defined in the Franca model
     *      to subscribe to.
     * @param attributeListener the listener to add.
     */
    public void registerAttributeListener(String attributeName, AttributeListener attributeListener);

    /**
     * Unregisters an attribute listener.
     *
     * @param attributeName the attribute name as defined in the Franca model
     *      to unsubscribe from.
     * @param attributeListener the listener to remove.
     */
    public void unregisterAttributeListener(String attributeName, AttributeListener attributeListener);

    /**
     * Registers a broadcast listener that gets notified in case the broadcast
     * is fired.
     *
     * @param broadcastName the broadcast name as defined in the Franca model
     *      to subscribe to.
     * @param broadcastListener the listener to add.
     */
    public void registerBroadcastListener(String broadcastName, BroadcastListener broadcastListener);

    /**
     * Unregisters a broadcast listener.
     *
     * @param broadcastName the broadcast name as defined in the Franca model
     *      to unsubscribe from.
     * @param broadcastListener the listener to remove.
     */
    public void unregisterBroadcastListener(String broadcastName, BroadcastListener broadcastListener);
}
