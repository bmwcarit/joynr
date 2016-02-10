package io.joynr.runtime;

import java.util.Set;

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

import javax.inject.Named;

import com.google.inject.Provides;

import io.joynr.discovery.DiscoveryClientModule;
import io.joynr.messaging.NoBackendMessagingModule;
import io.joynr.messaging.routing.GlobalAddressFactory;
import io.joynr.security.DummyPlatformSecurityManager;
import io.joynr.security.PlatformSecurityManager;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;

public abstract class ClusterControllerRuntimeModule extends AbstractRuntimeModule {
    public static final String GLOBAL_ADDRESS = "clustercontroller_global_address";

    @Override
    protected void configure() {
        super.configure();
        install(new DiscoveryClientModule());
        install(new NoBackendMessagingModule());

        bind(PlatformSecurityManager.class).to(DummyPlatformSecurityManager.class);
    }

    @Provides
    @Named(GLOBAL_ADDRESS)
    public Address provideGlobalAddress(Set<GlobalAddressFactory> addressFactories) {
        Address mqttAddress = null;
        Address channelAddress = null;
        Address otherAddress = null;
        for (GlobalAddressFactory addressFactory : addressFactories) {
            Address address = addressFactory.create();
            if (address instanceof MqttAddress) {
                mqttAddress = address;
            } else if (address instanceof ChannelAddress) {
                channelAddress = address;
            } else {
                otherAddress = address;
            }
        }

        if (mqttAddress != null) {
            return mqttAddress;
        }
        if (channelAddress != null) {
            return channelAddress;
        }
        return otherAddress;
    }
}
