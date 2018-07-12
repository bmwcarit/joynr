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
package io.joynr.messaging.routing;

import java.util.Map.Entry;
import java.util.concurrent.ConcurrentMap;
import java.util.Iterator;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Maps;
import com.google.inject.Singleton;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.system.RoutingTypes.Address;

@Singleton
public class RoutingTableImpl implements RoutingTable {

    private static final Logger logger = LoggerFactory.getLogger(RoutingTableImpl.class);

    private static class RoutingEntry {
        RoutingEntry(Address address, boolean isGloballyVisible, long expiryDateMs, boolean isSticky) {
            setAddress(address);
            setIsGloballyVisible(isGloballyVisible);
            this.expiryDateMs = expiryDateMs;
            this.isSticky = isSticky;
        }

        public Address getAddress() {
            return address;
        }

        public boolean getIsGloballyVisible() {
            return isGloballyVisible;
        }

        public long getExpiryDateMs() {
            return expiryDateMs;
        }

        public boolean getIsSticky() {
            return isSticky;
        }

        public void setAddress(Address address) {
            this.address = address;
        }

        public void setIsGloballyVisible(boolean isGloballyVisible) {
            this.isGloballyVisible = isGloballyVisible;
        }

        public void setExpiryDateMs(long expiryDateMs) {
            this.expiryDateMs = expiryDateMs;
        }

        public void setIsSticky(boolean isSticky) {
            this.isSticky = isSticky;
        }

        private Address address;
        private boolean isGloballyVisible;
        private long expiryDateMs;
        private boolean isSticky;
    }

    private ConcurrentMap<String, RoutingEntry> hashMap = Maps.newConcurrentMap();

    @Override
    public Address get(String participantId) {
        logger.trace("entering get(participantId={})", participantId);
        dumpRoutingTableEntry();
        RoutingEntry routingEntry = hashMap.get(participantId);
        if (routingEntry == null) {
            return null;
        }
        logger.trace("leaving get(participantId={}) = {}", participantId, routingEntry.getAddress());
        return routingEntry.getAddress();
    }

    private void dumpRoutingTableEntry() {
        if (logger.isTraceEnabled()) {
            StringBuilder message = new StringBuilder("Routing table entries:\n");
            for (Entry<String, RoutingEntry> eachEntry : hashMap.entrySet()) {
                message.append("\t> ")
                       .append(eachEntry.getKey())
                       .append("\t-\t")
                       .append(eachEntry.getValue().address)
                       .append("\t-\t")
                       .append(eachEntry.getValue().isGloballyVisible)
                       .append("\t-\t")
                       .append(eachEntry.getValue().expiryDateMs)
                       .append("\t-\t")
                       .append(eachEntry.getValue().isSticky)
                       .append("\n");
            }
            logger.trace(message.toString());
        }
    }

    @Override
    public void put(String participantId,
                    Address address,
                    boolean isGloballyVisible,
                    long expiryDateMs,
                    boolean sticky,
                    boolean allowUpdate) {
        RoutingEntry routingEntry = new RoutingEntry(address, isGloballyVisible, expiryDateMs, sticky);
        RoutingEntry result = hashMap.putIfAbsent(participantId, routingEntry);
        final boolean routingEntryAlreadyPresent = result != null;

        if (!routingEntryAlreadyPresent) {
            logger.trace("put(participantId={}, address={}, isGloballyVisible={}, expiryDateMs={}, sticky={}) successfully into routing table",
                         participantId,
                         address,
                         isGloballyVisible,
                         expiryDateMs,
                         sticky);
            return;
        }

        final boolean routingEntryChanged = !address.equals(result.getAddress())
                || result.getIsGloballyVisible() != isGloballyVisible;

        if (routingEntryChanged) {
            if (allowUpdate) {
                logger.debug("put(participantId={}, address={}, isGloballyVisible={}, expiryDateMs={}, sticky={}). Replacing previous entry with address={}, isGloballyVisible={}, expiryDateMs={}, sticky={}",
                             participantId,
                             address,
                             isGloballyVisible,
                             expiryDateMs,
                             sticky,
                             result.address,
                             result.isGloballyVisible,
                             result.expiryDateMs,
                             result.isSticky);
                mergeRoutingEntryAttributes(routingEntry, result.getExpiryDateMs(), result.getIsSticky());
                hashMap.put(participantId, routingEntry);
            } else {
                logger.warn("unable to update(participantId={}, address={}, isGloballyVisible={}, expiryDateMs={}, sticky={}) into routing table,"
                        + " since the participant ID is already associated with routing entry address={}, isGloballyVisible={}",
                            participantId,
                            address,
                            isGloballyVisible,
                            expiryDateMs,
                            sticky,
                            result.address,
                            result.isGloballyVisible);
            }
        } else {
            logger.trace("put(participantId={}, address={}, isGloballyVisible={}, expiryDateMs={}, sticky={}): Entry exists. Updating expiryDate and sticky-flag",
                         participantId,
                         address,
                         isGloballyVisible,
                         expiryDateMs,
                         sticky);
            mergeRoutingEntryAttributes(result, expiryDateMs, sticky);
        }
    }

    private void mergeRoutingEntryAttributes(RoutingEntry entry, long expiryDateMs, boolean isSticky) {
        // extend lifetime, if required
        if (entry.getExpiryDateMs() < expiryDateMs) {
            entry.setExpiryDateMs(expiryDateMs);
        }

        // make entry sticky, if required
        // if entry already was sticky, and new entry is not, keep the sticky attribute
        if (isSticky && !entry.getIsSticky()) {
            entry.setIsSticky(true);
        }
    }

    @Override
    public boolean containsKey(String participantId) {
        boolean containsKey = hashMap.containsKey(participantId);
        logger.trace("checking for participant: {} success: {}", participantId, containsKey);
        if (!containsKey) {
            dumpRoutingTableEntry();
        }
        return containsKey;
    }

    @Override
    public boolean getIsGloballyVisible(String participantId) {
        RoutingEntry routingEntry = hashMap.get(participantId);
        if (routingEntry == null) {
            throw new JoynrRuntimeException("participantId doesn't exist in the routing table");
        }
        return routingEntry.getIsGloballyVisible();
    }

    @Override
    public void setIsSticky(String participantId, boolean isSticky) {
        RoutingEntry routingEntry = hashMap.get(participantId);
        if (routingEntry == null) {
            throw new JoynrRuntimeException("participantId doesn't exist in the routing table");
        }
        routingEntry.setIsSticky(isSticky);
    }

    @Override
    public long getExpiryDateMs(String participantId) {
        RoutingEntry routingEntry = hashMap.get(participantId);
        if (routingEntry == null) {
            throw new JoynrRuntimeException("participantId doesn't exist in the routing table");
        }
        return routingEntry.getExpiryDateMs();
    }

    @Override
    public boolean getIsSticky(String participantId) {
        RoutingEntry routingEntry = hashMap.get(participantId);
        if (routingEntry == null) {
            throw new JoynrRuntimeException("participantId doesn't exist in the routing table");
        }
        return routingEntry.getIsSticky();
    }

    @Override
    public void remove(String participantId) {
        RoutingEntry routingEntry = hashMap.get(participantId);
        if (routingEntry != null) {
            logger.trace("removing(participantId={}, address={}, isGloballyVisible={}, expiryDateMs={}, sticky={}) from routing table",
                         participantId,
                         routingEntry.getAddress(),
                         routingEntry.getIsGloballyVisible(),
                         routingEntry.getExpiryDateMs(),
                         routingEntry.getIsSticky());
        }
        hashMap.remove(participantId);
    }

    @Override
    public void apply(AddressOperation addressOperation) {
        if (addressOperation == null) {
            throw new IllegalArgumentException();
        }
        for (RoutingEntry routingEntry : hashMap.values()) {
            addressOperation.perform(routingEntry.getAddress());
        }
    }

    public void purge() {
        logger.trace("purge: begin");
        Iterator<Entry<String, RoutingEntry>> it = hashMap.entrySet().iterator();
        long currentTimeMillis = System.currentTimeMillis();
        while (it.hasNext()) {
            Entry<String, RoutingEntry> e = it.next();
            if (logger.isTraceEnabled()) {
                logger.trace("check: participantId = {}, sticky = {}, expiryDateMs = {}, now = {}",
                             e.getKey(),
                             e.getValue().getIsSticky(),
                             e.getValue().getExpiryDateMs(),
                             currentTimeMillis);
            }

            if (!e.getValue().getIsSticky() && e.getValue().expiryDateMs < currentTimeMillis) {
                if (logger.isTraceEnabled()) {
                    logger.trace("purging(participantId={}, address={}, isGloballyVisible={}, expiryDateMs={}, sticky={}) from routing table",
                                 e.getKey(),
                                 e.getValue().getAddress(),
                                 e.getValue().getIsGloballyVisible(),
                                 e.getValue().getExpiryDateMs(),
                                 e.getValue().getIsSticky());
                }
                it.remove();
            }
        }
        logger.trace("purge: end");
    }
}
