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

import java.lang.reflect.Method;
import java.util.List;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.pubsub.subscription.SubscriptionListener;
import io.joynr.util.ReflectionUtils;

public final class InvocationReflectionsUtils {

    private InvocationReflectionsUtils() {
    }

    static Class<?>[] extractOutParameterTypes(SubscriptionListener listener) {
        try {
            List<Method> onReceiveMethods = ReflectionUtils.findMethodsByName(listener.getClass(), "onReceive");
            if (onReceiveMethods.size() != 1) {
                throw new IllegalArgumentException("listener must implement a single onReceive method");
            }

            Method onReceiveListenerMethod = onReceiveMethods.get(0);
            Class<?>[] outParameterTypes = onReceiveListenerMethod.getParameterTypes();
            return outParameterTypes;
        } catch (NoSuchMethodException e) {
            String message = String.format("Unable to find \"onReceive\" method on subscription listener object of type \"%s\".",
                                           listener.getClass().getName());
            throw new JoynrRuntimeException(message, e);
        }
    }

}
