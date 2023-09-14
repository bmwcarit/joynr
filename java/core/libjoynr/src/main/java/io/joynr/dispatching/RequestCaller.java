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
package io.joynr.dispatching;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.provider.CallContext;
import io.joynr.provider.JoynrProvider;

public class RequestCaller implements JoynrProvider {

    private Object provider;
    private Object proxy;

    public RequestCaller(Object proxy, Object provider) {
        this.proxy = proxy;
        this.provider = provider;
    }

    public void setContext(CallContext context) {

        if (provider instanceof AbstractJoynrProvider) {
            if (context != null) {
                AbstractJoynrProvider.setCallContext(context);
            }
        }
    }

    public void removeContext() {
        if (provider instanceof AbstractJoynrProvider) {
            AbstractJoynrProvider.removeCallContext();
        }
    }

    public Object invoke(Method method, Object[] params) throws IllegalAccessException, IllegalArgumentException,
                                                         InvocationTargetException {
        return method.invoke(proxy, params);
    }

    public Object getProxy() {
        return proxy;
    }

    public Object getProvider() {
        return provider;
    }
}
