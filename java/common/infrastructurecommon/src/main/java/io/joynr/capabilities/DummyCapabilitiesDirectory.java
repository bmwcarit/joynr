package io.joynr.capabilities;

/*
 * #%L
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

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;

import joynr.types.ProviderQosRequirements;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Lists;
import com.google.inject.Inject;
import com.google.inject.name.Named;

public class DummyCapabilitiesDirectory implements LocalCapabilitiesDirectory {
    private static final Logger logger = LoggerFactory.getLogger(DummyCapabilitiesDirectory.class);
    private static final DummyCapabilitiesDirectory instance = new DummyCapabilitiesDirectory();
    private ArrayList<CapabilityEntry> registeredCapabilities = Lists.newArrayList();

    @Inject
    @Named("joynr.messaging.channelId")
    String myChannelId;

    public static DummyCapabilitiesDirectory getInstance() {
        return instance;
    }

    @Override
    public RegistrationFuture addCapability(CapabilityEntry capabilityEntry) {
        capabilityEntry.addEndpoint(new JoynrMessagingEndpointAddress(myChannelId));
        registeredCapabilities.add(capabilityEntry);
        return new RegistrationFuture(RegistrationStatus.DONE, capabilityEntry.getParticipantId());
    }

    @Override
    public void removeCapability(CapabilityEntry interfaces) {
        logger.info("!!!!!!!!!!!!!!!removeCapabilities");

    }

    @Override
    public Collection<CapabilityEntry> getCapabilities(final String domain,
                                                       final String interfaceName,
                                                       ProviderQosRequirements requestedQos,
                                                       DiscoveryQos discoveryQos) {
        logger.info("!!!!!!!!!!!!!!!getCapabilities sync");
        return Collections.emptyList();
    }

    @Override
    public Collection<CapabilityEntry> getCapabilities(String participantId, DiscoveryQos discoveryQos) {
        logger.info("!!!!!!!!!!!!!!!getCapabilities");
        return Collections.emptyList();
    }

    @Override
    public void getCapabilities(String domain,
                                String interfaceName,
                                ProviderQosRequirements requestedQos,
                                DiscoveryQos discoveryQos,
                                CapabilitiesCallback capabilitiesCallback) {
        logger.info("!!!!!!!!!!!!!!!getCapabilities async");
        ArrayList<CapabilityEntry> foundCapabilities = Lists.newArrayList();
        for (CapabilityEntry ce : registeredCapabilities) {
            if (ce.getDomain().equals(domain) && ce.getInterfaceName().equals(interfaceName)) {
                foundCapabilities.add(ce);
            }
        }
        capabilitiesCallback.processCapabilitiesReceived(foundCapabilities);
    }

    @Override
    public void getCapabilities(String participantId, DiscoveryQos discoveryQos, CapabilitiesCallback callback) {
        logger.info("!!!!!!!!!!!!!!!getCapabilitiesForParticipantId");

    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        registeredCapabilities.clear();
    }

}
