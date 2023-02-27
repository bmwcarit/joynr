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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
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
    private static final Logger logger = LoggerFactory.getLogger(RoutingProviderImpl.class);

    @Inject(optional = true)
    @Named(MessagingPropertyKeys.GLOBAL_ADDRESS)
    private Address globalAddress = new Address();
    @Inject(optional = true)
    @Named(MessagingPropertyKeys.REPLY_TO_ADDRESS)
    private Address replyToAddress = new Address();

    /**
     * @param messageRouter handles the logic for the RoutingProvider
     * @param multicastReceiverRegistrar registry for multicast subscriber participantIds
     */
    @Inject
    public RoutingProviderImpl(final MessageRouter messageRouter,
                               final MulticastReceiverRegistrar multicastReceiverRegistrar) {
        this.messageRouter = messageRouter;
        this.multicastReceiverRegistrar = multicastReceiverRegistrar;
    }

    @Inject
    public void init() {
        globalAddressString = RoutingTypesUtil.toAddressString(globalAddress);
        replyToAddressString = RoutingTypesUtil.toAddressString(replyToAddress);
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
        globalAddressDeferred.resolve(globalAddressString);
        return new Promise<Deferred<String>>(globalAddressDeferred);
    }

    @Override
    public Promise<Deferred<String>> getReplyToAddress() {
        Deferred<String> replyToAddressDeferred = new Deferred<String>();
        replyToAddressDeferred.resolve(replyToAddressString);
        return new Promise<Deferred<String>>(replyToAddressDeferred);
    }

}
