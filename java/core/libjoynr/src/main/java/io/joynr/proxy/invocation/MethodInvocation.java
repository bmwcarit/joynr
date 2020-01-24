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

import io.joynr.proxy.Future;

/**
 * MethodInvocation contains the queuable information for a proxy method call
 */

public class MethodInvocation<T> extends Invocation<T> {

    private final Object proxy;
    private final Method method;
    private final Object[] args;

    public MethodInvocation(Object proxy, Method method, Object[] args, Future<T> future) {
        super(future);
        this.proxy = proxy;
        this.method = method;
        this.args = args == null ? null : args.clone();
    }

    public Object getProxy() {
        return proxy;
    }

    public Method getMethod() {
        return method;
    }

    public Object[] getArgs() {
        return args == null ? null : args.clone();
    }
}
