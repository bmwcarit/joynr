/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2021 BMW Car IT GmbH
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
package io.joynr.proxy.invocation;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import org.junit.Before;
import org.junit.Test;

import io.joynr.proxy.Future;
import io.joynr.pubsub.SubscriptionQos;
import joynr.UnicastSubscriptionQos;

public class SubscriptionInvocationTest {
    SubscriptionInvocation fixture;

    @Before
    public void setUp() throws Exception {
        fixture = mock(SubscriptionInvocation.class);
        when(fixture.hasSubscriptionId()).thenCallRealMethod();
    }

    @Test
    public void hasSubscriptionId_validID() {
        when(fixture.getSubscriptionId()).thenReturn("Valid ID");

        assertTrue(fixture.hasSubscriptionId());
    }

    @Test
    public void hasSubscriptionId_emptyID() {
        when(fixture.getSubscriptionId()).thenReturn("");

        assertFalse(fixture.hasSubscriptionId());
    }

    @Test
    public void hasSubscriptionId_nullId() {
        when(fixture.getSubscriptionId()).thenReturn(null);

        assertFalse(fixture.hasSubscriptionId());
    }

    @Test
    public void ctor() {
        Future<String> future = new Future<String>();
        String subscriptionName = new String();
        SubscriptionQos qos = new UnicastSubscriptionQos();
        Object proxy = new Object();
        SubscriptionInvocation subject = new SubscriptionInvocationStub(future, subscriptionName, qos, proxy);
        assertSame(future, subject.getFuture());
        assertSame(subscriptionName, subject.getSubscriptionName());
        assertSame(qos, subject.getQos());
        assertSame(proxy, subject.getProxy());
        assertNull(subject.getSubscriptionId());
    }

    private class SubscriptionInvocationStub extends SubscriptionInvocation {
        public SubscriptionInvocationStub(Future<String> future,
                                          String subscriptionName,
                                          SubscriptionQos qos,
                                          Object proxy) {
            super(future, subscriptionName, qos, proxy);
        }
    };
}
