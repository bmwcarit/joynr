package io.joynr.proxy;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.pubsub.publication.BroadcastFilter;
import io.joynr.pubsub.publication.BroadcastListener;

import java.util.ArrayList;
import java.util.List;

import joynr.types.ProviderQos;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class ProviderTest {

    static class MyProviderClass extends AbstractJoynrProvider {

        @Override
        public ProviderQos getProviderQos() {
            return new ProviderQos();
        }

        // change visibility from protected to public for testing purposes
        public void fireBroadcast(String broadcastName, List<BroadcastFilter> broadcastFilters, Object... values) {
            super.fireBroadcast(broadcastName, broadcastFilters, values);
        }

        @Override
        public Class<?> getProvidedInterface() {
            return getClass();
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

        ProviderTest.MyProviderClass provider = new ProviderTest.MyProviderClass();
        BroadcastListener broadcastListener = mock(BroadcastListener.class);
        provider.registerBroadcastListener(broadcastName, broadcastListener);

        List<BroadcastFilter> broadcastFilters = new ArrayList<BroadcastFilter>();
        BroadcastFilter broadcastFilter = mock(BroadcastFilter.class);
        broadcastFilters.add(broadcastFilter);

        provider.fireBroadcast(broadcastName, broadcastFilters, value1, value2);

        verify(broadcastListener).broadcastOccurred(eq(broadcastFilters), eq(value1), eq(value2));

    }

    @Test
    public void unregisteredBroadcastListenerNotCalledOnBroadcast() throws Exception {
        String broadcastName = "mybroadcast";
        String value1 = "value1";
        String value2 = "value2";

        ProviderTest.MyProviderClass provider = new ProviderTest.MyProviderClass();
        BroadcastListener broadcastListener = mock(BroadcastListener.class);
        provider.registerBroadcastListener(broadcastName, broadcastListener);
        provider.unregisterBroadcastListener(broadcastName, broadcastListener);

        List<BroadcastFilter> broadcastFilters = new ArrayList<BroadcastFilter>();
        BroadcastFilter broadcastFilter = mock(BroadcastFilter.class);
        broadcastFilters.add(broadcastFilter);

        provider.fireBroadcast(broadcastName, broadcastFilters, value1, value2);

        verify(broadcastListener, never()).broadcastOccurred(eq(broadcastFilters), eq(value1), eq(value2));
    }

}
