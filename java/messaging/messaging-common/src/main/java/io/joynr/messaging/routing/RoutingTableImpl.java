/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_ROUTING_TABLE_GRACE_PERIOD_MS;
import static io.joynr.messaging.MessagingPropertyKeys.GBID_ARRAY;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;

@Singleton
public class RoutingTableImpl implements RoutingTable {

    private static final Logger logger = LoggerFactory.getLogger(RoutingTableImpl.class);

    private ConcurrentMap<String, RoutingEntry> hashMap = new ConcurrentHashMap<>();
    private final long routingTableGracePeriodMs;
    private final Set<String> knownGbidsSet;
    private String gcdParticipantId;
    private final RoutingTableAddressValidator addressValidator;

    @Inject
    public RoutingTableImpl(@Named(PROPERTY_ROUTING_TABLE_GRACE_PERIOD_MS) long routingTableGracePeriodMs,
                            @Named(GBID_ARRAY) String[] gbidsArray,
                            final RoutingTableAddressValidator addressValidator) {
        this.routingTableGracePeriodMs = routingTableGracePeriodMs;
        knownGbidsSet = new HashSet<>();
        knownGbidsSet.addAll(Arrays.asList(gbidsArray));
        // GcdParticipantId will be set to the correct value via setGcdParticipantId(String) during
        // joynr startup, before the GCD address is added to the routing table
        // (in the constructor of StaticCapabilitiesProvisioning).
        this.gcdParticipantId = "";
        this.addressValidator = addressValidator;
    }

    @Override
    public void setGcdParticipantId(final String gcdParticipantId) {
        if (gcdParticipantId != null) {
            this.gcdParticipantId = gcdParticipantId;
        } else {
            throw new JoynrIllegalStateException("The provided gcdParticipantId is null.");
        }
    };

    @Override
    public Address get(String participantId) {
        return getInternal(participantId);
    }

    @Override
    public Address get(String participantId, String gbid) {
        Address address = getInternal(participantId);
        if (address != null && gcdParticipantId.equals(participantId) && address instanceof MqttAddress) {
            if (!knownGbidsSet.contains(gbid)) {
                logger.error("The provided gbid {} for the participantId {} is unknown", gbid, participantId);
                address = null;
            } else {
                MqttAddress mqttAddress = new MqttAddress((MqttAddress) address);
                mqttAddress.setBrokerUri(gbid);
                address = mqttAddress;
            }
        }
        return address;
    }

    private Address getInternal(String participantId) {
        synchronized (this) {
            RoutingEntry routingEntry = hashMap.get(participantId);
            if (routingEntry == null) {
                logger.warn("No routing table entry found for participantId {}", participantId);
                return null;
            }
            return routingEntry.getAddress();
        }
    }

    private void updateRoutingEntry(final String participantId,
                                    final RoutingEntry oldRoutingEntry,
                                    final RoutingEntry newRoutingEntry) {
        mergeRoutingEntryAttributes(newRoutingEntry, oldRoutingEntry.getExpiryDateMs(), oldRoutingEntry.getIsSticky());
        hashMap.put(participantId, newRoutingEntry);
        logger.debug("Updated routing entry participantId {}, address {}, isGloballyVisible {}, expiryDateMs {}, sticky {} from address {}, isGloballyVisible {}, expiryDateMs {}, sticky {}, refCount {}",
                     participantId,
                     newRoutingEntry.getAddress(),
                     newRoutingEntry.getIsGloballyVisible(),
                     newRoutingEntry.getExpiryDateMs(),
                     newRoutingEntry.getIsSticky(),
                     oldRoutingEntry.getAddress(),
                     oldRoutingEntry.getIsGloballyVisible(),
                     oldRoutingEntry.getExpiryDateMs(),
                     oldRoutingEntry.getIsSticky(),
                     oldRoutingEntry.getRefCount());
    }

    /**
     * This is only to be used for debugging purposes during development!
     * 
     * @deprecated
     */
    private void dumpRoutingTable() {
        if (logger.isTraceEnabled()) {
            StringBuilder message = new StringBuilder("Routing table entries:\n");
            for (Entry<String, RoutingEntry> entry : hashMap.entrySet()) {
                message.append("\t> ")
                       .append(entry.getKey())
                       .append("\t-\t")
                       .append(entry.getValue().getAddress())
                       .append("\t-\t")
                       .append(entry.getValue().getIsGloballyVisible())
                       .append("\t-\t")
                       .append(entry.getValue().getExpiryDateMs())
                       .append("\t-\t")
                       .append(entry.getValue().getIsSticky())
                       .append("\t-\t")
                       .append(entry.getValue().getRefCount())
                       .append("\n");
            }
            logger.trace(message.toString());
        }
    }

    @Override
    public void put(final String participantId,
                    final Address address,
                    final boolean isGloballyVisible,
                    long expiryDateMs) {
        final boolean sticky = false;
        put(participantId, address, isGloballyVisible, expiryDateMs, sticky);

    }

    @Override
    public void put(final String participantId,
                    final Address address,
                    final boolean isGloballyVisible,
                    long expiryDateMs,
                    final boolean sticky) {
        if (!addressValidator.isValidForRoutingTable(address)) {
            logger.trace("ParticipantId {} has an address unsupported within this process.", participantId);
            return;
        }
        try {
            expiryDateMs = Math.addExact(expiryDateMs, routingTableGracePeriodMs);
        } catch (ArithmeticException e) {
            expiryDateMs = Long.MAX_VALUE;
        }
        RoutingEntry newRoutingEntry = new RoutingEntry(address, isGloballyVisible, expiryDateMs, sticky);

        synchronized (this) {
            RoutingEntry oldRoutingEntry = hashMap.putIfAbsent(participantId, newRoutingEntry);
            final boolean routingEntryAlreadyPresent = oldRoutingEntry != null;

            if (!routingEntryAlreadyPresent) {
                logger.debug("Added routing entry participantId {}, address {}, isGloballyVisible {}, expiryDateMs {}, sticky {}, refCnt {}",
                             participantId,
                             address,
                             isGloballyVisible,
                             expiryDateMs,
                             sticky,
                             1);
                return;
            }
            oldRoutingEntry.incRefCount();
            newRoutingEntry.setRefCount(oldRoutingEntry.getRefCount());
            logger.debug("Increased reference count for routing entry participantId {}, address {}, new reference count: {}",
                         participantId,
                         oldRoutingEntry.getAddress(),
                         oldRoutingEntry.getRefCount());

            final boolean addressOrVisibilityOfRoutingEntryChanged = !address.equals(oldRoutingEntry.getAddress())
                    || oldRoutingEntry.getIsGloballyVisible() != isGloballyVisible;

            if (addressOrVisibilityOfRoutingEntryChanged) {
                if (oldRoutingEntry.getIsSticky()) {
                    logger.error("Refused to update sticky routing entry participantId {}, address {}, isGloballyVisible {}, to address {}, isGloballyVisible {}, expiryDateMs {}, sticky {}, refCnt {}",
                                 participantId,
                                 oldRoutingEntry.getAddress(),
                                 oldRoutingEntry.getIsGloballyVisible(),
                                 address,
                                 isGloballyVisible,
                                 expiryDateMs,
                                 sticky,
                                 oldRoutingEntry.getRefCount());
                } else if (addressValidator.allowUpdate(oldRoutingEntry, newRoutingEntry)) {
                    updateRoutingEntry(participantId, oldRoutingEntry, newRoutingEntry);
                } else {
                    logger.warn("Refused to update routing entry participantId {}, address {}, isGloballyVisible {}, expiryDateMs {}, sticky {}, to address {}, isGloballyVisible {}, expiryDateMs {}, sticky {}, refCnt {}",
                                participantId,
                                oldRoutingEntry.getAddress(),
                                oldRoutingEntry.getIsGloballyVisible(),
                                oldRoutingEntry.getExpiryDateMs(),
                                oldRoutingEntry.getIsSticky(),
                                address,
                                isGloballyVisible,
                                expiryDateMs,
                                sticky,
                                oldRoutingEntry.getRefCount());
                }
            } else {
                // only expiryDate or sticky flag of routing entry changed
                logger.trace("Updated routing entry participantId {}, address {}, isGloballyVisible {}, expiryDateMs {}, sticky {}, refCnt {} . Updated expiryDate and sticky-flag",
                             participantId,
                             address,
                             isGloballyVisible,
                             expiryDateMs,
                             sticky,
                             oldRoutingEntry.getRefCount());
                mergeRoutingEntryAttributes(oldRoutingEntry, expiryDateMs, sticky);
            }
        }
    }

    private void mergeRoutingEntryAttributes(final RoutingEntry entry,
                                             final long expiryDateMs,
                                             final boolean isSticky) {
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
        synchronized (this) {
            boolean containsKey = hashMap.containsKey(participantId);
            logger.trace("Checking for participant: {} success: {}", participantId, containsKey);
            return containsKey;
        }
    }

    @Override
    public boolean getIsGloballyVisible(String participantId) {
        synchronized (this) {
            RoutingEntry routingEntry = hashMap.get(participantId);
            if (routingEntry == null) {
                throw new JoynrRuntimeException("participantId doesn't exist in the routing table");
            }
            return routingEntry.getIsGloballyVisible();
        }
    }

    @Override
    public long getExpiryDateMs(String participantId) {
        synchronized (this) {
            RoutingEntry routingEntry = hashMap.get(participantId);
            if (routingEntry == null) {
                throw new JoynrRuntimeException("participantId doesn't exist in the routing table");
            }
            return routingEntry.getExpiryDateMs();
        }
    }

    @Override
    public boolean getIsSticky(String participantId) {
        synchronized (this) {
            RoutingEntry routingEntry = hashMap.get(participantId);
            if (routingEntry == null) {
                throw new JoynrRuntimeException("participantId doesn't exist in the routing table");
            }
            return routingEntry.getIsSticky();
        }
    }

    @Override
    public void remove(String participantId) {
        synchronized (this) {
            RoutingEntry routingEntry = hashMap.get(participantId);
            if (routingEntry != null) {
                if (routingEntry.isSticky) {
                    logger.warn("Cannot remove sticky routing entry (participantId={}, address={}, isGloballyVisible={}, expiryDateMs={}, sticky={}, refCnt={})",
                                participantId,
                                routingEntry.getAddress(),
                                routingEntry.getIsGloballyVisible(),
                                routingEntry.getExpiryDateMs(),
                                routingEntry.getIsSticky(),
                                routingEntry.getRefCount());
                } else {
                    routingEntry.decRefCount();
                    logger.debug("Decreased reference count for routing entry participantId {}, address {}, isGloballyVisible {}, expiryDateMs {}, sticky {}, new reference count: {}",
                                 participantId,
                                 routingEntry.getAddress(),
                                 routingEntry.getIsGloballyVisible(),
                                 routingEntry.getExpiryDateMs(),
                                 routingEntry.getIsSticky(),
                                 routingEntry.getRefCount());
                    if (routingEntry.getRefCount() > 0) {
                        return;
                    }
                    hashMap.remove(participantId);
                    logger.debug("Removed routing entry participantId {}, address {}, isGloballyVisible {}, expiryDateMs {}, sticky {}",
                                 participantId,
                                 routingEntry.getAddress(),
                                 routingEntry.getIsGloballyVisible(),
                                 routingEntry.getExpiryDateMs(),
                                 routingEntry.getIsSticky());
                }
            }
        }
    }

    @Override
    public void apply(AddressOperation addressOperation) {
        synchronized (this) {
            if (addressOperation == null) {
                throw new IllegalArgumentException();
            }
            for (RoutingEntry routingEntry : hashMap.values()) {
                addressOperation.perform(routingEntry.getAddress());
            }
        }
    }

    public void purge() {
        synchronized (this) {
            Iterator<Entry<String, RoutingEntry>> it = hashMap.entrySet().iterator();
            long currentTimeMillis = System.currentTimeMillis();
            while (it.hasNext()) {
                Entry<String, RoutingEntry> e = it.next();
                logger.trace("Check: participantId {}, sticky {}, expiryDateMs {}, refCnt {}",
                             e.getKey(),
                             e.getValue().getIsSticky(),
                             e.getValue().getExpiryDateMs(),
                             e.getValue().getRefCount());

                if (!e.getValue().getIsSticky() && e.getValue().expiryDateMs < currentTimeMillis) {
                    it.remove();
                    logger.trace("Purged routing entry participantId {}, address {}, isGloballyVisible {}, expiryDateMs {}, sticky {}, refCnt {}",
                                 e.getKey(),
                                 e.getValue().getAddress(),
                                 e.getValue().getIsGloballyVisible(),
                                 e.getValue().getExpiryDateMs(),
                                 e.getValue().getIsSticky(),
                                 e.getValue().getRefCount());
                }
            }
        }
    }

    public void incrementReferenceCount(String participantId) {
        synchronized (this) {
            RoutingEntry routingEntry = hashMap.get(participantId);
            if (routingEntry == null) {
                throw new JoynrIllegalStateException("No routing entry with participantId " + participantId + "found!");
            }
            routingEntry.incRefCount();
            logger.debug("Increased reference count for routing entry participantId {}, new reference count: {}",
                         participantId,
                         routingEntry.getRefCount());
        }
    }
}
