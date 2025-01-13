/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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
package io.joynr.jeeintegration;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;

import jakarta.enterprise.inject.spi.Bean;
import jakarta.enterprise.inject.spi.BeanManager;

public class BeanCallingProxy<T> implements InvocationHandler {

    private final BeanManager beanManager;
    private final Bean<T> bean;

    public BeanCallingProxy(Bean<T> bean, BeanManager beanManager) {
        this.bean = bean;
        this.beanManager = beanManager;
    }

    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        T instance = bean.create(beanManager.createCreationalContext(bean));
        return method.invoke(instance, args);
    }
}
