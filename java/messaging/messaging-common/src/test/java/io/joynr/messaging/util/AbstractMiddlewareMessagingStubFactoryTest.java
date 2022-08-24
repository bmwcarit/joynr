/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
package io.joynr.messaging.util;

import java.util.ArrayList;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.SuccessAction;
import joynr.system.RoutingTypes.Address;
import joynr.ImmutableMessage;

public class AbstractMiddlewareMessagingStubFactoryTest {
    private class TestAddress extends Address {
        private String text;

        public TestAddress(String text) {
            this.text = text;
        }

        public String getText() {
            return text;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj)
                return true;
            if (obj == null)
                return false;
            if (getClass() != obj.getClass())
                return false;
            if (!super.equals(obj))
                return false;
            TestAddress other = (TestAddress) obj;
            if (this.text == null) {
                if (other.text != null) {
                    return false;
                }
            } else if (!this.text.equals(other.text)) {
                return false;
            }
            return true;
        }

        @Override
        public int hashCode() {
            int result = super.hashCode();
            final int prime = 31;
            result = prime * result + ((this.text == null) ? 0 : this.text.hashCode());
            return result;
        }
    }

    private class TestMessagingStub implements IMessagingStub {
        private TestAddress address;

        public TestMessagingStub(TestAddress address) {
            this.address = address;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj)
                return true;
            if (obj == null)
                return false;
            if (getClass() != obj.getClass())
                return false;
            if (!super.equals(obj))
                return false;
            TestMessagingStub other = (TestMessagingStub) obj;
            if (this.address == null) {
                if (other.address != null) {
                    return false;
                }
            } else if (!this.address.equals(other.address)) {
                return false;
            }
            return true;
        }

        @Override
        public int hashCode() {
            int result = super.hashCode();
            final int prime = 31;
            result = prime * result + ((this.address == null) ? 0 : this.address.hashCode());
            return result;
        }

        public void transmit(ImmutableMessage immutableMessage,
                             SuccessAction successAction,
                             FailureAction failureAction) {
            // do nothing
        }
    }

    private class TestMiddleWareMessagingStubFactory
            extends AbstractMiddlewareMessagingStubFactory<TestMessagingStub, TestAddress> {
        protected TestMessagingStub createInternal(TestAddress address) {
            return new TestMessagingStub(address);
        }
    }

    private TestMiddleWareMessagingStubFactory subject;

    @Before
    public void setup() {
        subject = new TestMiddleWareMessagingStubFactory();
    }

    @Test
    public void cacheTest() {
        ArrayList<TestMessagingStub> messagingStubs = new ArrayList<>();
        int maxSize = subject.getMaxCacheSize();
        int overSize = 1000;

        assertTrue(maxSize > overSize);
        assertEquals(0, subject.getCacheSize());

        // The test verifies that the cache can hold maxSize entries
        // and eldest entries are evicted first if the maxSize is exceeded.
        // As long as entries are in the cache, the same objects are retrieved.

        // create maxSize + overSize stubs;
        // the first overSize stubs should get evicted when the last overSize stubs are inserted
        for (int i = 0; i < maxSize + overSize; i++) {
            TestMessagingStub messagingStub = (TestMessagingStub) subject.create(new TestAddress(Integer.toString(i)));
            assertNotNull(messagingStub);
            messagingStubs.add(messagingStub);
        }

        assertEquals(maxSize, subject.getCacheSize());

        // retrieve the last maxSize stubs again from cache, expect them to be unchanged (same objects)
        for (int i = overSize; i < maxSize + overSize; i++) {
            TestMessagingStub messagingStub = (TestMessagingStub) subject.create(new TestAddress(Integer.toString(i)));
            assertNotNull(messagingStub);
            assertSame(messagingStubs.get(i), (TestMessagingStub) messagingStub);
        }

        // retrieve the first overSize stubs, since they should have got evicted, new ones should be returned
        for (int i = 0; i < overSize; i++) {
            TestMessagingStub messagingStub = (TestMessagingStub) subject.create(new TestAddress(Integer.toString(i)));
            assertNotNull(messagingStub);
            assertNotSame(messagingStubs.get(i), (TestMessagingStub) messagingStub);
        }

        assertEquals(maxSize, subject.getCacheSize());

        subject.clearCache();
        assertEquals(0, subject.getCacheSize());
    }
}
