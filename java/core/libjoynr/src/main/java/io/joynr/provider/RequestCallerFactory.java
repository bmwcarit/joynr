package io.joynr.provider;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.dispatcher.RequestCaller;
import io.joynr.dispatcher.RequestCallerAsync;
import io.joynr.dispatcher.RequestCallerSync;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

public class RequestCallerFactory {
    public RequestCaller create(final JoynrProvider provider, Class<?> providedInterface) {
        Class<? extends RequestCaller> callerInterface = getRequestCallerType(provider);
        return (RequestCaller) Proxy.newProxyInstance(provider.getClass().getClassLoader(), new Class<?>[]{
                providedInterface, callerInterface }, new InvocationHandler() {

            @Override
            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                try {
                    return method.invoke(provider, args);
                } catch (InvocationTargetException ex) {
                    throw ex.getCause();
                }
            }

        });
    }

    private Class<? extends RequestCaller> getRequestCallerType(final JoynrProvider provider) {
        Class<? extends RequestCaller> callerInterface = provider instanceof JoynrProviderAsync ? RequestCallerAsync.class
                : RequestCallerSync.class;
        return callerInterface;
    }
}
