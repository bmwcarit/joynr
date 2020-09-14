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
package io.joynr.discovery.jee;

import static io.joynr.capabilities.directory.CapabilitiesDirectoryImpl.GCD_GBID;
import static io.joynr.capabilities.directory.CapabilitiesDirectoryImpl.VALID_GBIDS;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import javax.ejb.Stateless;
import javax.inject.Inject;
import javax.inject.Named;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.persistence.TypedQuery;
import javax.transaction.Transactional;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import io.joynr.capabilities.directory.util.GcdUtilities;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectorySubscriptionPublisher;
import joynr.infrastructure.GlobalCapabilitiesDirectorySync;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;

@Stateless
@ServiceProvider(serviceInterface = GlobalCapabilitiesDirectorySync.class)
@Transactional(rollbackOn = Exception.class)
public class GlobalCapabilitiesDirectoryEjb implements GlobalCapabilitiesDirectoryService {
    private static final Logger logger = LoggerFactory.getLogger(GlobalCapabilitiesDirectoryEjb.class);
    @PersistenceContext(unitName = "joynr-discovery-directory")
    private EntityManager entityManager;
    @SuppressWarnings("unused")
    private GlobalCapabilitiesDirectorySubscriptionPublisher gcdSubPublisher;

    private String gcdGbid;
    private Set<String> validGbids;
    private long defaultExpiryTimeMs;

    @Inject
    public GlobalCapabilitiesDirectoryEjb(@SubscriptionPublisher GlobalCapabilitiesDirectorySubscriptionPublisher gcdSubPublisher,
                                          @Named(GCD_GBID) String gcdGbid,
                                          @Named(VALID_GBIDS) String validGbidsString,
                                          @Named(PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS) long defaultExpiryTimeMs) {
        this.gcdSubPublisher = gcdSubPublisher;
        this.gcdGbid = gcdGbid;
        this.validGbids = GcdUtilities.convertArrayStringToSet(validGbidsString, gcdGbid);
        this.defaultExpiryTimeMs = defaultExpiryTimeMs;
    }

    @Override
    public synchronized void add(GlobalDiscoveryEntry[] globalDiscoveryEntries) {
        for (GlobalDiscoveryEntry entry : globalDiscoveryEntries) {
            if (entry != null) {
                add(entry);
            } else {
                logger.trace("Ignoring null entry passed in as part of array.");
            }
        }
    }

    @Override
    public synchronized void add(GlobalDiscoveryEntry globalDiscoveryEntry) {
        logger.debug("Adding global discovery entry to own gbid {}: {}", gcdGbid, globalDiscoveryEntry);
        if (!addInternal(globalDiscoveryEntry, gcdGbid)) {
            throw new ProviderRuntimeException("INTERNAL_ERROR: Unable to add DiscoveryEntry for "
                    + globalDiscoveryEntry.getParticipantId());
        }
    }

    private boolean addInternal(GlobalDiscoveryEntry globalDiscoveryEntry, String... gbids) {
        if (globalDiscoveryEntry.getDomain() == null || globalDiscoveryEntry.getInterfaceName() == null
                || globalDiscoveryEntry.getParticipantId() == null || globalDiscoveryEntry.getAddress() == null) {
            final String msg = String.format("DiscoveryEntry being registered is incomplete: %s", globalDiscoveryEntry);
            logger.error(msg);
            throw new ProviderRuntimeException(msg);
        }

        Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);
        String clusterControllerId = "";
        if (address instanceof MqttAddress) {
            // not always the clusterControllerId. If a unicast topic prefix is set, this clusterControllerId is a part of the topic
            clusterControllerId = ((MqttAddress) address).getTopic();
        } else if (address instanceof ChannelAddress) {
            clusterControllerId = ((ChannelAddress) address).getChannelId();
        } else {
            logger.error("Error adding DiscoveryEntry for participantId {}. Unknown address type: {}",
                         globalDiscoveryEntry.getParticipantId(),
                         globalDiscoveryEntry.getAddress());
            throw new ProviderRuntimeException("Unable to add DiscoveryEntry for participantId "
                    + globalDiscoveryEntry.getParticipantId() + ". Unknown address type: "
                    + globalDiscoveryEntry.getAddress());
        }

        String participantId = globalDiscoveryEntry.getParticipantId();
        String queryString = "FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.participantId = :participantId AND gdep.gbid = :gbid";
        try {
            for (String gbid : gbids) {
                String toAddGbid;
                if (gbid.isEmpty()) {
                    logger.warn("Received add with empty gbid for participantId: {} treating as ownGbid.",
                                participantId);
                    toAddGbid = gcdGbid;
                } else {
                    toAddGbid = gbid;
                }

                List<GlobalDiscoveryEntryPersisted> queryResult = entityManager.createQuery(queryString,
                                                                                            GlobalDiscoveryEntryPersisted.class)
                                                                               .setParameter("participantId",
                                                                                             participantId)
                                                                               .setParameter("gbid", toAddGbid)
                                                                               .getResultList();

                if (queryResult.size() > 1) {
                    logger.error("There should be max only one discovery entry! Found {} discovery entries: {}",
                                 queryResult.size(),
                                 queryResult);
                    throw new JoynrIllegalStateException("There should be max only one discovery entry! Found "
                            + queryResult.size() + " discovery entries");
                }

                if (address instanceof MqttAddress) {
                    ((MqttAddress) address).setBrokerUri(toAddGbid);
                    globalDiscoveryEntry.setAddress(RoutingTypesUtil.toAddressString(address));
                }
                GlobalDiscoveryEntryPersisted entity = new GlobalDiscoveryEntryPersisted(globalDiscoveryEntry,
                                                                                         clusterControllerId,
                                                                                         toAddGbid);
                if (queryResult.isEmpty()) {
                    logger.trace("Adding new discoveryEntry {} to the persisted entries.", globalDiscoveryEntry);
                    entityManager.persist(entity);
                } else {
                    logger.trace("Merging discoveryEntry {} to the persisted entries.", globalDiscoveryEntry);
                    entityManager.merge(entity);
                }
            }
        } catch (Exception e) {
            logger.error("Error adding discoveryEntry for participantId {} and gbids {}:",
                         participantId,
                         Arrays.toString(gbids),
                         e);
            return false;
        }
        return true;
    }

    @Override
    public synchronized void add(GlobalDiscoveryEntry globalDiscoveryEntry,
                                 String[] gbids) throws ApplicationException {
        logger.debug("Adding global discovery entry to {}: {}", Arrays.toString(gbids), globalDiscoveryEntry);
        switch (GcdUtilities.validateGbids(gbids, gcdGbid, validGbids)) {
        case INVALID:
            throw new ApplicationException(DiscoveryError.INVALID_GBID);
        case UNKNOWN:
            throw new ApplicationException(DiscoveryError.UNKNOWN_GBID);
        case OK:
            break;
        default:
            break;
        }
        if (!addInternal(globalDiscoveryEntry, gbids)) {
            throw new ApplicationException(DiscoveryError.INTERNAL_ERROR);
        }
    }

    @Override
    public void fireGlobalDiscoveryEntryChanged() {
        throw new UnsupportedOperationException("Not implemented yet");
    }

    @Override
    public synchronized GlobalDiscoveryEntry[] lookup(String[] domains, String interfaceName) {
        logger.debug("Looking up global discovery entries for domains {} and interfaceName {} and own Gbid {}",
                     Arrays.toString(domains),
                     interfaceName,
                     gcdGbid);

        String[] gcdGbidArray = { gcdGbid };
        GlobalDiscoveryEntry[] globalDiscoveryEntries = null;
        try {
            globalDiscoveryEntries = lookup(domains, interfaceName, gcdGbidArray);
        } catch (ApplicationException e) {
            logger.error("Error looking up global discovery entries for domains {} and interfaceName {} and own Gbid {}:",
                         Arrays.toString(domains),
                         interfaceName,
                         gcdGbid,
                         e);
            throw new ProviderRuntimeException("Error on lookup: " + e.getError());
        }
        return globalDiscoveryEntries;
    }

    @Override
    public synchronized GlobalDiscoveryEntry[] lookup(String[] domains,
                                                      String interfaceName,
                                                      String[] gbids) throws ApplicationException {
        logger.debug("Looking up global discovery entries for domains {} and interfaceName {} and Gbids {}",
                     Arrays.toString(domains),
                     interfaceName,
                     Arrays.toString(gbids));

        switch (GcdUtilities.validateGbids(gbids, gcdGbid, validGbids)) {
        case INVALID:
            throw new ApplicationException(DiscoveryError.INVALID_GBID);
        case UNKNOWN:
            throw new ApplicationException(DiscoveryError.UNKNOWN_GBID);
        case OK:
            break;
        default:
            break;
        }

        String[] adaptedGbidArray = gbids.clone();
        for (int i = 0; i < adaptedGbidArray.length; i++) {
            if (adaptedGbidArray[i].isEmpty()) {
                logger.warn("Received lookup with empty gbid for domains {} and interfaceName {}, treating as ownGbid.",
                            Arrays.toString(domains),
                            interfaceName);

                adaptedGbidArray[i] = gcdGbid;
            }
        }

        String queryString = "FROM GlobalDiscoveryEntryPersisted gdep "
                + "WHERE gdep.domain IN :domains AND gdep.interfaceName = :interfaceName AND gdep.gbid IN :gbids "
                + "ORDER BY gdep.participantId";
        try {
            List<GlobalDiscoveryEntryPersisted> queryResult = entityManager.createQuery(queryString,
                                                                                        GlobalDiscoveryEntryPersisted.class)
                                                                           .setParameter("domains",
                                                                                         new HashSet<String>(Arrays.asList(domains)))
                                                                           .setParameter("interfaceName", interfaceName)
                                                                           .setParameter("gbids",
                                                                                         Arrays.asList(adaptedGbidArray))
                                                                           .getResultList();

            if (queryResult.isEmpty()) {
                String queryCountString = "SELECT count(gdep) FROM GlobalDiscoveryEntryPersisted gdep " + "WHERE "
                        + "gdep.domain IN :domains AND gdep.interfaceName = :interfaceName";
                long numberOfEntriesInAllGbids = entityManager.createQuery(queryCountString, Long.class)
                                                              .setParameter("domains",
                                                                            new HashSet<String>(Arrays.asList(domains)))
                                                              .setParameter("interfaceName", interfaceName)
                                                              .getSingleResult();
                if (numberOfEntriesInAllGbids > 0) {

                    throw new ApplicationException(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
                }
            }

            GlobalDiscoveryEntry[] globalDiscoveryEntriesArray = GcdUtilities.chooseOneGlobalDiscoveryEntryPerParticipantId(queryResult,
                                                                                                                            gcdGbid);
            return globalDiscoveryEntriesArray;
        } catch (ApplicationException e) {
            throw e;
        } catch (Exception e) {
            logger.error("Error looking up global discovery entries for domains {} and interfaceName {} and Gbids {}: {}",
                         Arrays.toString(domains),
                         interfaceName,
                         Arrays.toString(gbids),
                         e);
            throw new ApplicationException(DiscoveryError.INTERNAL_ERROR);
        }
    }

    @Override
    public synchronized GlobalDiscoveryEntry lookup(String participantId) {
        logger.debug("Looking up global discovery entry for participantId {} and own Gbid {}", participantId, gcdGbid);
        String[] gcdGbidArray = { gcdGbid };
        try {
            return lookup(participantId, gcdGbidArray);
        } catch (ApplicationException e) {
            logger.error("Error looking up global discovery entry for participantId {} and own Gbid {}:",
                         participantId,
                         gcdGbid,
                         e);
            throw new ProviderRuntimeException("Error on lookup: " + e.getError());
        }
    }

    @Override
    public synchronized GlobalDiscoveryEntry lookup(String participantId, String[] gbids) throws ApplicationException {
        logger.debug("Looking up global discovery entry for participantId {} and Gbids {}",
                     participantId,
                     Arrays.toString(gbids));
        switch (GcdUtilities.validateGbids(gbids, gcdGbid, validGbids)) {
        case INVALID:
            throw new ApplicationException(DiscoveryError.INVALID_GBID);
        case UNKNOWN:
            throw new ApplicationException(DiscoveryError.UNKNOWN_GBID);
        case OK:
            break;
        default:
            break;
        }

        String[] addaptedGbidArray = gbids.clone();
        for (int i = 0; i < addaptedGbidArray.length; i++) {
            if (addaptedGbidArray[i].isEmpty()) {
                logger.warn("Received lookup with empty gbid for for participantId {}, treating as ownGbid",
                            participantId);

                addaptedGbidArray[i] = gcdGbid;
            }
        }

        try {
            String queryString = "FROM GlobalDiscoveryEntryPersisted gdep WHERE "
                    + "gdep.participantId = :participantId AND " + "gdep.gbid IN :gbids";

            List<GlobalDiscoveryEntryPersisted> queryResult = entityManager.createQuery(queryString,
                                                                                        GlobalDiscoveryEntryPersisted.class)
                                                                           .setParameter("participantId", participantId)
                                                                           .setParameter("gbids",
                                                                                         Arrays.asList(addaptedGbidArray))
                                                                           .getResultList();

            if (queryResult.isEmpty()) {
                String queryCountString = "SELECT count(gdep) FROM GlobalDiscoveryEntryPersisted gdep " + "WHERE "
                        + "gdep.participantId = :participantId";
                long numberOfEntriesInAllGbids = entityManager.createQuery(queryCountString, Long.class)
                                                              .setParameter("participantId", participantId)
                                                              .getSingleResult();
                if (numberOfEntriesInAllGbids > 0) {
                    throw new ApplicationException(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
                } else {
                    throw new ApplicationException(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
                }
            }

            return GcdUtilities.chooseOneGlobalDiscoveryEntry(queryResult, gcdGbid);
        } catch (ApplicationException e) {
            throw e;
        } catch (Exception e) {
            logger.error("Error looking up global discovery entry for participantId {} and Gbids {}:",
                         participantId,
                         Arrays.toString(gbids),
                         e);
            throw new ApplicationException(DiscoveryError.INTERNAL_ERROR);
        }
    }

    @Override
    public synchronized void remove(String[] participantIds) {
        int deletedCount = removeInternal(participantIds);
        logger.debug("Deleted {} entries (number of IDs passed in {})", deletedCount, participantIds.length);
    }

    private int removeInternal(String[] participantIds, String... gbids) {
        int deletedCount = 0;

        String[] addaptedGbidArray = gbids.clone();
        for (int i = 0; i < addaptedGbidArray.length; i++) {
            if (addaptedGbidArray[i].isEmpty()) {
                logger.warn("Remove participantIds {} with empty gbid among Gbids {}, defaulting to ownGbid",
                            participantIds,
                            Arrays.toString(gbids));

                addaptedGbidArray[i] = gcdGbid;
            }
        }

        if (gbids.length > 0) {
            logger.debug("Removing global discovery entries with participantIds {} from gbids {}",
                         Arrays.toString(participantIds),
                         Arrays.toString(addaptedGbidArray));
            String queryString = "DELETE FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.participantId IN :participantIds AND gdep.gbid IN :gbids";
            deletedCount = entityManager.createQuery(queryString, GlobalDiscoveryEntryPersisted.class)
                                        .setParameter("participantIds",
                                                      new HashSet<String>(Arrays.asList(participantIds)))
                                        .setParameter("gbids", new HashSet<String>(Arrays.asList(addaptedGbidArray)))
                                        .executeUpdate();
        } else {
            logger.debug("Removing global discovery entries with participantIds {} from own Gbid {}",
                         Arrays.toString(participantIds),
                         gcdGbid);
            String queryString = "DELETE FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.participantId IN :participantIds AND gdep.gbid = :gbid";
            deletedCount = entityManager.createQuery(queryString, GlobalDiscoveryEntryPersisted.class)
                                        .setParameter("participantIds",
                                                      new HashSet<String>(Arrays.asList(participantIds)))
                                        .setParameter("gbid", gcdGbid)
                                        .executeUpdate();
        }

        return deletedCount;
    }

    @Override
    public synchronized void remove(String participantId) {
        remove(new String[]{ participantId });
    }

    @Override
    public synchronized void remove(String participantId, String[] gbids) throws ApplicationException {
        switch (GcdUtilities.validateGbids(gbids, gcdGbid, validGbids)) {
        case INVALID:
            logger.error("Error removing participantId {}: INVALID GBIDs: {}", participantId, Arrays.toString(gbids));
            throw new ApplicationException(DiscoveryError.INVALID_GBID);
        case UNKNOWN:
            logger.error("Error removing participantId {}: UNKNOWN_GBID: {}", participantId, Arrays.toString(gbids));
            throw new ApplicationException(DiscoveryError.UNKNOWN_GBID);
        case OK:
            break;
        default:
            break;
        }
        int deletedCount;
        long numberOfEntriesInAllGbids = 0;
        try {
            deletedCount = removeInternal(new String[]{ participantId }, gbids);
            if (deletedCount == 0) {
                logger.warn("Error removing participantId {}. Participant is not registered in GBIDs {}.",
                            participantId,
                            Arrays.toString(gbids));

                String queryCountString = "SELECT count(gdep) FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.participantId = :participantId";
                numberOfEntriesInAllGbids = entityManager.createQuery(queryCountString, Long.class)
                                                         .setParameter("participantId", participantId)
                                                         .getSingleResult();
            } else {
                logger.debug("Deleted {} entries for participantId {}", deletedCount, participantId);
            }
        } catch (Exception e) {
            logger.error("Error removing discoveryEntry for participantId {} and gbids {}:",
                         participantId,
                         Arrays.toString(gbids),
                         e);
            throw new ApplicationException(DiscoveryError.INTERNAL_ERROR);
        }
        if (deletedCount == 0) {
            if (numberOfEntriesInAllGbids > 0) {
                throw new ApplicationException(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
            } else {
                throw new ApplicationException(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
            }
        }
    }

    @Override
    public synchronized void touch(String clusterControllerId) {
        logger.debug("Touch called. Updating discovery entries from cluster controller with id {}",
                     clusterControllerId);
        String queryString = "FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.clusterControllerId = :clusterControllerId";
        long now = System.currentTimeMillis();
        TypedQuery<GlobalDiscoveryEntryPersisted> query = entityManager.createQuery(queryString,
                                                                                    GlobalDiscoveryEntryPersisted.class);
        query.setParameter("clusterControllerId", clusterControllerId);
        for (GlobalDiscoveryEntryPersisted globalDiscoveryEntryPersisted : query.getResultList()) {
            globalDiscoveryEntryPersisted.setLastSeenDateMs(now);
            globalDiscoveryEntryPersisted.setExpiryDateMs(now + defaultExpiryTimeMs);
        }
    }

    @Override
    public synchronized void touch(String clusterControllerId, String[] participantIds) {
        logger.debug("Touch called. Updating discovery entries from cluster controller with id={}, participantIds={}.",
                     clusterControllerId,
                     participantIds);
        if (participantIds.length == 0) {
            logger.info("Touch(ccId={}, participantIds={}) succeeded: nothing to do, no participantIds provided.",
                        clusterControllerId,
                        participantIds);
            return;
        }
        String queryString = "UPDATE GlobalDiscoveryEntryPersisted gdep "
                + "SET gdep.lastSeenDateMs = :newLastSeenDateMs, gdep.expiryDateMs = :newExpiryDateMs "
                + "WHERE gdep.clusterControllerId = :clusterControllerId AND gdep.participantId IN :participantIds";
        long now = System.currentTimeMillis();
        int updatedCount = entityManager.createQuery(queryString, GlobalDiscoveryEntryPersisted.class)
                                        .setParameter("clusterControllerId", clusterControllerId)
                                        .setParameter("participantIds",
                                                      new HashSet<String>(Arrays.asList(participantIds)))
                                        .setParameter("newLastSeenDateMs", now)
                                        .setParameter("newExpiryDateMs", now + defaultExpiryTimeMs)
                                        .executeUpdate();
        entityManager.flush();
        entityManager.clear();
        if (participantIds.length > updatedCount) {
            logger.warn("Touch(ccId={}, participantIds={}) succeeded, but updated only {} entries.",
                        clusterControllerId,
                        participantIds,
                        updatedCount);
        } else {
            logger.info("Touch(ccId={}, participantIds={}) succeeded.", clusterControllerId, participantIds);
        }
    }

    @Override
    public synchronized void removeStale(String clusterControllerId, Long maxLastSeenDateMs) {
        logger.debug("RemoveStale called. Removing stale entries for ccId={}, maxLastSeenDateMs={}.",
                     clusterControllerId,
                     maxLastSeenDateMs);
        String queryString = "DELETE FROM GlobalDiscoveryEntryPersisted gdep "
                + "WHERE gdep.clusterControllerId = :clusterControllerId AND gdep.lastSeenDateMs < :maxLastSeenDateMs";
        try {
            int deletedCount = entityManager.createQuery(queryString, GlobalDiscoveryEntryPersisted.class)
                                            .setParameter("clusterControllerId", clusterControllerId)
                                            .setParameter("maxLastSeenDateMs", maxLastSeenDateMs)
                                            .executeUpdate();
            logger.info("Deleted {} stale entries for ccId={}, maxLastSeenDateMs={}.",
                        deletedCount,
                        clusterControllerId,
                        maxLastSeenDateMs);
        } catch (Exception e) {
            logger.error("RemoveStale for ccId={}, maxLastSeenDateMs={} failed.",
                         clusterControllerId,
                         maxLastSeenDateMs,
                         e);
            throw new ProviderRuntimeException("RemoveStale failed: " + e.toString());
        }
    }

}
