package io.joynr.capabilities;

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

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.system.DiscoveryProvider;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;

import java.util.Set;

import javax.annotation.CheckForNull;

public interface LocalCapabilitiesDirectory extends DiscoveryProvider {
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
     * Removes capabilities from the list of local capabilities and at the capabilities directory.
     *
     * @param discoveryEntry entry to remove
     */
    void remove(DiscoveryEntry discoveryEntry);

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
     * Searches for capabilities by domain and interface name.
     *
     * @param domains The Domains for which the search is to be done.
     * @param interfaceName The interface for which the search is to be done.
     * @param discoveryQos The discovery quality of service for the search.
     * @param capabilitiesCallback Callback to deliver the results asynchronously.
     */
    void lookup(String[] domains,
                String interfaceName,
                DiscoveryQos discoveryQos,
                CapabilitiesCallback capabilitiesCallback);

    /**
     * Searches for capability by participantId. This is an asynchronous method.
     *
     * @param participantId The participant id to search for.
     * @param discoveryQos The discovery quality of service for the search.
     * @param callback called if the capability with the given participant ID
     *      is retrieved. Or null if not found.
     */
    @CheckForNull
    void lookup(String participantId, DiscoveryQos discoveryQos, CapabilityCallback callback);

    /**
     * Searches for capability by participantId.
     *
     * @param participantId The participant id to search for.
     * @param discoveryQos The discovery quality of service for the search.
     * @return the capability with the given participant ID. Or null if not found.
     */
    @CheckForNull
    DiscoveryEntryWithMetaInfo lookup(String participantId, DiscoveryQos discoveryQos);

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

}
