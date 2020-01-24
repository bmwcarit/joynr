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
package io.joynr.proxy.invocation;

import static io.joynr.proxy.invocation.InvocationReflectionsUtils.extractOutParameterTypes;

import java.lang.reflect.Method;

import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import joynr.MulticastSubscriptionQos;

public class MulticastSubscribeInvocation extends SubscriptionInvocation {

    private final BroadcastSubscriptionListener listener;
    private Class<?>[] outParameterTypes;
    private String[] partitions;

    public MulticastSubscribeInvocation(Method method, Object[] args, Future<String> future) {
        super(future, getMulticastNameFromAnnotation(method), getQosParameter(args));
        listener = getSubscriptionListener(args);
        outParameterTypes = extractOutParameterTypes(listener);
        partitions = extractPartitions(args);
        if (argsHasSubscriptionId(args)) {
            setSubscriptionId((String) args[0]);
        }
    }

    private static String[] extractPartitions(Object[] args) {
        if (args[args.length - 1] instanceof String[]) {
            return (String[]) args[args.length - 1];
        }
        return new String[0];
    }

    private static String getMulticastNameFromAnnotation(Method method) {
        JoynrMulticast multicastAnnotation = method.getAnnotation(JoynrMulticast.class);
        if (multicastAnnotation == null) {
            throw new JoynrIllegalStateException("SubscribeTo... methods for multicasts must be annotated with JoynrMulticast annotation");
        }
        return multicastAnnotation.name();
    }

    @Override
    public MulticastSubscriptionQos getQos() {
        return (MulticastSubscriptionQos) super.getQos();
    }

    public BroadcastSubscriptionListener getListener() {
        return listener;
    }

    public Class<?>[] getOutParameterTypes() {
        return outParameterTypes == null ? null : outParameterTypes.clone();
    }

    public String[] getPartitions() {
        return partitions == null ? null : partitions.clone();
    }
}
