package io.joynr.jeeintegration.multicast;

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

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.provider.SubscriptionPublisher;

/**
 * This class is used to create a dynamic proxy around a {@link SubscriptionPublisher} instance which will
 * check each call to to a fire... method to ensure it's a non-selective, i.e. multicast, invocation.
 * If not, then we throw an {@link JoynrIllegalStateException}.
 */
public class SubscriptionPublisherWrapper implements InvocationHandler {

    private final Class providedInterface;
    private SubscriptionPublisher subscriptionPublisher;

    private SubscriptionPublisherWrapper(SubscriptionPublisher subscriptionPublisher, Class providedInterface) {
        this.subscriptionPublisher = subscriptionPublisher;
        this.providedInterface = providedInterface;
    }

    public static <T extends SubscriptionPublisher> T createWrapper(SubscriptionPublisher subscriptionPublisher,
                                                                    Class<T> providedInterface) {
        return (T) Proxy.newProxyInstance(providedInterface.getClassLoader(),
                                          new Class[]{ providedInterface },
                                          new SubscriptionPublisherWrapper(subscriptionPublisher, providedInterface));
    }

    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        if (method.getName().startsWith("fire") && !method.getName().endsWith("Changed")
                && method.getAnnotation(JoynrMulticast.class) == null) {
            throw new JoynrIllegalStateException("The JEE integration only allows use of multicasts. Firing selective broadcasts is not supported.");
        }
        return method.invoke(subscriptionPublisher, args);
    }
}
