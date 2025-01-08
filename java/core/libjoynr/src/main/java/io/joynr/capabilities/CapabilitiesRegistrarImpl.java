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
package io.joynr.capabilities;

import static io.joynr.util.VersionUtil.getVersionFromAnnotation;

import io.joynr.dispatching.rpc.RequestInterpreter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.dispatching.ProviderDirectory;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.provider.ProviderContainer;
import io.joynr.provider.ProviderContainerFactory;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.runtime.SystemServicesSettings;
import joynr.system.DiscoveryAsync;
import joynr.system.RoutingTypes.Address;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryError;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

@Singleton
public class CapabilitiesRegistrarImpl implements CapabilitiesRegistrar {

    private static final Logger logger = LoggerFactory.getLogger(CapabilitiesRegistrarImpl.class);

    private final DiscoveryAsync localDiscoveryAggregator;
    private final MessageRouter messageRouter;
    private final ParticipantIdStorage participantIdStorage;
    private final Address libjoynrMessagingAddress;
    private final ProviderDirectory providerDirectory;
    private final ProviderContainerFactory providerContainerFactory;

    private final RequestInterpreter requestInterpreter;

    private final long defaultExpiryTimeMs;

    @Inject
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public CapabilitiesRegistrarImpl(final DiscoveryAsync localDiscoveryAggregator,
                                     final ProviderContainerFactory providerContainerFactory,
                                     final MessageRouter messageRouter,
                                     final ProviderDirectory providerDirectory,
                                     final ParticipantIdStorage participantIdStorage,
                                     @Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS) final long defaultExpiryTimeMs,
                                     @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) final Address dispatcherAddress,
                                     final RequestInterpreter requestInterpreter) {
        super();
        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.providerContainerFactory = providerContainerFactory;
        this.messageRouter = messageRouter;
        this.providerDirectory = providerDirectory;
        this.participantIdStorage = participantIdStorage;
        this.defaultExpiryTimeMs = defaultExpiryTimeMs;
        this.libjoynrMessagingAddress = dispatcherAddress;
        this.requestInterpreter = requestInterpreter;
    }

    /*
     * (non-Javadoc)
     *
     * @see io.joynr.capabilities.CapabilitiesRegistrar# registerProvider(java.lang.String,
     * io.joynr.provider.JoynrProvider, java.lang.Class, boolean)
     */
    @Override
    public Future<Void> registerProvider(final String domain,
                                         final Object provider,
                                         final ProviderQos providerQos,
                                         final String[] gbids,
                                         final boolean awaitGlobalRegistration) {
        final DiscoveryEntry discoveryEntry = buildDiscoveryEntryAndAddProviderContainer(domain, provider, providerQos);
        try {
            addRoutingEntry(discoveryEntry, provider);
        } catch (final JoynrRuntimeException error) {
            final Future<Void> newFuture = new Future<>();
            newFuture.onFailure(error);
            return newFuture;
        }
        final CallbackWithModeledError<Void, DiscoveryError> callback = buildAddCallback(discoveryEntry);
        return localDiscoveryAggregator.add(callback, discoveryEntry, awaitGlobalRegistration, gbids);
    }

    @Override
    @Deprecated
    public Future<Void> registerInAllKnownBackends(final String domain,
                                                   final Object provider,
                                                   final ProviderQos providerQos,
                                                   final boolean awaitGlobalRegistration) {
        final DiscoveryEntry discoveryEntry = buildDiscoveryEntryAndAddProviderContainer(domain, provider, providerQos);
        try {
            addRoutingEntry(discoveryEntry, provider);
        } catch (final JoynrRuntimeException error) {
            final Future<Void> newFuture = new Future<>();
            newFuture.onFailure(error);
            return newFuture;
        }
        final CallbackWithModeledError<Void, DiscoveryError> callback = buildAddCallback(discoveryEntry);
        return localDiscoveryAggregator.addToAll(callback, discoveryEntry, awaitGlobalRegistration);
    }

    private DiscoveryEntry buildDiscoveryEntryAndAddProviderContainer(final String domain,
                                                                      final Object provider,
                                                                      final ProviderQos providerQos) {
        if (providerQos == null) {
            throw new JoynrRuntimeException("providerQos == null. It must not be null");
        }
        final ProviderContainer providerContainer = providerContainerFactory.create(provider);
        final String participantId = participantIdStorage.getProviderParticipantId(domain,
                                                                                   providerContainer.getInterfaceName(),
                                                                                   providerContainer.getMajorVersion());
        final String defaultPublicKeyId = "";
        final DiscoveryEntry discoveryEntry = new DiscoveryEntry(getVersionFromAnnotation(provider.getClass()),
                                                                 domain,
                                                                 providerContainer.getInterfaceName(),
                                                                 participantId,
                                                                 providerQos,
                                                                 System.currentTimeMillis(),
                                                                 System.currentTimeMillis() + defaultExpiryTimeMs,
                                                                 defaultPublicKeyId);
        providerDirectory.add(participantId, providerContainer);
        return discoveryEntry;
    }

    private void addRoutingEntry(final DiscoveryEntry discoveryEntry, final Object provider) {
        final boolean isGloballyVisible = (discoveryEntry.getQos().getScope() == ProviderScope.GLOBAL);
        final String participantId = discoveryEntry.getParticipantId();
        try {
            messageRouter.addNextHop(participantId, libjoynrMessagingAddress, isGloballyVisible);
        } catch (final Exception error) {
            // addNextHop throws when RoutingTable.put fails
            providerDirectory.remove(participantId);
            providerContainerFactory.removeProviderContainer(provider);
            throw (new JoynrRuntimeException("Error while adding routing entry: " + error));
        }
    }

    private CallbackWithModeledError<Void, DiscoveryError> buildAddCallback(final DiscoveryEntry discoveryEntry) {
        return new CallbackWithModeledError<>() {
            @Override
            public void onSuccess(final Void result) {
                logger.info("Successfully registered provider with participantId={} for domain={}, interfaceName={}, major={}, minor={}",
                            discoveryEntry.getParticipantId(),
                            discoveryEntry.getDomain(),
                            discoveryEntry.getInterfaceName(),
                            discoveryEntry.getProviderVersion().getMajorVersion(),
                            discoveryEntry.getProviderVersion().getMinorVersion());
            }

            @Override
            public void onFailure(final JoynrRuntimeException runtimeException) {
                logger.error("Error while registering provider with participantId={} for domain={}, interfaceName={}, major={}, minor={}:",
                             discoveryEntry.getParticipantId(),
                             discoveryEntry.getDomain(),
                             discoveryEntry.getInterfaceName(),
                             discoveryEntry.getProviderVersion().getMajorVersion(),
                             discoveryEntry.getProviderVersion().getMinorVersion(),
                             runtimeException);
                handleFailedAdd(discoveryEntry);
            }

            @Override
            public void onFailure(final DiscoveryError errorEnum) {
                logger.error("DiscoveryError while registering provider with participantId={} for domain={}, interfaceName={}, major={}, minor={}: {}",
                             discoveryEntry.getParticipantId(),
                             discoveryEntry.getDomain(),
                             discoveryEntry.getInterfaceName(),
                             discoveryEntry.getProviderVersion().getMajorVersion(),
                             discoveryEntry.getProviderVersion().getMinorVersion(),
                             errorEnum);
                handleFailedAdd(discoveryEntry);
            }

            private void handleFailedAdd(final DiscoveryEntry discoveryEntry) {
                try {
                    messageRouter.removeNextHop(discoveryEntry.getParticipantId());
                } catch (final Exception error) {
                    // removeNextHop throws only in case of libjoynr runtime if the communication with the CC fails
                    logger.error("Error while removing routing entry of provider with participantId={} for domain={}, interfaceName={}, major={}, minor={} after failed registration: ",
                                 discoveryEntry.getParticipantId(),
                                 discoveryEntry.getDomain(),
                                 discoveryEntry.getInterfaceName(),
                                 discoveryEntry.getProviderVersion().getMajorVersion(),
                                 discoveryEntry.getProviderVersion().getMinorVersion(),
                                 error);
                }
            }
        };
    }

    @Override
    public Future<Void> unregisterProvider(final String domain, final Object provider) {
        final String interfaceName = ProviderAnnotations.getInterfaceName(provider);
        final int majorVersion = ProviderAnnotations.getMajorVersion(provider);
        final int minorVersion = ProviderAnnotations.getMinorVersion(provider);
        final String participantId = participantIdStorage.getProviderParticipantId(domain, interfaceName, majorVersion);
        final Future<Void> newFuture = new Future<>();
        final Callback<Void> callback = new Callback<>() {
            @Override
            public void onSuccess(final Void result) {
                logger.info("Successfully unregistered provider with participantId={} for domain={}, interfaceName={}, major={}, minor={}",
                            participantId,
                            domain,
                            interfaceName,
                            majorVersion,
                            minorVersion);
                try {
                    messageRouter.removeNextHop(participantId);
                } catch (final Exception error) {
                    // removeNextHop throws only in case of libjoynr runtime if the communication with the CC fails
                    logger.error("Error while removing routing entry of unregistered provider with participantId={} for domain={}, interfaceName={}, major={}, minor={}: ",
                                 participantId,
                                 domain,
                                 interfaceName,
                                 majorVersion,
                                 minorVersion,
                                 error);
                }
                providerDirectory.remove(participantId);
                providerContainerFactory.removeProviderContainer(provider);
                requestInterpreter.removeAllMethodInformation(provider.getClass());
                newFuture.resolve();
            }

            @Override
            public void onFailure(final JoynrRuntimeException error) {
                logger.error("Error while unregistering provider with participantId={} for domain={}, interfaceName={}, major={}, minor={}: ",
                             participantId,
                             domain,
                             interfaceName,
                             majorVersion,
                             minorVersion,
                             error);
                newFuture.onFailure(error);
            }
        };

        localDiscoveryAggregator.remove(callback, participantId);
        return newFuture;
    }
}
