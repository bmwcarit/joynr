package io.joynr.capabilities;

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

import com.google.inject.AbstractModule;
import com.google.inject.name.Names;
import joynr.types.GlobalDiscoveryEntry;

import static io.joynr.messaging.MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY;
import static io.joynr.messaging.MessagingPropertyKeys.DOMAIN_ACCESS_CONTROLLER_DISCOVERY_ENTRY;

public class StaticCapabilitiesProvisioningModule extends AbstractModule {

    @Override
    protected void configure() {
        bind(CapabilitiesProvisioning.class).to(StaticCapabilitiesProvisioning.class).asEagerSingleton();
        bind(GlobalDiscoveryEntry.class).annotatedWith(Names.named(CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY))
                                        .toProvider(GlobalCapabilitiesDirectoryDiscoveryEntryProvider.class);
        bind(GlobalDiscoveryEntry.class).annotatedWith(Names.named(DOMAIN_ACCESS_CONTROLLER_DISCOVERY_ENTRY))
                                        .toProvider(GlobalDomainAccessControllerDiscoveryEntryProvider.class);
    }

}
