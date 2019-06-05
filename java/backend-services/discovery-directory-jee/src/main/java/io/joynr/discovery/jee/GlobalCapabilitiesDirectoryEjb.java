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

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Properties;
import java.util.stream.Collectors;

import javax.ejb.Stateless;
import javax.inject.Inject;
import javax.persistence.EntityManager;
import javax.persistence.TypedQuery;
import javax.transaction.Transactional;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import io.joynr.capabilities.directory.CapabilitiesDirectoryImpl;
import io.joynr.capabilities.directory.util.GcdUtilities;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import io.joynr.runtime.PropertyLoader;
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
@Transactional
public class GlobalCapabilitiesDirectoryEjb implements GlobalCapabilitiesDirectoryService {
    private static final Logger logger = LoggerFactory.getLogger(GlobalCapabilitiesDirectoryEjb.class);
    private EntityManager entityManager;
    @SuppressWarnings("unused")
    private GlobalCapabilitiesDirectorySubscriptionPublisher gcdSubPublisher;

    private String gcdGbid;

    @Inject
    public GlobalCapabilitiesDirectoryEjb(EntityManager entityManager,
                                          @SubscriptionPublisher GlobalCapabilitiesDirectorySubscriptionPublisher gcdSubPublisher) {
        this.entityManager = entityManager;
        this.gcdSubPublisher = gcdSubPublisher;
        this.gcdGbid = getGcdGbid();
    }

    private String getGcdGbid() {
        Properties envPropertiesAll = new Properties();
        envPropertiesAll.putAll(System.getenv());
        String gcdGbid = PropertyLoader.getPropertiesWithPattern(envPropertiesAll, CapabilitiesDirectoryImpl.GCD_GBID)
                                       .getProperty(CapabilitiesDirectoryImpl.GCD_GBID);
        if (gcdGbid == null || gcdGbid.isEmpty()) {
            gcdGbid = GcdUtilities.loadDefaultGbidsFromDefaultMessagingProperties()[0];
        }
        return gcdGbid;
    }

    @Override
    public void add(GlobalDiscoveryEntry[] globalDiscoveryEntries) {
        for (GlobalDiscoveryEntry entry : globalDiscoveryEntries) {
            if (entry != null) {
                add(entry);
            } else {
                logger.trace("Ignoring null entry passed in as part of array.");
            }
        }
    }

    @Override
    public void add(GlobalDiscoveryEntry globalDiscoveryEntry) {
        logger.debug("Adding global discovery entry {}", globalDiscoveryEntry);
        addInternal(globalDiscoveryEntry, gcdGbid);
    }

    private void addInternal(GlobalDiscoveryEntry globalDiscoveryEntry, String... gbids) {
        assert (gbids.length > 0);
        Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);
        String clusterControllerId = "";
        if (address instanceof MqttAddress) {
            clusterControllerId = ((MqttAddress) address).getTopic();
            ((MqttAddress) address).setBrokerUri(gbids[0]);
            globalDiscoveryEntry.setAddress(RoutingTypesUtil.toAddressString(address));
        } else if (address instanceof ChannelAddress) {
            clusterControllerId = ((ChannelAddress) address).getChannelId();
        } else {
            logger.error("Error adding DiscoveryEntry: " + globalDiscoveryEntry.getParticipantId()
                    + ". Unknown address type: " + globalDiscoveryEntry.getAddress());
            throw new ProviderRuntimeException("Unable to add DiscoveryEntry for "
                    + globalDiscoveryEntry.getParticipantId() + ". Unknown address type: "
                    + globalDiscoveryEntry.getAddress());
        }
        GlobalDiscoveryEntryPersisted entity = new GlobalDiscoveryEntryPersisted(globalDiscoveryEntry,
                                                                                 clusterControllerId);
        GlobalDiscoveryEntryPersisted persisted = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                     entity.getParticipantId());
        if (persisted == null) {
            entityManager.persist(entity);
        } else {
            entityManager.merge(entity);
        }
    }

    @Override
    public void add(GlobalDiscoveryEntry globalDiscoveryEntry, String[] gbids) throws ApplicationException {
        switch (GcdUtilities.validateGbids(gbids, gcdGbid)) {
        case INVALID:
            throw new ApplicationException(DiscoveryError.INVALID_GBID);
        case UNKNOWN:
            throw new ApplicationException(DiscoveryError.UNKNOWN_GBID);
        case OK:
            try {
                addInternal(globalDiscoveryEntry, gbids);
            } catch (ProviderRuntimeException e) {
                logger.error("Error adding DiscoveryEntry: {}", e);
                throw e;
            }
        }
    }

    @Override
    public void fireGlobalDiscoveryEntryChanged() {
        throw new UnsupportedOperationException("Not implemented yet");
    }

    @Override
    public GlobalDiscoveryEntry[] lookup(String[] domains, String interfaceName) {
        logger.debug("Looking up global discovery entries for domains {} and interface name {}",
                     domains,
                     interfaceName);
        String queryString = "from GlobalDiscoveryEntryPersisted gdep where gdep.domain in :domains and gdep.interfaceName = :interfaceName";
        List<GlobalDiscoveryEntryPersisted> queryResult = entityManager.createQuery(queryString,
                                                                                    GlobalDiscoveryEntryPersisted.class)
                                                                       .setParameter("domains",
                                                                                     new HashSet<String>(Arrays.asList(domains)))
                                                                       .setParameter("interfaceName", interfaceName)
                                                                       .getResultList();
        logger.debug("Found discovery entries: {}", queryResult);
        return queryResult.stream().map(entry -> {
            return new GlobalDiscoveryEntry(entry);
        }).collect(Collectors.toSet()).toArray(new GlobalDiscoveryEntry[queryResult.size()]);
    }

    @Override
    public GlobalDiscoveryEntry[] lookup(String[] domains,
                                         String interfaceName,
                                         String[] gbids) throws ApplicationException {
        GlobalDiscoveryEntry[] globalDiscoveryEntries = null;
        switch (GcdUtilities.validateGbids(gbids, gcdGbid)) {
        case INVALID:
            throw new ApplicationException(DiscoveryError.INVALID_GBID);
        case UNKNOWN:
            throw new ApplicationException(DiscoveryError.UNKNOWN_GBID);
        case OK:
            globalDiscoveryEntries = lookup(domains, interfaceName);
        }
        return globalDiscoveryEntries;
    }

    @Override
    public GlobalDiscoveryEntry lookup(String participantId) {
        logger.debug("Looking up global discovery entry for participant ID {}", participantId);
        GlobalDiscoveryEntryPersisted queryResult = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                       participantId);
        logger.debug("Found entry {}", queryResult);
        return queryResult == null ? null : new GlobalDiscoveryEntry(queryResult);
    }

    @Override
    public GlobalDiscoveryEntry lookup(String participantId, String[] gbids) throws ApplicationException {
        GlobalDiscoveryEntry globalDiscoveryEntry = null;
        switch (GcdUtilities.validateGbids(gbids, gcdGbid)) {
        case INVALID:
            throw new ApplicationException(DiscoveryError.INVALID_GBID);
        case UNKNOWN:
            throw new ApplicationException(DiscoveryError.UNKNOWN_GBID);
        case OK:
            globalDiscoveryEntry = lookup(participantId);
        }
        return globalDiscoveryEntry;
    }

    @Override
    public void remove(String[] participantIds) {
        int deletedCount = removeInternal(participantIds);
        logger.debug("Deleted {} entries (number of IDs passed in {})", deletedCount, participantIds.length);
    }

    private int removeInternal(String... participantIds) {
        logger.debug("Removing global discovery entries with IDs {}", Arrays.toString(participantIds));
        String queryString = "delete from GlobalDiscoveryEntryPersisted gdep where gdep.participantId in :participantIds";
        int deletedCount = entityManager.createQuery(queryString, GlobalDiscoveryEntryPersisted.class)
                                        .setParameter("participantIds",
                                                      new HashSet<String>(Arrays.asList(participantIds)))
                                        .executeUpdate();
        return deletedCount;
    }

    @Override
    public void remove(String participantId) {
        remove(new String[]{ participantId });
    }

    @Override
    public void remove(String participantId, String[] gbids) throws ApplicationException {
        switch (GcdUtilities.validateGbids(gbids, gcdGbid)) {
        case INVALID:
            logger.error("Unable to remove participantId {}: INVALID GBIDs: {}", participantId, Arrays.toString(gbids));
            throw new ApplicationException(DiscoveryError.INVALID_GBID);
        case UNKNOWN:
            logger.error("Unable to remove participantId {}: UNKNOWN_GBID: {}", participantId, Arrays.toString(gbids));
            throw new ApplicationException(DiscoveryError.UNKNOWN_GBID);
        case OK:
            int deletedCount = removeInternal(participantId);
            if (deletedCount == 0) {
                logger.error("Participant is not registered, NO_ENTRY_FOR_PARTICIPANT {} to be removed", participantId);
                throw new ApplicationException(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
            } else {
                logger.debug("Deleted {} entries for participantId {})", deletedCount, participantId);
            }
        }
    }

    @Override
    public void touch(String clusterControllerId) {
        logger.debug("Touch called. Updating discovery entries from cluster controller with id: "
                + clusterControllerId);
        String queryString = "select gdep from GlobalDiscoveryEntryPersisted gdep where gdep.clusterControllerId = :clusterControllerId";
        long now = System.currentTimeMillis();
        TypedQuery<GlobalDiscoveryEntryPersisted> query = entityManager.createQuery(queryString,
                                                                                    GlobalDiscoveryEntryPersisted.class);
        query.setParameter("clusterControllerId", clusterControllerId);
        for (GlobalDiscoveryEntryPersisted globalDiscoveryEntryPersisted : query.getResultList()) {
            globalDiscoveryEntryPersisted.setLastSeenDateMs(now);
        }
    }

}
