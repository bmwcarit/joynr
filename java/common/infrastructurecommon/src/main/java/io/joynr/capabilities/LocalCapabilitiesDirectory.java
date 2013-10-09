package io.joynr.capabilities;

/*
 * #%L
 * joynr::java::common::infrastructurecommon
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

import io.joynr.arbitration.DiscoveryQos;

import java.util.Collection;

import joynr.types.ProviderQosRequirements;

public interface LocalCapabilitiesDirectory {
    /**
     * Adds a capability to the list of registered local capabilities. May also transmit the updated list to the
     * capabilities directory.
     * 
     * @return
     */
    RegistrationFuture addCapability(CapabilityEntry capabilityEntry);

    /**
     * Removes capabilities from the list of local capabilities and at the capabilities directory.
     * 
     * @param interfaces
     */
    void removeCapability(CapabilityEntry capability);

    /**
     * Searches for capabilities by domain and interface name.
     * 
     * @param domain
     * @param interfaceName
     * @param providerQos
     * @param maxAgeOfCachedProviders
     * @return
     */
    Collection<CapabilityEntry> getCapabilities(final String domain,
                                                final String interfaceName,
                                                ProviderQosRequirements requestedQos,
                                                DiscoveryQos discoveryQos);

    /**
     * Searches for capabilities by participantId.
     * 
     * @param participantId
     * @param maxAgeOfCachedProviders
     * @return
     */
    Collection<CapabilityEntry> getCapabilities(String participantId, DiscoveryQos discoveryQos);

    void getCapabilities(String domain,
                         String interfaceName,
                         ProviderQosRequirements requestedQos,
                         DiscoveryQos discoveryQos,
                         CapabilitiesCallback capabilitiesCallback);

    void getCapabilities(String participantId, DiscoveryQos discoveryQos, CapabilitiesCallback callback);

    /**
     * Shuts down the local capabilities directory and all used thread pools.
     * @param unregisterAllRegisteredCapabilities if set to true, all added capabilities that are not removed up to
     * this point, will be removed automatically 
     */
    void shutdown(boolean unregisterAllRegisteredCapabilities);

}
