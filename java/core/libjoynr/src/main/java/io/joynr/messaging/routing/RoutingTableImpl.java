package io.joynr.messaging.routing;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import io.joynr.messaging.ConfigurableMessagingSettings;

import java.util.Map.Entry;
import java.util.concurrent.ConcurrentMap;

import joynr.system.RoutingTypes.Address;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Maps;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class RoutingTableImpl implements RoutingTable {
    private static final Logger logger = LoggerFactory.getLogger(RoutingTableImpl.class);
    ConcurrentMap<String, Address> hashMap = Maps.newConcurrentMap();

    @Inject
    public RoutingTableImpl(@Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_PARTICIPANT_ID) String channelUrlDirectoryParticipantId,
                            @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_ADDRESS) Address channelUrlDirectoryAddress,
                            @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID) String capabilitiesDirectoryParticipantId,
                            @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS) Address capabiltitiesDirectoryAddress) {
        this.put(capabilitiesDirectoryParticipantId, capabiltitiesDirectoryAddress);
        this.put(channelUrlDirectoryParticipantId, channelUrlDirectoryAddress);
    }

    public Address get(String participantId) {
        logger.debug("lookup participant: " + participantId);
        for (Entry<String, Address> eachEntry : hashMap.entrySet()) {
            logger.trace(eachEntry.getKey() + ": " + eachEntry.getValue());
        }
        return hashMap.get(participantId);
    }

    public Address put(String participantId, Address address) {
        logger.debug("adding endpoint address: " + participantId + ": " + address);
        return hashMap.putIfAbsent(participantId, address);
    }

    public boolean containsKey(String participantId) {
        boolean containsKey = hashMap.containsKey(participantId);
        logger.debug("checking for participant: " + participantId + " success: " + containsKey);
        if (!containsKey) {
            for (String eachkey : hashMap.keySet()) {
                logger.trace("MessagingEndpointDirectory: " + eachkey + ": " + this.get(eachkey));
            }
        }
        return containsKey;
    }

    public void remove(String participantId) {
        hashMap.remove(participantId);

    }
}
