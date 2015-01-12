package io.joynr.proxy.invocation;

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

import io.joynr.dispatcher.rpc.annotation.JoynrRpcBroadcast;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;

import java.lang.reflect.Method;

import joynr.BroadcastFilterParameters;

/**
 * BroadcastSubscribeInvocation contains the queuable information for a subscribeTo<broadcast> call
 */
public class BroadcastSubscribeInvocation extends SubscriptionInvocation {
    private String subscriptionId = "";
    private final BroadcastSubscriptionListener broadcastSubscriptionListener;
    private final String broadcastName;
    private final SubscriptionQos qos;
    private final BroadcastFilterParameters filterParameters;

    public BroadcastSubscribeInvocation(Method method, Object[] args, Future<?> future) {
        super(future);
        JoynrRpcBroadcast broadcastAnnotation = method.getAnnotation(JoynrRpcBroadcast.class);
        broadcastName = broadcastAnnotation.broadcastName();
        int argumentCounter = 0;

        broadcastSubscriptionListener = (BroadcastSubscriptionListener) args[argumentCounter++];
        qos = (SubscriptionQos) args[argumentCounter++];

        if (args.length > argumentCounter) {
            if (args[argumentCounter] instanceof BroadcastFilterParameters) {
                filterParameters = (BroadcastFilterParameters) args[argumentCounter++];
            } else {
                filterParameters = new BroadcastFilterParameters();
            }
            if (args.length > argumentCounter) {
                if (args[argumentCounter] instanceof String) {
                    subscriptionId = (String) args[argumentCounter++];
                } else {
                    throw new JoynrIllegalStateException("Third parameter of subscribeTo... has to be of type String");
                }
            }
        } else {
            filterParameters = new BroadcastFilterParameters();
        }
    }

    public BroadcastSubscribeInvocation(String broadcastName,
                                        BroadcastSubscriptionListener broadcastSubscriptionListener,
                                        SubscriptionQos qos,
                                        Future<?> future) {
        super(future);
        this.broadcastName = broadcastName;
        this.filterParameters = null;
        this.qos = qos;
        this.broadcastSubscriptionListener = broadcastSubscriptionListener;
    }

    @Override
    public String getSubscriptionId() {
        return subscriptionId;
    }

    public void setSubscriptionId(String subscriptionId) {
        this.subscriptionId = subscriptionId;
    }

    public BroadcastSubscriptionListener getBroadcastSubscriptionListener() {
        return broadcastSubscriptionListener;
    }

    public SubscriptionQos getQos() {
        return qos;
    }

    public BroadcastFilterParameters getFilterParameters() {
        return filterParameters;
    }

    public String getBroadcastName() {
        return broadcastName;
    }
}
