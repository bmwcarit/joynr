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
package io.joynr.capabilities;

import static io.joynr.util.VersionUtil.getVersionFromAnnotation;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.runtime.SystemServicesSettings;
import joynr.system.Discovery;
import joynr.system.DiscoveryProvider;
import joynr.system.RoutingTypes.Address;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class InProcessCapabilitiesProvisioning extends DefaultCapabilitiesProvisioning {

    private static final long NO_EXPIRY = Long.MAX_VALUE;
    private String discoveryProviderParticipantId;
    private String systemServicesDomain;
    private Address discoveryProviderAddress;

    @Inject
    public InProcessCapabilitiesProvisioning(@Named(SystemServicesSettings.PROPERTY_CC_DISCOVERY_PROVIDER_PARTICIPANT_ID) String discoveryProviderParticipantId,
                                             @Named(SystemServicesSettings.PROPERTY_SYSTEM_SERVICES_DOMAIN) String systemServicesDomain,
                                             @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS) Address discoveryProviderAddress) {
        this.discoveryProviderParticipantId = discoveryProviderParticipantId;
        this.systemServicesDomain = systemServicesDomain;
        this.discoveryProviderAddress = discoveryProviderAddress;
    }

    @Override
    public Collection<GlobalDiscoveryEntry> getDiscoveryEntries() {

        List<GlobalDiscoveryEntry> provisionedList = new ArrayList<>();
        String defaultPulicKeyId = "";
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        provisionedList.add(CapabilityUtils.newGlobalDiscoveryEntry(getVersionFromAnnotation(DiscoveryProvider.class),
                                                                    systemServicesDomain,
                                                                    Discovery.INTERFACE_NAME,
                                                                    discoveryProviderParticipantId,
                                                                    providerQos,
                                                                    System.currentTimeMillis(),
                                                                    NO_EXPIRY,
                                                                    defaultPulicKeyId,
                                                                    discoveryProviderAddress));

        return provisionedList;
    }
}
