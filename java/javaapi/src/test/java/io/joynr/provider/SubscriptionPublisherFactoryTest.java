package io.joynr.provider;

/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import io.joynr.exceptions.JoynrRuntimeException;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testSubscriptionPublisherImpl;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

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

    @Test
    public void testCreateWithWrongProvider() throws Exception {
        JoynrProvider providerSpy = mock(JoynrProvider.class);
        try {
            subscriptionPublisherFactory.create(providerSpy);
            assertFalse("Expected exception didn't arrive when calling ProviderContainerFactory.create "
                    + "with wrong parameter", true);
        } catch (JoynrRuntimeException e) {
            //expected exception
        }
    }

}
