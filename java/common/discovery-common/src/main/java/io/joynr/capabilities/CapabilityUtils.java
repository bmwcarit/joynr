package io.joynr.capabilities;

import java.io.IOException; /*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
import java.util.Collection;
import java.util.List;
import javax.annotation.CheckForNull;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.collect.Lists;
import com.google.inject.Inject;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;
import joynr.types.DiscoveryEntry;

/**
 * Conversion helpers for CapabilityInformation, CapabilityEntry and DiscoveryEntry
 */
public class CapabilityUtils {

    @Inject
    private static ObjectMapper objectMapper;

    public static DiscoveryEntry capabilityEntry2DiscoveryEntry(CapabilityEntry capabilityEntry) {
        return new DiscoveryEntry(capabilityEntry.getProviderVersion(),
                                  capabilityEntry.getDomain(),
                                  capabilityEntry.getInterfaceName(),
                                  capabilityEntry.getParticipantId(),
                                  capabilityEntry.getProviderQos(),
                                  System.currentTimeMillis());
    }

    @CheckForNull
    public static GlobalDiscoveryEntry capabilityEntry2GlobalDiscoveryEntry(CapabilityEntry capabilityEntry) {
        String address = null;
        if (capabilityEntry.getAddresses() != null && !capabilityEntry.getAddresses().isEmpty()) {
            try {
                address = objectMapper.writeValueAsString(capabilityEntry.getAddresses().get(0));
            } catch (JsonProcessingException e) {
                throw new JoynrRuntimeException(e);
            }
        }

        return new GlobalDiscoveryEntry(capabilityEntry.getProviderVersion(),
                                        capabilityEntry.getDomain(),
                                        capabilityEntry.getInterfaceName(),
                                        capabilityEntry.getParticipantId(),
                                        capabilityEntry.getProviderQos(),
                                        System.currentTimeMillis(),
                                        address);
    }

    public static DiscoveryEntry globalDiscoveryEntry2DiscoveryEntry(GlobalDiscoveryEntry globalDiscoveryEntry) {
        return new DiscoveryEntry(globalDiscoveryEntry.getProviderVersion(),
                                  globalDiscoveryEntry.getDomain(),
                                  globalDiscoveryEntry.getInterfaceName(),
                                  globalDiscoveryEntry.getParticipantId(),
                                  globalDiscoveryEntry.getQos(),
                                  System.currentTimeMillis());
    }

    public static CapabilityEntry globalDiscoveryEntry2CapabilityEntry(GlobalDiscoveryEntry globalDiscoveryEntry) {
        Address address;
        try {
            address = objectMapper.readValue(globalDiscoveryEntry.getAddress(), Address.class);
        } catch (IOException e) {
            throw new JoynrRuntimeException(e);
        }

        return new CapabilityEntryImpl(globalDiscoveryEntry.getProviderVersion(),
                                       globalDiscoveryEntry.getDomain(),
                                       globalDiscoveryEntry.getInterfaceName(),
                                       globalDiscoveryEntry.getQos(),
                                       globalDiscoveryEntry.getParticipantId(),
                                       System.currentTimeMillis(),
                                       address);
    }

    public static Collection<CapabilityEntry> globalDiscoveryEntryList2Entries(List<GlobalDiscoveryEntry> globalDiscoveryEntries) {
        Collection<CapabilityEntry> capEntryCollection = Lists.newArrayList();
        for (GlobalDiscoveryEntry globalDiscoveryEntry : globalDiscoveryEntries) {
            capEntryCollection.add(globalDiscoveryEntry2CapabilityEntry(globalDiscoveryEntry));
        }
        return capEntryCollection;
    }

    public static GlobalDiscoveryEntry newGlobalDiscoveryEntry(String domain,
                                                               String interfaceName,
                                                               String participantId,
                                                               ProviderQos qos,
                                                               Long lastSeenDateMs,
                                                               Address address) {
        return newGlobalDiscoveryEntry(new Version(),
                                       domain,
                                       interfaceName,
                                       participantId,
                                       qos,
                                       lastSeenDateMs,
                                       address);
    }

    public static GlobalDiscoveryEntry newGlobalDiscoveryEntry(Version providerVesion,
                                                               String domain,
                                                               String interfaceName,
                                                               String participantId,
                                                               ProviderQos qos,
                                                               Long lastSeenDateMs,
                                                               Address address) {
        return new GlobalDiscoveryEntry(providerVesion,
                                        domain,
                                        interfaceName,
                                        participantId,
                                        qos,
                                        lastSeenDateMs,
                                        serializeAddress(address));
    }

    public static GlobalDiscoveryEntry discoveryEntry2GlobalDiscoveryEntry(DiscoveryEntry discoveryEntry,
                                                                           Address globalAddress) {

        return new GlobalDiscoveryEntry(discoveryEntry.getProviderVersion(),
                                        discoveryEntry.getDomain(),
                                        discoveryEntry.getInterfaceName(),
                                        discoveryEntry.getParticipantId(),
                                        discoveryEntry.getQos(),
                                        System.currentTimeMillis(),
                                        serializeAddress(globalAddress));
    }

    private static String serializeAddress(Address globalAddress) {
        String serializedAddress;
        try {
            serializedAddress = objectMapper.writeValueAsString(globalAddress);
        } catch (JsonProcessingException e) {
            throw new JoynrRuntimeException(e);
        }
        return serializedAddress;
    }

    public static CapabilityEntry discoveryEntry2CapEntry(DiscoveryEntry discoveryEntry, String globalAddressString) {
        return discoveryEntry2CapEntry(discoveryEntry, RoutingTypesUtil.fromAddressString(globalAddressString));
    }

    public static CapabilityEntry discoveryEntry2CapEntry(DiscoveryEntry discoveryEntry, Address globalAddress) {
        return new CapabilityEntryImpl(discoveryEntry.getProviderVersion(),
                                       discoveryEntry.getDomain(),
                                       discoveryEntry.getInterfaceName(),
                                       discoveryEntry.getQos(),
                                       discoveryEntry.getParticipantId(),
                                       System.currentTimeMillis(),
                                       globalAddress);
    }

    public static Collection<DiscoveryEntry> capabilityEntries2DiscoveryEntries(Collection<CapabilityEntry> capEntryList) {
        Collection<DiscoveryEntry> discoveryEntries = Lists.newArrayList();
        for (CapabilityEntry capabilityEntry : capEntryList) {
            discoveryEntries.add(capabilityEntry2DiscoveryEntry(capabilityEntry));
        }

        return discoveryEntries;
    }
}
