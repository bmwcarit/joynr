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
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

import jakarta.ejb.Singleton;
import jakarta.enterprise.inject.Produces;
import jakarta.enterprise.inject.spi.InjectionPoint;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.provider.SubscriptionPublisher;

@Singleton
public class SubscriptionPublisherProducer {

    private static final Logger logger = LoggerFactory.getLogger(SubscriptionPublisherProducer.class);

    private ConcurrentMap<Class<?>, SubscriptionPublisher> subscriptionPublishers = new ConcurrentHashMap<>();

    @Produces
    @io.joynr.jeeintegration.api.SubscriptionPublisher
    public SubscriptionPublisher getSubscriptionPublisher(InjectionPoint injectionPoint) {
        logger.info("Looking for subscription publisher for: {}", injectionPoint);
        logger.trace("Type {}, Member {}, Annotated {} ",
                     injectionPoint.getType(),
                     injectionPoint.getMember(),
                     injectionPoint.getAnnotated());
        Class beanClass = injectionPoint.getBean() == null ? injectionPoint.getMember().getDeclaringClass()
                : injectionPoint.getBean().getBeanClass();
        logger.info("Bean class is: {}", beanClass);
        if (!subscriptionPublishers.containsKey(beanClass)) {
            Class subscriptionPublisherClass = (Class) injectionPoint.getAnnotated().getBaseType();
            SubscriptionPublisher newSubscriptionPublisher = (SubscriptionPublisher) Proxy.newProxyInstance(subscriptionPublisherClass.getClassLoader(),
                                                                                                            new Class[]{
                                                                                                                    subscriptionPublisherClass },
                                                                                                            new InvocationHandler() {
                                                                                                                @Override
                                                                                                                public Object invoke(Object proxy,
                                                                                                                                     Method method,
                                                                                                                                     Object[] args) throws Throwable {
                                                                                                                    throw new IllegalStateException("No subscription publisher set for "
                                                                                                                            + subscriptionPublisherClass
                                                                                                                            + " on "
                                                                                                                            + beanClass);
                                                                                                                }
                                                                                                            });
            subscriptionPublishers.putIfAbsent(beanClass, newSubscriptionPublisher);
        }
        return subscriptionPublishers.get(beanClass);
    }

    public void add(SubscriptionPublisher subscriptionPublisher, Class forBeanClass) {
        subscriptionPublishers.put(forBeanClass, subscriptionPublisher);
    }

}
