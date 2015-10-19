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

import io.joynr.dispatcher.rpc.annotation.JoynrRpcSubscription;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;

import java.lang.reflect.Method;

import com.fasterxml.jackson.core.type.TypeReference;

/**
 * AttributeSubscribeInvocation contains the queuable information for a {@literal subscribeTo<attribute>} call
 */
public class AttributeSubscribeInvocation extends SubscriptionInvocation {
    private final AttributeSubscriptionListener<?> attributeSubscriptionListener;
    private final Class<?> attributeTypeReference;
    private String subscriptionId = "";
    private final SubscriptionQos qos;
    private final String attributeName;

    public AttributeSubscribeInvocation(Method method, Object[] args, Future<?> future) {
        super(future);
        JoynrRpcSubscription subscriptionAnnotation = method.getAnnotation(JoynrRpcSubscription.class);
        if (subscriptionAnnotation == null) {
            throw new JoynrIllegalStateException("SubscribeTo... methods must be annotated with JoynrRpcSubscription annotation");
        }
        attributeName = subscriptionAnnotation.attributeName();
        if (args[0] == null || !AttributeSubscriptionListener.class.isAssignableFrom(args[0].getClass())) {
            throw new JoynrIllegalStateException("First parameter of subscribeTo... has to implement AttributeSubscriptionListener");
        }
        attributeTypeReference = subscriptionAnnotation.attributeType();

        attributeSubscriptionListener = (AttributeSubscriptionListener<?>) args[0];
        if (args[1] == null || !SubscriptionQos.class.isAssignableFrom(args[1].getClass())) {
            throw new JoynrIllegalStateException("Second parameter of subscribeTo... has to be of type SubscriptionQos");
        }

        qos = (SubscriptionQos) args[1];

        if (args.length > 2) {
            if (args[2] != null && args[2] instanceof String) {
                subscriptionId = (String) args[2];
            } else {
                throw new JoynrIllegalStateException("Third parameter of subscribeTo... has to be of type String");
            }
        }
    }

    public AttributeSubscribeInvocation(String attributeName,
                                        Class<? extends TypeReference<?>> attributeTypeReference,
                                        AttributeSubscriptionListener<?> attributeSubscriptionListener,
                                        SubscriptionQos qos,
                                        Future<?> future) {
        super(future);
        this.attributeTypeReference = attributeTypeReference;
        this.attributeSubscriptionListener = attributeSubscriptionListener;
        this.attributeName = attributeName;
        this.qos = qos;
    }

    @Override
    public String getSubscriptionId() {
        return subscriptionId;
    }

    public AttributeSubscriptionListener<?> getAttributeSubscriptionListener() {
        return attributeSubscriptionListener;
    }

    public Class<?> getAttributeTypeReference() {
        return attributeTypeReference;
    }

    public SubscriptionQos getQos() {
        return qos;
    }

    public void setSubscriptionId(String subscriptionId) {
        this.subscriptionId = subscriptionId;
    }

    public String getAttributeName() {
        return attributeName;
    }
}
