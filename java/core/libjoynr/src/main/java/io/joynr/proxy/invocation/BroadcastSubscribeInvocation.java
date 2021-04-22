/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2021 BMW Car IT GmbH
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
package io.joynr.proxy.invocation;

import static io.joynr.proxy.invocation.InvocationReflectionsUtils.extractOutParameterTypes;

import java.lang.reflect.Method;

import io.joynr.dispatcher.rpc.annotation.JoynrRpcBroadcast;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import joynr.BroadcastFilterParameters;
import joynr.OnChangeSubscriptionQos;

/**
 * BroadcastSubscribeInvocation contains the queuable information for a {@literal subscribeTo<broadcast>} call
 */
public class BroadcastSubscribeInvocation extends SubscriptionInvocation {
    private final BroadcastSubscriptionListener broadcastSubscriptionListener;
    private final BroadcastFilterParameters filterParameters;
    private final Class<?>[] outParameterTypes;

    private static BroadcastFilterParameters getFilterParameters(Object[] args) {
        try {
            if (argsHasSubscriptionId(args)) {
                return (BroadcastFilterParameters) args[3];
            }
            return (BroadcastFilterParameters) args[2];
        } catch (ClassCastException e) {
            throw new JoynrIllegalStateException("subscribeTo must be passed a SubscriptionQos");
        }
    }

    public BroadcastSubscribeInvocation(Method method, Object[] args, Future<String> future, Object proxy) {
        super(future, getSubscriptionNameFromAnnotation(method), getQosParameter(args), proxy);
        boolean isSelectiveBroadcast = method.getAnnotation(JoynrRpcBroadcast.class) != null;

        // For broadcast subscriptions the args array contains the following entries:
        // (optional) subscriptionId
        // BroadcastSubscriptionListener
        // OnChangeSubscriptionQos
        // (isSelectiveBroadcast) ? BroadcastFilterParameter

        broadcastSubscriptionListener = getSubscriptionListener(args);
        outParameterTypes = extractOutParameterTypes(broadcastSubscriptionListener);

        if (isSelectiveBroadcast) {
            filterParameters = getFilterParameters(args);
        } else {
            filterParameters = new BroadcastFilterParameters();
        }

        if (argsHasSubscriptionId(args)) {
            setSubscriptionId((String) args[0]);
        }
    }

    private static String getSubscriptionNameFromAnnotation(Method method) {
        JoynrRpcBroadcast broadcastAnnotation = method.getAnnotation(JoynrRpcBroadcast.class);
        if (broadcastAnnotation != null) {
            return broadcastAnnotation.broadcastName();
        }
        throw new JoynrIllegalStateException("SubscribeTo... methods must be annotated with JoynrRpcSubscription annotation");
    }

    public BroadcastSubscribeInvocation(String broadcastName,
                                        BroadcastSubscriptionListener broadcastSubscriptionListener,
                                        OnChangeSubscriptionQos qos,
                                        Future<String> future,
                                        Object proxy) {
        super(future, broadcastName, qos, proxy);
        this.filterParameters = null;
        this.broadcastSubscriptionListener = broadcastSubscriptionListener;
        outParameterTypes = extractOutParameterTypes(broadcastSubscriptionListener);
    }

    public BroadcastSubscriptionListener getBroadcastSubscriptionListener() {
        return broadcastSubscriptionListener;
    }

    @Override
    public OnChangeSubscriptionQos getQos() {
        return (OnChangeSubscriptionQos) super.getQos();
    }

    public BroadcastFilterParameters getFilterParameters() {
        return filterParameters;
    }

    public String getBroadcastName() {
        return getSubscriptionName();
    }

    public Class<?>[] getOutParameterTypes() {
        return outParameterTypes == null ? null : outParameterTypes.clone();
    }
}
