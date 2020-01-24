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

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrCommunicationException;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;

/**
 * The DiscoveryEntryStore stores a list of providers and the interfaces
 * they offer (DiscoveryEntry).
 */
public class DiscoveryEntryStoreInMemory<T extends DiscoveryEntry> implements DiscoveryEntryStore<T> {

    private static final Logger logger = LoggerFactory.getLogger(DiscoveryEntryStoreInMemory.class);

    Map<String, Long> registeredCapabilitiesTime = new HashMap<String, Long>();
    Map<String, List<String>> interfaceAddressToCapabilityMapping = new HashMap<String, List<String>>();
    Map<String, String> participantIdToCapabilityMapping = new HashMap<String, String>();
    Map<String, T> capabilityKeyToCapabilityMapping = new HashMap<String, T>();
    Map<Address, List<String>> endPointAddressToCapabilityMapping = new HashMap<Address, List<String>>();

    // Do not sychronize on a Boolean
    // Fixes FindBug warning: DL: Synchronization on Boolean
    private Object storeLock = new Object();

    @Inject
    public DiscoveryEntryStoreInMemory(CapabilitiesProvisioning staticProvisioning) {
        logger.trace("creating CapabilitiesStore {} with static provisioning", this);
        //add(staticProvisioning.getCapabilityEntries());
    }

    /*
     * (non-Javadoc)
     * @see io.joynr.capabilities.CapabilitiesStore#add(io.joynr.
     * capabilities .DiscoveryEntry)
     */
    @Override
    public synchronized void add(T discoveryEntry) {
        if (discoveryEntry.getDomain() == null || discoveryEntry.getInterfaceName() == null
                || discoveryEntry.getParticipantId() == null) {
            String message = "discoveryEntry being registered is not complete: " + discoveryEntry;
            logger.error(message);
            throw new JoynrCommunicationException(message);
        }

        synchronized (storeLock) {
            String discoveryEntryId = domainInterfaceParticipantIdKey(discoveryEntry.getDomain(),
                                                                      discoveryEntry.getInterfaceName(),
                                                                      discoveryEntry.getParticipantId());
            T entry = capabilityKeyToCapabilityMapping.get(discoveryEntryId);
            // check if a DiscoveryEntry with the same Id already exists
            if (entry != null) {
                remove(discoveryEntry.getParticipantId());
            }

            // update participantId to capability mapping
            capabilityKeyToCapabilityMapping.put(discoveryEntryId, discoveryEntry);

            // update time mapping
            registeredCapabilitiesTime.put(discoveryEntryId, System.currentTimeMillis());

            // update interfaceDomain to capability mapping
            String domainInterfaceId = domainInterfaceKey(discoveryEntry.getDomain(),
                                                          discoveryEntry.getInterfaceName());

            // if domainInterfaceId not in the mapping, map it to an empty map,
            // otherwise use the mapping that is  already there
            List<String> newMapping = new ArrayList<String>();
            List<String> mapping = interfaceAddressToCapabilityMapping.putIfAbsent(domainInterfaceId, newMapping);
            if (mapping == null) {
                mapping = newMapping;
            }

            mapping.add(discoveryEntryId);

            // update participantId to capability mapping
            String participantId = discoveryEntry.getParticipantId();

            participantIdToCapabilityMapping.put(participantId, discoveryEntryId);
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
            logger.error("Could not find discoveryEntry to remove with Id: {}", participantId);
        }
        return removedSuccessfully;
    }

    private boolean removeDiscoveryEntryFromStore(String participantId) {
        String discoveryEntryId = participantIdToCapabilityMapping.get(participantId);
        if (discoveryEntryId == null) {
            return false;
        }
        T capability = capabilityKeyToCapabilityMapping.get(discoveryEntryId);

        if (capability == null) {
            return false;
        }

        String domainInterfaceId = domainInterfaceKey(capability.getDomain(), capability.getInterfaceName());
        T entry = capabilityKeyToCapabilityMapping.remove(discoveryEntryId);
        // check if a discoveryEntry with the same ID already exists
        if (entry == null) {
            return false;
        }

        // update time mapping
        registeredCapabilitiesTime.remove(discoveryEntryId);

        // update interfaceDomain to capability mapping
        List<String> mapping = interfaceAddressToCapabilityMapping.get(domainInterfaceId);
        if (mapping != null) {
            if (!mapping.remove(discoveryEntryId)) {
                logger.error("Could not find capability to remove from interfaceDomainToCapabilityMapping: {}",
                             discoveryEntryId);
            }
            if (mapping.isEmpty()) {
                interfaceAddressToCapabilityMapping.remove(domainInterfaceId);
            }
        } else {
            logger.error("Could not find capability to remove from interfaceDomainToCapabilityMapping: {}",
                         discoveryEntryId);
        }

        // update participantId to capability mapping
        if (participantIdToCapabilityMapping.remove(participantId) == null) {
            logger.error("Could not find capability to remove from participantIdToCapabilityMapping: {}",
                         discoveryEntryId);
        }

        return true;

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

        logger.debug("Capabilities found: {}", capabilitiesList.toString());
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

            logger.debug("Capability for participantId {} found: {}", participantId, discoveryEntry);
            if (discoveryEntry instanceof GlobalDiscoveryEntry
                    && !checkAge(registeredCapabilitiesTime.get(discoveryEntryId), cacheMaxAge)) {
                return Optional.empty();
            }

            return Optional.of(discoveryEntry);
        }
    }

    @Override
    public HashSet<T> getAllDiscoveryEntries() {
        HashSet<T> allDiscoveryEntries = new HashSet<T>();

        return allDiscoveryEntries;

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
    public void touch(String clusterControllerId) {
        // TODO Auto-generated method stub
    }
}
