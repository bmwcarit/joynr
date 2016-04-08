package io.joynr.capabilities;

/*
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

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.system.RoutingTypes.Address;
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

    public static DiscoveryEntry globalDiscoveryEntry2DiscoveryEntry(GlobalDiscoveryEntry globalDiscoveryEntry) {
        return new DiscoveryEntry(globalDiscoveryEntry.getProviderVersion(),
                                  globalDiscoveryEntry.getDomain(),
                                  globalDiscoveryEntry.getInterfaceName(),
                                  globalDiscoveryEntry.getParticipantId(),
                                  globalDiscoveryEntry.getQos(),
                                  System.currentTimeMillis());
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
}
