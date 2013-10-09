package io.joynr.dispatcher;

/*
 * #%L
 * joynr::java::core::libjoynr
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.endpoints.EndpointAddressBase;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.messaging.ConfigurableMessagingSettings;

import java.util.concurrent.ConcurrentMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Maps;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

@Singleton
public class MessagingEndpointDirectory {
    private static final Logger logger = LoggerFactory.getLogger(MessagingEndpointDirectory.class);
    ConcurrentMap<String, EndpointAddressBase> hashMap = Maps.newConcurrentMap();

    String channelUrlDirectoryParticipantId;
    String channelUrlDirectoryChannelId;
    private final String capabilitiesDirectoryParticipantId;
    private final String capabiltitiesDirectoryChannelId;

    @Inject
    public MessagingEndpointDirectory(@Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_PARTICIPANT_ID) String channelUrlDirectoryParticipantId,
                                      @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_CHANNEL_ID) String channelUrlDirectoryChannelId,
                                      @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID) String capabilitiesDirectoryParticipantId,
                                      @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabiltitiesDirectoryChannelId) {
        this.channelUrlDirectoryParticipantId = channelUrlDirectoryParticipantId;
        this.channelUrlDirectoryChannelId = channelUrlDirectoryChannelId;
        this.capabilitiesDirectoryParticipantId = capabilitiesDirectoryParticipantId;
        this.capabiltitiesDirectoryChannelId = capabiltitiesDirectoryChannelId;
        putPreconfiguredEntries();
    }

    public EndpointAddressBase get(String participantId) {
        logger.debug("lookup participant: " + participantId);
        for (String eachkey : hashMap.keySet()) {
            logger.trace(eachkey + ": " + hashMap.get(eachkey));
        }
        return hashMap.get(participantId);
    }

    public EndpointAddressBase put(String participantId, EndpointAddressBase endpointAddress) {
        // logger.error("ADDING ENDPOINT", new Exception());
        logger.debug("adding endpoint address: " + participantId + ": " + endpointAddress);
        return hashMap.putIfAbsent(participantId, endpointAddress);
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

    private void putPreconfiguredEntries() {
        this.put(capabilitiesDirectoryParticipantId, new JoynrMessagingEndpointAddress(capabiltitiesDirectoryChannelId));

        this.put(channelUrlDirectoryParticipantId, new JoynrMessagingEndpointAddress(channelUrlDirectoryChannelId));

    }

    public void remove(String participantId) {
        hashMap.remove(participantId);

    }
}
