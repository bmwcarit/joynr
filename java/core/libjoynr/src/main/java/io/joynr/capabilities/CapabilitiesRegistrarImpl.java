package io.joynr.capabilities;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.dispatcher.RequestCaller;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.RequestCallerFactory;
import io.joynr.pubsub.publication.PublicationManager;
import joynr.system.routingtypes.Address;

import com.google.inject.Inject;
import com.google.inject.Singleton;

@Singleton
public class CapabilitiesRegistrarImpl implements CapabilitiesRegistrar {
    private LocalCapabilitiesDirectory localCapabilitiesDirectory;
    private RequestCallerFactory requestCallerFactory;
    private RequestReplyDispatcher dispatcher;
    private final MessageRouter messageRouter;
    private final PublicationManager publicationManager;
    private ParticipantIdStorage participantIdStorage;
    private Address libjoynrMessagingAddress;

    @Inject
    public CapabilitiesRegistrarImpl(LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                     RequestCallerFactory requestCallerFactory,
                                     RequestReplyDispatcher dispatcher,
                                     MessageRouter messageRouter,
                                     PublicationManager publicationManager,
                                     ParticipantIdStorage participantIdStorage,
                                     @Named(ConfigurableMessagingSettings.PROPERTY_LIBJOYNR_MESSAGING_ADDRESS) Address libjoynrMessagingAddress) {
        super();
        this.localCapabilitiesDirectory = localCapabilitiesDirectory;
        this.requestCallerFactory = requestCallerFactory;
        this.dispatcher = dispatcher;
        this.messageRouter = messageRouter;
        this.publicationManager = publicationManager;
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
    public RegistrationFuture registerProvider(final String domain, JoynrProvider provider) {
        String participantId = participantIdStorage.getProviderParticipantId(domain, provider.getProvidedInterface());
        CapabilityEntry capabilityEntry = new CapabilityEntryImpl(domain,
                                                                  provider.getInterfaceName(),
                                                                  provider.getProviderQos(),
                                                                  participantId,
                                                                  System.currentTimeMillis());
        RequestCaller requestCaller = requestCallerFactory.create(provider);

        messageRouter.addNextHop(participantId, libjoynrMessagingAddress);
        dispatcher.addRequestCaller(participantId, requestCaller);
        RegistrationFuture ret = localCapabilitiesDirectory.add(capabilityEntry);
        publicationManager.restoreQueuedSubscription(participantId, requestCaller);
        return ret;
    }

    @Override
    public void unregisterProvider(String domain, JoynrProvider provider) {

        String participantId = participantIdStorage.getProviderParticipantId(domain, provider.getProvidedInterface());
        CapabilityEntry capabilityEntry = new CapabilityEntryImpl(domain,
                                                                  provider.getInterfaceName(),
                                                                  provider.getProviderQos(),
                                                                  participantId,
                                                                  System.currentTimeMillis());
        localCapabilitiesDirectory.remove(capabilityEntry);
        dispatcher.removeRequestCaller(participantId);
        publicationManager.stopPublicationByProviderId(participantId);
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        localCapabilitiesDirectory.shutdown(unregisterAllRegisteredCapabilities);

    }

}
