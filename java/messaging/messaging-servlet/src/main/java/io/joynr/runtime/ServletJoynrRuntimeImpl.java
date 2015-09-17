package io.joynr.runtime;

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

import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.JoynrProvider;
import io.joynr.proxy.ProxyInvocationHandlerFactory;

import javax.inject.Named;

import joynr.system.routingtypes.Address;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;

public class ServletJoynrRuntimeImpl extends JoynrRuntimeImpl {

    @Inject
    public ServletJoynrRuntimeImpl(ObjectMapper objectMapper,
                                   LocalCapabilitiesDirectory localCapabilitiesDirectory,
                                   ProxyInvocationHandlerFactory proxyInvocationHandlerFactory,
                                   MessageRouter messageRouter,
                                   @Named(ConfigurableMessagingSettings.PROPERTY_LIBJOYNR_MESSAGING_ADDRESS) Address libjoynrMessagingAddress) {
        super(objectMapper,
              localCapabilitiesDirectory,
              proxyInvocationHandlerFactory,
              messageRouter,
              libjoynrMessagingAddress);
    }

    @Override
    /**
     * Unregistering currently is not receiving any answers, cauing timrout exceptions
     * The reason is that the unregister happens in the ServletContextListener at contextDestroyed
     * which happens after the servlet has already been destroyed. Since the response to unregister 
     * would have to arrive via the messaging receiver servlet, this is obviously too late to unregister,
     * but there is no obvious fix (other than create a long polling message receiver for the unregister)
     * since the servelet lifecycle does not consist of any further usful events.
     */
    public void unregisterProvider(String domain, JoynrProvider provider) {
        try {
            super.unregisterProvider(domain, provider);
        } catch (JoynrCommunicationException e) {

        }
    }

}
