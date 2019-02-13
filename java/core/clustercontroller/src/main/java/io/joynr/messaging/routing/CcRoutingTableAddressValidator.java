/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock.ReadLock;
import java.util.concurrent.locks.ReentrantReadWriteLock.WriteLock;

import com.google.inject.Inject;

import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.ReplyToAddressProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.WebSocketAddress;

public class CcRoutingTableAddressValidator extends AbstractRoutingTableAddressValidator {

    private Set<Address> ownAddresses;
    private final ReentrantReadWriteLock ownAddressesLock;
    private final WriteLock ownAddressWriteLock;
    private final ReadLock ownAddressReadLock;

    @Inject
    public CcRoutingTableAddressValidator(final GlobalAddressProvider globalAddressProvider,
                                          final ReplyToAddressProvider replyToAddressProvider) {
        ownAddresses = new HashSet<>();
        ownAddressesLock = new ReentrantReadWriteLock();
        ownAddressWriteLock = ownAddressesLock.writeLock();
        ownAddressReadLock = ownAddressesLock.readLock();
        globalAddressProvider.registerGlobalAddressesReadyListener(new TransportReadyListener() {
            @Override
            public void transportReady(Address address) {
                ownAddressWriteLock.lock();
                try {
                    ownAddresses.add(address);
                } finally {
                    ownAddressWriteLock.unlock();
                }
            }
        });
        replyToAddressProvider.registerGlobalAddressesReadyListener(new TransportReadyListener() {
            @Override
            public void transportReady(Address address) {
                ownAddressWriteLock.lock();
                try {
                    ownAddresses.add(address);
                } finally {
                    ownAddressWriteLock.unlock();
                }
            }
        });
    }

    @Override
    public boolean isValidForRoutingTable(final Address address) {
        ownAddressReadLock.lock();
        try {
            return !(address instanceof WebSocketAddress)
                    && !ownAddresses.stream().anyMatch(ownAddress -> ownAddress.equals(address));
        } finally {
            ownAddressReadLock.unlock();
        }
    }

}
