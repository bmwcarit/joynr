package io.joynr.messaging.routing;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
import joynr.system.RoutingTypes.Address;

@Singleton
public class RoutingTableImpl implements RoutingTable {

    private static final Logger logger = LoggerFactory.getLogger(RoutingTableImpl.class);

    private ConcurrentMap<String, Address> hashMap = Maps.newConcurrentMap();

    @Override
    public Address get(String participantId) {
        logger.trace("entering get(participantId={})", participantId);
        dumpRoutingTableEntry();
        Address result = hashMap.get(participantId);
        logger.trace("leaving get(participantId={}) = {}", result);
        return result;
    }

    private void dumpRoutingTableEntry() {
        if (logger.isTraceEnabled()) {
            StringBuilder message = new StringBuilder("Routing table entries:\n");
            for (Entry<String, Address> eachEntry : hashMap.entrySet()) {
                message.append("\t> ")
                       .append(eachEntry.getKey())
                       .append("\t-\t")
                       .append(eachEntry.getValue())
                       .append("\n");
            }
            logger.trace(message.toString());
        }
    }

    @Override
    public Address put(String participantId, Address address) {
        logger.trace("entering put(participantId={}, address={})", participantId, address);
        Address result = hashMap.putIfAbsent(participantId, address);
        // NOTE: ConcurrentMap cannot contain null values, this means if result is not null the new
        //       address was not added to the routing table
        if (result != null) {
            logger.warn("unable to put(participantId={}, address={}) into routing table,"
                    + " since the participant ID is already associated with address={}", participantId, address, result);
        } else {
            logger.trace("put(participantId={}, address={}) successfully into routing table", participantId, address);
        }
        return result;
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
    public void remove(String participantId) {
        hashMap.remove(participantId);
    }

    @Override
    public void apply(AddressOperation addressOperation) {
        if (addressOperation == null) {
            throw new IllegalArgumentException();
        }
        for (Address address : hashMap.values()) {
            addressOperation.perform(address);
        }
    }

}
