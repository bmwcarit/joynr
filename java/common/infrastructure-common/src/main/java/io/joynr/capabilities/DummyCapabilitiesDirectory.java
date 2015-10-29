package io.joynr.capabilities;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import java.util.ArrayList;

import javax.annotation.CheckForNull;

import joynr.types.DiscoveryEntry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Lists;
import com.google.inject.Inject;
import com.google.inject.name.Named;

public class DummyCapabilitiesDirectory extends AbstractLocalCapabilitiesDirectory {
    private static final Logger logger = LoggerFactory.getLogger(DummyCapabilitiesDirectory.class);
    private static final DummyCapabilitiesDirectory instance = new DummyCapabilitiesDirectory();
    private ArrayList<DiscoveryEntry> registeredCapabilities = Lists.newArrayList();

    @Inject
    @Named("joynr.messaging.channelId")
    String myChannelId;

    public static DummyCapabilitiesDirectory getInstance() {
        return instance;
    }

    @Override
    public RegistrationFuture add(DiscoveryEntry discoveryEntry) {
        registeredCapabilities.add(discoveryEntry);
        notifyCapabilityAdded(discoveryEntry);
        return new RegistrationFuture(RegistrationStatus.DONE, discoveryEntry.getParticipantId());
    }

    @Override
    public void remove(DiscoveryEntry interfaces) {
        logger.info("!!!!!!!!!!!!!!!removeCapabilities");

    }

    @Override
    public void lookup(String domain,
                       String interfaceName,
                       DiscoveryQos discoveryQos,
                       CapabilitiesCallback capabilitiesCallback) {
        logger.info("!!!!!!!!!!!!!!!getCapabilities async");
        ArrayList<DiscoveryEntry> foundCapabilities = Lists.newArrayList();
        for (DiscoveryEntry ce : registeredCapabilities) {
            if (ce.getDomain().equals(domain) && ce.getInterfaceName().equals(interfaceName)) {
                foundCapabilities.add(ce);
            }
        }
        capabilitiesCallback.processCapabilitiesReceived(foundCapabilities);
    }

    @Override
    @CheckForNull
    public void lookup(String participantId, DiscoveryQos discoveryQos, CapabilityCallback callback) {
        logger.info("!!!!!!!!!!!!!!!getCapabilitiesForParticipantId");
    }

    @Override
    @CheckForNull
    public DiscoveryEntry lookup(String participantId, DiscoveryQos discoveryQos) {
        logger.info("!!!!!!!!!!!!!!!getCapabilitiesForParticipantId");
        DiscoveryEntry retrievedDiscoveryEntry = null;
        for (DiscoveryEntry entry : registeredCapabilities) {
            if (entry.getParticipantId().equals(participantId)) {
                retrievedDiscoveryEntry = entry;
                break;
            }
        }
        return retrievedDiscoveryEntry;
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        registeredCapabilities.clear();
    }
}
