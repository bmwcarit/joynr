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
package io.joynr.messaging.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.when;

import java.util.Set;
import java.util.regex.Pattern;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.messaging.util.MulticastWildcardRegexFactory;

@RunWith(MockitoJUnitRunner.class)
public class InMemoryMulticastReceiverRegistryTest {

    private InMemoryMulticastReceiverRegistry subject;

    @Mock
    private MulticastWildcardRegexFactory multicastWildcardRegexFactory;

    @Before
    public void setup() {
        subject = new InMemoryMulticastReceiverRegistry(multicastWildcardRegexFactory);
    }

    @Test
    public void testAddAndRetrieveAndRemove() {
        String multicastId = "multicastId";
        when(multicastWildcardRegexFactory.createIdPattern(multicastId)).thenReturn(Pattern.compile(multicastId));
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

    @Test
    public void testRetrieveWithSinglePartitionWildcard() {
        String wildcardId = "one/+/three";
        when(multicastWildcardRegexFactory.createIdPattern(wildcardId)).thenReturn(Pattern.compile("one/[^/]+/three"));

        String partitionedId = "one/two/three";
        when(multicastWildcardRegexFactory.createIdPattern(partitionedId)).thenReturn(Pattern.compile(partitionedId));

        String nonMatchingId = "four/five/six";
        when(multicastWildcardRegexFactory.createIdPattern(nonMatchingId)).thenReturn(Pattern.compile(nonMatchingId));

        String participantIdOne = "123";
        String participantIdTwo = "456";
        String participantIdThree = "789";

        subject.registerMulticastReceiver(wildcardId, participantIdOne);
        subject.registerMulticastReceiver(partitionedId, participantIdTwo);
        subject.registerMulticastReceiver(nonMatchingId, participantIdThree);

        Set<String> result = subject.getReceivers(partitionedId);

        assertNotNull(result);
        assertEquals(2, result.size());
        assertTrue(result.contains(participantIdOne));
        assertTrue(result.contains(participantIdTwo));
        assertFalse(result.contains(participantIdThree));
    }

    @Test
    public void testRetrieveWithMultiLevelWildcard() {
        String wildcardId = "one/two/*";
        when(multicastWildcardRegexFactory.createIdPattern(wildcardId)).thenReturn(Pattern.compile("one/two/.*"));

        String partitionedId = "one/two/three/four";
        String participantId = "123";

        subject.registerMulticastReceiver(wildcardId, participantId);

        Set<String> result = subject.getReceivers(partitionedId);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(participantId, result.iterator().next());
    }

    @Test
    public void testAddAndRemoveWithMultipleParticipantIds() {
        // use real MulticastWildcardRegexFactory which returns
        // different Pattern objects even when called with same regex
        subject = new InMemoryMulticastReceiverRegistry(new MulticastWildcardRegexFactory());
        String multicastId = "multicastId";
        String participantId1 = "participantId1";
        String participantId2 = "participantId2";
        String participantId3 = "participantId3";

        subject.registerMulticastReceiver(multicastId, participantId1);
        subject.registerMulticastReceiver(multicastId, participantId2);
        subject.registerMulticastReceiver(multicastId, participantId3);

        Set<String> result = subject.getReceivers(multicastId);
        assertNotNull(result);
        assertEquals(3, result.size());

        subject.unregisterMulticastReceiver(multicastId, participantId2);
        result = subject.getReceivers(multicastId);
        assertNotNull(result);
        assertEquals(2, result.size());

        subject.unregisterMulticastReceiver(multicastId, participantId3);
        result = subject.getReceivers(multicastId);
        assertNotNull(result);
        assertEquals(1, result.size());

        subject.unregisterMulticastReceiver(multicastId, participantId1);
        result = subject.getReceivers(multicastId);
        assertNotNull(result);
        assertEquals(0, result.size());
    }
}
