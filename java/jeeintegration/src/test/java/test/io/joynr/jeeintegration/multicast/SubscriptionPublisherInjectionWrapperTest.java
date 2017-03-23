package test.io.joynr.jeeintegration.multicast;

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

import static org.junit.Assert.assertNotNull;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import javax.enterprise.inject.spi.Annotated;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;
import javax.enterprise.inject.spi.InjectionPoint;
import javax.enterprise.util.AnnotationLiteral;
import javax.inject.Inject;

import com.google.common.collect.Sets;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import io.joynr.jeeintegration.multicast.SubscriptionPublisherInjectionWrapper;
import io.joynr.jeeintegration.multicast.SubscriptionPublisherProducer;
import io.joynr.provider.SubscriptionPublisherInjection;
import joynr.exceptions.ApplicationException;
import joynr.jeeintegration.servicelocator.MyServiceSubscriptionPublisher;
import joynr.jeeintegration.servicelocator.MyServiceSync;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionPublisherInjectionWrapperTest {

    private static final Logger logger = LoggerFactory.getLogger(SubscriptionPublisherInjectionWrapperTest.class);

    private static final AnnotationLiteral<SubscriptionPublisher> SUBSCRIPTION_PUBLISHER_ANNOTATION_LITERAL = new AnnotationLiteral<SubscriptionPublisher>() {
    };

    @ServiceProvider(serviceInterface = MyServiceSync.class)
    private class MyServiceBean implements MyServiceSync {

        private MyServiceSubscriptionPublisher myServiceSubscriptionPublisher;

        @Inject
        public MyServiceBean(@SubscriptionPublisher MyServiceSubscriptionPublisher myServiceSubscriptionPublisher) {
            this.myServiceSubscriptionPublisher = myServiceSubscriptionPublisher;
        }

        @Override
        public String callMe(String parameterOne) {
            return null;
        }

        @Override
        public void callMeWithException() throws ApplicationException {
        }
    }

    @Mock
    private MyServiceBean myServiceBean;

    @Mock
    private Bean myServiceBeanBean;

    @Mock
    private InjectionPoint subscriptionPublisherInjectionPoint;

    @Mock
    private Annotated annotated;

    @Mock
    private BeanManager beanManager;

    @Mock
    private Bean subscriptionPublisherProducerBean;

    @Mock
    private SubscriptionPublisherProducer subscriptionPublisherProducer;

    private SubscriptionPublisherInjection subject;
    private SubscriptionPublisherInjectionWrapper invocationHandler;

    @Before
    public void setup() {
        when(beanManager.getBeans(eq(SubscriptionPublisherProducer.class))).thenReturn(Sets.newHashSet(subscriptionPublisherProducerBean));
        when(beanManager.getReference(eq(subscriptionPublisherProducerBean),
                                      eq(SubscriptionPublisherProducer.class),
                                      any())).thenReturn(subscriptionPublisherProducer);
        when(myServiceBeanBean.getBeanClass()).thenReturn(MyServiceBean.class);
        when(myServiceBeanBean.getInjectionPoints()).thenReturn(Sets.newHashSet(subscriptionPublisherInjectionPoint));
        when(subscriptionPublisherInjectionPoint.getQualifiers()).thenReturn(Sets.newHashSet(SUBSCRIPTION_PUBLISHER_ANNOTATION_LITERAL));
        when(subscriptionPublisherInjectionPoint.getAnnotated()).thenReturn(annotated);
        when(annotated.getBaseType()).thenReturn(MyServiceSubscriptionPublisher.class);
        invocationHandler = SubscriptionPublisherInjectionWrapper.createInvocationHandler(myServiceBeanBean,
                                                                                          beanManager);
        subject = (SubscriptionPublisherInjection) invocationHandler.createProxy();
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
