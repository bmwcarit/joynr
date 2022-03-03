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

import static org.junit.Assert.assertNotNull;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.verify;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.jeeintegration.multicast.SubscriptionPublisherWrapper;
import joynr.jeeintegration.servicelocator.MyServiceSubscriptionPublisher;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionPublisherWrapperTest {

    @Mock
    private MyServiceSubscriptionPublisher subscriptionPublisherMock;

    private MyServiceSubscriptionPublisher subject;

    @Before
    public void setup() {
        subject = SubscriptionPublisherWrapper.createWrapper(subscriptionPublisherMock,
                                                             MyServiceSubscriptionPublisher.class);
        assertNotNull(subject);
    }

    @Test
    public void testCallMulticastMethod() {
        subject.fireMyMulticast("some value");
        verify(subscriptionPublisherMock).fireMyMulticast(anyString());
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testCallSelectiveBroadcastMethod() {
        subject.fireMySelectiveBroadcast("some value");
    }
}
