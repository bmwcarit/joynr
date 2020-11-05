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

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.util.ObjectMapper;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderScope;

public class StaticCapabilitiesProvisioningWithRoutingTableInsertion extends StaticCapabilitiesProvisioning {
    @Inject
    public StaticCapabilitiesProvisioningWithRoutingTableInsertion(@Named(PROPERTY_PROVISIONED_CAPABILITIES_FILE) String provisionedCapabilitiesFile,
                                                                   @Named(CHANNELID) String localChannelId,
                                                                   ObjectMapper objectMapper,
                                                                   RoutingTable routingTable,
                                                                   LegacyCapabilitiesProvisioning legacyCapabilitiesProvisioning,
                                                                   ResourceContentProvider resourceContentProvider,
                                                                   @Named(MessagingPropertyKeys.GBID_ARRAY) String[] gbids) {
        super(provisionedCapabilitiesFile,
              localChannelId,
              objectMapper,
              legacyCapabilitiesProvisioning,
              resourceContentProvider,
              gbids);
        addAddressesToRoutingTable(routingTable);
    }

    private void addAddressesToRoutingTable(RoutingTable routingTable) {
        for (GlobalDiscoveryEntry globalDiscoveryEntry : discoveryEntries) {
            if (GlobalCapabilitiesDirectory.INTERFACE_NAME.equals(globalDiscoveryEntry.getInterfaceName())) {
                routingTable.setGcdParticipantId(globalDiscoveryEntry.getParticipantId());
            }
            boolean isGloballyVisible = (globalDiscoveryEntry.getQos().getScope() == ProviderScope.GLOBAL);
            final long expiryDateMs = Long.MAX_VALUE;
            final boolean isSticky = true;
            routingTable.put(globalDiscoveryEntry.getParticipantId(),
                             CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry),
                             isGloballyVisible,
                             expiryDateMs,
                             isSticky);
        }
    }
}
