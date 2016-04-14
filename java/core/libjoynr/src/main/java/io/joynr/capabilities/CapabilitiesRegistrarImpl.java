package io.joynr.capabilities;

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

import javax.annotation.CheckForNull;
import javax.inject.Named;
import io.joynr.runtime.SystemServicesSettings;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.inject.Inject;
import com.google.inject.Singleton;
import io.joynr.dispatching.RequestCaller;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.RequestCallerFactory;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import joynr.system.DiscoveryAsync;
import joynr.system.RoutingTypes.Address;
import joynr.types.CommunicationMiddleware;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;

@Singleton
public class CapabilitiesRegistrarImpl implements CapabilitiesRegistrar {

    private static final Logger logger = LoggerFactory.getLogger(CapabilitiesRegistrarImpl.class);

    private DiscoveryAsync localDiscoveryAggregator;
    private RequestCallerFactory requestCallerFactory;
    private final MessageRouter messageRouter;
    private ParticipantIdStorage participantIdStorage;
    private Address libjoynrMessagingAddress;
    private RequestCallerDirectory requestCallerDirectory;

    @Inject
    public CapabilitiesRegistrarImpl(DiscoveryAsync localDiscoveryAggregator,
                                     RequestCallerFactory requestCallerFactory,
                                     MessageRouter messageRouter,
                                     RequestCallerDirectory requestCallerDirectory,
                                     ParticipantIdStorage participantIdStorage,
                                     @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress) {
        super();
        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.requestCallerFactory = requestCallerFactory;
        this.messageRouter = messageRouter;
        this.requestCallerDirectory = requestCallerDirectory;
        this.participantIdStorage = participantIdStorage;
        this.libjoynrMessagingAddress = dispatcherAddress;
    }

    /*
     * (non-Javadoc)
     *
     * @see io.joynr.capabilities.CapabilitiesRegistrar# registerProvider(java.lang.String,
     * io.joynr.provider.JoynrProvider, java.lang.Class)
     */
    @Override
    public Future<Void> registerProvider(final String domain, JoynrProvider provider, ProviderQos providerQos) {
        String participantId = participantIdStorage.getProviderParticipantId(domain, provider.getProvidedInterface());
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(domain,
                                                           provider.getInterfaceName(),
                                                           participantId,
                                                           providerQos,
                                                           new CommunicationMiddleware[]{ CommunicationMiddleware.JOYNR });
        RequestCaller requestCaller = requestCallerFactory.create(provider);

        messageRouter.addNextHop(participantId, libjoynrMessagingAddress);
        requestCallerDirectory.add(participantId, requestCaller);

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
    public void unregisterProvider(String domain, JoynrProvider provider) {

        final String participantId = participantIdStorage.getProviderParticipantId(domain,
                                                                                   provider.getProvidedInterface());
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
        requestCallerDirectory.remove(participantId);
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        /* save added capabilities to unregister at shutdown ?
        if(unregisterAllRegisteredCapabilities) {
            for (Map.Entry<String, JoynrProvider> entry : registeredLocalProviders.entrySet()) {
                String domain = entry.getKey();
                JoynrProvider joynrProvider = entry.getValue();
                unregisterProvider(domain, joynrProvider);
            }
        }
        registeredLocalProviders.clear();
         */
        if (unregisterAllRegisteredCapabilities) {
            logger.warn("unregisterAllRegisteredCapabilities is not implemented!");
        }
    }

}
