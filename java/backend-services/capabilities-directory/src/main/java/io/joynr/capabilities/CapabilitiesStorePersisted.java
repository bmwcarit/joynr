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
import io.joynr.exceptions.JoynrCommunicationException;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;

import javax.annotation.CheckForNull;
import javax.annotation.Nonnull;
import javax.persistence.EntityManager;
import javax.persistence.EntityTransaction;
import javax.transaction.Transactional;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Provider;

/**
 * The CapabilitiesStore stores a list of provider channelIds and the interfaces
 * they offer.
 * Capability informations are stored in a concurrentHashMap. Using a in memory
 * database could be possible optimization.
 */
public class CapabilitiesStorePersisted implements CapabilitiesStore {

    private static final Logger logger = LoggerFactory.getLogger(CapabilitiesStorePersisted.class);
    protected Provider<EntityManager> entityManagerProvider;

    // Do not sychronize on a Boolean
    // Fixes FindBug warning: DL: Synchronization on Boolean
    private Object capsLock = new Object();

    @Inject
    public CapabilitiesStorePersisted(CapabilitiesProvisioning staticProvisioning,
                                      Provider<EntityManager> entityManagerProvider) {
        this.entityManagerProvider = entityManagerProvider;
        logger.debug("creating CapabilitiesStore {} with static provisioning", this);
        add(staticProvisioning.getCapabilityEntries());
    }

    /*
     * (non-Javadoc)
     * @see io.joynr.capabilities.CapabilitiesStore#add(io.joynr.
     * capabilities .CapabilityEntry)
     */
    @Override
    public synchronized void add(CapabilityEntry capabilityEntry) {
        if (!(capabilityEntry instanceof CapabilityEntryPersisted)) {
            return;
        }

        if (capabilityEntry.getDomain() == null || capabilityEntry.getInterfaceName() == null
                || capabilityEntry.getParticipantId() == null || capabilityEntry.getEndpointAddresses() == null
                || capabilityEntry.getEndpointAddresses().isEmpty()) {
            String message = "capabilityEntry being registered is not complete: " + capabilityEntry;
            logger.error(message);
            throw new JoynrCommunicationException(message);
        }

        EntityManager entityManager = entityManagerProvider.get();
        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            entityManager.merge(capabilityEntry);
            transaction.commit();
        } catch (Exception e) {
            logger.error("unable to add capabilityEntry: {}, reason: {}", capabilityEntry, e.getMessage());
        } finally {
        }
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
        EntityManager entityManager = entityManagerProvider.get();
        CapabilityEntryPersisted capabilityEntry = entityManager.find(CapabilityEntryPersisted.class, participantId);

        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            entityManager.remove(capabilityEntry);
            transaction.commit();
        } catch (Exception e) {
            logger.error("unable to remove capability: {} reason: {}", participantId, e.getMessage());
            return false;
        } finally {
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
    @SuppressWarnings("unchecked")
    @Override
    @Transactional
    public Collection<CapabilityEntry> lookup(final String domain, final String interfaceName, long cacheMaxAge) {
        EntityManager entityManager = entityManagerProvider.get();
        String query = "select ce from CapabilityEntryPersisted ce inner join ce.providerQos qos where ce.participantId = qos.participantId and ce.domain=:domain and ce.interfaceName=:interfaceName";
        List<CapabilityEntry> capabilitiesList = entityManager.createQuery(query)
                                                              .setParameter("domain", domain)
                                                              .setParameter("interfaceName", interfaceName)
                                                              .getResultList();

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
    @Transactional
    public CapabilityEntry lookup(String participantId, long cacheMaxAge) {
        EntityManager entityManager = entityManagerProvider.get();
        return entityManager.find(CapabilityEntryPersisted.class, participantId);
    }

    /*
     * (non-Javadoc)
     * @see io.joynr.capabilities.CapabilitiesStore#getAllCapabilities()
     */
    @Override
    @Transactional
    public HashSet<CapabilityEntry> getAllCapabilities() {
        EntityManager entityManager = entityManagerProvider.get();
        List<CapabilityEntryPersisted> allCapabilityEntries = entityManager.createQuery("Select capabilityEntry from CapabilityEntryPersisted capabilityEntry",
                                                                                        CapabilityEntryPersisted.class)
                                                                           .getResultList();

        return new HashSet<CapabilityEntry>(allCapabilityEntries);

    }

    /*
     * (non-Javadoc)
     * @see
     * io.joynr.capabilities.CapabilitiesStore#hasCapability(io.joynr.capabilities
     * .CapabilityEntry)
     */
    @Override
    public boolean hasCapability(@Nonnull CapabilityEntry capabilityEntry) {
        if (capabilityEntry instanceof CapabilityEntryPersisted) {
            EntityManager entityManager = entityManagerProvider.get();
            CapabilityEntryPersisted searchingForCapabilityEntry = (CapabilityEntryPersisted) capabilityEntry;
            CapabilityEntryPersisted foundCapability = entityManager.find(CapabilityEntryPersisted.class,
                                                                          searchingForCapabilityEntry.participantId);
            return capabilityEntry.equals(foundCapability);
        } else {
            return false;
        }
    }
}
