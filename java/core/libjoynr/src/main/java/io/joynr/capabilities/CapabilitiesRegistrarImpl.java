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

import io.joynr.dispatching.RequestCaller;
import io.joynr.dispatching.RequestCallerDirectory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.RequestCallerFactory;

import javax.inject.Named;

import joynr.system.routingtypes.Address;

import com.google.inject.Inject;
import com.google.inject.Singleton;

@Singleton
public class CapabilitiesRegistrarImpl implements CapabilitiesRegistrar {
    private LocalCapabilitiesDirectory localCapabilitiesDirectory;
    private RequestCallerFactory requestCallerFactory;
    private final MessageRouter messageRouter;
    private ParticipantIdStorage participantIdStorage;
    private Address libjoynrMessagingAddress;
    private RequestCallerDirectory requestCallerDirectory;

    @Inject
    public CapabilitiesRegistrarImpl(LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                     RequestCallerFactory requestCallerFactory,
                                     MessageRouter messageRouter,
                                     RequestCallerDirectory requestCallerDirectory,
                                     ParticipantIdStorage participantIdStorage,
                                     @Named(ConfigurableMessagingSettings.PROPERTY_LIBJOYNR_MESSAGING_ADDRESS) Address libjoynrMessagingAddress) {
        super();
        this.localCapabilitiesDirectory = localCapabilitiesDirectory;
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
    public RegistrationFuture registerProvider(final String domain, JoynrProvider provider) {
        String participantId = participantIdStorage.getProviderParticipantId(domain, provider.getProvidedInterface());
        CapabilityEntry capabilityEntry = new CapabilityEntryImpl(domain,
                                                                  provider.getInterfaceName(),
                                                                  provider.getProviderQos(),
                                                                  participantId,
                                                                  System.currentTimeMillis());
        RequestCaller requestCaller = requestCallerFactory.create(provider);

        messageRouter.addNextHop(participantId, libjoynrMessagingAddress);
        requestCallerDirectory.addCaller(participantId, requestCaller);
        RegistrationFuture ret = localCapabilitiesDirectory.add(capabilityEntry);
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
        requestCallerDirectory.removeCaller(participantId);
    }

    @Override
    public void shutdown(boolean unregisterAllRegisteredCapabilities) {
        localCapabilitiesDirectory.shutdown(unregisterAllRegisteredCapabilities);

    }

}
