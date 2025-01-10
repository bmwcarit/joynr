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
package io.joynr.jeeintegration.multicast;

import static org.junit.Assert.assertNotNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.annotation.Annotation;
import java.util.Arrays;
import java.util.HashSet;

import jakarta.enterprise.inject.spi.Annotated;
import jakarta.enterprise.inject.spi.Bean;
import jakarta.enterprise.inject.spi.BeanManager;
import jakarta.enterprise.inject.spi.InjectionPoint;
import jakarta.enterprise.util.AnnotationLiteral;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import io.joynr.provider.SubscriptionPublisherInjection;
import joynr.exceptions.ApplicationException;
import joynr.jeeintegration.servicelocator.MyServiceSubscriptionPublisher;
import joynr.jeeintegration.servicelocator.MyServiceSync;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionPublisherInjectionWrapperTest {

    private static final AnnotationLiteral<SubscriptionPublisher> SUBSCRIPTION_PUBLISHER_ANNOTATION_LITERAL = new AnnotationLiteral<SubscriptionPublisher>() {
        private static final long serialVersionUID = 1L;
    };

    @ServiceProvider(serviceInterface = MyServiceSync.class)
    private class MyServiceBean implements MyServiceSync {

        @Override
        public String callMe(String parameterOne) {
            return null;
        }

        @Override
        public void callMeWithException() throws ApplicationException {
        }
    }

    @Mock
    private Bean myServiceBeanBean;

    @Mock
    private InjectionPoint subscriptionPublisherInjectionPoint;

    @Mock
    private Annotated annotated;

    @Mock
    private BeanManager beanManager;

    @Mock
    private Bean<?> subscriptionPublisherProducerBean;

    @Mock
    private SubscriptionPublisherProducer subscriptionPublisherProducer;

    private SubscriptionPublisherInjection<MyServiceSubscriptionPublisher> subject;
    private SubscriptionPublisherInjectionWrapper invocationHandler;

    @Before
    public void setup() {
        when(beanManager.getBeans(eq(SubscriptionPublisherProducer.class))).thenReturn(new HashSet<Bean<?>>(Arrays.asList(subscriptionPublisherProducerBean)));
        when(beanManager.getReference(eq(subscriptionPublisherProducerBean),
                                      eq(SubscriptionPublisherProducer.class),
                                      any())).thenReturn(subscriptionPublisherProducer);
        when(myServiceBeanBean.getBeanClass()).thenReturn(MyServiceBean.class);
        when(myServiceBeanBean.getInjectionPoints()).thenReturn(new HashSet<InjectionPoint>(Arrays.asList(subscriptionPublisherInjectionPoint)));
        when(subscriptionPublisherInjectionPoint.getQualifiers()).thenReturn(new HashSet<Annotation>(Arrays.asList((Annotation) SUBSCRIPTION_PUBLISHER_ANNOTATION_LITERAL)));
        when(subscriptionPublisherInjectionPoint.getAnnotated()).thenReturn(annotated);
        when(annotated.getBaseType()).thenReturn(MyServiceSubscriptionPublisher.class);
        invocationHandler = SubscriptionPublisherInjectionWrapper.createInvocationHandler(myServiceBeanBean,
                                                                                          beanManager);
        subject = (SubscriptionPublisherInjection<MyServiceSubscriptionPublisher>) invocationHandler.createProxy();
        assertNotNull(subject);
    }

    @Test
    public void testCreateAndRegisterSubscriptionPublisher() {
        MyServiceSubscriptionPublisher subscriptionPublisher = mock(MyServiceSubscriptionPublisher.class);
        subject.setSubscriptionPublisher(subscriptionPublisher);
        verify(subscriptionPublisherProducer).add(any(), eq(MyServiceBean.class));
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testCallWithNull() {
        subject.setSubscriptionPublisher(null);
    }
}
