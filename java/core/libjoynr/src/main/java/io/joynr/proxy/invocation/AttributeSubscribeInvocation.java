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

import java.lang.reflect.Method;

import com.fasterxml.jackson.core.type.TypeReference;

import io.joynr.dispatcher.rpc.annotation.JoynrRpcSubscription;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;

/**
 * AttributeSubscribeInvocation contains the queuable information for a {@literal subscribeTo<attribute>} call
 */
public class AttributeSubscribeInvocation extends SubscriptionInvocation {
    private final AttributeSubscriptionListener<?> attributeSubscriptionListener;
    private final Class<?> attributeTypeReference;

    /**
     *
     * @param method method information
     * @param args contains the arguments passed to the subscribeTo call. subscribe to can be called with or
     * without a subscriptionId in position 0.
     * @param future result future for the subscribe invocation
     */
    public AttributeSubscribeInvocation(Method method, Object[] args, Future<String> future, Object proxy) {
        super(future, getAttributeNameFromAnnotation(method), getQosParameter(args), proxy);
        attributeTypeReference = getAnnotationFromMethod(method).attributeType();
        attributeSubscriptionListener = getSubscriptionListener(args);

        if (argsHasSubscriptionId(args)) {
            setSubscriptionId((String) args[0]);
        }
    }

    private static JoynrRpcSubscription getAnnotationFromMethod(Method method) {
        JoynrRpcSubscription subscriptionAnnotation = method.getAnnotation(JoynrRpcSubscription.class);
        if (subscriptionAnnotation == null) {
            throw new JoynrIllegalStateException("SubscribeTo... methods must be annotated with JoynrRpcSubscription annotation");
        }
        return subscriptionAnnotation;
    }

    private static String getAttributeNameFromAnnotation(Method method) {
        return getAnnotationFromMethod(method).attributeName();
    }

    public AttributeSubscribeInvocation(String attributeName,
                                        Class<? extends TypeReference<?>> attributeTypeReference,
                                        AttributeSubscriptionListener<?> attributeSubscriptionListener,
                                        SubscriptionQos qos,
                                        Future<String> future,
                                        Object proxy) {
        super(future, attributeName, qos, proxy);
        this.attributeTypeReference = attributeTypeReference;
        this.attributeSubscriptionListener = attributeSubscriptionListener;
    }

    public AttributeSubscriptionListener<?> getAttributeSubscriptionListener() {
        return attributeSubscriptionListener;
    }

    public Class<?> getAttributeTypeReference() {
        return attributeTypeReference;
    }

    public String getAttributeName() {
        return getSubscriptionName();
    }
}
