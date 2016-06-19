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

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID;
import static io.joynr.messaging.MessagingPropertyKeys.CAPABILITYDIRECTORYURL;
import static io.joynr.messaging.MessagingPropertyKeys.CHANNELID;
import static io.joynr.messaging.MessagingPropertyKeys.DISCOVERYDIRECTORYURL;

import static io.joynr.runtime.JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP;

import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import javax.inject.Named;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.TypeLiteral;
import com.google.inject.assistedinject.FactoryModuleBuilder;
import com.google.inject.multibindings.MapBinder;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;

import io.joynr.arbitration.ArbitratorFactory;
import io.joynr.capabilities.CapabilitiesProvisioning;
import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.CapabilitiesRegistrarImpl;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.ParticipantIdStorage;
import io.joynr.capabilities.PropertiesFileParticipantIdStorage;
import io.joynr.capabilities.StaticCapabilitiesProvisioning;
import io.joynr.context.JoynrMessageScopeModule;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.dispatching.DispatcherImpl;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.RequestReplyManagerImpl;
import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.dispatching.subscription.PublicationManager;
import io.joynr.dispatching.subscription.PublicationManagerImpl;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.dispatching.subscription.SubscriptionManagerImpl;
import io.joynr.exceptions.JoynrDelayMessageException;
import io.joynr.logging.JoynrAppenderManagerFactory;
import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.IMessaging;
import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.MessagingSettings;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.inprocess.InProcessLibjoynrMessagingSkeleton;
import io.joynr.messaging.inprocess.InProcessMessageSerializerFactory;
import io.joynr.messaging.inprocess.InProcessMessagingStubFactory;
import io.joynr.messaging.routing.GlobalAddressFactory;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.messaging.routing.RoutingTableImpl;
import io.joynr.messaging.serialize.AbstractMiddlewareMessageSerializerFactory;
import io.joynr.messaging.serialize.MessageSerializerFactory;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.proxy.ProxyBuilderFactoryImpl;
import io.joynr.proxy.ProxyInvocationHandler;
import io.joynr.proxy.ProxyInvocationHandlerFactory;
import io.joynr.proxy.ProxyInvocationHandlerImpl;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.GlobalDomainAccessController;
import joynr.system.DiscoveryAsync;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

abstract class AbstractRuntimeModule extends AbstractModule {

    private static final Logger logger = LoggerFactory.getLogger(AbstractRuntimeModule.class);

    private static final long NO_EXPIRY = Long.MAX_VALUE;

    MapBinder<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory<? extends IMessaging, ? extends Address>> messagingStubFactory;
    MapBinder<Class<? extends Address>, AbstractMiddlewareMessageSerializerFactory<? extends Address>> messageSerializerFactory;
    MapBinder<Class<? extends Address>, IMessagingSkeleton> messagingSkeletonFactory;

    @Override
    protected void configure() {
        requestStaticInjection(CapabilityUtils.class,
                               RpcUtils.class,
                               ArbitratorFactory.class,
                               JoynrDelayMessageException.class,
                               JoynrAppenderManagerFactory.class);

        install(new JsonMessageSerializerModule());
        install(new FactoryModuleBuilder().implement(ProxyInvocationHandler.class, ProxyInvocationHandlerImpl.class)
                                          .build(ProxyInvocationHandlerFactory.class));
        install(new JoynrMessageScopeModule());

        messagingStubFactory = MapBinder.newMapBinder(binder(), new TypeLiteral<Class<? extends Address>>() {
        }, new TypeLiteral<AbstractMiddlewareMessagingStubFactory<? extends IMessaging, ? extends Address>>() {
        }, Names.named(MessagingStubFactory.MIDDLEWARE_MESSAGING_STUB_FACTORIES));
        messagingStubFactory.addBinding(InProcessAddress.class).to(InProcessMessagingStubFactory.class);

        messageSerializerFactory = MapBinder.newMapBinder(binder(), new TypeLiteral<Class<? extends Address>>() {
        }, new TypeLiteral<AbstractMiddlewareMessageSerializerFactory<? extends Address>>() {
        }, Names.named(MessageSerializerFactory.MIDDLEWARE_MESSAGE_SERIALIZER_FACTORIES));
        messageSerializerFactory.addBinding(InProcessAddress.class).to(InProcessMessageSerializerFactory.class);

        messagingSkeletonFactory = MapBinder.newMapBinder(binder(), new TypeLiteral<Class<? extends Address>>() {
        }, new TypeLiteral<IMessagingSkeleton>() {
        }, Names.named(MessagingSkeletonFactory.MIDDLEWARE_MESSAGING_SKELETONS));
        messagingSkeletonFactory.addBinding(InProcessAddress.class).to(InProcessLibjoynrMessagingSkeleton.class);

        // other address types must be added to the Multibinder to support global addressing. Created here to make
        // sure the Set exists, even if empty.
        Multibinder.newSetBinder(binder(), new TypeLiteral<GlobalAddressFactory<? extends Address>>() {
        });

        bind(ProxyBuilderFactory.class).to(ProxyBuilderFactoryImpl.class);
        bind(RequestReplyManager.class).to(RequestReplyManagerImpl.class);
        bind(SubscriptionManager.class).to(SubscriptionManagerImpl.class);
        bind(PublicationManager.class).to(PublicationManagerImpl.class);
        bind(Dispatcher.class).to(DispatcherImpl.class);
        bind(LocalDiscoveryAggregator.class).in(Singleton.class);
        bind(DiscoveryAsync.class).to(LocalDiscoveryAggregator.class);
        bind(CapabilitiesRegistrar.class).to(CapabilitiesRegistrarImpl.class);
        bind(ParticipantIdStorage.class).to(PropertiesFileParticipantIdStorage.class);
        bind(MessagingSettings.class).to(ConfigurableMessagingSettings.class);
        bind(RoutingTable.class).to(RoutingTableImpl.class).asEagerSingleton();
        bind(CapabilitiesProvisioning.class).to(StaticCapabilitiesProvisioning.class).asEagerSingleton();

        ThreadFactory namedThreadFactory = new ThreadFactoryBuilder().setNameFormat("joynr.Cleanup-%d").build();
        ScheduledExecutorService cleanupExecutor = Executors.newSingleThreadScheduledExecutor(namedThreadFactory);
        bind(ScheduledExecutorService.class).annotatedWith(Names.named(JOYNR_SCHEDULER_CLEANUP))
                                            .toInstance(cleanupExecutor);
    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS)
    Address getDispatcherAddress() {
        return new InProcessAddress();
    }

    @Provides
    @Singleton
    @Named(ConfigurableMessagingSettings.PROPERTY_CAPABILITIES_DIRECTORY_ADDRESS)
    // CHECKSTYLE:OFF
    Address getCapabilitiesDirectoryAddress(CapabilitiesProvisioning capabilitiesProvisioning,
                                            ObjectMapper objectMapper,
                                            @Named(DISCOVERYDIRECTORYURL) String discoveryDirectoryUrl,
                                            @Named(CAPABILITYDIRECTORYURL) String deprecatedCapabilityDirectoryUrl,
                                            @Named(CHANNELID) String channelId,
                                            @Named(PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String domain,
                                            @Named(PROPERTY_CAPABILITIES_DIRECTORY_PARTICIPANT_ID) String participantId,
                                            @Named(PROPERTY_CAPABILITIES_DIRECTORY_CHANNEL_ID) String capabilitiesDirectoryChannelId) {
        // CHECKSTYLE:ON
        Address address = getProvisionedAddressForInterface(capabilitiesProvisioning,
                                                            GlobalCapabilitiesDirectory.INTERFACE_NAME);
        // The following falls back to the deprecated properties if no
        // global domain access controller was provisioned via JSON.
        // Will be removed in the future.
        if (address == null) {
            // deprecated: will be removed by 2016-12-31
            if (deprecatedCapabilityDirectoryUrl != null && deprecatedCapabilityDirectoryUrl.length() > 0) {
                address = getAddress(deprecatedCapabilityDirectoryUrl, channelId, capabilitiesDirectoryChannelId);
            } else {
                address = getAddress(discoveryDirectoryUrl, channelId, capabilitiesDirectoryChannelId);
            }
            addDiscoveryEntry(capabilitiesProvisioning,
                              address,
                              GlobalCapabilitiesDirectory.class,
                              GlobalCapabilitiesDirectory.INTERFACE_NAME,
                              capabilitiesDirectoryChannelId,
                              domain,
                              participantId);
        }
        return address;
    }

    @Provides
    @Singleton
    @Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_ADDRESS)
    Address getDomainAccessControllerAddress(CapabilitiesProvisioning capabilitiesProvisioning,
                                             @Named(DISCOVERYDIRECTORYURL) String discoveryDirectoryUrl,
                                             @Named(CAPABILITYDIRECTORYURL) String deprecatedCapabilityDirectoryUrl,
                                             @Named(CHANNELID) String channelId,
                                             @Named(PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String domain,
                                             @Named(PROPERTY_DOMAIN_ACCESS_CONTROLLER_PARTICIPANT_ID) String participantId,
                                             @Named(PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID) String domainAccessControllerChannelId) {
        Address address = getProvisionedAddressForInterface(capabilitiesProvisioning,
                                                            GlobalDomainAccessController.INTERFACE_NAME);
        // The following falls back to the deprecated properties if no
        // global domain access controller was provisioned via JSON.
        // Will be removed in the future.
        if (address == null) {
            if (deprecatedCapabilityDirectoryUrl != null && deprecatedCapabilityDirectoryUrl.length() > 0) {
                address = getAddress(deprecatedCapabilityDirectoryUrl, channelId, domainAccessControllerChannelId);
            } else {
                address = getAddress(discoveryDirectoryUrl, channelId, domainAccessControllerChannelId);
            }
            addDiscoveryEntry(capabilitiesProvisioning,
                              address,
                              GlobalDomainAccessController.class,
                              GlobalDomainAccessController.INTERFACE_NAME,
                              channelId,
                              domainAccessControllerChannelId,
                              participantId);
        }
        return address;
    }

    private void addDiscoveryEntry(CapabilitiesProvisioning capabilitiesProvisioning,
                                   Address address,
                                   Class<?> interfaceClass,
                                   String interfaceName,
                                   String channelId,
                                   String domain,
                                   String participantId) {
        DiscoveryEntry discoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                                domain,
                                                                                interfaceName,
                                                                                participantId,
                                                                                new ProviderQos(),
                                                                                System.currentTimeMillis(),
                                                                                NO_EXPIRY,
                                                                                "",
                                                                                address);
        capabilitiesProvisioning.getDiscoveryEntries().add(discoveryEntry);
    }

    private Address getProvisionedAddressForInterface(CapabilitiesProvisioning capabilitiesProvisioning,
                                                      String interfaceName) {
        Address result = null;
        for (DiscoveryEntry discoveryEntry : capabilitiesProvisioning.getDiscoveryEntries()) {
            if (discoveryEntry instanceof GlobalDiscoveryEntry
                    && interfaceName.equals(discoveryEntry.getInterfaceName())) {
                result = CapabilityUtils.getAddressFromGlobalDiscoveryEntry((GlobalDiscoveryEntry) discoveryEntry);
            }
        }
        return result;
    }

    @Provides
    @Named(MessageRouter.SCHEDULEDTHREADPOOL)
    ScheduledExecutorService provideMessageSchedulerThreadPoolExecutor(@Named(ConfigurableMessagingSettings.PROPERTY_MESSAGING_MAXIMUM_PARALLEL_SENDS) int maximumParallelSends) {
        ThreadFactory schedulerNamedThreadFactory = new ThreadFactoryBuilder().setNameFormat("joynr.MessageScheduler-scheduler-%d")
                                                                              .build();
        ScheduledThreadPoolExecutor scheduler = new ScheduledThreadPoolExecutor(maximumParallelSends,
                                                                                schedulerNamedThreadFactory);
        scheduler.setKeepAliveTime(100, TimeUnit.SECONDS);
        scheduler.allowCoreThreadTimeOut(true);
        return scheduler;
    }

    private Address getAddress(String serverUrl, String localChannelId, String targetChannelId) {
        if (localChannelId.equals(targetChannelId)) {
            return new InProcessAddress();
        } else if (serverUrl.startsWith("http")) {
            return new ChannelAddress(serverUrl, targetChannelId);
        } else {
            return new MqttAddress(serverUrl, targetChannelId);
        }
    }
}
