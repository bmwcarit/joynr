package io.joynr.proxy;

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

import io.joynr.dispatching.RequestReplyDispatcher;
import io.joynr.dispatching.RequestReplyManager;
import io.joynr.dispatching.rpc.RpcAsyncRequestReplyCaller;
import io.joynr.dispatching.rpc.RpcUtils;
import io.joynr.dispatching.rpc.SynchronizedReplyCaller;
import io.joynr.dispatching.subscription.SubscriptionManager;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;

import java.io.IOException;
import java.lang.reflect.Method;

import javax.annotation.CheckForNull;

import joynr.MethodMetaInformation;
import joynr.Reply;
import joynr.Request;
import joynr.exceptions.ApplicationException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;

//TODO maybe Connector should not be a dynamic proxy. ProxyInvocationHandler could call execute...Method() directly.
final class JoynrMessagingConnectorInvocationHandler implements ConnectorInvocationHandler {
    @SuppressWarnings("unused")
    private static final Logger logger = LoggerFactory.getLogger(JoynrMessagingConnectorInvocationHandler.class);

    private final String toParticipantId;
    private final String fromParticipantId;

    private final MessagingQos qosSettings;

    private final RequestReplyManager requestReplyManager;
    private final RequestReplyDispatcher dispatcher;

    private final SubscriptionManager subscriptionManager;

    JoynrMessagingConnectorInvocationHandler(String toParticipantId,
                                             String fromParticipantId,
                                             MessagingQos qosSettings,
                                             RequestReplyManager requestReplyManager,
                                             RequestReplyDispatcher dispatcher,
                                             SubscriptionManager subscriptionManager) {
        this.toParticipantId = toParticipantId;
        this.fromParticipantId = fromParticipantId;

        this.qosSettings = qosSettings;

        this.requestReplyManager = requestReplyManager;
        this.dispatcher = dispatcher;
        this.subscriptionManager = subscriptionManager;

    }

    @SuppressWarnings("unchecked")
    @Override
    public Future<?> executeAsyncMethod(Method method, Object[] params, Future<?> future)
                                                                                         throws JoynrSendBufferFullException,
                                                                                         JoynrMessageNotSentException,
                                                                                         JsonGenerationException,
                                                                                         JsonMappingException,
                                                                                         IOException {

        if (method == null) {
            throw new IllegalArgumentException("method cannot be null");
        }

        MethodMetaInformation methodMetaInformation = JoynrMessagingConnectorFactory.ensureMethodMetaInformationPresent(method);
        if (methodMetaInformation.getCallbackAnnotation() == null) {
            throw new JoynrIllegalStateException("All async methods need to have a annotated callback parameter.");
        }

        int callbackIndex = methodMetaInformation.getCallbackIndex();
        ICallback callback = (ICallback) params[callbackIndex];

        Object[] paramsWithoutCallback = new Object[params.length - 1];
        copyArrayWithoutElement(params, paramsWithoutCallback, callbackIndex);
        Class<?>[] paramDatatypes = method.getParameterTypes();
        Class<?>[] paramDatatypesWithoutCallback = new Class<?>[paramDatatypes.length - 1];
        copyArrayWithoutElement(paramDatatypes, paramDatatypesWithoutCallback, callbackIndex);

        Request request = new Request(method.getName(), paramsWithoutCallback, paramDatatypesWithoutCallback);
        String requestReplyId = request.getRequestReplyId();

        RpcAsyncRequestReplyCaller<?> callbackWrappingReplyCaller = new RpcAsyncRequestReplyCaller(requestReplyId,
                                                                                                   callback,
                                                                                                   future,
                                                                                                   method,
                                                                                                   methodMetaInformation);

        dispatcher.addReplyCaller(requestReplyId, callbackWrappingReplyCaller, qosSettings.getRoundTripTtl_ms());
        requestReplyManager.sendRequest(fromParticipantId, toParticipantId, request, qosSettings.getRoundTripTtl_ms());
        return future;
    }

    private void copyArrayWithoutElement(Object[] fromArray, Object[] toArray, int removeIndex) {
        System.arraycopy(fromArray, 0, toArray, 0, removeIndex);
        System.arraycopy(fromArray, removeIndex + 1, toArray, removeIndex, toArray.length - removeIndex);
    }

    @CheckForNull
    @Override
    public Object executeSyncMethod(Method method, Object[] args) throws ApplicationException, JoynrRuntimeException,
                                                                 JsonGenerationException, JsonMappingException,
                                                                 IOException, InstantiationException,
                                                                 IllegalAccessException {

        // TODO does a method with 0 args pass in an empty args array, or null for args?
        if (method == null) {
            throw new IllegalArgumentException("method cannot be null");
        }

        MethodMetaInformation methodMetaInformation = JoynrMessagingConnectorFactory.ensureMethodMetaInformationPresent(method);

        Request request = new Request(method.getName(), args, method.getParameterTypes());
        Reply reply;
        String requestReplyId = request.getRequestReplyId();
        SynchronizedReplyCaller synchronizedReplyCaller = new SynchronizedReplyCaller(fromParticipantId,
                                                                                      toParticipantId,
                                                                                      requestReplyId,
                                                                                      request);
        dispatcher.addReplyCaller(requestReplyId, synchronizedReplyCaller, qosSettings.getRoundTripTtl_ms());
        reply = (Reply) requestReplyManager.sendSyncRequest(fromParticipantId,
                                                            toParticipantId,
                                                            request,
                                                            synchronizedReplyCaller,
                                                            qosSettings.getRoundTripTtl_ms());
        if (reply.getError() == null) {
            if (method.getReturnType().equals(void.class)) {
                return null;
            }
            return RpcUtils.reconstructReturnedObject(method, methodMetaInformation, reply.getResponse().toArray());
        } else if (reply.getError() instanceof ApplicationException) {
            throw (ApplicationException) reply.getError();
        } else {
            throw (JoynrRuntimeException) reply.getError();
        }

    }

    @Override
    public void executeSubscriptionMethod(UnsubscribeInvocation unsubscribeInvocation)
                                                                                      throws JoynrSendBufferFullException,
                                                                                      JoynrMessageNotSentException,
                                                                                      JsonGenerationException,
                                                                                      JsonMappingException, IOException {

        subscriptionManager.unregisterSubscription(fromParticipantId,
                                                   toParticipantId,
                                                   unsubscribeInvocation.getSubscriptionId(),
                                                   qosSettings);
    }

    @Override
    public void executeSubscriptionMethod(AttributeSubscribeInvocation attributeSubscription)
                                                                                             throws JoynrSendBufferFullException,
                                                                                             JoynrMessageNotSentException,
                                                                                             JsonGenerationException,
                                                                                             JsonMappingException,
                                                                                             IOException {

        subscriptionManager.registerAttributeSubscription(fromParticipantId, toParticipantId, attributeSubscription);
    }

    @Override
    public void executeSubscriptionMethod(BroadcastSubscribeInvocation broadcastSubscription)
                                                                                             throws JoynrSendBufferFullException,
                                                                                             JoynrMessageNotSentException,
                                                                                             JsonGenerationException,
                                                                                             JsonMappingException,
                                                                                             IOException {

        subscriptionManager.registerBroadcastSubscription(fromParticipantId, toParticipantId, broadcastSubscription);
    }

}
