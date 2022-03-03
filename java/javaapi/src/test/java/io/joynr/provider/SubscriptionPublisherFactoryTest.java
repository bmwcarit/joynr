/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
package io.joynr.provider;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnitRunner;

import joynr.test.NonInstantiable;
import joynr.tests.DefaulttestProvider;
import joynr.tests.test;
import joynr.tests.testProvider;
import joynr.tests.testSubscriptionPublisherImpl;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionPublisherFactoryTest {

    SubscriptionPublisherFactory subscriptionPublisherFactory;

    @Before
    public void setUp() throws Exception {
        subscriptionPublisherFactory = new SubscriptionPublisherFactory();
    }

    @Test
    public void testCreateWithCorrectProvider() throws Exception {
        DefaulttestProvider providerSpy = Mockito.spy(new DefaulttestProvider());
        subscriptionPublisherFactory.create(providerSpy);
        verify(providerSpy, times(1)).setSubscriptionPublisher(any(testSubscriptionPublisherImpl.class));
    }

    @Test(expected = IllegalArgumentException.class)
    public void testCreateWithWrongProvider() throws Exception {
        JoynrProvider providerSpy = mock(JoynrProvider.class);
        subscriptionPublisherFactory.create(providerSpy);
    }

    @JoynrInterface(name = "test/WithoutSubscriptionPublisher", provider = testProvider.class,
                    provides = testProvider.class)
    private static interface testProviderWithoutSubscriptionPublisher {
        /*
         *  In this case, no matching SubscriptionPublisherImpl exists since the provided class is incorrect
         */
    }

    @Test(expected = IllegalArgumentException.class)
    public void testCreateWithMissingSubscriptionPublisher() throws Exception {
        testProviderWithoutSubscriptionPublisher providerSpy = mock(testProviderWithoutSubscriptionPublisher.class);
        subscriptionPublisherFactory.create(providerSpy);
    }

    @JoynrInterface(name = "test/NonInstantiable", provider = testProvider.class, provides = NonInstantiable.class)
    private static interface testProviderWithNonInstantiableSubscriptionPublisher {
        /*
         *  In this case, a matching SubscriptionPublisherImpl exists {@link joynr.test.NonInstantiableSubscriptionProviderImpl}.
         *  However, the subscription publisher cannot be instantiated, as it has no nullable constructor.
         */

    }

    @Test(expected = IllegalArgumentException.class)
    public void testCreateWithNonInstantiableSubscriptionPublisher() throws Exception {
        testProviderWithNonInstantiableSubscriptionPublisher providerSpy = mock(testProviderWithNonInstantiableSubscriptionPublisher.class);
        subscriptionPublisherFactory.create(providerSpy);
    }
}
