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
package io.joynr.messaging.routing;

import java.util.Optional;

import joynr.system.RoutingTypes.Address;

public abstract class GlobalAddressFactory<T extends Address> {

    /**
     *
     * @return a globally addressable address
     */
    public abstract T create();

    /**
     * @param transport
     *          the transport, for which the support request is triggered
     * @return the boolean representing the transport supported by this Address type.
     */
    public abstract boolean supportsTransport(Optional<String> transport);

    /**
     *
     * @param listener is notified when the global address is ready. The default
     * implementation assumes that the address is ready immediately. Override for
     * Address types that must be discovered etc.
     */
    public void registerGlobalAddressReady(TransportReadyListener listener) {
        listener.transportReady(Optional.ofNullable(create()));
    }

}
