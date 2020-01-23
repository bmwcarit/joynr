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
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock.ReadLock;
import java.util.concurrent.locks.ReentrantReadWriteLock.WriteLock;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;

import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.ReplyToAddressProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

public class CcRoutingTableAddressValidator implements RoutingTableAddressValidator {

    private static final Logger logger = LoggerFactory.getLogger(CcRoutingTableAddressValidator.class);

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
            public void transportReady(Optional<Address> address) {
                ownAddressWriteLock.lock();
                try {
                    ownAddresses.add(address.isPresent() ? address.get() : null);
                } finally {
                    ownAddressWriteLock.unlock();
                }
            }
        });
        replyToAddressProvider.registerGlobalAddressesReadyListener(new TransportReadyListener() {
            @Override
            public void transportReady(Optional<Address> address) {
                ownAddressWriteLock.lock();
                try {
                    ownAddresses.add(address.isPresent() ? address.get() : null);
                } finally {
                    ownAddressWriteLock.unlock();
                }
            }
        });
    }

    @Override
    public boolean isValidForRoutingTable(final Address address) {
        if (address instanceof WebSocketAddress) {
            logger.error("WebSocketAddress will not be used for CC Routing Table: {}", address);
            return false;
        }
        ownAddressReadLock.lock();
        try {
            if (ownAddresses.stream().anyMatch(ownAddress -> ownAddress.equals(address))) {
                logger.trace("Address will not be used for Routing Table since it refers to ourselves: {}", address);
                return false;
            }
        } catch (Exception e) {
            logger.error("Exception in isValidForRoutingTable", e);
            return false;
        } finally {
            ownAddressReadLock.unlock();
        }
        return true;
    }

    @Override
    public boolean allowUpdate(final RoutingEntry oldEntry, final RoutingEntry newEntry) {
        // precedence: InProcessAddress > WebSocketClientAddress > MqttAddress/ChannelAddress > WebSocketAddress
        if (newEntry.address instanceof InProcessAddress) {
            return true;
        } else if (!(oldEntry.getAddress() instanceof InProcessAddress)) {
            if (newEntry.address instanceof WebSocketClientAddress) {
                return true;
            } else if (!(oldEntry.getAddress() instanceof WebSocketClientAddress)) {
                // old address is MqttAddress/ChannelAddress or WebSocketAddress
                if (newEntry.address instanceof MqttAddress || newEntry.address instanceof ChannelAddress) {
                    return true;
                } else if (oldEntry.getAddress() instanceof WebSocketAddress) {
                    // old address is WebSocketAddress
                    if (newEntry.getAddress() instanceof WebSocketAddress) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

}
