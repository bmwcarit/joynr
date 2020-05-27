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

import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;

import javax.persistence.EntityManager;
import javax.persistence.EntityTransaction;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.Singleton;
import com.google.inject.persist.PersistService;

import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;

/**
 * The GlobalDiscoveryEntryPersistedStorePersisted stores a list of providers with their global
 * address and the interfaces they offer (GlobalDiscoveryEntryPersisted).
 */
@Singleton
public class GlobalDiscoveryEntryPersistedStorePersisted
        implements GlobalDiscoveryEntryStore<GlobalDiscoveryEntryPersisted> {

    private static final Logger logger = LoggerFactory.getLogger(GlobalDiscoveryEntryPersistedStorePersisted.class);
    private EntityManager entityManager;

    @Inject
    public GlobalDiscoveryEntryPersistedStorePersisted(CapabilitiesProvisioning staticProvisioning,
                                                       Provider<EntityManager> entityManagerProvider,
                                                       PersistService persistService) {
        persistService.start();
        entityManager = entityManagerProvider.get();

        logger.debug("Creating CapabilitiesStore with static provisioning");
    }

    @Override
    public synchronized void add(GlobalDiscoveryEntryPersisted globalDiscoveryEntry, String[] gbids) {
        logger.debug("Adding discovery entry: {}", globalDiscoveryEntry);

        Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);
        String participantId = globalDiscoveryEntry.getParticipantId();
        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            for (String gbid : gbids) {
                GlobalDiscoveryEntryPersistedKey key = new GlobalDiscoveryEntryPersistedKey();
                key.setGbid(gbid);
                key.setParticipantId(participantId);
                GlobalDiscoveryEntryPersisted oldEntity = entityManager.find(GlobalDiscoveryEntryPersisted.class, key);

                GlobalDiscoveryEntryPersisted entity = new GlobalDiscoveryEntryPersisted(globalDiscoveryEntry,
                                                                                         globalDiscoveryEntry.getClusterControllerId(),
                                                                                         gbid);
                if (address instanceof MqttAddress) {
                    ((MqttAddress) address).setBrokerUri(gbid);
                    entity.setAddress(RoutingTypesUtil.toAddressString(address));
                }

                if (oldEntity == null) {
                    logger.trace("Adding new discoveryEntry {} to the persisted entries.", globalDiscoveryEntry);
                    entityManager.persist(entity);
                } else {
                    logger.trace("Merging discoveryEntry {} to the persisted entries.", globalDiscoveryEntry);
                    entityManager.merge(entity);
                }
            }
            transaction.commit();
            logger.trace("Add({}) committed successfully", participantId);
        } catch (Exception e) {
            logger.error("Add({}) failed: ", participantId, e);
            throw e;
        } finally {
            if (transaction.isActive()) {
                logger.error("Add({}): rollback.", participantId);
                transaction.rollback();
            }
        }
    }

    @Override
    public synchronized int remove(String participantId, String[] gbids) {
        int deletedCount = 0;
        String queryString = "DELETE FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.participantId = :participantId AND gdep.gbid IN :gbids";

        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            deletedCount = entityManager.createQuery(queryString)
                                        .setParameter("participantId", participantId)
                                        .setParameter("gbids", new HashSet<String>(Arrays.asList(gbids)))
                                        .executeUpdate();

            if (deletedCount == 0) {
                logger.warn("Error removing participantId {}. Participant is not registered in GBIDs {}.",
                            participantId,
                            Arrays.toString(gbids));

                String queryCountString = "SELECT count(gdep) FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.participantId = :participantId";
                long numberOfEntriesInAllGbids = entityManager.createQuery(queryCountString, Long.class)
                                                              .setParameter("participantId", participantId)
                                                              .getSingleResult();
                if (numberOfEntriesInAllGbids > 0) {
                    // NO_ENTRY_FOR_SELECTED_BACKENDS
                    deletedCount = -1;
                } else {
                    // NO_ENTRY_FOR_PARTICIPANT
                    deletedCount = 0;
                }
            }

            transaction.commit();
            entityManager.clear();
            logger.trace("Remove({}) committed successfully", participantId);
        } catch (Exception e) {
            logger.error("Remove({}) failed.", participantId, e);
        } finally {
            if (transaction.isActive()) {
                logger.error("Remove({}): rollback.", participantId);
                transaction.rollback();
            }
        }

        return deletedCount;
    }

    @Override
    public synchronized Collection<GlobalDiscoveryEntryPersisted> lookup(final String[] domains,
                                                                         final String interfaceName) {
        String queryString = "FROM GlobalDiscoveryEntryPersisted gdep "
                + "WHERE gdep.domain IN :domains AND gdep.interfaceName = :interfaceName "
                + "ORDER BY gdep.participantId";
        List<GlobalDiscoveryEntryPersisted> queryResult = entityManager.createQuery(queryString,
                                                                                    GlobalDiscoveryEntryPersisted.class)
                                                                       .setParameter("domains",
                                                                                     new HashSet<String>(Arrays.asList(domains)))
                                                                       .setParameter("interfaceName", interfaceName)
                                                                       .getResultList();
        return queryResult;
    }

    @Override
    public synchronized Optional<Collection<GlobalDiscoveryEntryPersisted>> lookup(String participantId) {
        String queryString = "FROM GlobalDiscoveryEntryPersisted gdep WHERE " + "gdep.participantId = :participantId";
        Collection<GlobalDiscoveryEntryPersisted> queryResult = entityManager.createQuery(queryString,
                                                                                          GlobalDiscoveryEntryPersisted.class)
                                                                             .setParameter("participantId",
                                                                                           participantId)
                                                                             .getResultList();
        return Optional.ofNullable(queryResult);
    }

    @Override
    public Set<GlobalDiscoveryEntryPersisted> getAllDiscoveryEntries() {
        List<GlobalDiscoveryEntryPersisted> allCapabilityEntries = entityManager.createQuery("FROM GlobalDiscoveryEntryPersisted gdep",
                                                                                             GlobalDiscoveryEntryPersisted.class)
                                                                                .getResultList();
        Set<GlobalDiscoveryEntryPersisted> result = new HashSet<>(allCapabilityEntries);
        logger.debug("Retrieved all discovery entries: {}", result);
        return result;
    }

    @Override
    public boolean hasDiscoveryEntry(GlobalDiscoveryEntryPersisted discoveryEntry) {
        String query = "FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.clusterControllerId = :clusterControllerId";
        String clusterControllerId = discoveryEntry.getClusterControllerId();
        @SuppressWarnings("unchecked")
        List<GlobalDiscoveryEntryPersisted> capabilitiesList = entityManager.createQuery(query)
                                                                            .setParameter("clusterControllerId",
                                                                                          clusterControllerId)
                                                                            .getResultList();

        if (capabilitiesList.size() > 1) {
            logger.warn("There are {} discovery entries for {}",
                        capabilitiesList.size(),
                        discoveryEntry.getParticipantId());
        }
        return discoveryEntry.equals(capabilitiesList.get(0));
    }

    @Override
    public synchronized void touch(String clusterControllerId) {
        String query = "FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.clusterControllerId = :clusterControllerId";
        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            @SuppressWarnings("unchecked")
            List<GlobalDiscoveryEntryPersisted> capabilitiesList = entityManager.createQuery(query)
                                                                                .setParameter("clusterControllerId",
                                                                                              clusterControllerId)
                                                                                .getResultList();
            for (GlobalDiscoveryEntryPersisted discoveryEntry : capabilitiesList) {
                long previousLastSeenDateMs = ((GlobalDiscoveryEntryPersisted) discoveryEntry).getLastSeenDateMs();
                ((GlobalDiscoveryEntryPersisted) discoveryEntry).setLastSeenDateMs(System.currentTimeMillis());
                logger.trace("Updated discovery entry for participantId {}, last seen date old {} -> new {}",
                             discoveryEntry.getParticipantId(),
                             previousLastSeenDateMs,
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

    @Override
    public synchronized void touch(String clusterControllerId, String[] participantIds) {
        final String msg = String.format("Error: touch method for clusterControllerId %s and participantIds %s is not implemented yet.",
                                         clusterControllerId,
                                         Arrays.toString(participantIds));
        logger.error(msg);
        throw new ProviderRuntimeException(msg);
    }

    @Override
    public synchronized int removeStale(String clusterControllerId, Long maxLastSeenDateMs) {
        int deletedCount = 0;
        String queryString = "DELETE FROM GlobalDiscoveryEntryPersisted gdep "
                + "WHERE gdep.clusterControllerId = :clusterControllerId AND gdep.lastSeenDateMs < :maxLastSeenDateMs";

        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            deletedCount = entityManager.createQuery(queryString)
                                        .setParameter("clusterControllerId", clusterControllerId)
                                        .setParameter("maxLastSeenDateMs", maxLastSeenDateMs)
                                        .executeUpdate();
            transaction.commit();
            entityManager.clear();
            logger.trace("RemoveStale(ccId={}, maxLastSeenDateMs={}) committed successfully",
                         clusterControllerId,
                         maxLastSeenDateMs);
        } catch (RuntimeException e) {
            logger.error("RemoveStale(ccId={}, maxLastSeenDateMs={}) failed.",
                         clusterControllerId,
                         maxLastSeenDateMs,
                         e);
            throw e;
        } finally {
            if (transaction.isActive()) {
                logger.error("RemoveStale(ccId={}, maxLastSeenDateMs={}): rollback.",
                             clusterControllerId,
                             maxLastSeenDateMs);
                transaction.rollback();
            }
        }
        return deletedCount;
    }

}
