package io.joynr.runtime;

import java.util.Map;

import javax.inject.Named;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import com.google.common.collect.Maps;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Names;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.channel.ChannelMessagingSkeleton;
import io.joynr.messaging.channel.ChannelMessagingStubFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.MessageRouterImpl;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;

/**
 *  Use this module if you want to run libjoynr and cluster controller in one process
 */
public class CCInProcessRuntimeModule extends ClusterControllerRuntimeModule {

    @Override
    protected void configure() {
        super.configure();
        bind(JoynrRuntime.class).to(ClusterControllerRuntime.class).in(Singleton.class);
        bind(MessageRouter.class).to(MessageRouterImpl.class).in(Singleton.class);
        bind(IMessagingSkeleton.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_CLUSTERCONTROLER_MESSAGING_SKELETON))
                                      .to(ChannelMessagingSkeleton.class)
                                      .in(Singleton.class);
    }

    @SuppressWarnings("rawtypes")
    @Provides
    @Singleton
    Map<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory> provideMessagingStubFactories(ChannelMessagingStubFactory channelMessagingStubFactory) {
        Map<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory> factories = Maps.newHashMap();
        factories.put(ChannelAddress.class, channelMessagingStubFactory);
        return factories;
    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS)
    public Address provideCCMessagingAddress() {
        return new InProcessAddress();
    }

}
