package io.joynr.messaging.routing;

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

import java.util.Map.Entry;
import java.util.concurrent.ConcurrentMap;

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
        RoutingEntry(Address address, boolean isGloballyVisible) {
            setAddress(address);
            setIsGloballyVisible(isGloballyVisible);
        }

        public Address getAddress() {
            return address;
        }

        public boolean getIsGloballyVisible() {
            return isGloballyVisible;
        }

        public void setAddress(Address address) {
            this.address = address;
        }

        public void setIsGloballyVisible(boolean isGloballyVisible) {
            this.isGloballyVisible = isGloballyVisible;
        }

        private Address address;
        private boolean isGloballyVisible;
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
        logger.trace("leaving get(participantId={}) = {}", routingEntry.getAddress());
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
                       .append("\n");
            }
            logger.trace(message.toString());
        }
    }

    @Override
    public Address put(String participantId, Address address, boolean isGloballyVisible) {
        logger.trace("entering put(participantId={}, address={}, isGloballyVisible={})",
                     participantId,
                     address,
                     isGloballyVisible);
        RoutingEntry routingEntry = new RoutingEntry(address, isGloballyVisible);
        RoutingEntry result = hashMap.putIfAbsent(participantId, routingEntry);
        // NOTE: ConcurrentMap cannot contain null values, this means if result is not null the new
        //       address was not added to the routing table
        // putIfAbsent returns null if there is no V mapped to K.
        // Otherwise it returns the old mapped V and no insertion to the Routing table takes place
        if (result != null) {
            if (!address.equals(result.getAddress()) || result.getIsGloballyVisible() != isGloballyVisible) {
                logger.warn("unable to update(participantId={}, address={}, isGloballyVisible={}) into routing table,"
                                    + " since the participant ID is already associated with routing entry address={}, isGloballyVisible={}",
                            participantId,
                            address,
                            isGloballyVisible,
                            address,
                            isGloballyVisible);
            }
            return result.getAddress();
        } else {
            logger.trace("put(participantId={}, address={}, isGloballyVisible={}) successfully into routing table",
                         participantId,
                         address,
                         isGloballyVisible);
            return null;
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
            throw new JoynrRuntimeException("participateId doesn't exist in the routing table");
        }
        return routingEntry.getIsGloballyVisible();
    }

    @Override
    public void remove(String participantId) {
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

}
