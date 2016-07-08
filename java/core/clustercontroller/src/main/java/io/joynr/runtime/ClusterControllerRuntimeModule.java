package io.joynr.runtime;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import com.google.inject.name.Names;

import io.joynr.discovery.DiscoveryClientModule;
import io.joynr.messaging.NoBackendMessagingModule;
import io.joynr.messaging.routing.RoutingProviderImpl;
import io.joynr.security.DummyPlatformSecurityManager;
import io.joynr.security.PlatformSecurityManager;
import joynr.system.RoutingProvider;
import joynr.system.RoutingTypes.Address;

public abstract class ClusterControllerRuntimeModule extends AbstractRuntimeModule {
    public static final String GLOBAL_ADDRESS = "clustercontroller_global_address";

    @Override
    protected void configure() {
        super.configure();
        install(new DiscoveryClientModule());
        install(new NoBackendMessagingModule());
        bind(RoutingProvider.class).to(RoutingProviderImpl.class);

        bind(PlatformSecurityManager.class).to(DummyPlatformSecurityManager.class);
        bind(Address.class).annotatedWith(Names.named(GLOBAL_ADDRESS)).toProvider(GlobalAddressProvider.class);
    }
}
