package io.joynr.capabilities;

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

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.endpoints.EndpointAddressBase;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;

import joynr.types.ProviderQosRequirements;

public interface CapabilitiesStore {

    public abstract void registerCapability(CapabilityEntry capabilityEntry);

    public abstract void registerCapabilities(Collection<? extends CapabilityEntry> interfaces);

    public abstract boolean removeCapability(CapabilityEntry capEntry);

    public abstract void removeCapabilities(Collection<? extends CapabilityEntry> interfaces);

    public abstract ArrayList<CapabilityEntry> findCapabilitiesForEndpointAddress(EndpointAddressBase endpoint,
                                                                                  DiscoveryQos discoveryQos);

    public abstract Collection<CapabilityEntry> findCapabilitiesForInterfaceAddress(String domain,
                                                                                    String interfaceName,
                                                                                    ProviderQosRequirements requestedQos,
                                                                                    DiscoveryQos discoveryQos);

    public abstract ArrayList<CapabilityEntry> findCapabilitiesForParticipantId(String participantId,
                                                                                DiscoveryQos discoveryQos);

    public abstract HashSet<CapabilityEntry> getAllCapabilities();

    public abstract boolean hasCapability(CapabilityEntry capabilityEntry);

}