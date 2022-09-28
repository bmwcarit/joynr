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

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

import javax.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.ReplyToAddressProvider;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingAbstractProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BinderAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.system.RoutingTypes.UdsAddress;
import joynr.system.RoutingTypes.UdsClientAddress;
import joynr.system.RoutingTypes.WebSocketAddress;
import joynr.system.RoutingTypes.WebSocketClientAddress;

/**
 * Implements the RoutingProvider interface to receive routing information from connected libjoynrs
 *
 */
public class RoutingProviderImpl extends RoutingAbstractProvider {
    private MessageRouter messageRouter;
    private MulticastReceiverRegistrar multicastReceiverRegistrar;
    private String globalAddressString;
    private String replyToAddressString;
    private List<Deferred<String>> unresolvedGlobalAddressDeferreds = new ArrayList<Deferred<String>>();
    private List<Deferred<String>> unresolvedReplyToAddressDeferreds = new ArrayList<Deferred<String>>();
    private static final Logger logger = LoggerFactory.getLogger(RoutingProviderImpl.class);

    /**
     * @param messageRouter handles the logic for the RoutingProvider
     * @param multicastReceiverRegistrar registry for multicast subscriber participantIds
     * @param globalAddressProvider provider for the global address of a single cluster controller or a clustered application if shared subscriptions are enabled
     * @param replyToAddressProvider provider for the cluster controller's replyTo address
     */
    @Inject
    public RoutingProviderImpl(final MessageRouter messageRouter,
                               final MulticastReceiverRegistrar multicastReceiverRegistrar,
                               GlobalAddressProvider globalAddressProvider,
                               ReplyToAddressProvider replyToAddressProvider) {
        this.messageRouter = messageRouter;
        this.multicastReceiverRegistrar = multicastReceiverRegistrar;

        globalAddressProvider.registerGlobalAddressesReadyListener(new TransportReadyListener() {
            @Override
            public void transportReady(Optional<Address> address) {
                synchronized (unresolvedGlobalAddressDeferreds) {
                    globalAddressString = RoutingTypesUtil.toAddressString(address.isPresent() ? address.get() : null);
                    for (Deferred<String> globalAddressDeferred : unresolvedGlobalAddressDeferreds) {
                        globalAddressDeferred.resolve(globalAddressString);
                    }
                    unresolvedGlobalAddressDeferreds.clear();
                    globalAddressChanged(globalAddressString);
                }
            }
        });
        replyToAddressProvider.registerGlobalAddressesReadyListener(new TransportReadyListener() {
            @Override
            public void transportReady(Optional<Address> address) {
                synchronized (unresolvedReplyToAddressDeferreds) {
                    replyToAddressString = RoutingTypesUtil.toAddressString(address.isPresent() ? address.get() : null);
                    for (Deferred<String> replyToAddressDeferred : unresolvedReplyToAddressDeferreds) {
                        replyToAddressDeferred.resolve(replyToAddressString);
                    }
                    unresolvedReplyToAddressDeferreds.clear();
                    replyToAddressChanged(replyToAddressString);
                }
            }
        });
    }

    private Promise<DeferredVoid> resolvedDeferred() {
        final DeferredVoid deferred = new DeferredVoid();
        deferred.resolve();
        return new Promise<>(deferred);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId,
                                            joynr.system.RoutingTypes.ChannelAddress address,
                                            Boolean isGloballyVisible) {
        throw new IllegalArgumentException(joynr.system.RoutingTypes.ChannelAddress.class.getCanonicalName()
                + " no longer supported.");
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, MqttAddress address, Boolean isGloballyVisible) {
        // If it throws, the error will be forwarded to the calling proxy
        messageRouter.addNextHop(participantId, address, isGloballyVisible);
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId,
                                            joynr.system.RoutingTypes.BrowserAddress address,
                                            Boolean isGloballyVisible) {
        throw new IllegalArgumentException(joynr.system.RoutingTypes.BrowserAddress.class.getCanonicalName()
                + " no longer supported.");
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, WebSocketAddress address, Boolean isGloballyVisible) {
        // If it throws, the error will be forwarded to the calling proxy
        messageRouter.addNextHop(participantId, address, isGloballyVisible);
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, UdsAddress address, Boolean isGloballyVisible) {
        final DeferredVoid deferred = new DeferredVoid();
        final String message = "UdsAddress is not supported in Java";
        logger.error(message);
        deferred.reject(new ProviderRuntimeException(message));
        return new Promise<>(deferred);
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId,
                                            BinderAddress binderAddress,
                                            Boolean isGloballyVisible) {
        // If it throws, the error will be forwarded to the calling proxy
        messageRouter.addNextHop(participantId, binderAddress, isGloballyVisible);
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId,
                                            WebSocketClientAddress address,
                                            Boolean isGloballyVisible) {
        // If it throws, the error will be forwarded to the calling proxy
        messageRouter.addNextHop(participantId, address, isGloballyVisible);
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> addNextHop(String participantId, UdsClientAddress address, Boolean isGloballyVisible) {
        final DeferredVoid deferred = new DeferredVoid();
        final String msg = "UdsClientAddress is not supported in Java";
        logger.error(msg);
        deferred.reject(new ProviderRuntimeException(msg));
        return new Promise<>(deferred);
    }

    @Override
    public Promise<DeferredVoid> removeNextHop(String participantId) {
        messageRouter.removeNextHop(participantId);
        return resolvedDeferred();
    }

    @Override
    public Promise<ResolveNextHopDeferred> resolveNextHop(String participantId) {
        boolean resolved = messageRouter.resolveNextHop(participantId);
        ResolveNextHopDeferred deferred = new ResolveNextHopDeferred();
        deferred.resolve(resolved);
        return new Promise<>(deferred);
    }

    @Override
    public Promise<DeferredVoid> addMulticastReceiver(String multicastId,
                                                      String subscriberParticipantId,
                                                      String providerParticipantId) {
        multicastReceiverRegistrar.addMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        return resolvedDeferred();
    }

    @Override
    public Promise<DeferredVoid> removeMulticastReceiver(String multicastId,
                                                         String subscriberParticipantId,
                                                         String providerParticipantId) {
        multicastReceiverRegistrar.removeMulticastReceiver(multicastId, subscriberParticipantId, providerParticipantId);
        return resolvedDeferred();
    }

    @Override
    public Promise<Deferred<String>> getGlobalAddress() {
        Deferred<String> globalAddressDeferred = new Deferred<String>();
        synchronized (unresolvedGlobalAddressDeferreds) {
            if (globalAddressString != null) {
                globalAddressDeferred.resolve(globalAddressString);
            } else {
                unresolvedGlobalAddressDeferreds.add(globalAddressDeferred);
            }
        }
        return new Promise<Deferred<String>>(globalAddressDeferred);
    }

    @Override
    public Promise<Deferred<String>> getReplyToAddress() {
        Deferred<String> replyToAddressDeferred = new Deferred<String>();
        synchronized (unresolvedReplyToAddressDeferreds) {
            if (replyToAddressString != null) {
                replyToAddressDeferred.resolve(replyToAddressString);
            } else {
                unresolvedReplyToAddressDeferreds.add(replyToAddressDeferred);
            }
        }
        return new Promise<Deferred<String>>(replyToAddressDeferred);
    }

}
