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
package io.joynr.runtime;

import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.NoBackendGlobalAddressFactory;
import io.joynr.messaging.routing.GlobalAddressFactory;
import io.joynr.messaging.routing.TransportReadyListener;
import joynr.system.RoutingTypes.Address;

public class GlobalAddressProvider implements Provider<Address> {
    public static final String GLOBAL_ADDRESS_FACTORIES = "global_address_factories";

    private Set<GlobalAddressFactory<? extends Address>> addressFactories;

    @Inject(optional = true)
    @Named(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT)
    /**
     * primaryGlobalTransport is optional, but must be set if more than one GlobalAddressFactory is registered.
     * The primary address is how remote cluster controllers can reach this cluster controller's providers
     */
    String primaryGlobalTransport;

    @Inject
    public GlobalAddressProvider(@Named(GLOBAL_ADDRESS_FACTORIES) Set<GlobalAddressFactory<? extends Address>> addressFactories) {
        this.addressFactories = addressFactories;
    }

    public void registerGlobalAddressesReadyListener(TransportReadyListener listener) {
        GlobalAddressFactory<? extends Address> addressFactory = getPrimaryGlobalAddressFactory();
        addressFactory.registerGlobalAddressReady(listener);
    }

    public GlobalAddressFactory<? extends Address> getAddressFactoryForTransport(String transport) {
        for (GlobalAddressFactory<? extends Address> addressFactory : addressFactories) {
            if (addressFactory.supportsTransport(Optional.ofNullable(transport))) {
                return addressFactory;
            }
        }
        return null;
    }

    public GlobalAddressFactory<? extends Address> getAddressFactoryByClass(Class<GlobalAddressFactory<? extends Address>> targetAddressFactoryClass) {
        if (targetAddressFactoryClass == null) {
            return null;
        }

        for (GlobalAddressFactory<? extends Address> addressFactory : addressFactories) {
            if (addressFactory.getClass().isAssignableFrom(targetAddressFactoryClass)) {
                return addressFactory;
            }
        }

        return null;
    }

    @Override
    public Address get() {
        try {
            GlobalAddressFactory<? extends Address> addressFactory = getPrimaryGlobalAddressFactory();
            return addressFactory.create();
        } catch (IllegalStateException e) {
            return null;
        }
    }

    private GlobalAddressFactory<? extends Address> getPrimaryGlobalAddressFactory() {
        GlobalAddressFactory<? extends Address> addressFactory = getAddressFactoryForTransport(primaryGlobalTransport);
        if (addressFactory == null) {
            // no need to set the primary global transport if only one possible transport is registered
            if (addressFactories.size() == 1) {
                addressFactory = addressFactories.iterator().next();
            } else if (addressFactories.size() == 0) {
                throw new IllegalStateException("no global transport was registered");
            } else {
                addressFactories = addressFactories.stream()
                                                   .filter(factory -> !NoBackendGlobalAddressFactory.class.isInstance(factory))
                                                   .collect(Collectors.toSet());
            }
            if (addressFactories.size() > 1) {
                throw new IllegalStateException("multiple global transports were registered but "
                        + MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT + " was not set.");
            } else {
                addressFactory = addressFactories.iterator().next();
            }
        }
        return addressFactory;
    }
}
