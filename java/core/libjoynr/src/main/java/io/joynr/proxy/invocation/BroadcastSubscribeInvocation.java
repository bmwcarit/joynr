package io.joynr.proxy.invocation;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import java.lang.reflect.Method;

import io.joynr.dispatcher.rpc.ReflectionUtils;
import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcBroadcast;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.SubscriptionQos;
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

    public BroadcastSubscribeInvocation(Method method, Object[] args, Future<String> future) {
        super(future, getSubscriptionNameFromAnnotation(method), (SubscriptionQos) args[1]);
        boolean isSelectiveBroadcast = method.getAnnotation(JoynrRpcBroadcast.class) != null;

        // For broadcast subscriptions the args array contains the following entries:
        // args[0] : BroadcastSubscriptionListener
        // args[1] : OnChangeSubscriptionQos
        // args[2] : (isSelectiveBroadcast) ? BroadcastFilterParameter : optional subscription ID
        // args[3] : (isSelectiveBroadcast) ? optional subscription ID : not available
        broadcastSubscriptionListener = (BroadcastSubscriptionListener) args[0];
        outParameterTypes = extractOutParameterTypes(broadcastSubscriptionListener);

        if (isSelectiveBroadcast) {
            filterParameters = (BroadcastFilterParameters) args[2];
            setSubscriptionId((args.length == 4) ? (String) args[3] : "");
        } else {
            filterParameters = new BroadcastFilterParameters();
            setSubscriptionId((args.length == 3) ? (String) args[2] : "");
        }
    }

    private static String getSubscriptionNameFromAnnotation(Method method) {
        JoynrRpcBroadcast broadcastAnnotation = method.getAnnotation(JoynrRpcBroadcast.class);
        if (broadcastAnnotation != null) {
            return broadcastAnnotation.broadcastName();
        }
        JoynrMulticast multicastAnnotation = method.getAnnotation(JoynrMulticast.class);
        if (multicastAnnotation != null) {
            return multicastAnnotation.name();
        }
        throw new JoynrIllegalStateException("SubscribeTo... methods must be annotated with either JoynrRpcSubscription or JoynrMulticast  annotation");
    }

    public BroadcastSubscribeInvocation(String broadcastName,
                                        BroadcastSubscriptionListener broadcastSubscriptionListener,
                                        OnChangeSubscriptionQos qos,
                                        Future<String> future) {
        super(future, broadcastName, qos);
        this.filterParameters = null;
        this.broadcastSubscriptionListener = broadcastSubscriptionListener;
        outParameterTypes = extractOutParameterTypes(broadcastSubscriptionListener);
    }

    private static Class<?>[] extractOutParameterTypes(BroadcastSubscriptionListener listener) {
        try {
            Method onReceiveListenerMethod = ReflectionUtils.findMethod(listener.getClass(), "onReceive");
            Class<?>[] outParameterTypes = onReceiveListenerMethod.getParameterTypes();
            return outParameterTypes;
        } catch (NoSuchMethodException e) {
            // this should not happen since a broadcast listener must have an
            // onReceive method
            String message = String.format("Unable to find \"onReceive\" method on subscription listener object of type \"%s\".",
                                           listener.getClass().getName());
            throw new JoynrRuntimeException(message, e);
        }
    }

    public BroadcastSubscriptionListener getBroadcastSubscriptionListener() {
        return broadcastSubscriptionListener;
    }

    public OnChangeSubscriptionQos getQos() {
        return (OnChangeSubscriptionQos) super.getQos();
    }

    public BroadcastFilterParameters getFilterParameters() {
        return filterParameters;
    }

    public String getBroadcastName() {
        return getSubscriptionName();
    }

    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "EI_EXPOSE_REP", justification = "BroadcastSubscribeInvocation is just a data container and only accessed by trusted code. So exposing internal representation is by design.")
    public Class<?>[] getOutParameterTypes() {
        return outParameterTypes;
    }
}
