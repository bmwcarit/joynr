package io.joynr.runtime;

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

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadFactory;

import javax.inject.Named;

import org.apache.http.client.config.RequestConfig;
import org.apache.http.impl.client.CloseableHttpClient;

import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.assistedinject.FactoryModuleBuilder;
import com.google.inject.name.Names;

import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.DispatcherImpl;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.RequestReplyManagerImpl;
import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.logging.JoynrAppenderManagerFactory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingSettings;
import io.joynr.messaging.channel.ChannelMessagingSkeleton;
import io.joynr.messaging.http.operation.HttpClientProvider;
import io.joynr.messaging.http.operation.HttpDefaultRequestConfigProvider;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.MessageRouterImpl;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.messaging.routing.RoutingTableImpl;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.proxy.ProxyBuilderFactoryImpl;
import io.joynr.proxy.ProxyInvocationHandler;
import io.joynr.proxy.ProxyInvocationHandlerFactory;
import io.joynr.proxy.ProxyInvocationHandlerImpl;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;

abstract class DefaultRuntimeModule extends AbstractModule {

    @Override
    protected void configure() {
        install(new FactoryModuleBuilder().implement(ProxyInvocationHandler.class, ProxyInvocationHandlerImpl.class)
                                          .build(ProxyInvocationHandlerFactory.class));

        bind(ProxyBuilderFactory.class).to(ProxyBuilderFactoryImpl.class);
        bind(RequestConfig.class).toProvider(HttpDefaultRequestConfigProvider.class).in(Singleton.class);
        bind(RequestReplyManager.class).to(RequestReplyManagerImpl.class);
        bind(Dispatcher.class).to(DispatcherImpl.class);
        bind(MessageRouter.class).to(MessageRouterImpl.class).in(Singleton.class);
        bind(MessagingSettings.class).to(ConfigurableMessagingSettings.class);
        bind(RoutingTable.class).to(RoutingTableImpl.class).asEagerSingleton();

        bind(CloseableHttpClient.class).toProvider(HttpClientProvider.class).in(Singleton.class);

        requestStaticInjection(RpcUtils.class, JoynrAppenderManagerFactory.class);

        ThreadFactory namedThreadFactory = new ThreadFactoryBuilder().setNameFormat("joynr.Cleanup-%d").build();
        ScheduledExecutorService cleanupExecutor = Executors.newSingleThreadScheduledExecutor(namedThreadFactory);
        bind(ScheduledExecutorService.class).annotatedWith(Names.named(JOYNR_SCHEDULER_CLEANUP))
                                            .toInstance(cleanupExecutor);
    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_LIBJOYNR_MESSAGING_ADDRESS)
    Address getLibJoynrMessagingAddress() {
        return new InProcessAddress();
    }

    @Provides
    @Singleton
    @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS)
    Address getCapabilitiesDirectoryAddress(@Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                            @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabilitiesDirectoryChannelId) {
        return getAddress(channelId, capabilitiesDirectoryChannelId);
    }

    @Provides
    @Singleton
    @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_ADDRESS)
    Address getChannelUrlDirectoryAddress(@Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                          @Named(ConfigurableMessagingSettings.PROPERTY_CHANNEL_URL_DIRECTORY_CHANNEL_ID) String channelUrlDirectoryChannelId) {
        return getAddress(channelId, channelUrlDirectoryChannelId);
    }

    @Provides
    @Singleton
    @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_ADDRESS)
    Address getDomainAccessControllerAddress(@Named(MessagingPropertyKeys.CHANNELID) String channelId,
                                             @com.google.inject.name.Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID) String domainAccessControllerChannelId) {
        return getAddress(channelId, domainAccessControllerChannelId);
    }

    @Provides
    @Singleton
    @Named(ConfigurableMessagingSettings.PROPERTY_CLUSTERCONTROLER_MESSAGING_SKELETON)
    IMessaging getClusterControllerMessagingSkeleton(MessageRouter messageRouter) {
        return new ChannelMessagingSkeleton(messageRouter);
    }

    private Address getAddress(String localChannelId, String targetChannelId) {
        if (localChannelId.equals(targetChannelId)) {
            return new InProcessAddress();
        } else {
            return new ChannelAddress(targetChannelId);
        }
    }
}
