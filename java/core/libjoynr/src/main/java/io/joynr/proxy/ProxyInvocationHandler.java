package io.joynr.proxy;

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

import io.joynr.arbitration.ArbitrationResult;
import io.joynr.exceptions.JoynrRuntimeException;
import joynr.exceptions.ApplicationException;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public abstract class ProxyInvocationHandler implements InvocationHandler {
    private static final Logger logger = LoggerFactory.getLogger(ProxyInvocationHandler.class);
    protected Throwable throwable;

    abstract Object invoke(Method method, Object[] args) throws ApplicationException;

    public abstract void abort(JoynrRuntimeException exception);

    abstract void createConnector(ArbitrationResult result);

    /**
     * This method can be called to specify a throwable which will be thrown each time
     * {@link #invoke(Object, Method, Object[])} is called.
     *
     * @param throwable
     *            the throwable to be thrown when invoke is called.
     */
    void setThrowableForInvoke(Throwable throwable) {
        this.throwable = throwable;
    }

    /**
     * The InvocationHandler invoke method is mapped to the ProxyInvocationHandler.invoke which does not need the proxy
     * object (as the object upon which the method is actually called is the remote provider.
     */
    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        if (throwable != null) {
            throw throwable;
        }
        try {
            return invoke(method, args);
        } catch (Exception e) {
            if (this.throwable != null) {
                logger.debug("exception caught: {} overriden by: {}", e.getMessage(), throwable.getMessage());
                throw throwable;
            } else {
                throw e;
            }
        }
    }
}
