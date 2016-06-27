package io.joynr.jeeintegration;

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

import static java.lang.String.format;

import io.joynr.jeeintegration.api.security.JoynrCallingPrincipal;
import io.joynr.jeeintegration.context.JoynrJeeMessageContext;
import io.joynr.messaging.JoynrMessageCreator;
import io.joynr.provider.AbstractDeferred;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.Promise;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

import javax.enterprise.context.spi.CreationalContext;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Injector;

/**
 * This class wraps an EJB which is decorated with {@link io.joynr.jeeintegration.api.ServiceProvider} and has a valid
 * service interface specified (that is it extends {@link JoynrProvider}). When the bean is discovered in
 * {@link JoynrIntegrationBean#initialise()} an instance of this class is registered as the provider with the joynr
 * runtime. When joynr wants to call a method of the specified service interface, then this instance will obtain a
 * reference to the bean via the {@link JoynrIntegrationBean#beanManager} and will delegate to the corresponding method
 * on that bean (i.e. with the same name and parameters). The result is then wrapped in a deferred / promise and
 * returned.
 */
public class ProviderWrapper implements InvocationHandler {

    private static final Logger LOG = LoggerFactory.getLogger(ProviderWrapper.class);

    private static final List<Method> OBJECT_METHODS = Arrays.asList(Object.class.getMethods());

    private Bean<?> bean;
    private BeanManager beanManager;
    private Injector injector;

    /**
     * Initialises the instance with the service interface which will be exposed and the bean reference it is meant to
     * wrap.
     *
     * @param bean
     *            the bean reference to which calls will be delegated.
     * @param beanManager
     *            the bean manager
     * @param injector
     */
    public ProviderWrapper(Bean<?> bean, BeanManager beanManager, Injector injector) {
        this.bean = bean;
        this.beanManager = beanManager;
        this.injector = injector;
    }

    /**
     * When a method is invoked via a joynr call, then it is delegated to an instance of the bean with which this
     * instance was initialised, if the method is part of the business interface and to this instance if it was part of
     * the {@link JoynrProvider} interface or the <code>Object</code> class.
     *
     * @param proxy
     *            the proxy object on which the method was called.
     * @param method
     *            the specific method which was called.
     * @param args
     *            the arguments with which the method was called.
     *
     * @return the result of the delegate method call on the EJB, but wrapped in a promise, as all the provider methods
     *         in joynr are declared that way.
     */
    @SuppressWarnings({ "unchecked", "rawtypes" })
    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        boolean isProviderMethod = matchesJoynrProviderMethod(method);
        Method delegateToMethod = getMethodFromInterfaces(bean.getBeanClass(), method, isProviderMethod);
        Object delegate = createDelegateForMethod(method, isProviderMethod);
        Object result;
        try {
            if (delegateToMethod != method) {
                JoynrJeeMessageContext.getInstance().activate();
                copyMessageCreatorInfo();
            }
            result = delegateToMethod.invoke(delegate, args);
            if (delegate != this) {
                AbstractDeferred deferred;
                if (result == null && method.getReturnType().equals(Void.class)) {
                    deferred = new DeferredVoid();
                    ((DeferredVoid) deferred).resolve();
                } else {
                    deferred = new Deferred();
                    ((Deferred) deferred).resolve(result);
                }
                Promise promiseResult = new Promise(deferred);
                return promiseResult;
            }
        } finally {
            if (delegateToMethod != method) {
                JoynrJeeMessageContext.getInstance().deactivate();
            }
        }
        return result;
    }

    private void copyMessageCreatorInfo() {
        JoynrMessageCreator joynrMessageCreator = injector.getInstance(JoynrMessageCreator.class);
        Set<Bean<?>> beans = beanManager.getBeans(JoynrCallingPrincipal.class);
        if (beans.size() != 1) {
            throw new IllegalStateException("There must be exactly one EJB of type "
                    + JoynrCallingPrincipal.class.getName() + ". Found " + beans.size());
        }
        @SuppressWarnings("unchecked")
        Bean<JoynrCallingPrincipal> bean = (Bean<JoynrCallingPrincipal>) beans.iterator().next();
        JoynrCallingPrincipal reference = (JoynrCallingPrincipal) beanManager.getReference(bean,
                                                                                           JoynrCallingPrincipal.class,
                                                                                           beanManager.createCreationalContext(bean));
        String messageCreatorId = joynrMessageCreator.getMessageCreatorId();
        LOG.trace("Setting user '{}' for message processing context.", messageCreatorId);
        reference.setUsername(messageCreatorId);
    }

    @SuppressWarnings({ "rawtypes", "unchecked" })
    private Object createDelegateForMethod(Method method, boolean isProviderMethod) {
        if (OBJECT_METHODS.contains(method) || isProviderMethod) {
            return this;
        }
        return bean.create((CreationalContext) beanManager.createCreationalContext(bean));
    }

    private Method getMethodFromInterfaces(Class<?> beanClass,
                                           Method method,
                                           boolean isProviderMethod) throws NoSuchMethodException {
        String name = method.getName();
        Class<?>[] parameterTypes = method.getParameterTypes();
        Method result = method;
        if (!isProviderMethod) {
            result = null;
            for (Class<?> interfaceClass : beanClass.getInterfaces()) {
                try {
                    if ((result = interfaceClass.getMethod(name, parameterTypes)) != null) {
                        break;
                    }
                } catch (NoSuchMethodException | SecurityException e) {
                    if (LOG.isTraceEnabled()) {
                        LOG.trace(format("Method %s not found on interface %s", name, interfaceClass));
                    }
                }
            }
        }
        return result == null ? method : result;
    }

    private boolean matchesJoynrProviderMethod(Method method) {
        boolean result = false;
        for (Method joynrProviderMethod : JoynrProvider.class.getMethods()) {
            if (joynrProviderMethod.getName().equals(method.getName())
                    && Arrays.equals(joynrProviderMethod.getParameterTypes(), method.getParameterTypes())) {
                result = true;
                break;
            }
        }
        return result;
    }

}
