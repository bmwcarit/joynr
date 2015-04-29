package io.joynr.capabilities;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
import io.joynr.exceptions.JoynrCommunicationException;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

import javax.annotation.CheckForNull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.base.Function;
import com.google.common.collect.Collections2;
import com.google.inject.Inject;

/**
 * The CapabilitiesStore stores a list of provider channelIds and the interfaces
 * they offer.
 * Capability informations are stored in a concurrentHashMap. Using a in memory
 * database could be possible optimization.
 */
public class CapabilitiesStoreImpl implements CapabilitiesStore {

    private static final Logger logger = LoggerFactory.getLogger(CapabilitiesStoreImpl.class);

    // private ConcurrentLinkedQueue<CapabilityEntry> registeredCapabilities =
    // new
    // ConcurrentLinkedQueue<CapabilityEntry>();
    ConcurrentHashMap<String, Long> registeredCapabilitiesTime = new ConcurrentHashMap<String, Long>();
    ConcurrentHashMap<String, List<String>> interfaceAddressToCapabilityMapping = new ConcurrentHashMap<String, List<String>>();
    ConcurrentHashMap<String, String> participantIdToCapabilityMapping = new ConcurrentHashMap<String, String>();
    ConcurrentHashMap<String, CapabilityEntry> capabilityKeyToCapabilityMapping = new ConcurrentHashMap<String, CapabilityEntry>();
    ConcurrentHashMap<EndpointAddressBase, List<String>> endPointAddressToCapabilityMapping = new ConcurrentHashMap<EndpointAddressBase, List<String>>();
    ConcurrentHashMap<String, List<EndpointAddressBase>> capabilityKeyToEndPointAddressMapping = new ConcurrentHashMap<String, List<EndpointAddressBase>>();

    // Do not sychronize on a Boolean
    // Fixes FindBug warning: DL: Synchronization on Boolean
    private Object capsLock = new Object();

    @Inject
    public CapabilitiesStoreImpl(CapabilitiesProvisioning staticProvisioning) {
        logger.debug("creating CapabilitiesStore {} with static provisioning", this);
        add(staticProvisioning.getCapabilityEntries());
    }

    /*
     * (non-Javadoc)
     * @see io.joynr.capabilities.CapabilitiesStore#add(io.joynr.
     * capabilities .CapabilityEntry)
     */
    @Override
    public void add(CapabilityEntry capabilityEntry) {
        if (capabilityEntry.getDomain() == null || capabilityEntry.getInterfaceName() == null
                || capabilityEntry.getParticipantId() == null || capabilityEntry.getEndpointAddresses() == null
                || capabilityEntry.getEndpointAddresses().isEmpty()) {
            String message = "capabilityEntry being registered is not complete: " + capabilityEntry;
            logger.error(message);
            throw new JoynrCommunicationException(message);
        }

        synchronized (capsLock) {
            String capabilityEntryId = getInterfaceAddressParticipantKeyForCapability(capabilityEntry.getDomain(),
                                                                                      capabilityEntry.getInterfaceName(),
                                                                                      capabilityEntry.getParticipantId());
            CapabilityEntry entry = capabilityKeyToCapabilityMapping.get(capabilityEntryId);
            // check if a capabilityEntry with the same ID already exists
            if (entry != null) {
                remove(capabilityEntry.getParticipantId());
            }

            // update participantId to capability mapping
            // checkMapDoesNotContainId(capabilityKeyToCapabilityMapping,
            // capabilityEntryId, 1);
            capabilityKeyToCapabilityMapping.put(capabilityEntryId, capabilityEntry);

            // update time mapping
            // checkMapDoesNotContainId(registeredCapabilitiesTime,
            // capabilityEntryId, 2);
            registeredCapabilitiesTime.put(capabilityEntryId, System.currentTimeMillis());

            // update interfaceDomain to capability mapping
            String domainInterfaceId = getInterfaceAddressKeyForCapability(capabilityEntry.getDomain(),
                                                                           capabilityEntry.getInterfaceName());

            // if domainInterfaceId not in the mapping, map it to an empty map,
            // otherwise use the mapping that is
            // already there
            // fixes FindBugs error: Sequence of calls to concurrent abstraction
            // may not be atomic
            List<String> newMapping = new ArrayList<String>();
            List<String> mapping = interfaceAddressToCapabilityMapping.putIfAbsent(domainInterfaceId, newMapping);
            if (mapping == null) {
                mapping = newMapping;
            } // end fix for FindBugs error;

            // checkListDoesNotContainId(mapping, domainInterfaceId, 3);
            mapping.add(capabilityEntryId);

            // update participantId to capability mapping
            String participantId = capabilityEntry.getParticipantId();

            participantIdToCapabilityMapping.put(participantId, capabilityEntryId);

            // update endpointAddress to capability mapping
            // CA: at this point it is already ensured that the list of
            // endpointaddresses is not null
            // checkMapDoesNotContainId(capabilityKeyToEndPointAddressMapping,
            // capabilityEntryId, 5);
            capabilityKeyToEndPointAddressMapping.put(capabilityEntryId, capabilityEntry.getEndpointAddresses());
            for (EndpointAddressBase endpointAddress : capabilityEntry.getEndpointAddresses()) {

                // fixes FindBugs error: Sequence of calls to concurrent
                // abstraction may not be atomic
                newMapping = new ArrayList<String>();
                mapping = endPointAddressToCapabilityMapping.putIfAbsent(endpointAddress, newMapping);
                if (mapping == null) {
                    mapping = newMapping;
                } // end fix for FindBugs error;
                // checkListDoesNotContainId(mapping, capabilityEntryId, 6);
                mapping.add(capabilityEntryId);
            }
        }
    }

    // private void checkMapDoesNotContainId(Map<String, ?> map, String key, int
    // errorID)
    // throws JoynCommunicationException {
    // if (map.containsKey(key)) {
    // String message = "internal state error " + errorID
    // + " in io.joynr.capabilities.CapabilitiesStoreImpl: entry with id " + key
    // + " should not exist in the store";
    // throw new JoynCommunicationException(message);
    // }
    // }

    // private void checkListDoesNotContainId(List<String> map, String key, int
    // errorID) throws
    // JoynCommunicationException {
    // if (map.contains(key)) {
    // String message = "internal state error " + errorID
    // + " in io.joynr.capabilities.CapabilitiesStoreImpl: entry with id " + key
    // + " should not exist in the store";
    // throw new JoynCommunicationException(message);
    // }
    // }

    String getInterfaceAddressParticipantKeyForCapability(String domain, String interfaceName, String participantId) {
        return (getInterfaceAddressKeyForCapability(domain, interfaceName) + "|" + participantId).toLowerCase();
    }

    String getInterfaceAddressKeyForCapability(String domain, String interfaceName) {
        return (domain + "|" + interfaceName).toLowerCase();
    }

    /*
     * (non-Javadoc)
     * @see
     * io.joynr.capabilities.CapabilitiesStore#add(java.util
     * .Collection)
     */
    @Override
    public void add(Collection<? extends CapabilityEntry> entries) {
        if (entries != null) {
            for (CapabilityEntry entry : entries) {
                add(entry);
            }
        }
    }

    /*
     * (non-Javadoc)
     * @see io.joynr.capabilities.CapabilitiesStore#remove(String)
     */
    @Override
    public boolean remove(String participantId) {
        // removedValue = registeredCapabilities.remove(capEntry);
        boolean removedSuccessfully = false;

        synchronized (capsLock) {
            removedSuccessfully = removeCapabilityFromStore(participantId);
        }
        if (!removedSuccessfully) {
            logger.error("Could not find capability to remove with Id: {}", participantId);
        }
        return removedSuccessfully;
    }

    private boolean removeCapabilityFromStore(String participantId) {
        String capabilityEntryId = participantIdToCapabilityMapping.get(participantId);
        if (capabilityEntryId == null) {
            return false;
        }
        CapabilityEntry capability = capabilityKeyToCapabilityMapping.get(capabilityEntryId);

        if (capability == null) {
            return false;
        }

        String domainInterfaceId = getInterfaceAddressKeyForCapability(capability.getDomain(),
                                                                       capability.getInterfaceName());
        CapabilityEntry entry = capabilityKeyToCapabilityMapping.remove(capabilityEntryId);
        // check if a capabilityEntry with the same ID already exists
        if (entry == null) {
            return false;
        }

        // check if a capabilityEntry with the same ID already exists in
        // capabilityKeyToEndPointAddessMapping
        if (capabilityKeyToEndPointAddressMapping.remove(capabilityEntryId) == null) {
            return false;
        }

        // update time mapping
        registeredCapabilitiesTime.remove(capabilityEntryId);

        // update interfaceDomain to capability mapping
        List<String> mapping = interfaceAddressToCapabilityMapping.get(domainInterfaceId);
        if (mapping != null) {
            if (!mapping.remove(capabilityEntryId)) {
                logger.error("Could not find capability to remove from interfaceDomainToCapabilityMapping: {}",
                             capabilityEntryId);
            }
            if (mapping.isEmpty()) {
                interfaceAddressToCapabilityMapping.remove(domainInterfaceId);
            }
        } else {
            logger.error("Could not find capability to remove from interfaceDomainToCapabilityMapping: {}",
                         capabilityEntryId);
        }

        // update participantId to capability mapping
        if (participantIdToCapabilityMapping.remove(participantId) == null) {
            logger.error("Could not find capability to remove from participantIdToCapabilityMapping: {}",
                         capabilityEntryId);
        }

        // update endpointAddress to capapbility mapping
        for (EndpointAddressBase endpointAddress : entry.getEndpointAddresses()) {
            mapping = endPointAddressToCapabilityMapping.get(endpointAddress);
            if (mapping != null) {
                if (!mapping.remove(capabilityEntryId)) {
                    logger.error("Could not find capability to remove from endPointAddressToCapabilityMapping: {}",
                                 capabilityEntryId);
                }
                if (mapping.isEmpty()) {
                    endPointAddressToCapabilityMapping.remove(endpointAddress);
                }
            } else {
                logger.error("Could not find capability to remove from endPointAddressToCapabilityMapping: {}",
                             capabilityEntryId);
            }
        }

        return true;
    }

    /*
     * (non-Javadoc)
     * @see
     * io.joynr.capabilities.CapabilitiesStore#remove(java.util.Collection)
     */
    @Override
    public void remove(Collection<String> participantIds) {
        for (String participantId : participantIds) {
            remove(participantId);
        }
    }

    private boolean checkAge(Long timeStamp, long maxAcceptedAge) {
        return timeStamp != null && ((System.currentTimeMillis() - timeStamp) <= maxAcceptedAge);
    }

    /*
     * (non-Javadoc)
     * @see
     * io.joynr.capabilities.CapabilitiesStore#lookup
     * (java.lang.String, java.lang.String)
     */
    @Override
    public Collection<CapabilityEntry> lookup(final String domain, final String interfaceName) {
        return lookup(domain, interfaceName, DiscoveryQos.NO_MAX_AGE);
    }

    /*
     * (non-Javadoc)
     * @see
     * io.joynr.capabilities.CapabilitiesStore#lookup
     * (java.lang.String, java.lang.String, long)
     */
    @Override
    public Collection<CapabilityEntry> lookup(final String domain, final String interfaceName, long cacheMaxAge) {

        ArrayList<CapabilityEntry> capabilitiesList = new ArrayList<CapabilityEntry>();

        synchronized (capsLock) {
            String key = getInterfaceAddressKeyForCapability(domain, interfaceName);
            List<String> map = interfaceAddressToCapabilityMapping.get(key);
            if (map != null) {
                for (String capId : map) {
                    CapabilityEntry ce = capabilityKeyToCapabilityMapping.get(capId);

                    if (ce != null && ce.getDomain().equals(domain)
                            && ce.getInterfaceName().equalsIgnoreCase(interfaceName)) {
                        if (checkQoSMatches(capId, ce, cacheMaxAge)) {
                            capabilitiesList.add(ce);
                        }
                    }

                }
            }
        }

        logger.debug("Capabilities found: {}", capabilitiesList.toString());
        return capabilitiesList;
    }

    /*
     * (non-Javadoc)
     * @see
     * io.joynr.capabilities.CapabilitiesStore#lookup
     * (java.lang.String, io.joynr.arbitration.DiscoveryQos)
     */
    @Override
    @CheckForNull
    public CapabilityEntry lookup(String participantId, long cacheMaxAge) {

        synchronized (capsLock) {
            String capabilityEntryId = participantIdToCapabilityMapping.get(participantId);
            if (capabilityEntryId == null) {
                return null;
            }

            CapabilityEntry capabilityEntry = capabilityKeyToCapabilityMapping.get(capabilityEntryId);
            logger.debug("Capability for participantId {} found: {}", participantId, capabilityEntry);
            if (checkAge(registeredCapabilitiesTime.get(capabilityEntryId), cacheMaxAge)) {
                return capabilityEntry;
            }

            return null;
        }
    }

    /*
     * (non-Javadoc)
     * @see io.joynr.capabilities.CapabilitiesStore#getAllCapabilities()
     */
    @Override
    public HashSet<CapabilityEntry> getAllCapabilities() {
        synchronized (capsLock) {
            return new HashSet<CapabilityEntry>(Collections2.transform(registeredCapabilitiesTime.keySet(),
                                                                       new Function<String, CapabilityEntry>() {
                                                                           @Override
                                                                           public CapabilityEntry apply(String input) {
                                                                               // prevent warning about potential use of null as
                                                                               // param to capabilityKeyToCapabilityMapping.get(input)
                                                                               if (input == null) {
                                                                                   return null;
                                                                               }
                                                                               return capabilityKeyToCapabilityMapping.get(input);
                                                                           }
                                                                       }));
        }
    }

    private boolean checkQoSMatches(String capId, CapabilityEntry capInfo, long cacheMaxAge) {
        // TODO accept QoS parameters which are higher than/include the
        // requested ones
        // ProviderQos providerQos = capInfo.getProviderQos();
        // TODO implement ProviderQosRequirements and adjust this section
        /*
         * if (providerQos.getPriority() != requestedQos ... priority)
         *
         * for (Map.Entry<String, String> qosEntry :
         * capInfo.getProviderQos().getQos()) { String requestedValue =
         * requestedQos.get(qosEntry.getKey()); if
         * (requestedValue.compareTo(qosEntry.getValue()) != 0) { return false;
         * } }
         */
        // If capability is local ignore age, otherwise check cache age.
        return checkAge(registeredCapabilitiesTime.get(capId), cacheMaxAge);
    }

    /*
     * (non-Javadoc)
     * @see
     * io.joynr.capabilities.CapabilitiesStore#hasCapability(io.joynr.capabilities
     * .CapabilityEntry)
     */
    @Override
    public boolean hasCapability(CapabilityEntry capabilityEntry) {
        synchronized (capsLock) {
            String capabilityEntryId = getInterfaceAddressParticipantKeyForCapability(capabilityEntry.getDomain(),
                                                                                      capabilityEntry.getInterfaceName(),
                                                                                      capabilityEntry.getParticipantId());
            return registeredCapabilitiesTime.containsKey(capabilityEntryId);
        }
    }
}
