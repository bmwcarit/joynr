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
package io.joynr.provider;

import static org.mockito.Mockito.mock;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.pubsub.publication.BroadcastFilterImpl;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testSubscriptionPublisher;

@RunWith(MockitoJUnitRunner.class)
public class DefaultInterfaceProviderTest {
    private DefaulttestProvider fixture = new DefaulttestProvider();

    @Test
    public void addBroadcastFilterBeforeSettingSubscriptionPublisher() {
        testSubscriptionPublisher subscriptionPublisher = mock(testSubscriptionPublisher.class);
        //expect to queued broadcast filter
        fixture.addBroadcastFilter(new BroadcastFilterImpl("testFilter1"));
        Mockito.verify(subscriptionPublisher, Mockito.never()).addBroadcastFilter(Mockito.<BroadcastFilterImpl> any());

        //after setting the subscription publisher, the queued broadcast filters should be forwarded
        fixture.setSubscriptionPublisher(subscriptionPublisher);
        Mockito.verify(subscriptionPublisher, Mockito.times(1)).addBroadcastFilter(Mockito.<BroadcastFilterImpl> any());

        fixture.addBroadcastFilter(new BroadcastFilterImpl("testFilter2"));
        Mockito.verify(subscriptionPublisher, Mockito.times(2)).addBroadcastFilter(Mockito.<BroadcastFilterImpl> any());
    }
}
