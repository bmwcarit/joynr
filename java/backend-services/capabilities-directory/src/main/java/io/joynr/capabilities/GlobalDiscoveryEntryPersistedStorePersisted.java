/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS;

import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;

import jakarta.persistence.EntityManager;
import jakarta.persistence.EntityTransaction;
import jakarta.persistence.LockModeType;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.Singleton;
import com.google.inject.name.Named;
import com.google.inject.persist.PersistService;

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
    private final long defaultExpiryTimeMs;

    @Inject
    public GlobalDiscoveryEntryPersistedStorePersisted(CapabilitiesProvisioning staticProvisioning,
                                                       Provider<EntityManager> entityManagerProvider,
                                                       PersistService persistService,
                                                       @Named(PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS) String defaultExpiryTimeMs) {
        persistService.start();
        entityManager = entityManagerProvider.get();
        this.defaultExpiryTimeMs = Long.parseLong(defaultExpiryTimeMs);

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
            entityManager.createNativeQuery("LOCK TABLE discovery_entries in ACCESS EXCLUSIVE MODE").executeUpdate();
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
            entityManager.clear();
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
        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            entityManager.createNativeQuery("LOCK TABLE discovery_entries in ACCESS EXCLUSIVE MODE").executeUpdate();

            for (String gbid : gbids) {
                GlobalDiscoveryEntryPersistedKey key = new GlobalDiscoveryEntryPersistedKey();
                key.setGbid(gbid);
                key.setParticipantId(participantId);
                GlobalDiscoveryEntryPersisted entity = entityManager.find(GlobalDiscoveryEntryPersisted.class, key);
                if (entity != null) {
                    entityManager.remove(entity);
                    deletedCount++;
                }
            }

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
        List<GlobalDiscoveryEntryPersisted> queryResult = null;
        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            queryResult = entityManager.createQuery(queryString, GlobalDiscoveryEntryPersisted.class)
                                       .setLockMode(LockModeType.PESSIMISTIC_READ)
                                       .setParameter("domains", new HashSet<String>(Arrays.asList(domains)))
                                       .setParameter("interfaceName", interfaceName)
                                       .getResultList();
            transaction.commit();
            entityManager.clear();
            logger.trace("Lookup({}, {}) committed successfully", Arrays.toString(domains), interfaceName);
        } catch (Exception e) {
            logger.error("Lookup({}, {}) failed.", Arrays.toString(domains), interfaceName, e);
        } finally {
            if (transaction.isActive()) {
                logger.error("Lookup({}, {}): rollback.", Arrays.toString(domains), interfaceName);
                transaction.rollback();
            }
        }
        return queryResult;
    }

    @Override
    public synchronized Optional<Collection<GlobalDiscoveryEntryPersisted>> lookup(String participantId) {
        String queryString = "FROM GlobalDiscoveryEntryPersisted gdep WHERE " + "gdep.participantId = :participantId";
        Collection<GlobalDiscoveryEntryPersisted> queryResult = null;
        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            queryResult = entityManager.createQuery(queryString, GlobalDiscoveryEntryPersisted.class)
                                       .setLockMode(LockModeType.PESSIMISTIC_READ)
                                       .setParameter("participantId", participantId)
                                       .getResultList();

            transaction.commit();
            entityManager.clear();
            logger.trace("Lookup({}) committed successfully", participantId);
        } catch (Exception e) {
            logger.error("Lookup({}) failed.", participantId, e);
        } finally {
            if (transaction.isActive()) {
                logger.error("Lookup({}): rollback.", participantId);
                transaction.rollback();
            }
        }
        return Optional.ofNullable(queryResult);
    }

    @Override
    public synchronized void touch(String clusterControllerId) {
        String query = "FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.clusterControllerId = :clusterControllerId";
        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            entityManager.createNativeQuery("LOCK TABLE discovery_entries in ACCESS EXCLUSIVE MODE").executeUpdate();
            @SuppressWarnings("unchecked")
            List<GlobalDiscoveryEntryPersisted> capabilitiesList = entityManager.createQuery(query)
                                                                                .setParameter("clusterControllerId",
                                                                                              clusterControllerId)
                                                                                .getResultList();
            touchEntries(capabilitiesList);
            transaction.commit();
            entityManager.clear();
            logger.trace("Touch(ccId={}) committed successfully.", clusterControllerId);
        } catch (RuntimeException e) {
            logger.error("Touch(ccId={}) failed.", clusterControllerId, e);
            throw e;
        } finally {
            if (transaction.isActive()) {
                logger.error("Touch(ccId={}): rollback.", clusterControllerId);
                transaction.rollback();
            }
        }
    }

    private void touchEntries(List<GlobalDiscoveryEntryPersisted> capabilitiesList) {
        long now = System.currentTimeMillis();
        for (GlobalDiscoveryEntryPersisted discoveryEntry : capabilitiesList) {
            long previousLastSeenDateMs = discoveryEntry.getLastSeenDateMs();
            long previousExpiryDateMs = discoveryEntry.getExpiryDateMs();
            discoveryEntry.setLastSeenDateMs(now);
            discoveryEntry.setExpiryDateMs(now + defaultExpiryTimeMs);
            logger.trace("Updated discovery entry for participantId {}, last seen date old={} -> new={}, expiry date old={} -> new={}.",
                         discoveryEntry.getParticipantId(),
                         previousLastSeenDateMs,
                         discoveryEntry.getLastSeenDateMs(),
                         previousExpiryDateMs,
                         discoveryEntry.getExpiryDateMs());
        }
    }

    @Override
    public synchronized void touch(String clusterControllerId, String[] participantIds) {
        if (participantIds.length == 0) {
            logger.trace("Touch(ccId={}, participantIds={}): nothing to do, no participantIds provided.",
                         clusterControllerId,
                         participantIds);
            return;
        }
        String query = "FROM GlobalDiscoveryEntryPersisted gdep "
                + "WHERE gdep.clusterControllerId = :clusterControllerId AND gdep.participantId IN :participantIds";
        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            entityManager.createNativeQuery("LOCK TABLE discovery_entries in ACCESS EXCLUSIVE MODE").executeUpdate();
            @SuppressWarnings("unchecked")
            List<GlobalDiscoveryEntryPersisted> capabilitiesList = entityManager.createQuery(query)
                                                                                .setParameter("clusterControllerId",
                                                                                              clusterControllerId)
                                                                                .setParameter("participantIds",
                                                                                              new HashSet<>(Arrays.asList(participantIds)))
                                                                                .getResultList();
            touchEntries(capabilitiesList);
            transaction.commit();
            entityManager.clear();
            if (participantIds.length > capabilitiesList.size()) {
                logger.warn("Touch(ccId={}, participantIds={}) committed successfully, but updated only {} entries: {}.",
                            clusterControllerId,
                            participantIds,
                            capabilitiesList.size(),
                            capabilitiesList);
            } else {
                logger.trace("Touch(ccId={}, participantIds={}) committed successfully.",
                             clusterControllerId,
                             participantIds);
            }
        } catch (RuntimeException e) {
            logger.error("Touch(ccId={}, participantIds={}) failed.", clusterControllerId, participantIds, e);
            throw e;
        } finally {
            if (transaction.isActive()) {
                logger.error("Touch(ccId={}, participantIds={}): rollback.", clusterControllerId, participantIds);
                transaction.rollback();
            }
        }

    }

    @Override
    public synchronized int removeStale(String clusterControllerId, Long maxLastSeenDateMs) {
        int deletedCount = 0;
        String queryString = "DELETE FROM GlobalDiscoveryEntryPersisted gdep "
                + "WHERE gdep.clusterControllerId = :clusterControllerId AND gdep.lastSeenDateMs < :maxLastSeenDateMs";

        EntityTransaction transaction = entityManager.getTransaction();
        try {
            transaction.begin();
            entityManager.createNativeQuery("LOCK TABLE discovery_entries in ACCESS EXCLUSIVE MODE").executeUpdate();
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
