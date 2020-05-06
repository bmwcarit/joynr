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
package io.joynr.jeeintegration;

import static java.lang.String.format;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

import javax.ejb.EJBException;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Injector;

import io.joynr.dispatcher.rpc.MultiReturnValuesContainer;
import io.joynr.exceptions.JoynrException;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.security.JoynrCallingPrincipal;
import io.joynr.jeeintegration.context.JoynrJeeMessageContext;
import io.joynr.jeeintegration.multicast.SubscriptionPublisherInjectionWrapper;
import io.joynr.messaging.JoynrMessageCreator;
import io.joynr.messaging.JoynrMessageMetaInfo;
import io.joynr.provider.AbstractDeferred;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.MultiValueDeferred;
import io.joynr.provider.Promise;
import io.joynr.provider.SubscriptionPublisher;
import io.joynr.provider.SubscriptionPublisherInjection;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;

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

    private static final Logger logger = LoggerFactory.getLogger(ProviderWrapper.class);

    private static final List<Method> OBJECT_METHODS = Arrays.asList(Object.class.getMethods());

    private static final String SET_SUBSCRIPTION_PUBLISHER_METHOD_NAME = "setSubscriptionPublisher";
    // Sanity check that the method exists
    static {
        try {
            SubscriptionPublisherInjection.class.getMethod(SET_SUBSCRIPTION_PUBLISHER_METHOD_NAME,
                                                           SubscriptionPublisher.class);
        } catch (NoSuchMethodException e) {
            logger.error("Expecting to find method named {} with one argument of type {}, but not found on {}",
                         SET_SUBSCRIPTION_PUBLISHER_METHOD_NAME,
                         SubscriptionPublisher.class,
                         SubscriptionPublisherInjection.class);
        }
    }

    private Bean<?> bean;
    private BeanManager beanManager;
    private Injector injector;
    private Class<?> serviceInterface;

    /**
     * Initialises the instance with the service interface which will be exposed and the bean reference it is meant to
     * wrap.
     *
     * @param bean
     *            the bean reference to which calls will be delegated.
     * @param beanManager
     *            the bean manager.
     * @param injector the Guice injector.
     */
    public ProviderWrapper(Bean<?> bean, BeanManager beanManager, Injector injector) {
        ServiceProvider serviceProvider = bean.getBeanClass().getAnnotation(ServiceProvider.class);
        serviceInterface = serviceProvider.serviceInterface();
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
    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        boolean isProviderMethod = matchesJoynrProviderMethod(method);
        Method delegateToMethod = getMethodFromInterfaces(bean.getBeanClass(), method, isProviderMethod);
        Object delegate = createDelegateForMethod(method, isProviderMethod);
        Object result = null;
        try {
            if (isProviderMethod(method, delegateToMethod)) {
                JoynrJeeMessageContext.getInstance().activate();
                copyMessageCreatorInfo();
                copyMessageContext();
            }
            JoynrException joynrException = null;
            try {
                result = delegateToMethod.invoke(delegate, args);
            } catch (InvocationTargetException e) {
                joynrException = getJoynrExceptionFromInvocationException(e);
            }
            if (delegate != this) {
                AbstractDeferred deferred = createAndResolveOrRejectDeferred(delegateToMethod, result, joynrException);
                Promise<AbstractDeferred> promiseResult = new Promise<>(deferred);
                return promiseResult;
            }
        } finally {
            if (isProviderMethod(method, delegateToMethod)) {
                JoynrJeeMessageContext.getInstance().deactivate();
            }
        }
        return result;
    }

    @SuppressWarnings("unchecked")
    private AbstractDeferred createAndResolveOrRejectDeferred(Method method,
                                                              Object result,
                                                              JoynrException joynrException) {
        AbstractDeferred deferred;
        if (result == null && method.getReturnType().getTypeName().equals("void")) {
            deferred = new DeferredVoid();
            if (joynrException == null) {
                ((DeferredVoid) deferred).resolve();
            }
        } else {
            if (result instanceof MultiReturnValuesContainer) {
                deferred = new MultiValueDeferred();
                if (joynrException == null) {
                    ((MultiValueDeferred) deferred).resolve(((MultiReturnValuesContainer) result).getValues());
                }
            } else {
                deferred = new Deferred<Object>();
                if (joynrException == null) {
                    ((Deferred<Object>) deferred).resolve(result);
                }
            }
        }
        if (joynrException != null) {
            logger.debug("Provider method invocation resulted in provider runtime exception - rejecting the deferred {} with {}",
                         deferred,
                         joynrException);
            if (joynrException instanceof ApplicationException) {
                try {
                    Method rejectMethod = AbstractDeferred.class.getDeclaredMethod("reject",
                                                                                   new Class[]{ JoynrException.class });
                    rejectMethod.setAccessible(true);
                    rejectMethod.invoke(deferred, new Object[]{ joynrException });
                } catch (NoSuchMethodException | InvocationTargetException | IllegalAccessException e) {
                    logger.warn("Unable to set {} as rejection reason on {}. Wrapping in ProviderRuntimeException instead.",
                                joynrException,
                                deferred);
                    deferred.reject(new ProviderRuntimeException(((ApplicationException) joynrException).getMessage()));
                }
            } else if (joynrException instanceof ProviderRuntimeException) {
                deferred.reject((ProviderRuntimeException) joynrException);
            }
        }
        return deferred;
    }

    private JoynrException getJoynrExceptionFromInvocationException(InvocationTargetException e) throws InvocationTargetException {
        JoynrException joynrException = null;
        if (e.getCause() instanceof EJBException) {
            // an EJBException is only thrown when the exception is not in the throws declaration
            // ApplicationExceptions are always declared with throw and thus EJBExceptions won't be caused by them.
            Exception exception = ((EJBException) e.getCause()).getCausedByException();
            if (exception instanceof ProviderRuntimeException) {
                joynrException = (ProviderRuntimeException) exception;
            } else {
                joynrException = new ProviderRuntimeException("Unexpected exception from provider: "
                        + (exception == null ? e.getCause().toString() : exception.toString()));
            }
        } else if (e.getCause() instanceof ProviderRuntimeException || e.getCause() instanceof ApplicationException) {
            joynrException = (JoynrException) e.getCause();
        }

        if (joynrException == null) {
            throw e;
        }
        logger.trace("Returning joynr exception: {}", joynrException);
        return joynrException;
    }

    private boolean isProviderMethod(Method method, Method delegateToMethod) {
        boolean result = delegateToMethod != method;
        if (method.getDeclaringClass().equals(SubscriptionPublisherInjection.class)) {
            result = false;
        }
        return result;
    }

    private void copyMessageCreatorInfo() {
        JoynrMessageCreator joynrMessageCreator = injector.getInstance(JoynrMessageCreator.class);
        JoynrCallingPrincipal reference = getUniqueBeanReference(JoynrCallingPrincipal.class);

        String messageCreatorId = joynrMessageCreator.getMessageCreatorId();
        logger.trace("Setting user '{}' for message processing context.", messageCreatorId);
        reference.setUsername(messageCreatorId);
    }

    private void copyMessageContext() {
        JoynrMessageMetaInfo joynrMessageContext = injector.getInstance(JoynrMessageMetaInfo.class);
        JoynrJeeMessageMetaInfo jeeMessageContext = getUniqueBeanReference(JoynrJeeMessageMetaInfo.class);

        logger.trace("Setting message context for message processing context.");
        jeeMessageContext.setMessageContext(joynrMessageContext.getMessageContext());
    }

    private <T> T getUniqueBeanReference(Class<T> beanClass) {
        Set<Bean<?>> beans = beanManager.getBeans(beanClass);
        if (beans.size() != 1) {
            throw new IllegalStateException("There must be exactly one EJB of type " + beanClass.getName() + ". Found "
                    + beans.size());
        }

        @SuppressWarnings("unchecked")
        Bean<T> bean = (Bean<T>) beans.iterator().next();

        @SuppressWarnings("unchecked")
        T reference = (T) beanManager.getReference(bean, beanClass, beanManager.createCreationalContext(bean));

        return reference;
    }

    private Object createDelegateForMethod(Method method, boolean isProviderMethod) {
        if (OBJECT_METHODS.contains(method) || isProviderMethod) {
            return this;
        }
        if (SET_SUBSCRIPTION_PUBLISHER_METHOD_NAME.equals(method.getName())
                && SubscriptionPublisherInjection.class.isAssignableFrom(method.getDeclaringClass())) {
            return SubscriptionPublisherInjectionWrapper.createInvocationHandler(bean, beanManager).createProxy();
        }
        return beanManager.getReference(bean, serviceInterface, beanManager.createCreationalContext(bean));
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
                    if (logger.isTraceEnabled()) {
                        logger.trace(format("Method %s not found on interface %s", name, interfaceClass));
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
