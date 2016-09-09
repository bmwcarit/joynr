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

import static io.joynr.proxy.invocation.InvocationReflectionsUtils.extractOutParameterTypes;

import java.lang.reflect.Method;

import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.proxy.Future;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.BroadcastSubscriptionListener;
import joynr.OnChangeSubscriptionQos;

public class MulticastSubscribeInvocation extends SubscriptionInvocation {

    private final BroadcastSubscriptionListener listener;
    private Class<?>[] outParameterTypes;

    public MulticastSubscribeInvocation(Method method, Object[] args, Future<String> future) {
        super(future, getMulticastNameFromAnnotation(method), (SubscriptionQos) args[1]);
        if (!(args[0] instanceof BroadcastSubscriptionListener)) {
            throw new JoynrIllegalStateException("First parameter to " + method
                    + " must be non-null and a BroadcastSubscriptionListener");
        } else {
            listener = (BroadcastSubscriptionListener) args[0];
        }
        if (args.length > 2 && args[2] instanceof String) {
            setSubscriptionId((String) args[2]);
        }
        outParameterTypes = extractOutParameterTypes(listener);
    }

    private static String getMulticastNameFromAnnotation(Method method) {
        JoynrMulticast multicastAnnotation = method.getAnnotation(JoynrMulticast.class);
        if (multicastAnnotation == null) {
            throw new JoynrIllegalStateException("SubscribeTo... methods for multicasts must be annotated with JoynrMulticast annotation");
        }
        return multicastAnnotation.name();
    }

    public OnChangeSubscriptionQos getQos() {
        return (OnChangeSubscriptionQos) super.getQos();
    }

    public BroadcastSubscriptionListener getListener() {
        return listener;
    }

    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "EI_EXPOSE_REP", justification = "MulticastSubscribeInvocation is just a data container and only accessed by trusted code. So exposing internal representation is by design.")
    public Class<?>[] getOutParameterTypes() {
        return outParameterTypes;
    }
}
