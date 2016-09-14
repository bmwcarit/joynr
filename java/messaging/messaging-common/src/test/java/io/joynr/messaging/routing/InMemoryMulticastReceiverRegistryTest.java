package io.joynr.messaging.routing;

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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import java.util.Set;

import org.junit.Before;
import org.junit.Test;

public class InMemoryMulticastReceiverRegistryTest {

    private InMemoryMulticastReceiverRegistry subject;

    @Before
    public void setup() {
        subject = new InMemoryMulticastReceiverRegistry();
    }

    @Test
    public void testAddAndRetrieveAndRemove() {
        String multicastId = "multicastId";
        String participantId = "participantId";
        subject.registerMulticastReceiver(multicastId, participantId);
        Set<String> result = subject.getReceivers(multicastId);
        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(participantId, result.iterator().next());
        subject.unregisterMulticastReceiver(multicastId, participantId);
        result = subject.getReceivers(multicastId);
        assertNotNull(result);
        assertEquals(0, result.size());
    }

}
