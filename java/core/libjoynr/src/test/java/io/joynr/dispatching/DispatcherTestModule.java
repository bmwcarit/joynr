package io.joynr.dispatching;

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

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;

import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadFactory;

import javax.inject.Named;

import com.google.common.collect.Maps;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Names;

import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.dispatching.subscription.PublicationManager;
import io.joynr.dispatching.subscription.PublicationManagerImpl;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.dispatching.subscription.SubscriptionManagerImpl;
import io.joynr.messaging.AbstractMessagingStubFactory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessageReceiver;
import io.joynr.messaging.MessageSender;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.MessageRouterImpl;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.messaging.routing.RoutingTableImpl;
import io.joynr.proxy.JoynrMessagingConnectorFactory;
import io.joynr.runtime.SystemServicesSettings;
import joynr.Request;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;

public class DispatcherTestModule extends AbstractModule {

    @Override
    protected void configure() {

        bind(MessageSender.class).to(MessageSenderReceiverMock.class);
        bind(MessageReceiver.class).to(MessageSenderReceiverMock.class);
        bind(RequestReplyManager.class).to(RequestReplyManagerImpl.class);
        bind(MessageRouter.class).to(MessageRouterImpl.class);
        bind(RoutingTable.class).to(RoutingTableImpl.class).asEagerSingleton();
        bind(Dispatcher.class).to(DispatcherImpl.class);
        bind(SubscriptionManager.class).to(SubscriptionManagerImpl.class);
        bind(PublicationManager.class).to(PublicationManagerImpl.class);

        requestStaticInjection(RpcUtils.class, Request.class, JoynrMessagingConnectorFactory.class);

        ThreadFactory namedThreadFactory = new ThreadFactoryBuilder().setNameFormat("joynr.Cleanup-%d").build();
        ScheduledExecutorService cleanupExecutor = Executors.newSingleThreadScheduledExecutor(namedThreadFactory);
        bind(ScheduledExecutorService.class).annotatedWith(Names.named(JOYNR_SCHEDULER_CLEANUP))
                                            .toInstance(cleanupExecutor);
    }

    @Provides
    @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS)
    Address getCapabilitiesDirectoryAddress(@Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabilitiesDirectoryChannelId) {
        return new ChannelAddress(capabilitiesDirectoryChannelId);
    }

    @Provides
    @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_ADDRESS)
    Address getChannelUrlDirectoryAddress(@Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_CHANNEL_ID) String channelUrlDirectoryChannelId) {
        return new ChannelAddress(channelUrlDirectoryChannelId);
    }

    @Provides
    @Singleton
    @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_ADDRESS)
    Address getDomainAccessControllerAddress(@com.google.inject.name.Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID) String domainAccessControllerChannelId) {
        return new ChannelAddress(domainAccessControllerChannelId);
    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS)
    Address getDiscoveryProviderAddress() {
        return new InProcessAddress();
    }

    @SuppressWarnings("rawtypes")
    @Provides
    @Singleton
    Map<Class<? extends Address>, AbstractMessagingStubFactory> provideMessagingStubFactories() {
        Map<Class<? extends Address>, AbstractMessagingStubFactory> factories = Maps.newHashMap();
        return factories;
    }
}
