package io.joynr.dispatching.subscription;

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

import static org.junit.Assert.assertEquals;

import io.joynr.exceptions.JoynrIllegalStateException;
import org.junit.Test;

public class MulticastIdUtilTest {

    @Test
    public void testCreateSimpleId() {
        assertEquals("id/name", MulticastIdUtil.createMulticastId("id", "name"));
    }

    @Test
    public void testCreateIdWithPartitions() {
        assertEquals("id/name/one/two", MulticastIdUtil.createMulticastId("id", "name", "one", "two"));
    }

    @Test
    public void testCreateIdWithSinglePositionWildcard() {
        assertEquals("id/name/one/+/three", MulticastIdUtil.createMulticastId("id", "name", "one", "+", "three"));
    }

    @Test
    public void testCreateIdWithMultilevelWildcard() {
        assertEquals("id/name/one/two/three/*",
                     MulticastIdUtil.createMulticastId("id", "name", "one", "two", "three", "*"));
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testCreateIdWithInvalidChars() {
        MulticastIdUtil.createMulticastId("id", "name", "one", "_ ./$", "\uD83D\uDE33");
    }

}
