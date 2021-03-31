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
package io.joynr.jeeintegration.multicast;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.lang.reflect.Type;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;
import javax.enterprise.inject.spi.InjectionPoint;
import javax.enterprise.util.AnnotationLiteral;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.provider.SubscriptionPublisher;
import io.joynr.provider.SubscriptionPublisherInjection;

/**
 * This class is used to {@link #createInvocationHandler(Bean, BeanManager) create an invocation handler}
 * which can be used to {@link #createProxy() create a proxy} which intercepts
 * the subscription publisher provided by joynr and
 * {@link SubscriptionPublisherProducer#add(SubscriptionPublisher, Class) adds it to the producer}
 * so that it can be injected in bean instances in the CDI runtime.
 * The subscription publishers which are
 * {@link SubscriptionPublisherInjection#setSubscriptionPublisher(SubscriptionPublisher) provided by the joynr runtime}
 * are further wrapped in a {@link SubscriptionPublisherWrapper}, which is responsbile for ensuring that only multicast
 * publications are made from within the JEE container runtime.
 */
public class SubscriptionPublisherInjectionWrapper implements InvocationHandler {

    private static final Logger logger = LoggerFactory.getLogger(SubscriptionPublisherInjectionWrapper.class);

    private static final List<Method> OBJECT_METHODS = Arrays.asList(Object.class.getMethods());
    private static final AnnotationLiteral<io.joynr.jeeintegration.api.SubscriptionPublisher> SUBSCRIPTION_PUBLISHER_ANNOTATION_LITERAL = new AnnotationLiteral<io.joynr.jeeintegration.api.SubscriptionPublisher>() {
    };

    private final Class<? extends SubscriptionPublisherInjection> proxiedInterface;
    private final Class<? extends SubscriptionPublisher> subscriptionPublisherClass;
    private final Class beanClass;
    private SubscriptionPublisher subscriptionPublisher;
    private SubscriptionPublisherProducer subscriptionPublisherProducer;

    private SubscriptionPublisherInjectionWrapper(Class<? extends SubscriptionPublisherInjection> proxiedInterface,
                                                  Class<? extends SubscriptionPublisher> subscriptionPublisherClass,
                                                  SubscriptionPublisherProducer subscriptionPublisherProducer,
                                                  Class beanClass) {
        this.proxiedInterface = proxiedInterface;
        this.subscriptionPublisherClass = subscriptionPublisherClass;
        this.subscriptionPublisherProducer = subscriptionPublisherProducer;
        this.beanClass = beanClass;
    }

    public static SubscriptionPublisherInjectionWrapper createInvocationHandler(Bean<?> bean, BeanManager beanManager) {
        SubscriptionPublisherProducer subscriptionPublisherProducer = getSubscriptionPublisherProducerReference(beanManager);
        Class proxiedInterface = SubscriptionPublisherInjection.class;
        Class subscriptionPublisherClass = null;
        Class beanClass = bean.getBeanClass();
        for (InjectionPoint injectionPoint : bean.getInjectionPoints()) {
            if (!injectionPoint.getQualifiers().contains(SUBSCRIPTION_PUBLISHER_ANNOTATION_LITERAL)) {
                continue;
            }
            Type baseType = injectionPoint.getAnnotated().getBaseType();
            if (baseType instanceof Class && SubscriptionPublisher.class.isAssignableFrom((Class) baseType)) {
                subscriptionPublisherClass = (Class) baseType;
                break;
            }
        }
        logger.debug("Found injector {} and publisher {} classes.", proxiedInterface, subscriptionPublisherClass);
        if (subscriptionPublisherClass == null || proxiedInterface == null) {
            throw new JoynrIllegalStateException("Cannot create subscription publisher injection wrapper proxy for bean which doesn't inject a concrete SubscriptionPublisher.");
        }
        return new SubscriptionPublisherInjectionWrapper(proxiedInterface,
                                                         subscriptionPublisherClass,
                                                         subscriptionPublisherProducer,
                                                         beanClass);
    }

    private static SubscriptionPublisherProducer getSubscriptionPublisherProducerReference(BeanManager beanManager) {
        Set<Bean<?>> beans = beanManager.getBeans(SubscriptionPublisherProducer.class);
        Bean<SubscriptionPublisherProducer> bean = (Bean<SubscriptionPublisherProducer>) beans.iterator().next();
        SubscriptionPublisherProducer reference = (SubscriptionPublisherProducer) beanManager.getReference(bean,
                                                                                                           SubscriptionPublisherProducer.class,
                                                                                                           beanManager.createCreationalContext(bean));
        return reference;
    }

    public Object createProxy() {
        return Proxy.newProxyInstance(proxiedInterface.getClassLoader(), new Class[]{ proxiedInterface }, this);
    }

    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        if (OBJECT_METHODS.contains(method)) {
            return method.invoke(this, args);
        }
        if (args == null || args.length == 0 || args[0] == null
                || !subscriptionPublisherClass.isAssignableFrom(args[0].getClass())) {
            throw new JoynrIllegalStateException(String.format("Cannot set the subscription publisher with args %s. First argument must be a non-null instance of a %s.",
                                                               Arrays.toString(args),
                                                               subscriptionPublisherClass));
        }
        subscriptionPublisherProducer.add((SubscriptionPublisher) args[0], beanClass);
        return null;
    }
}
