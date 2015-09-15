package io.joynr.capabilities.directory;

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

import io.joynr.capabilities.CapabilitiesProvisioning;
import io.joynr.capabilities.CapabilitiesStore;
import io.joynr.capabilities.CapabilitiesStorePersisted;
import io.joynr.capabilities.CapabilityEntry;
import io.joynr.capabilities.CapabilityEntryPersisted;
import io.joynr.capabilities.CustomParameterPersisted;
import io.joynr.capabilities.DefaultCapabilitiesProvisioning;
import io.joynr.capabilities.ProviderQosPersisted;
import io.joynr.endpoints.AddressPersisted;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.infrastructure.GlobalCapabilitiesDirectoryAbstractProvider;
import joynr.system.routingtypes.Address;
import joynr.types.CustomParameter;
import joynr.types.ProviderQos;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.name.Named;

public class CapabilitiesDirectoryModule extends AbstractModule {

    @Override
    protected void configure() {
        bind(GlobalCapabilitiesDirectoryAbstractProvider.class).to(CapabilitiesDirectoryImpl.class);
        bind(CapabilitiesStore.class).to(CapabilitiesStorePersisted.class);
        bind(CapabilityEntry.class).to(CapabilityEntryPersisted.class);
        bind(CustomParameter.class).to(CustomParameterPersisted.class);
        bind(ProviderQos.class).to(ProviderQosPersisted.class);
        bind(Address.class).to(AddressPersisted.class);

        bind(CapabilitiesProvisioning.class).to(DefaultCapabilitiesProvisioning.class);
    }

    @Provides
    @Named(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL)
    String provideCapabilitiesDirectoryDomain(@Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String capDirDomain) {
        return capDirDomain;
    }

    @Provides
    @Named(MessagingPropertyKeys.CHANNELID)
    String provideCapabilitiesDirectoryChannelId(@Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabilitiesDirectoryChannelId) {
        return capabilitiesDirectoryChannelId;
    }

}
