package io.joynr.discovery;

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

import io.joynr.capabilities.GlobalCapabilitiesDirectoryClient;
import io.joynr.capabilities.directory.CapabilitiesClientDummy;
import io.joynr.capabilities.directory.CapabilitiesDirectoryImpl;
import io.joynr.channel.ChannelUrlDirectoyImpl;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.infrastructure.ChannelUrlDirectoryAbstractProvider;
import joynr.infrastructure.GlobalCapabilitiesDirectoryAbstractProvider;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.name.Named;

public class DiscoveryDirectoriesModule extends AbstractModule {

    @Override
    protected void configure() {
        bind(ChannelUrlDirectoryAbstractProvider.class).to(ChannelUrlDirectoyImpl.class);
        bind(GlobalCapabilitiesDirectoryAbstractProvider.class).to(CapabilitiesDirectoryImpl.class);
        bind(GlobalCapabilitiesDirectoryClient.class).to(CapabilitiesClientDummy.class);
    }

    @Provides
    @Named(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL)
    String provideCapabilitiesDirectoryDomain(@Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String discoveryDirectoryDomain) {
        return discoveryDirectoryDomain;
    }

    @Provides
    @Named(MessagingPropertyKeys.CHANNELID)
    String provideCapabilitiesDirectoryChannelId(@Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String discoveryDirectoriesChannelId) {
        return discoveryDirectoriesChannelId;
    }

}
