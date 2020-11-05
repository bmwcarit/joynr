/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import static java.lang.String.format;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.google.inject.Inject;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.util.ObjectMapper;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

/**
 * Conversion helpers for CapabilityInformation, CapabilityEntry and DiscoveryEntry
 */
public class CapabilityUtils {

    private static final Logger logger = LoggerFactory.getLogger(CapabilityUtils.class);

    @Inject
    private static ObjectMapper objectMapper;

    public static DiscoveryEntry globalDiscoveryEntry2DiscoveryEntry(GlobalDiscoveryEntry globalDiscoveryEntry) {
        return new DiscoveryEntry(globalDiscoveryEntry.getProviderVersion(),
                                  globalDiscoveryEntry.getDomain(),
                                  globalDiscoveryEntry.getInterfaceName(),
                                  globalDiscoveryEntry.getParticipantId(),
                                  globalDiscoveryEntry.getQos(),
                                  globalDiscoveryEntry.getLastSeenDateMs(),
                                  globalDiscoveryEntry.getExpiryDateMs(),
                                  globalDiscoveryEntry.getPublicKeyId());
    }

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public static GlobalDiscoveryEntry newGlobalDiscoveryEntry(Version providerVesion,
                                                               String domain,
                                                               String interfaceName,
                                                               String participantId,
                                                               ProviderQos qos,
                                                               Long lastSeenDateMs,
                                                               Long expiryDateMs,
                                                               String publicKeyId,
                                                               Address address) {
        // CHECKSTYLE ON
        return new GlobalDiscoveryEntry(providerVesion,
                                        domain,
                                        interfaceName,
                                        participantId,
                                        qos,
                                        lastSeenDateMs,
                                        expiryDateMs,
                                        publicKeyId,
                                        serializeAddress(address));
    }

    public static GlobalDiscoveryEntry discoveryEntry2GlobalDiscoveryEntry(DiscoveryEntry discoveryEntry,
                                                                           Address globalAddress) {

        return new GlobalDiscoveryEntry(discoveryEntry.getProviderVersion(),
                                        discoveryEntry.getDomain(),
                                        discoveryEntry.getInterfaceName(),
                                        discoveryEntry.getParticipantId(),
                                        discoveryEntry.getQos(),
                                        discoveryEntry.getLastSeenDateMs(),
                                        discoveryEntry.getExpiryDateMs(),
                                        discoveryEntry.getPublicKeyId(),
                                        serializeAddress(globalAddress));
    }

    public static Address getAddressFromGlobalDiscoveryEntry(GlobalDiscoveryEntry globalDiscoveryEntry) {
        if (globalDiscoveryEntry == null || globalDiscoveryEntry.getAddress() == null) {
            throw new IllegalArgumentException("Neither globalDiscoveryEntry nor its address can be null.");
        }
        logger.trace("Attempting to deserialize {} as an Address.", globalDiscoveryEntry.getAddress());
        Address result;
        try {
            result = objectMapper.readValue(globalDiscoveryEntry.getAddress(), Address.class);
        } catch (IOException e) {
            throw new IllegalArgumentException(format("Global discovery entry address value %s cannot be deserialized as an Address.",
                                                      globalDiscoveryEntry.getAddress()),
                                               e);
        }
        return result;
    }

    public static String serializeAddress(Address globalAddress) {
        String serializedAddress;
        try {
            serializedAddress = objectMapper.writeValueAsString(globalAddress);
        } catch (JsonProcessingException e) {
            throw new JoynrRuntimeException(e);
        }
        return serializedAddress;
    }

    public static DiscoveryEntry convertToDiscoveryEntry(DiscoveryEntryWithMetaInfo entry) {
        return new DiscoveryEntry(entry);
    }

    public static List<DiscoveryEntry> convertToDiscoveryEntryList(Collection<DiscoveryEntryWithMetaInfo> entriesWithMetaInfo) {
        List<DiscoveryEntry> entries = new ArrayList<DiscoveryEntry>(entriesWithMetaInfo.size());
        for (DiscoveryEntryWithMetaInfo entry : entriesWithMetaInfo) {
            entries.add(convertToDiscoveryEntry(entry));
        }
        return entries;
    }

    public static Set<DiscoveryEntry> convertToDiscoveryEntrySet(Collection<DiscoveryEntryWithMetaInfo> entriesWithMetaInfo) {
        Set<DiscoveryEntry> entries = new HashSet<DiscoveryEntry>(entriesWithMetaInfo.size());
        for (DiscoveryEntryWithMetaInfo entry : entriesWithMetaInfo) {
            entries.add(convertToDiscoveryEntry(entry));
        }
        return entries;
    }

    public static DiscoveryEntryWithMetaInfo convertToDiscoveryEntryWithMetaInfo(boolean isLocal,
                                                                                 DiscoveryEntry entry) {
        return new DiscoveryEntryWithMetaInfo(entry.getProviderVersion(),
                                              entry.getDomain(),
                                              entry.getInterfaceName(),
                                              entry.getParticipantId(),
                                              entry.getQos(),
                                              entry.getLastSeenDateMs(),
                                              entry.getExpiryDateMs(),
                                              entry.getPublicKeyId(),
                                              isLocal);
    }

    public static List<DiscoveryEntryWithMetaInfo> convertToDiscoveryEntryWithMetaInfoList(boolean isLocal,
                                                                                           Collection<? extends DiscoveryEntry> entries) {
        List<DiscoveryEntryWithMetaInfo> entriesWithMetaInfo = new ArrayList<DiscoveryEntryWithMetaInfo>(entries.size());
        for (DiscoveryEntry entry : entries) {
            entriesWithMetaInfo.add(convertToDiscoveryEntryWithMetaInfo(isLocal, entry));
        }
        return entriesWithMetaInfo;
    }

    public static Set<DiscoveryEntryWithMetaInfo> convertToDiscoveryEntryWithMetaInfoSet(boolean isLocal,
                                                                                         Collection<? extends DiscoveryEntry> entries) {
        Set<DiscoveryEntryWithMetaInfo> entriesWithMetaInfo = new HashSet<DiscoveryEntryWithMetaInfo>(entries.size());
        for (DiscoveryEntry entry : entries) {
            entriesWithMetaInfo.add(convertToDiscoveryEntryWithMetaInfo(isLocal, entry));
        }
        return entriesWithMetaInfo;
    }
}
