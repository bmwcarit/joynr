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
package io.joynr.capabilities;

import static io.joynr.messaging.MessagingPropertyKeys.CHANNELID;
import static java.lang.String.format;

import java.io.IOException;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.util.ObjectMapper;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.GlobalDomainAccessControlListEditor;
import joynr.infrastructure.GlobalDomainAccessController;
import joynr.infrastructure.GlobalDomainRoleController;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;

/**
 * Loads a set of JSON encoded {@link GlobalDiscoveryEntry discovery entries} from the file referenced by the property
 * named {@link #PROPERTY_PROVISIONED_CAPABILITIES_FILE joynr.capabilities.provisioned.file} and makes them available
 * via {@link #getDiscoveryEntries()}.
 *
 * This component will fail-fast - that is, it will throw a {@link JoynrRuntimeException} during initialization if the
 * JSON read from the file the property refers to cannot bit parsed, or the file cannot be found.
 */
public class StaticCapabilitiesProvisioning implements CapabilitiesProvisioning {

    public static final String PROPERTY_PROVISIONED_CAPABILITIES_FILE = "joynr.capabilities.provisioned.file";
    private static Logger logger = LoggerFactory.getLogger(StaticCapabilitiesProvisioning.class);
    private final ResourceContentProvider resourceContentProvider;
    private final String[] gbids;
    private final HashSet<String> internalInterfaces;

    protected Collection<GlobalDiscoveryEntry> discoveryEntries;

    @Inject
    public StaticCapabilitiesProvisioning(@Named(PROPERTY_PROVISIONED_CAPABILITIES_FILE) String provisionedCapabilitiesFile,
                                          @Named(CHANNELID) String localChannelId,
                                          ObjectMapper objectMapper,
                                          LegacyCapabilitiesProvisioning legacyCapabilitiesProvisioning,
                                          ResourceContentProvider resourceContentProvider,
                                          @Named(MessagingPropertyKeys.GBID_ARRAY) String[] gbids) {
        internalInterfaces = new HashSet<String>();
        internalInterfaces.add(GlobalCapabilitiesDirectory.INTERFACE_NAME);
        internalInterfaces.add(GlobalDomainAccessController.INTERFACE_NAME);
        internalInterfaces.add(GlobalDomainRoleController.INTERFACE_NAME);
        internalInterfaces.add(GlobalDomainAccessControlListEditor.INTERFACE_NAME);
        discoveryEntries = new HashSet<>();
        this.gbids = gbids.clone();
        this.resourceContentProvider = resourceContentProvider;
        addEntriesFromJson(provisionedCapabilitiesFile, objectMapper, localChannelId);
        logger.trace("{} provisioned discovery entries loaded from JSON: {}",
                     discoveryEntries.size(),
                     discoveryEntries);
        overrideEntriesFromLegacySettings(legacyCapabilitiesProvisioning);
        logger.trace("{} provisioned discovery entries after adding legacy entries: {}",
                     discoveryEntries.size(),
                     discoveryEntries);
        logger.debug("Statically provisioned discovery entries loaded: {}", discoveryEntries);
    }

    private void overrideEntriesFromLegacySettings(LegacyCapabilitiesProvisioning legacyCapabilitiesProvisioning) {
        GlobalDiscoveryEntry globalCapabilitiesEntry = legacyCapabilitiesProvisioning.getDiscoveryEntryForInterface(GlobalCapabilitiesDirectory.class);
        if (globalCapabilitiesEntry != null) {
            removeExistingEntryForInterface(GlobalCapabilitiesDirectory.INTERFACE_NAME);
            discoveryEntries.add(globalCapabilitiesEntry);
        }
        GlobalDiscoveryEntry domainAccessControllerEntry = legacyCapabilitiesProvisioning.getDiscoveryEntryForInterface(GlobalDomainAccessController.class);
        if (domainAccessControllerEntry != null) {
            removeExistingEntryForInterface(GlobalDomainAccessController.INTERFACE_NAME);
            discoveryEntries.add(domainAccessControllerEntry);
        }
    }

    private void removeExistingEntryForInterface(String interfaceName) {
        DiscoveryEntry entryToRemove = null;
        for (DiscoveryEntry discoveryEntry : discoveryEntries) {
            if (discoveryEntry instanceof GlobalDiscoveryEntry
                    && interfaceName.equals(((GlobalDiscoveryEntry) discoveryEntry).getInterfaceName())) {
                entryToRemove = discoveryEntry;
                break;
            }
        }
        if (entryToRemove != null) {
            discoveryEntries.remove(entryToRemove);
        }
    }

    private void addEntriesFromJson(String provisionedCapabilitiesJsonFilename,
                                    ObjectMapper objectMapper,
                                    String localChannelId) {
        String provisionedCapabilitiesJsonString = resourceContentProvider.readFromFileOrResourceOrUrl(provisionedCapabilitiesJsonFilename);
        logger.trace("Statically provisioned capabilities JSON read: {}", provisionedCapabilitiesJsonString);
        List<GlobalDiscoveryEntry> newEntries = null;
        try {
            newEntries = objectMapper.readValue(provisionedCapabilitiesJsonString,
                                                new TypeReference<List<GlobalDiscoveryEntry>>() {
                                                });
            for (GlobalDiscoveryEntry globalDiscoveryEntry : newEntries) {
                // TODO use Long.MAX_VALUE for lastSeenDate of provisioned entries?
                globalDiscoveryEntry.setLastSeenDateMs(System.currentTimeMillis());
                globalDiscoveryEntry.setExpiryDateMs(Long.MAX_VALUE);
                Address address = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);
                if (internalInterfaces.contains(globalDiscoveryEntry.getInterfaceName())
                        && address instanceof MqttAddress) {
                    ((MqttAddress) address).setBrokerUri(gbids[0]);
                    globalDiscoveryEntry.setAddress(CapabilityUtils.serializeAddress(address));
                }
                substituteInProcessAddressIfLocal(objectMapper, localChannelId, globalDiscoveryEntry, address);
                discoveryEntries.add(globalDiscoveryEntry);
            }
        } catch (IOException e) {
            String message = format("Unable to load provisioned capabilities. Invalid JSON value: %s",
                                    provisionedCapabilitiesJsonString);
            throw new JoynrRuntimeException(message, e);
        }
    }

    private void substituteInProcessAddressIfLocal(ObjectMapper objectMapper,
                                                   String localChannelId,
                                                   GlobalDiscoveryEntry globalDiscoveryEntry,
                                                   Address address) throws JsonProcessingException {
        if ((address instanceof ChannelAddress && localChannelId.equals(((ChannelAddress) address).getChannelId()))
                || (address instanceof MqttAddress && localChannelId.equals(((MqttAddress) address).getTopic()))) {
            Address localAddress = new InProcessAddress();
            globalDiscoveryEntry.setAddress(objectMapper.writeValueAsString(localAddress));
        }
    }

    @Override
    public Collection<GlobalDiscoveryEntry> getDiscoveryEntries() {
        return discoveryEntries;
    }

}
