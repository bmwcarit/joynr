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

import io.joynr.dispatcher.RequestCaller;
import io.joynr.dispatcher.RequestReplyDispatcher;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.RequestCallerFactory;
import io.joynr.pubsub.publication.PublicationManager;

import com.google.inject.Inject;
import com.google.inject.Singleton;

@Singleton
public class CapabilitiesRegistrarImpl implements CapabilitiesRegistrar {
    private LocalCapabilitiesDirectory localCapabilitiesDirectory;
    private RequestCallerFactory requestCallerFactory;
    private RequestReplyDispatcher dispatcher;
    private final PublicationManager publicationManager;
    private ParticipantIdStorage participantIdStorage;

    @Inject
    public CapabilitiesRegistrarImpl(LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                     RequestCallerFactory requestCallerFactory,
                                     RequestReplyDispatcher dispatcher,
                                     PublicationManager publicationManager,
                                     ParticipantIdStorage participantIdStorage) {
        super();
        this.localCapabilitiesDirectory = localCapabilitiesDirectory;
        this.requestCallerFactory = requestCallerFactory;
        this.dispatcher = dispatcher;
        this.publicationManager = publicationManager;
        this.participantIdStorage = participantIdStorage;
    }

    /*
     * (non-Javadoc)
     * 
     * @see io.joynr.capabilities.CapabilitiesRegistrar# registerCapability(java.lang.String,
     * io.joynr.provider.JoynrProvider, java.lang.Class)
     */
    @Override
    public <T extends JoynrInterface> RegistrationFuture registerCapability(final String domain,
                                                                            JoynrProvider provider,
                                                                            final Class<T> providedInterface,
                                                                            String authenticationToken) {
        String participantId = participantIdStorage.getProviderParticipantId(domain,
                                                                             providedInterface,
                                                                             authenticationToken);
        CapabilityEntry capabilityEntry = new CapabilityEntry(domain,
                                                              providedInterface,
                                                              provider.getProviderQos(),
                                                              participantId);
        RequestCaller requestCaller = requestCallerFactory.create(provider, providedInterface);

        dispatcher.addRequestCaller(participantId, requestCaller);
        RegistrationFuture ret = localCapabilitiesDirectory.add(capabilityEntry);
        publicationManager.restoreQueuedSubscription(participantId, requestCaller);
        return ret;
    }

    @Override
    public <T extends JoynrInterface> void unregisterCapability(String domain,
                                                                JoynrProvider provider,
                                                                final Class<T> providedInterface,
                                                                String authenticationToken) {
        String participantId = participantIdStorage.getProviderParticipantId(domain,
                                                                             providedInterface,
                                                                             authenticationToken);
        CapabilityEntry capabilityEntry = new CapabilityEntry(domain,
                                                              providedInterface,
                                                              provider.getProviderQos(),
                                                              participantId);
        localCapabilitiesDirectory.remove(capabilityEntry);
        dispatcher.removeRequestCaller(participantId);
        publicationManager.stopPublicationByProviderId(participantId);
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        localCapabilitiesDirectory.shutdown(unregisterAllRegisteredCapabilities);

    }

}
