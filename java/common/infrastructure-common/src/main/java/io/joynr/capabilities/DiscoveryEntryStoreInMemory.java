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

import static java.lang.String.format;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.TreeMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrCommunicationException;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderScope;

/**
 * The DiscoveryEntryStore stores a list of providers and the interfaces
 * they offer (DiscoveryEntry). The store can be optionally limited for
 * non-sticky entries, if maximumNumberOfNonStickyEntries is set to a positive value.
 */
public class DiscoveryEntryStoreInMemory<T extends DiscoveryEntry> implements DiscoveryEntryStore<T> {

    private static final Logger logger = LoggerFactory.getLogger(DiscoveryEntryStoreInMemory.class);

    Map<String, Long> registeredCapabilitiesTime = new HashMap<String, Long>();
    Map<String, List<String>> interfaceAddressToCapabilityMapping = new HashMap<String, List<String>>();
    Map<String, String> participantIdToCapabilityMapping = new HashMap<String, String>();
    Map<String, T> capabilityKeyToCapabilityMapping = new HashMap<String, T>();
    // maps queueId -> participiantId, also ordered
    TreeMap<Long, String> queueIdToParticipantIdMapping = new TreeMap<>();
    // maps participiantId -> queueId
    Map<String, Long> participantIdToQueueIdMapping = new HashMap<String, Long>();
    private long counter = 0;
    private int maximumNumberOfNonStickyEntries;

    // Do not synchronize on a Boolean
    // Fixes FindBug warning: DL: Synchronization on Boolean
    private Object storeLock = new Object();

    public DiscoveryEntryStoreInMemory(int maximumNumberOfNonStickyEntries) {
        logger.info("Creating CapabilitiesStore, maximumNumberOfNonStickyEntries = {}",
                    maximumNumberOfNonStickyEntries);
        this.maximumNumberOfNonStickyEntries = maximumNumberOfNonStickyEntries;
    }

    private void recreateQueueIdToParticipantIdMapping() {
        TreeMap<Long, String> newMap = new TreeMap<>();
        participantIdToQueueIdMapping = new HashMap<>();
        counter = 0;
        for (String participantId : queueIdToParticipantIdMapping.values()) {
            counter++;
            newMap.put(counter, participantId);
            participantIdToQueueIdMapping.put(participantId, counter);
        }
        counter++;
        queueIdToParticipantIdMapping = newMap;
    }

    /*
     * (non-Javadoc)
     * @see io.joynr.capabilities.CapabilitiesStore#add(io.joynr.
     * capabilities .DiscoveryEntry)
     */
    @Override
    public void add(T discoveryEntry) {
        if (discoveryEntry == null || discoveryEntry.getDomain() == null || discoveryEntry.getInterfaceName() == null
                || discoveryEntry.getParticipantId() == null) {
            String message = format("Entry being registered is null or not complete: %s", discoveryEntry);
            logger.error(message);
            throw new JoynrCommunicationException(message);
        }
        String participantId = discoveryEntry.getParticipantId();
        logger.trace("About to add entry for participantId {} to DiscoveryEntryStoreInMemory.", participantId);
        synchronized (storeLock) {
            // Check whether an entry for this participantId already exists, if so, remove it
            boolean removedSuccessfully = removeDiscoveryEntryFromStore(participantId);
            if (removedSuccessfully) {
                logger.trace("Removed old entry for participantId {} from DiscoveryEntryStoreInMemory.", participantId);
            }

            // store limit will only be used if enabled, and only for non-sticky entries;
            // an entry is considered sticky, when its expiryDateMs is equal to Long.MAX_VALUE
            if (maximumNumberOfNonStickyEntries > 0 && discoveryEntry.getExpiryDateMs() != Long.MAX_VALUE) {
                // Since regular entries are non-sticky we should almost always arrive here in case the store size is limited
                if (queueIdToParticipantIdMapping.size() >= maximumNumberOfNonStickyEntries) {
                    // Since the key is numeric, firstEntry() should deliver the entry with smallest
                    // key value which should be the oldest entry since the counter gets increased
                    // for each new invocation
                    Map.Entry<Long, String> entry = queueIdToParticipantIdMapping.firstEntry();
                    String oldestParticipantId = entry.getValue();
                    removedSuccessfully = removeDiscoveryEntryFromStore(oldestParticipantId);
                    if (removedSuccessfully) {
                        logger.debug("Removed oldest entry for participantId {}, queueId {} from DiscoveryEntryStoreInMemory.",
                                     oldestParticipantId,
                                     entry.getKey());
                    } else {
                        logger.error("Could not remove oldest entry with participantId {}, queueId {} from DiscoveryEntryStoreInMemory.",
                                     oldestParticipantId,
                                     entry.getKey());
                    }
                }

                counter++;
                if (Long.MAX_VALUE == counter) {
                    recreateQueueIdToParticipantIdMapping();
                }

                String oldParticipantId = queueIdToParticipantIdMapping.put(counter, participantId);
                if (oldParticipantId != null) {
                    logger.error("Found existing entry queueId {} in queueIdToParticipantIdMapping, old participantId {}, new participantId {}",
                                 counter,
                                 oldParticipantId,
                                 participantId);
                }

                Long oldQueueId = participantIdToQueueIdMapping.put(participantId, counter);
                if (oldQueueId != null) {
                    logger.error("Found existing entry participantId {} in participantIdToQueueIdMapping, old queueId {}, new queueId {}",
                                 participantId,
                                 oldQueueId,
                                 counter);
                }
            }

            String discoveryEntryId = domainInterfaceParticipantIdKey(discoveryEntry.getDomain(),
                                                                      discoveryEntry.getInterfaceName(),
                                                                      participantId);

            // update participantId to combined key [domain + interfaceName + participantId] mapping
            String oldDiscoveryEntryId = participantIdToCapabilityMapping.put(participantId, discoveryEntryId);
            if (oldDiscoveryEntryId != null) {
                logger.error("Found existing entry participantId {} in participantIdToCapabilityMapping, old discoveryEntryId {}, new discoveryEntryId {}",
                             participantId,
                             oldDiscoveryEntryId,
                             discoveryEntryId);
            }

            // update combined key [domain + interfaceName + participantId] to time mapping
            long newTimeMs = System.currentTimeMillis();
            Long oldTimeMs = registeredCapabilitiesTime.put(discoveryEntryId, newTimeMs);
            if (oldTimeMs != null) {
                logger.error("Found existing entry discoveryEntryId {} in registeredCapabilitiesTime, old timeMs {}, new timeMs {}",
                             discoveryEntryId,
                             oldTimeMs,
                             newTimeMs);
            }

            // update combined key [domain + interfaceName + participantId] to capability mapping
            DiscoveryEntry oldDiscoveryEntry = capabilityKeyToCapabilityMapping.put(discoveryEntryId, discoveryEntry);
            if (oldDiscoveryEntry != null) {
                logger.error("Found existing entry discoveryEntryId {} in capabilityKeyToCapabilityMapping, old discoveryEntry {}, new discoveryEntry {}",
                             discoveryEntryId,
                             oldDiscoveryEntry,
                             discoveryEntry);
            }

            // update combined key [domain + interface] to capability mapping
            String domainInterfaceId = domainInterfaceKey(discoveryEntry.getDomain(),
                                                          discoveryEntry.getInterfaceName());

            // if domainInterfaceId not in the mapping, map it to an empty map,
            // otherwise use the mapping that is already there, putIfAbsent is thus intentionally used
            List<String> newMapping = new ArrayList<String>();
            List<String> mapping = interfaceAddressToCapabilityMapping.putIfAbsent(domainInterfaceId, newMapping);
            if (mapping == null) {
                mapping = newMapping;
            }

            mapping.add(discoveryEntryId);

            logger.debug("Added entry for participantId {} to DiscoveryEntryStoreInMemory.", participantId);
        }
    }

    @Override
    public void add(Collection<T> entries) {
        if (entries != null) {
            for (T entry : entries) {
                add(entry);
            }
        }
    }

    @Override
    public boolean remove(String participantId) {
        boolean removedSuccessfully = false;

        synchronized (storeLock) {
            removedSuccessfully = removeDiscoveryEntryFromStore(participantId);
        }
        if (!removedSuccessfully) {
            logger.error("Could not remove discoveryEntry for participantId {} from DiscoveryEntryStoreInMemory.",
                         participantId);
        } else {
            logger.debug("Removed entry for participantId {} from DiscoveryEntryStoreInMemory.", participantId);
        }
        return removedSuccessfully;
    }

    private boolean removeDiscoveryEntryFromStore(String participantId) {
        logger.trace("removeDiscoveryEntryFromStore for participantId {}", participantId);
        // get combined key [domain + interfaceName + participantId], if existing
        String discoveryEntryId = participantIdToCapabilityMapping.remove(participantId);
        if (discoveryEntryId == null) {
            // valid case, if entry is not part of the cache yet
            return false;
        }

        // remove entry from [domain + interfaceName + participantId] -> time mapping
        if (registeredCapabilitiesTime.remove(discoveryEntryId) == null) {
            logger.error("Could not find participantId {} to remove from registeredCapabilitiesTime, discoveryEntryId {}",
                         participantId,
                         discoveryEntryId);
        }

        T capability = capabilityKeyToCapabilityMapping.remove(discoveryEntryId);
        if (capability == null) {
            logger.error("Could not find discoveryEntryId {} to remove from capabilityKeyToCapabilityMapping",
                         discoveryEntryId);
            // since capability data is required for further processing, quit here if the entry could not be found
            if (maximumNumberOfNonStickyEntries > 0) {
                // try to remove entry from remaining maps as well without further error checking
                // since it is unclear whether entry was non-sticky
                Long queueId = participantIdToQueueIdMapping.remove(participantId);
                if (queueId != null) {
                    queueIdToParticipantIdMapping.remove(queueId);
                }
            }
            return false;
        }

        boolean removedSuccessfully = true;

        // update [interface + domain] to capability mapping
        // there can be multiple entries serving same interface / domain so remove only 
        // matching one from list stored here, if list gets emptied by this, remove the list as well
        String domainInterfaceId = domainInterfaceKey(capability.getDomain(), capability.getInterfaceName());
        List<String> mapping = interfaceAddressToCapabilityMapping.get(domainInterfaceId);
        if (mapping != null) {
            if (!mapping.remove(discoveryEntryId)) {
                logger.error("Could not find discoveryEntryId {} to remove from list returned for domainInterfaceId {} in interfaceDomainToCapabilityMapping",
                             discoveryEntryId,
                             domainInterfaceId);
                removedSuccessfully = false;
            }
            if (mapping.isEmpty()) {
                interfaceAddressToCapabilityMapping.remove(domainInterfaceId);
            }
        } else {
            logger.error("Could not find domainInterfaceId {} in interfaceAddressToCapabilityMapping, in order to remove discoveryEntryId {} from associated list",
                         domainInterfaceId,
                         discoveryEntryId);
            removedSuccessfully = false;
        }

        // store limit will only be used if configured, and only for non-sticky entries;
        // an entry is considered sticky, when its expiryDateMs is equal to Long.MAX_VALUE
        if (maximumNumberOfNonStickyEntries > 0 && capability.getExpiryDateMs() != Long.MAX_VALUE) {
            Long queueId = participantIdToQueueIdMapping.remove(participantId);
            if (queueId == null) {
                logger.error("Could not find participantId {} to remove from participantIdToQueueIdMapping, discoveryEntryId {}",
                             participantId,
                             discoveryEntryId);
                removedSuccessfully = false;
            } else {
                if (queueIdToParticipantIdMapping.remove(queueId) == null) {
                    logger.error("Could not find queueid {} to remove from queueIdToParticipantIdMapping, discoveryEntryId {}",
                                 queueId,
                                 discoveryEntryId);
                    removedSuccessfully = false;
                }
            }
        }

        return removedSuccessfully;
    }

    @Override
    public void remove(Collection<String> participantIds) {
        for (String participantId : participantIds) {
            remove(participantId);
        }
    }

    @Override
    public Collection<T> lookup(final String[] domains, final String interfaceName) {
        return lookup(domains, interfaceName, DiscoveryQos.NO_MAX_AGE);
    }

    @Override
    public Collection<T> lookup(final String[] domains, final String interfaceName, long cacheMaxAge) {
        ArrayList<T> capabilitiesList = new ArrayList<T>();

        synchronized (storeLock) {
            for (String domain : domains) {
                String domainInterfacekey = domainInterfaceKey(domain, interfaceName);
                List<String> matchingDiscoveryEntries = interfaceAddressToCapabilityMapping.get(domainInterfacekey);
                if (matchingDiscoveryEntries != null) {
                    // check that sure cache age is OK
                    for (String capId : matchingDiscoveryEntries) {
                        T discoveryEntry = capabilityKeyToCapabilityMapping.get(capId);

                        if (discoveryEntry instanceof GlobalDiscoveryEntry
                                && !checkAge(registeredCapabilitiesTime.get(capId), cacheMaxAge)) {
                            continue;
                        }

                        capabilitiesList.add(discoveryEntry);
                    }
                }
            }
        }

        logger.trace("Capabilities found: {}", capabilitiesList.toString());
        return capabilitiesList;
    }

    @Override
    public Optional<T> lookup(String participantId, long cacheMaxAge) {

        synchronized (storeLock) {
            String discoveryEntryId = participantIdToCapabilityMapping.get(participantId);
            if (discoveryEntryId == null) {
                return Optional.empty();
            }

            T discoveryEntry = capabilityKeyToCapabilityMapping.get(discoveryEntryId);

            logger.trace("Capability for participantId {} found: {}", participantId, discoveryEntry);
            if (discoveryEntry instanceof GlobalDiscoveryEntry
                    && !checkAge(registeredCapabilitiesTime.get(discoveryEntryId), cacheMaxAge)) {
                return Optional.empty();
            }

            return Optional.of(discoveryEntry);
        }
    }

    @Override
    public Collection<T> lookupGlobalEntries(final String[] domains, final String interfaceName) {
        Collection<T> capabilitiesList = lookup(domains, interfaceName);
        Collection<T> result = new HashSet<>();
        for (T cap : capabilitiesList) {
            if (cap.getQos().getScope().equals(ProviderScope.GLOBAL)) {
                result.add(cap);
            }
        }
        return result;
    }

    @Override
    public HashSet<T> getAllDiscoveryEntries() {
        HashSet<T> allDiscoveryEntries = new HashSet<T>();

        return allDiscoveryEntries;

    }

    @Override
    public HashSet<T> getAllGlobalEntries() {
        HashSet<T> allGlobalEntries = new HashSet<T>();
        synchronized (storeLock) {
            for (T discoveryEntry : capabilityKeyToCapabilityMapping.values()) {
                if (discoveryEntry.getQos().getScope().equals(ProviderScope.GLOBAL)) {
                    allGlobalEntries.add(discoveryEntry);
                }
            }
        }
        return allGlobalEntries;
    }

    @Override
    public boolean hasDiscoveryEntry(DiscoveryEntry discoveryEntry) {
        synchronized (storeLock) {
            String discoveryEntryId = domainInterfaceParticipantIdKey(discoveryEntry.getDomain(),
                                                                      discoveryEntry.getInterfaceName(),
                                                                      discoveryEntry.getParticipantId());
            return registeredCapabilitiesTime.containsKey(discoveryEntryId);
        }
    }

    String domainInterfaceParticipantIdKey(String domain, String interfaceName, String participantId) {
        return (domainInterfaceKey(domain, interfaceName) + "|" + participantId).toLowerCase();
    }

    String domainInterfaceKey(String domain, String interfaceName) {
        return (domain + "|" + interfaceName).toLowerCase();
    }

    private boolean checkAge(Long timeStamp, long maxAcceptedAge) {
        return timeStamp != null && ((System.currentTimeMillis() - timeStamp) <= maxAcceptedAge);
    }

    @Override
    public String[] touchDiscoveryEntries(long lastSeenDateMs, long expiryDateMs) {
        List<String> participantIds = new ArrayList<>();

        synchronized (storeLock) {
            for (T discoveryEntry : capabilityKeyToCapabilityMapping.values()) {
                if (discoveryEntry.getLastSeenDateMs() < lastSeenDateMs) {
                    discoveryEntry.setLastSeenDateMs(lastSeenDateMs);
                }
                if (discoveryEntry.getExpiryDateMs() < expiryDateMs) {
                    discoveryEntry.setExpiryDateMs(expiryDateMs);
                }
                if (discoveryEntry.getQos().getScope() == ProviderScope.GLOBAL) {
                    participantIds.add(discoveryEntry.getParticipantId());
                }
            }
        }
        return participantIds.toArray(new String[participantIds.size()]);
    }

    @Override
    public void touchDiscoveryEntries(String[] participantIds, long lastSeenDateMs, long expiryDateMs) {
        synchronized (storeLock) {
            for (String participantId : participantIds) {
                if (!participantIdToCapabilityMapping.containsKey(participantId)) {
                    continue;
                }
                String discoveryEntryId = participantIdToCapabilityMapping.get(participantId);
                T foundDiscoveryEntry = capabilityKeyToCapabilityMapping.get(discoveryEntryId);
                if (foundDiscoveryEntry == null) {
                    continue;
                }
                foundDiscoveryEntry.setLastSeenDateMs(lastSeenDateMs);
                foundDiscoveryEntry.setExpiryDateMs(expiryDateMs);
            }
        }
    }
}
