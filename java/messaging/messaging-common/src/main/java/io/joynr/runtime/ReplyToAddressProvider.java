package io.joynr.runtime;

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

import java.util.Set;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.TransportReadyListener;
import io.joynr.messaging.routing.GlobalAddressFactory;
import joynr.system.RoutingTypes.Address;

public class ReplyToAddressProvider implements Provider<Address> {
    public static final String REPLY_TO_ADDRESS_PROVIDER = "reply_to_address_provider";

    private Set<GlobalAddressFactory<? extends Address>> replyToAddressFactories;

    @Inject(optional = true)
    @Named(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT)
    /**
     * primaryGlobalTransport is optional, but must be set if more than one GlobalAddressFactory is registered.
     * The primary address is how remote cluster controllers can reach this cluster controller's providers
     */
    String primaryGlobalTransport;

    @Inject
    public ReplyToAddressProvider(@Named(REPLY_TO_ADDRESS_PROVIDER) Set<GlobalAddressFactory<? extends Address>> addressFactories) {
        this.replyToAddressFactories = addressFactories;
    }

    public void registerGlobalAddressesReadyListener(TransportReadyListener listener) {
        GlobalAddressFactory<? extends Address> addressFactory = getPrimaryReplyToAddressFactory();
        addressFactory.registerGlobalAddressReady(listener);
    }

    public GlobalAddressFactory<? extends Address> getAddressFactoryForTransport(String transport) {
        for (GlobalAddressFactory<? extends Address> addressFactory : replyToAddressFactories) {
            if (addressFactory.supportsTransport(transport)) {
                return addressFactory;
            }
        }
        return null;
    }

    public GlobalAddressFactory<? extends Address> getAddressFactoryByClass(Class<GlobalAddressFactory<? extends Address>> targetAddressFactoryClass) {
        if (targetAddressFactoryClass == null) {
            return null;
        }

        for (GlobalAddressFactory<? extends Address> addressFactory : replyToAddressFactories) {
            if (addressFactory.getClass().isAssignableFrom(targetAddressFactoryClass)) {
                return addressFactory;
            }
        }

        return null;
    }

    @Override
    public Address get() {
        try {
            GlobalAddressFactory<? extends Address> addressFactory = getPrimaryReplyToAddressFactory();
            return addressFactory.create();
        } catch (IllegalStateException e) {
            return null;
        }
    }

    private GlobalAddressFactory<? extends Address> getPrimaryReplyToAddressFactory() {
        GlobalAddressFactory<? extends Address> addressFactory = getAddressFactoryForTransport(primaryGlobalTransport);
        if (addressFactory == null) {
            // no need to set the primary global transport if only one possible transport is registered
            if (replyToAddressFactories.size() == 1) {
                addressFactory = replyToAddressFactories.iterator().next();
            } else if (replyToAddressFactories.size() == 0) {
                throw new IllegalStateException("no global transport was registered");
            } else if (replyToAddressFactories.size() > 1) {
                throw new IllegalStateException("multiple global transports were registered but "
                        + MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT + " was not set.");
            }
        }
        return addressFactory;
    }
}
