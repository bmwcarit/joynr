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

import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import java.util.ArrayList;
import java.util.List;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.pubsub.publication.BroadcastFilter;
import io.joynr.pubsub.publication.BroadcastListener;

@RunWith(MockitoJUnitRunner.class)
public class SubscriptionPublisherTest {

    static class MySubscriptionPublisher extends AbstractSubscriptionPublisher {

        // change visibility from protected to public for testing purposes
        @Override
        public void fireBroadcast(String broadcastName, List<BroadcastFilter> broadcastFilters, Object... values) {
            super.fireBroadcast(broadcastName, broadcastFilters, values);
        }

    }

    @Before
    public void setUp() throws Exception {
    }

    @Test
    public void registeredBroadcastListenerIsCalledOnBroadcast() throws Exception {
        String broadcastName = "mybroadcast";
        String value1 = "value1";
        String value2 = "value2";

        MySubscriptionPublisher subscriptionPublisher = new MySubscriptionPublisher();
        BroadcastListener broadcastListener = mock(BroadcastListener.class);

        subscriptionPublisher.registerBroadcastListener(broadcastName, broadcastListener);

        List<BroadcastFilter> broadcastFilters = new ArrayList<BroadcastFilter>();
        BroadcastFilter broadcastFilter = mock(BroadcastFilter.class);
        broadcastFilters.add(broadcastFilter);

        subscriptionPublisher.fireBroadcast(broadcastName, broadcastFilters, value1, value2);

        verify(broadcastListener).broadcastOccurred(eq(broadcastFilters), eq(value1), eq(value2));

    }

    @Test
    public void unregisteredBroadcastListenerNotCalledOnBroadcast() throws Exception {
        String broadcastName = "mybroadcast";
        String value1 = "value1";
        String value2 = "value2";

        MySubscriptionPublisher subscriptionPublisher = new MySubscriptionPublisher();
        BroadcastListener broadcastListener = mock(BroadcastListener.class);
        subscriptionPublisher.registerBroadcastListener(broadcastName, broadcastListener);
        subscriptionPublisher.unregisterBroadcastListener(broadcastName, broadcastListener);

        List<BroadcastFilter> broadcastFilters = new ArrayList<BroadcastFilter>();
        BroadcastFilter broadcastFilter = mock(BroadcastFilter.class);
        broadcastFilters.add(broadcastFilter);

        subscriptionPublisher.fireBroadcast(broadcastName, broadcastFilters, value1, value2);

        verify(broadcastListener, never()).broadcastOccurred(eq(broadcastFilters), eq(value1), eq(value2));
    }

}
