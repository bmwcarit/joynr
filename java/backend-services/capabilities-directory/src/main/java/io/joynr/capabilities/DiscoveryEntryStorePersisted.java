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
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import javax.annotation.CheckForNull;
import javax.annotation.Nonnull;
import javax.persistence.EntityManager;
import javax.persistence.EntityTransaction;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.Singleton;
import com.google.inject.persist.PersistService;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrCommunicationException;

/**
 * The CapabilitiesStore stores a list of provider channelIds and the interfaces
 * they offer.
 * Capability informations are stored in a concurrentHashMap. Using a in memory
 * database could be possible optimization.
 */
@Singleton
public class DiscoveryEntryStorePersisted<T extends GlobalDiscoveryEntryPersisted> implements DiscoveryEntryStore<T> {

    private static final Logger logger = LoggerFactory.getLogger(DiscoveryEntryStorePersisted.class);
    private EntityManager entityManager;

    // Do not sychronize on a Boolean
    // Fixes FindBug warning: DL: Synchronization on Boolean
    private Object capsLock = new Object();

    @Inject
    public DiscoveryEntryStorePersisted(CapabilitiesProvisioning staticProvisioning,
                                        Provider<EntityManager> entityManagerProvider,
                                        PersistService persistService) {
        persistService.start();
        entityManager = entityManagerProvider.get();
        logger.debug("creating CapabilitiesStore {} with static provisioning", this);
    }

    /*
     * (non-Javadoc)
     * @see io.joynr.capabilities.CapabilitiesStore#add(io.joynr.
     * capabilities .DiscoveryEntry)
     */
    @Override
    public synchronized void add(GlobalDiscoveryEntryPersisted discoveryEntry) {
        logger.debug("adding discovery entry: {}", discoveryEntry);
        if (!(discoveryEntry instanceof GlobalDiscoveryEntryPersisted)) {
            return;
        }
        GlobalDiscoveryEntryPersisted globalDiscoveryEntry = (GlobalDiscoveryEntryPersisted) discoveryEntry;
        if (globalDiscoveryEntry.getDomain() == null || globalDiscoveryEntry.getInterfaceName() == null
                || globalDiscoveryEntry.getParticipantId() == null || globalDiscoveryEntry.getAddress() == null) {
            String message = "discoveryEntry being registered is not complete: " + discoveryEntry;
            logger.error(message);
            throw new JoynrCommunicationException(message);
        }

        GlobalDiscoveryEntryPersisted discoveryEntryFound = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                               discoveryEntry.getParticipantId());

        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            if (discoveryEntryFound != null) {
                entityManager.merge(discoveryEntry);
            } else {
                entityManager.persist(discoveryEntry);
            }
            transaction.commit();
        } catch (Exception e) {
            if (transaction.isActive()) {
                transaction.rollback();
            }
            logger.error("unable to add discoveryEntry: " + discoveryEntry, e);
        }
    }

    @Override
    public void add(Collection<T> entries) {
        if (entries != null) {
            for (GlobalDiscoveryEntryPersisted entry : entries) {
                add(entry);
            }
        }
    }

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
        GlobalDiscoveryEntryPersisted discoveryEntry = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                          participantId);

        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            entityManager.remove(discoveryEntry);
            transaction.commit();
        } catch (Exception e) {
            if (transaction.isActive()) {
                transaction.rollback();
            }
            logger.error("unable to remove capability: " + participantId, e);
            return false;
        } finally {
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

    @SuppressWarnings("unchecked")
    @Override
    public Collection<T> lookup(final String[] domains, final String interfaceName, long cacheMaxAge) {
        String query = "from GlobalDiscoveryEntryPersisted where domain=:domain and interfaceName=:interfaceName";
        List<T> result = new ArrayList<>();
        for (String domain : domains) {
            List<T> capabilitiesList = entityManager.createQuery(query)
                                                    .setParameter("domain", domain)
                                                    .setParameter("interfaceName", interfaceName)
                                                    .getResultList();
            result.addAll(capabilitiesList);
        }

        logger.debug("looked up {}, {}, {} and found {}", Arrays.toString(domains), interfaceName, cacheMaxAge, result);
        return result;
    }

    @Override
    @CheckForNull
    public T lookup(String participantId, long cacheMaxAge) {
        @SuppressWarnings("unchecked")
        T result = (T) entityManager.find(GlobalDiscoveryEntryPersisted.class, participantId);
        logger.debug("looked up {}, {} and found {}", participantId, cacheMaxAge, result);
        return result;
    }

    @Override
    public Set<T> getAllDiscoveryEntries() {
        List<GlobalDiscoveryEntryPersisted> allCapabilityEntries = entityManager.createQuery("Select discoveryEntry from GlobalDiscoveryEntryPersisted discoveryEntry",
                                                                                             GlobalDiscoveryEntryPersisted.class)
                                                                                .getResultList();
        @SuppressWarnings("unchecked")
        Set<T> result = (Set<T>) new HashSet<GlobalDiscoveryEntryPersisted>(allCapabilityEntries);
        logger.debug("Retrieved all discovery entries: {}", result);
        return result;
    }

    @Override
    public boolean hasDiscoveryEntry(@Nonnull GlobalDiscoveryEntryPersisted discoveryEntry) {
        if (discoveryEntry instanceof GlobalDiscoveryEntryPersisted) {
            GlobalDiscoveryEntryPersisted searchingForDiscoveryEntry = (GlobalDiscoveryEntryPersisted) discoveryEntry;
            GlobalDiscoveryEntryPersisted foundCapability = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                               searchingForDiscoveryEntry.getParticipantId());
            return discoveryEntry.equals(foundCapability);
        } else {
            return false;
        }
    }

    @Override
    public void touch(String clusterControllerId) {
        String query = "from GlobalDiscoveryEntryPersisted where clusterControllerId=:clusterControllerId";
        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            @SuppressWarnings("unchecked")
            List<T> capabilitiesList = entityManager.createQuery(query)
                                                    .setParameter("clusterControllerId", clusterControllerId)
                                                    .getResultList();
            for (T discoveryEntry : capabilitiesList) {
                logger.trace("  --> BEFORE entry {} last seen {}",
                             discoveryEntry.getParticipantId(),
                             discoveryEntry.getLastSeenDateMs());
                ((GlobalDiscoveryEntryPersisted) discoveryEntry).setLastSeenDateMs(System.currentTimeMillis());
                logger.trace("  --> AFTER  entry {} last seen {}",
                             discoveryEntry.getParticipantId(),
                             discoveryEntry.getLastSeenDateMs());
            }
            transaction.commit();
        } catch (RuntimeException e) {
            if (transaction.isActive()) {
                transaction.rollback();
            }
            logger.error("Error updating last seen date for cluster controller with ID {}", clusterControllerId, e);
        }

    }
}
