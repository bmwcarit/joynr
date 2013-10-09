package io.joynr.capabilities.directory;

/*
 * #%L
 * joynr::java::backend-services::capabilities directory
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
import io.joynr.capabilities.CapabilitiesStore;
import io.joynr.capabilities.CapabilitiesStoreImpl;
import io.joynr.capabilities.CapabilityEntry;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;
import io.joynr.messaging.ConfigurableMessagingSettings;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.Collection;
import java.util.List;

import joynr.infrastructure.GlobalCapabilitiesDirectoryAbstractProvider;
import joynr.types.CapabilityInformation;
import joynr.types.ProviderQos;
import joynr.types.ProviderQosRequirements;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Lists;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

/**
 * The capabilities directory implementation for server-side capabilities querying.
 * 
 * 
 * Capability informations are stored in a concurrentHashMap. Using a in memory database could be possible optimization.
 */

// TODO Evaluate pro /cons of a in memory database
// TODO Using the interfaceAddress as the key may increase performance in most
// requests.

@Singleton
public class CapabilitiesDirectoryImpl extends GlobalCapabilitiesDirectoryAbstractProvider {
    private static final Logger logger = LoggerFactory.getLogger(CapabilitiesDirectoryImpl.class);
    // TODO replace by jackson objectMapper
    private Gson gson = new GsonBuilder().setPrettyPrinting().create();
    private CapabilitiesStore capabiltiesStore = new CapabilitiesStoreImpl();

    private File dumpFile;

    @Inject
    public CapabilitiesDirectoryImpl(@Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabiltitiesDirectoryChannelId) {
        File dumpDir = new File("capabilitiesdump");
        if (!dumpDir.mkdir()) {
            logger.trace("Could not create capabilitiesDump directory (maybe already existed)");
        }
        dumpFile = new File("capabilitiesdump/capabilities.json");
        if (dumpFile.exists()) {
            if (!dumpFile.delete()) {
                logger.warn("Could not delete dumpfile");
            }
        }
    }

    @Override
    public void registerCapability(CapabilityInformation capabilityInformation) {

        CapabilityEntry capabilityEntry = capabilityInformation2Entry(capabilityInformation);
        logger.debug("registered capability: {}", capabilityEntry);
        capabiltiesStore.registerCapability(capabilityEntry);
        dumpCapabilities();
    }

    private CapabilityEntry capabilityInformation2Entry(CapabilityInformation capabilityInformation) {
        return CapabilityEntry.fromCapabilityInformation(capabilityInformation);
    }

    @Override
    public void registerCapabilities(List<CapabilityInformation> capabilitiesInformation) {
        // TODO check interfaces before adding them
        List<CapabilityEntry> capabilityEntries = Lists.newArrayList();
        for (CapabilityInformation capInfo : capabilitiesInformation) {
            capabilityEntries.add(capabilityInformation2Entry(capInfo));
        }

        logger.debug("registered capabilities: interface {}", capabilityEntries.toString());

        capabiltiesStore.registerCapabilities(capabilityEntries);
        dumpCapabilities();
    }

    @Override
    public void unregisterCapability(CapabilityInformation capability) {
        CapabilityEntry capabilityEntry = capabilityInformation2Entry(capability);
        logger.debug("removed capabilities: interface {}", capabilityEntry);
        capabiltiesStore.removeCapability(capabilityEntry);
        dumpCapabilities();
    }

    @Override
    public void unregisterCapabilities(List<CapabilityInformation> capabilities) {
        // TODO who is allowed to remove capabilities
        List<CapabilityEntry> capabilityEntries = Lists.newArrayList();
        for (CapabilityInformation capInfo : capabilities) {
            capabilityEntries.add(capabilityInformation2Entry(capInfo));
        }
        logger.debug("Removing capabilities: Capabilities {}", capabilityEntries);
        capabiltiesStore.removeCapabilities(capabilityEntries);
        dumpCapabilities();
    }

    @Override
    public List<CapabilityInformation> lookupCapabilities(final String domain,
                                                          final String interfaceName,
                                                          ProviderQosRequirements qos) {
        logger.debug("Searching channels for domain: " + domain + " interfaceName: " + interfaceName + " {}", qos);
        List<CapabilityInformation> capabilityInformationList = Lists.newArrayList();
        Collection<CapabilityEntry> entryCollection = capabiltiesStore.findCapabilitiesForInterfaceAddress(domain,
                                                                                                           interfaceName,
                                                                                                           qos,
                                                                                                           DiscoveryQos.NO_FILTER);
        for (CapabilityEntry entry : entryCollection) {
            capabilityInformationList.add(entry.toCapabilityInformation());
        }
        return capabilityInformationList;
    }

    @Override
    public List<CapabilityInformation> getCapabilitiesForChannelId(String channelId) {
        logger.debug("Searching capabilities for client id: {}", channelId);
        List<CapabilityInformation> capabilityInformationList = Lists.newArrayList();
        Collection<CapabilityEntry> entryCollection = capabiltiesStore.findCapabilitiesForEndpointAddress(new JoynrMessagingEndpointAddress(channelId),
                                                                                                          DiscoveryQos.NO_FILTER);
        for (CapabilityEntry entry : entryCollection) {
            capabilityInformationList.add(entry.toCapabilityInformation());
        }
        return capabilityInformationList;
    }

    synchronized void dumpCapabilities() {
        OutputStreamWriter fileWriter = null;
        try {
            fileWriter = new OutputStreamWriter(new FileOutputStream(dumpFile), "UTF-8");
            String json = gson.toJson(capabiltiesStore.getAllCapabilities());
            fileWriter.write(json);
        } catch (IOException e) {
            logger.error("dumpCapabilities: could not create capabilities directory dump file", e);
        } finally {
            if (fileWriter != null) {
                try {
                    fileWriter.close();
                } catch (IOException e) {
                    logger.error("dumpCapabilities: could not close fileWriter for capabilities directory dump file", e);
                }
            }
        }

    }

    @Override
    public List<CapabilityInformation> getCapabilitiesForParticipantId(String forParticipantId) {
        logger.debug("Searching capabilities for participantId: {}", forParticipantId);
        List<CapabilityInformation> capabilityInformationList = Lists.newArrayList();
        Collection<CapabilityEntry> entryCollection = capabiltiesStore.findCapabilitiesForParticipantId(forParticipantId,
                                                                                                        DiscoveryQos.NO_FILTER);
        for (CapabilityEntry entry : entryCollection) {
            capabilityInformationList.add(entry.toCapabilityInformation());
        }
        return capabilityInformationList;
    }

    @Override
    public ProviderQos getProviderQos() {
        // TODO set capDirImpl provider QoS
        return providerQos;
    }

}
