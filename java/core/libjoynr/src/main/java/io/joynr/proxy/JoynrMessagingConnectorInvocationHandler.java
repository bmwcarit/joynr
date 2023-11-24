/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2023 BMW Car IT GmbH
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
package io.joynr.proxy;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Optional;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.common.ExpiryDate;
import io.joynr.dispatcher.rpc.MultiReturnValuesContainer;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.ReplyCallerDirectory;
import io.joynr.dispatching.rpc.RpcAsyncRequestReplyCaller;
import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;
import joynr.MethodMetaInformation;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.exceptions.ApplicationException;
import joynr.types.DiscoveryEntryWithMetaInfo;

import static io.joynr.proxy.JoynrMessagingConnectorFactory.ensureMethodMetaInformationPresent;

//TODO maybe Connector should not be a dynamic proxy. ProxyInvocationHandler could call execute...Method() directly.
final class JoynrMessagingConnectorInvocationHandler implements ConnectorInvocationHandler {
    private static final Logger logger = LoggerFactory.getLogger(JoynrMessagingConnectorInvocationHandler.class);

    private final Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries;
    private final String fromParticipantId;

    private final MessagingQos qosSettings;

    private final RequestReplyManager requestReplyManager;
    private final ReplyCallerDirectory replyCallerDirectory;

    private final SubscriptionManager subscriptionManager;
    private final StatelessAsyncIdCalculator statelessAsyncIdCalculator;
    private final String statelessAsyncParticipantId;

    // CHECKSTYLE:OFF
    JoynrMessagingConnectorInvocationHandler(final Set<DiscoveryEntryWithMetaInfo> toDiscoveryEntries,
                                             final String fromParticipantId,
                                             final MessagingQos qosSettings,
                                             final RequestReplyManager requestReplyManager,
                                             final ReplyCallerDirectory replyCallerDirectory,
                                             final SubscriptionManager subscriptionManager,
                                             final StatelessAsyncIdCalculator statelessAsyncIdCalculator,
                                             final String statelessAsyncParticipantId) {
        // CHECKSTYLE:ON
        this.toDiscoveryEntries = toDiscoveryEntries;
        this.fromParticipantId = fromParticipantId;
        this.statelessAsyncParticipantId = statelessAsyncParticipantId;

        this.qosSettings = qosSettings;

        this.requestReplyManager = requestReplyManager;
        this.replyCallerDirectory = replyCallerDirectory;
        this.subscriptionManager = subscriptionManager;

        this.statelessAsyncIdCalculator = statelessAsyncIdCalculator;
    }

    private static class StrippedArguments {
        public MessagingQos messagingQos;
        public Object[] params;
        public Class<?>[] paramDatatypes;
    }

    private StrippedArguments getStrippedArguments(final Object[] params, final Class<?>[] paramDatatypes) {
        // If a MessagingQos is present as last argument then strip it off and extract the value
        final StrippedArguments strippedArguments = new StrippedArguments();
        if (params != null && params.length > 0 && paramDatatypes[params.length - 1].equals(MessagingQos.class)) {
            strippedArguments.messagingQos = (MessagingQos) params[params.length - 1];
            strippedArguments.params = Arrays.copyOf(params, params.length - 1);
            strippedArguments.paramDatatypes = Arrays.copyOf(paramDatatypes, paramDatatypes.length - 1);
        } else {
            strippedArguments.messagingQos = qosSettings;
            strippedArguments.params = params;
            strippedArguments.paramDatatypes = paramDatatypes;
        }
        return strippedArguments;
    }

    @SuppressWarnings("unchecked")
    @Override
    public Future<?> executeAsyncMethod(final Object proxy,
                                        final Method method,
                                        final Object[] params,
                                        final Future<?> future) {
        ensureDiscoveryEntriesSizeIsOne("async");

        final MethodMetaInformation methodMetaInformation = ensureMethodMetaInformationPresent(method);
        if (methodMetaInformation.getCallbackAnnotation() == null) {
            throw new JoynrIllegalStateException("All async methods need to have a annotated callback parameter.");
        }

        final int callbackIndex = methodMetaInformation.getCallbackIndex();
        final ICallback callback = (ICallback) params[callbackIndex];

        final Object[] paramsWithoutCallback = new Object[params.length - 1];
        copyArrayWithoutElement(params, paramsWithoutCallback, callbackIndex);
        final Class<?>[] paramDatatypes = method.getParameterTypes();
        final Class<?>[] paramDatatypesWithoutCallback = new Class<?>[paramDatatypes.length - 1];
        copyArrayWithoutElement(paramDatatypes, paramDatatypesWithoutCallback, callbackIndex);

        final StrippedArguments strippedArguments = getStrippedArguments(paramsWithoutCallback,
                                                                         paramDatatypesWithoutCallback);
        final Request request = new Request(method.getName(),
                                            strippedArguments.params,
                                            strippedArguments.paramDatatypes);
        final String requestReplyId = request.getRequestReplyId();

        @SuppressWarnings("rawtypes")
        final RpcAsyncRequestReplyCaller<?> callbackWrappingReplyCaller = new RpcAsyncRequestReplyCaller(proxy,
                                                                                                         requestReplyId,
                                                                                                         Optional.ofNullable(callback),
                                                                                                         future,
                                                                                                         method,
                                                                                                         methodMetaInformation);

        final ExpiryDate expiryDate = ExpiryDate.fromRelativeTtl(strippedArguments.messagingQos.getRoundTripTtl_ms());

        replyCallerDirectory.addReplyCaller(requestReplyId, callbackWrappingReplyCaller, expiryDate);
        requestReplyManager.sendRequest(fromParticipantId,
                                        toDiscoveryEntries.iterator().next(),
                                        request,
                                        strippedArguments.messagingQos);
        return future;
    }

    private void copyArrayWithoutElement(final Object[] fromArray, final Object[] toArray, final int removeIndex) {
        System.arraycopy(fromArray, 0, toArray, 0, removeIndex);
        System.arraycopy(fromArray, removeIndex + 1, toArray, removeIndex, toArray.length - removeIndex);
    }

    @Override
    public void executeStatelessAsyncMethod(final Method method, final Object[] args) {
        ensureDiscoveryEntriesSizeIsOne("stateless async");

        MessageIdCallback messageIdCallback = null;
        int messageIdCallbackIndex = -1;
        for (int index = 0; index < args.length; index++) {
            if (args[index] instanceof MessageIdCallback) {
                messageIdCallback = (MessageIdCallback) args[index];
                messageIdCallbackIndex = index;
            }
        }
        if (messageIdCallback == null) {
            throw new JoynrIllegalStateException("Stateless async method calls must have a MessageIdCallback as one of their arguments.");
        }
        final Object[] paramsWithoutMessageIdCallback = new Object[args.length - 1];
        copyArrayWithoutElement(args, paramsWithoutMessageIdCallback, messageIdCallbackIndex);
        final Class<?>[] paramDatatypes = method.getParameterTypes();
        final Class<?>[] paramDatatypesWithoutMessageIdCallback = new Class<?>[paramDatatypes.length - 1];
        copyArrayWithoutElement(paramDatatypes, paramDatatypesWithoutMessageIdCallback, messageIdCallbackIndex);

        final Request request = new Request(method.getName(),
                                            paramsWithoutMessageIdCallback,
                                            paramDatatypesWithoutMessageIdCallback,
                                            statelessAsyncIdCalculator.calculateStatelessCallbackRequestReplyId(method),
                                            statelessAsyncIdCalculator.calculateStatelessCallbackMethodId(method));
        requestReplyManager.sendRequest(statelessAsyncParticipantId,
                                        toDiscoveryEntries.iterator().next(),
                                        request,
                                        qosSettings,
                                        true);
        messageIdCallback.accept(request.getRequestReplyId());
    }

    @Override
    public Object executeSyncMethod(final Method method, final Object[] args) throws ApplicationException {
        ensureDiscoveryEntriesSizeIsOne("sync");

        final StrippedArguments strippedArguments = getStrippedArguments(args, method.getParameterTypes());

        final Request request = new Request(method.getName(),
                                            strippedArguments.params,
                                            strippedArguments.paramDatatypes);
        final String requestReplyId = request.getRequestReplyId();

        final SynchronizedReplyCaller synchronizedReplyCaller = new SynchronizedReplyCaller(fromParticipantId,
                                                                                            requestReplyId,
                                                                                            request);
        final ExpiryDate expiryDate = ExpiryDate.fromRelativeTtl(strippedArguments.messagingQos.getRoundTripTtl_ms());
        replyCallerDirectory.addReplyCaller(requestReplyId, synchronizedReplyCaller, expiryDate);
        final Reply reply = requestReplyManager.sendSyncRequest(fromParticipantId,
                                                                toDiscoveryEntries.iterator().next(),
                                                                request,
                                                                synchronizedReplyCaller,
                                                                strippedArguments.messagingQos);
        if (reply.getError() == null) {
            if (method.getReturnType().equals(void.class)) {
                logger.debug("REQUEST returns successful: requestReplyId: {}, method {}, response: [void]",
                             requestReplyId,
                             method.getName());
                return null;
            }
            final MethodMetaInformation methodMetaInformation = JoynrMessagingConnectorFactory.ensureMethodMetaInformationPresent(method);
            final Object response = RpcUtils.reconstructReturnedObject(method,
                                                                       methodMetaInformation,
                                                                       reply.getResponse());
            if (logger.isTraceEnabled()) {
                final String responseString;
                if (response instanceof MultiReturnValuesContainer) {
                    responseString = Arrays.deepToString(((MultiReturnValuesContainer) response).getValues());
                } else {
                    responseString = response.toString();
                }
                logger.trace("REQUEST returns successful: requestReplyId: {}, method {}, response: {}",
                             requestReplyId,
                             method.getName(),
                             responseString);
            } else {
                logger.debug("REQUEST returns successful: requestReplyId: {}, method {}, response: [not available with current loglevel]",
                             requestReplyId,
                             method.getName());
            }
            return response;
        } else if (reply.getError() instanceof ApplicationException) {
            if (logger.isTraceEnabled()) {
                logger.trace("REQUEST returns error: requestReplyId: {}, method {}, response: {}",
                             requestReplyId,
                             method.getName(),
                             reply.getError());
            } else {
                logger.debug("REQUEST returns error: requestReplyId: {}, method {}, response: {}",
                             requestReplyId,
                             method.getName(),
                             reply.getError().toString());
            }
            final MethodMetaInformation methodMetaInformation = ensureMethodMetaInformationPresent(method);
            if (!methodMetaInformation.hasModelledErrors()) {
                String message = "An ApplicationException was received, but none was expected."
                        + " Is the provider version incompatible with the consumer? " + reply.getError();
                logger.debug(message);
                throw new JoynrRuntimeException(message);
            } else {
                throw (ApplicationException) reply.getError();
            }
        } else {
            if (logger.isTraceEnabled()) {
                logger.trace("REQUEST returns error: requestReplyId: {}, method {}, response: {}",
                             requestReplyId,
                             method.getName(),
                             reply.getError());
            } else {
                logger.debug("REQUEST returns error: requestReplyId: {}, method {}, response: {}",
                             requestReplyId,
                             method.getName(),
                             reply.getError().toString());
            }
            throw (JoynrRuntimeException) reply.getError();
        }

    }

    @Override
    public void executeOneWayMethod(final Method method, final Object[] args) {
        ensureDiscoveryEntriesAreNotEmpty("an oneWayMethod");

        final StrippedArguments strippedArguments = getStrippedArguments(args, method.getParameterTypes());

        logger.debug("ONEWAYREQUEST call proxy: method: {}, params: {}, proxy participantId: {}, provider discovery entries: {}",
                     method.getName(),
                     strippedArguments.params,
                     fromParticipantId,
                     toDiscoveryEntries);

        final OneWayRequest request = new OneWayRequest(method.getName(),
                                                        strippedArguments.params,
                                                        strippedArguments.paramDatatypes);
        requestReplyManager.sendOneWayRequest(fromParticipantId,
                                              toDiscoveryEntries,
                                              request,
                                              strippedArguments.messagingQos);
    }

    @Override
    public void executeSubscriptionMethod(final UnsubscribeInvocation unsubscribeInvocation) {
        ensureDiscoveryEntriesAreNotEmpty("a subscription method");

        subscriptionManager.unregisterSubscription(fromParticipantId,
                                                   toDiscoveryEntries,
                                                   unsubscribeInvocation.getSubscriptionId(),
                                                   qosSettings);
    }

    @Override
    public void executeSubscriptionMethod(final AttributeSubscribeInvocation attributeSubscription) {
        ensureDiscoveryEntriesAreNotEmpty("a subscription method");

        logger.debug("SUBSCRIPTION call proxy: subscriptionId: {}, attribute: {}, qos: {}, proxy participantId: {}, provider discovery entries: {}",
                     attributeSubscription.getSubscriptionId(),
                     attributeSubscription.getSubscriptionName(),
                     attributeSubscription.getQos(),
                     fromParticipantId,
                     toDiscoveryEntries);
        subscriptionManager.registerAttributeSubscription(fromParticipantId, toDiscoveryEntries, attributeSubscription);
    }

    @Override
    public void executeSubscriptionMethod(final BroadcastSubscribeInvocation broadcastSubscription) {
        ensureDiscoveryEntriesAreNotEmpty("a subscription method");

        logger.debug("SUBSCRIPTION call proxy: subscriptionId: {}, broadcast: {}, qos: {}, proxy participantId: {}, provider discovery entries: {}",
                     broadcastSubscription.getSubscriptionId(),
                     broadcastSubscription.getBroadcastName(),
                     broadcastSubscription.getQos(),
                     fromParticipantId,
                     toDiscoveryEntries);
        subscriptionManager.registerBroadcastSubscription(fromParticipantId, toDiscoveryEntries, broadcastSubscription);
    }

    @Override
    public void executeSubscriptionMethod(final MulticastSubscribeInvocation multicastSubscription) {
        subscriptionManager.registerMulticastSubscription(fromParticipantId, toDiscoveryEntries, multicastSubscription);
    }

    private void ensureDiscoveryEntriesSizeIsOne(final String methodType) {
        if (toDiscoveryEntries.size() != 1) {
            throw new JoynrIllegalStateException("You can't execute " + methodType
                    + " methods for multiple participants.");
        }
    }

    private void ensureDiscoveryEntriesAreNotEmpty(final String methodType) {
        if (toDiscoveryEntries.isEmpty()) {
            throw new JoynrIllegalStateException("You must have at least one participant to be able to execute "
                    + methodType + ".");
        }
    }
}
