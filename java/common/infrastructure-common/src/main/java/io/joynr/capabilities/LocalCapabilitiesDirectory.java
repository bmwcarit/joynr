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
package io.joynr.capabilities;

import java.util.Set;

import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.runtime.ShutdownListener;
import joynr.system.DiscoveryProvider;
import joynr.types.DiscoveryEntry;

public interface LocalCapabilitiesDirectory extends DiscoveryProvider, ShutdownListener {
    public static final String JOYNR_SCHEDULER_CAPABILITIES_FRESHNESS = "joynr.scheduler.capabilities.freshness";

    /**
     * Adds a capability to the list of registered local capabilities. May also transmit the updated list to the
     * capabilities directory.
     *
     * @param discoveryEntry The capability to be added.
     * @return future to get the async result of the call
     */
    @Override
    Promise<DeferredVoid> add(DiscoveryEntry discoveryEntry);

    /**
     * Adds a capability to the list of registered local capabilities. May also transmit the updated list to the
     * capabilities directory.
     *
     * @param discoveryEntry The capability to be added.
     * @param awaitGlobalRegistration True, if add should wait for global registration to complete
     * @return future to get the async result of the call
     */
    @Override
    Promise<DeferredVoid> add(DiscoveryEntry discoveryEntry, Boolean awaitGlobalRegistration);

    /**
     * Adds a listener for capability changes to the directory.
     * @param listener the listener to add.
     */
    void addCapabilityListener(CapabilityListener listener);

    /**
     * Removes a listener for capability changes from the directory.
     * @param listener the listener to remove.
     */
    void removeCapabilityListener(CapabilityListener listener);

    /**
     *
     * @return a set of all capabilities registered with this local directory
     */
    Set<DiscoveryEntry> listLocalCapabilities();

    /**
     * Shuts down the local capabilities directory and all used thread pools.
     * @param unregisterAllRegisteredCapabilities if set to true, all added capabilities that are not removed up to
     * this point, will be removed automatically
     */
    void shutdown(boolean unregisterAllRegisteredCapabilities);

    /**
     * Removes stale providers of the cluster controller 
     */
    void removeStaleProvidersOfClusterController();

}
