package test.io.joynr.jeeintegration;

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

import static org.junit.Assert.assertNotNull;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.jeeintegration.SubscriptionPublisherInjectionWrapper;
import io.joynr.jeeintegration.api.ServiceProvider;
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

    @ServiceProvider(serviceInterface = MyServiceSync.class)
    private class MyServiceBean implements MyServiceSync,
            SubscriptionPublisherInjection<MyServiceSubscriptionPublisher> {

        @Override
        public void setSubscriptionPublisher(MyServiceSubscriptionPublisher subscriptionPublisher) {
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

    private SubscriptionPublisherInjection subject;

    @Before
    public void setup() {
        subject = (SubscriptionPublisherInjection) SubscriptionPublisherInjectionWrapper.createWrapper(myServiceBean,
                                                                                                       MyServiceBean.class);
        assertNotNull(subject);
    }

    @Test
    public void testCreateAndCallWrapper() {
        subject.setSubscriptionPublisher(mock(MyServiceSubscriptionPublisher.class));
        verify(myServiceBean).setSubscriptionPublisher(any());
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testCallWithNull() {
        subject.setSubscriptionPublisher(null);
    }
}
