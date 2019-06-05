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
package io.joynr.capabilities.directory;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.name.Named;

import io.joynr.capabilities.CustomParameterPersisted;
import io.joynr.capabilities.ProviderQosPersisted;
import io.joynr.endpoints.AddressPersisted;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.infrastructure.GlobalCapabilitiesDirectoryAbstractProvider;
import joynr.system.RoutingTypes.Address;
import joynr.types.CustomParameter;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;

public class CapabilitiesDirectoryModule extends AbstractModule {

    @Override
    protected void configure() {
        bind(GlobalCapabilitiesDirectoryAbstractProvider.class).to(CapabilitiesDirectoryImpl.class);
        bind(CustomParameter.class).to(CustomParameterPersisted.class);
        bind(ProviderQos.class).to(ProviderQosPersisted.class);
        bind(Address.class).to(AddressPersisted.class);
    }

    @Provides
    @Named(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL)
    String provideCapabilitiesDirectoryDomain(@Named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY) GlobalDiscoveryEntry capabilitiesDirectoryEntry) {
        return capabilitiesDirectoryEntry.getDomain();
    }

    @Provides
    @Named(MessagingPropertyKeys.CHANNELID)
    String provideCapabilitiesDirectoryChannelId(@Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabilitiesDirectoryChannelId) {
        return capabilitiesDirectoryChannelId;
    }

}
