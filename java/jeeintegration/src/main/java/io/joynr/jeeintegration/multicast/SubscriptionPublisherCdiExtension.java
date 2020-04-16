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

import java.lang.annotation.Annotation;
import java.lang.reflect.Member;
import java.lang.reflect.Type;
import java.util.Set;

import javax.enterprise.event.Observes;
import javax.enterprise.inject.spi.Annotated;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.Extension;
import javax.enterprise.inject.spi.InjectionPoint;
import javax.enterprise.inject.spi.ProcessInjectionPoint;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.provider.SubscriptionPublisher;

public class SubscriptionPublisherCdiExtension implements Extension {

    private static final Logger logger = LoggerFactory.getLogger(SubscriptionPublisherCdiExtension.class);

    public void alterSubscriptionPublishInjectionPoints(@Observes ProcessInjectionPoint processInjectionPoint) {
        final InjectionPoint injectionPoint = processInjectionPoint.getInjectionPoint();
        logger.info("Looking at injection point: {}", injectionPoint);
        if (injectionPoint.getType() instanceof Class
                && SubscriptionPublisher.class.isAssignableFrom((Class) injectionPoint.getType())) {
            logger.info("Re-writing injection point type from {} to {} on bean {}",
                        injectionPoint.getType(),
                        SubscriptionPublisher.class,
                        injectionPoint.getBean());
            final Bean<?> bean = injectionPoint.getBean();
            InjectionPoint newInjectionPoint = new InjectionPoint() {
                @Override
                public Type getType() {
                    return SubscriptionPublisher.class;
                }

                @Override
                public Set<Annotation> getQualifiers() {
                    return injectionPoint.getQualifiers();
                }

                @Override
                public Bean<?> getBean() {
                    return bean;
                }

                @Override
                public Member getMember() {
                    return injectionPoint.getMember();
                }

                @Override
                public Annotated getAnnotated() {
                    return injectionPoint.getAnnotated();
                }

                @Override
                public boolean isDelegate() {
                    return injectionPoint.isDelegate();
                }

                @Override
                public boolean isTransient() {
                    return injectionPoint.isTransient();
                }
            };
            processInjectionPoint.setInjectionPoint(newInjectionPoint);
        }
    }

}
