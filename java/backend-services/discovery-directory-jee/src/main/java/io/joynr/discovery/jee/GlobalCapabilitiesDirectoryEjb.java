package io.joynr.discovery.jee;

import java.util.Arrays;

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

import java.util.List;
import java.util.stream.Collectors;

import javax.ejb.Stateless;
import javax.inject.Inject;
import javax.persistence.EntityManager;
import javax.persistence.TypedQuery;
import javax.transaction.Transactional;

import com.google.common.collect.Sets;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import io.joynr.jeeintegration.api.ServiceProvider;
import joynr.infrastructure.GlobalCapabilitiesDirectorySync;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.GlobalDiscoveryEntry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@Stateless
@ServiceProvider(serviceInterface = GlobalCapabilitiesDirectorySync.class)
@Transactional
public class GlobalCapabilitiesDirectoryEjb implements GlobalCapabilitiesDirectorySync {

    private static final Logger logger = LoggerFactory.getLogger(GlobalCapabilitiesDirectoryEjb.class);

    private EntityManager entityManager;

    @Inject
    public GlobalCapabilitiesDirectoryEjb(EntityManager entityManager) {
        this.entityManager = entityManager;
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
        Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);
        String clusterControllerId;
        if (address instanceof MqttAddress) {
            clusterControllerId = ((MqttAddress) address).getTopic();
        } else if (address instanceof ChannelAddress) {
            clusterControllerId = ((ChannelAddress) address).getChannelId();
        } else {
            clusterControllerId = String.valueOf(address);
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
    public GlobalDiscoveryEntry[] lookup(String[] domains, String interfaceName) {
        logger.debug("Looking up global discovery entries for domains {} and interface name {}", domains, interfaceName);
        String queryString = "from GlobalDiscoveryEntryPersisted gdep where gdep.domain in :domains and gdep.interfaceName = :interfaceName";
        List<GlobalDiscoveryEntryPersisted> queryResult = entityManager.createQuery(queryString,
                                                                                    GlobalDiscoveryEntryPersisted.class)
                                                                       .setParameter("domains",
                                                                                     Sets.newHashSet(domains))
                                                                       .setParameter("interfaceName", interfaceName)
                                                                       .getResultList();
        logger.debug("Found discovery entries: {}", queryResult);
        return queryResult.stream().map(entry -> { return new GlobalDiscoveryEntry(entry); }).collect(Collectors.toSet()).toArray(new GlobalDiscoveryEntry[queryResult.size()]);
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
    public void remove(String[] participantIds) {
        logger.debug("Removing global discovery entries with IDs {}", Arrays.toString(participantIds));
        String queryString = "delete from GlobalDiscoveryEntryPersisted gdep where gdep.participantId in :participantIds";
        int deletedCount = entityManager.createQuery(queryString, GlobalDiscoveryEntryPersisted.class)
                                        .setParameter("participantIds", Sets.newHashSet(participantIds))
                                        .executeUpdate();
        logger.debug("Deleted {} entries (number of IDs passed in {})", deletedCount, participantIds.length);
    }

    @Override
    public void remove(String participantId) {
        remove(new String[]{ participantId });
    }

    @Override
    public void touch(String clusterControllerId) {
        logger.debug("Touch called. Updating discovery entries from cluster controller with id: " + clusterControllerId);
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
