package io.joynr.capabilities;

import javax.annotation.CheckForNull;
import javax.inject.Named;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.Singleton;

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

import io.joynr.dispatching.RequestCaller;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.exceptions.JoynrException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.RequestCallerFactory;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import joynr.system.DiscoveryAsync;
import joynr.system.RoutingTypes.Address;
import joynr.types.CommunicationMiddleware;
import joynr.types.DiscoveryEntry;

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
                                     @Named(ConfigurableMessagingSettings.PROPERTY_LIBJOYNR_MESSAGING_ADDRESS) Address libjoynrMessagingAddress) {
        super();
        this.localDiscoveryAggregator = localDiscoveryAggregator;
        this.requestCallerFactory = requestCallerFactory;
        this.messageRouter = messageRouter;
        this.requestCallerDirectory = requestCallerDirectory;
        this.participantIdStorage = participantIdStorage;
        this.libjoynrMessagingAddress = libjoynrMessagingAddress;
    }

    /*
     * (non-Javadoc)
     *
     * @see io.joynr.capabilities.CapabilitiesRegistrar# registerProvider(java.lang.String,
     * io.joynr.provider.JoynrProvider, java.lang.Class)
     */
    @Override
    public Future<Void> registerProvider(final String domain, JoynrProvider provider) {
        String participantId = participantIdStorage.getProviderParticipantId(domain, provider.getProvidedInterface());
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(domain,
                                                           provider.getInterfaceName(),
                                                           participantId,
                                                           provider.getProviderQos(),
                                                           new CommunicationMiddleware[]{ CommunicationMiddleware.JOYNR });
        RequestCaller requestCaller = requestCallerFactory.create(provider);

        messageRouter.addNextHop(participantId, libjoynrMessagingAddress);
        requestCallerDirectory.addCaller(participantId, requestCaller);

        Callback<Void> callback = new Callback<Void>() {
            @Override
            public void onSuccess(@CheckForNull Void result) {

            }

            @Override
            public void onFailure(JoynrException error) {
                logger.error("Error while registering Provider:", error);
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
            public void onFailure(JoynrException error) {
                logger.error("Error while unregistering provider: ", error);
            }
        };
        localDiscoveryAggregator.remove(callback, participantId);
        requestCallerDirectory.removeCaller(participantId);
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
