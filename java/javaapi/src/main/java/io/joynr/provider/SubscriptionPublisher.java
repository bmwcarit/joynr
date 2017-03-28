package io.joynr.provider;

/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

import io.joynr.pubsub.publication.BroadcastFilterImpl;

public interface SubscriptionPublisher {

    /**
     * Adds a broadcast filter to the provider. The filter is specific for a
     * single broadcast as defined in the Franca model. It will be executed
     * once for each subscribed client whenever the broadcast is fired. Clients
     * set individual filter parameters to control filter behavior.
     *
     * @param filter the filter to add.
     */
    public void addBroadcastFilter(BroadcastFilterImpl filter);

    /**
     * Adds multiple broadcast filters to the provider.
     *
     * @see SubscriptionPublisher#addBroadcastFilter(BroadcastFilterImpl filter)
     *
     * @param filters the filters to add.
     */
    public void addBroadcastFilter(BroadcastFilterImpl... filters);
}
