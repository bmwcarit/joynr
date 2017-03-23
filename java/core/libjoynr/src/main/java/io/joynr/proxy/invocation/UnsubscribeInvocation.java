package io.joynr.proxy.invocation;

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

import java.lang.reflect.Method;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.proxy.Future;

public class UnsubscribeInvocation extends Invocation<String> {

    private final String subscriptionId;

    public UnsubscribeInvocation(Method method, Object[] args, Future<String> future) {
        super(future);
        if (args[0] == null || !String.class.isAssignableFrom(args[0].getClass())) {
            throw new JoynrIllegalStateException("First parameter of unsubscribe... has to be a String containing the subscriptionId");
        }
        subscriptionId = (String) args[0];
    }

    public String getSubscriptionId() {
        return subscriptionId;
    }
}