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

import static io.joynr.util.VersionUtil.getVersionFromAnnotation;

import javax.annotation.CheckForNull;
import javax.inject.Named;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;

import io.joynr.dispatching.ProviderDirectory;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.provider.ProviderContainer;
import io.joynr.provider.ProviderContainerFactory;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.runtime.SystemServicesSettings;
import joynr.system.DiscoveryAsync;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

@Singleton
public class CapabilitiesRegistrarImpl implements CapabilitiesRegistrar {

    private static final Logger logger = LoggerFactory.getLogger(CapabilitiesRegistrarImpl.class);

    private DiscoveryAsync localDiscoveryAggregator;
    private final MessageRouter messageRouter;
    private ParticipantIdStorage participantIdStorage;
    private Address libjoynrMessagingAddress;
    private ProviderDirectory providerDirectory;
    private ProviderContainerFactory providerContainerFactory;

    private long defaultExpiryTimeMs;

    @Inject
    public CapabilitiesRegistrarImpl(DiscoveryAsync localDiscoveryAggregator,
                                     ProviderContainerFactory providerContainerFactory,
                                     MessageRouter messageRouter,
                                     ProviderDirectory providerDirectory,
                                     ParticipantIdStorage participantIdStorage,
                                     @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS) long defaultExpiryTimeMs,
                                     @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress) {
        super();
        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.providerContainerFactory = providerContainerFactory;
        this.messageRouter = messageRouter;
        this.providerDirectory = providerDirectory;
        this.participantIdStorage = participantIdStorage;
        this.defaultExpiryTimeMs = defaultExpiryTimeMs;
        this.libjoynrMessagingAddress = dispatcherAddress;
    }

    /*
     * (non-Javadoc)
     *
     * @see io.joynr.capabilities.CapabilitiesRegistrar# registerProvider(java.lang.String,
     * io.joynr.provider.JoynrProvider, java.lang.Class)
     */
    @Override
    public Future<Void> registerProvider(final String domain, Object provider, ProviderQos providerQos) {
        if (providerQos == null) {
            throw new JoynrRuntimeException("providerQos == null. It must not be null");
        }
        ProviderContainer providerContainer = providerContainerFactory.create(provider);
        String participantId = participantIdStorage.getProviderParticipantId(domain,
                                                                             providerContainer.getInterfaceName());
        String defaultPublicKeyId = "";
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(getVersionFromAnnotation(provider.getClass()),
                                                           domain,
                                                           providerContainer.getInterfaceName(),
                                                           participantId,
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           System.currentTimeMillis() + defaultExpiryTimeMs,
                                                           defaultPublicKeyId);
        final boolean isGloballyVisible = (discoveryEntry.getQos().getScope() == ProviderScope.GLOBAL);
        messageRouter.addNextHop(participantId, libjoynrMessagingAddress, isGloballyVisible);
        providerDirectory.add(participantId, providerContainer);

        Callback<Void> callback = new Callback<Void>() {
            @Override
            public void onSuccess(@CheckForNull Void result) {

            }

            @Override
            public void onFailure(JoynrRuntimeException runtimeException) {
                logger.error("Unexpected Error while registering Provider:", runtimeException);

            }
        };
        return localDiscoveryAggregator.add(callback, discoveryEntry);
    }

    @Override
    public void unregisterProvider(String domain, Object provider) {

        final String participantId = participantIdStorage.getProviderParticipantId(domain,
                                                                                   ProviderAnnotations.getInterfaceName(provider));
        Callback<Void> callback = new Callback<Void>() {
            @Override
            public void onSuccess(@CheckForNull Void result) {
            }

            @Override
            public void onFailure(JoynrRuntimeException error) {
                logger.error("Error while unregistering provider: ", error);
            }
        };
        localDiscoveryAggregator.remove(callback, participantId);
        providerDirectory.remove(participantId);
    }
}
