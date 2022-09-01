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
package io.joynr.messaging.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

import org.junit.Before;
import org.junit.Test;

import io.joynr.exceptions.JoynrIllegalStateException;

public class MulticastWildcardRegexFactoryTest {

    private MulticastWildcardRegexFactory subject;

    @Before
    public void setup() {
        subject = new MulticastWildcardRegexFactory();
    }

    @Test
    public void testReplaceLeadingSingleLevelWildcard() {
        Pattern pattern = subject.createIdPattern("+/one/two/three");
        assertNotNull(pattern);
        assertEquals("[^/]+/one/two/three", pattern.pattern());
        assertTrue(pattern.matcher("anything/one/two/three").matches());
        assertTrue(pattern.matcher("1/one/two/three").matches());
        assertTrue(pattern.matcher("_hello_!/one/two/three").matches());
        assertFalse(pattern.matcher("one/two/three").matches());
        assertFalse(pattern.matcher("one/any/two/three").matches());
        assertFalse(pattern.matcher("/one/two/three").matches());
        assertFalse(pattern.matcher("five/six/one/two/three").matches());
    }

    @Test
    public void testOnlySingleLevelWildcard() {
        Pattern pattern = subject.createIdPattern("+");
        assertNotNull(pattern);
        assertEquals("[^/]+", pattern.pattern());
        assertTrue(pattern.matcher("onelevelhere").matches());
        assertFalse(pattern.matcher("one/two").matches());
        assertFalse(pattern.matcher("/one").matches());
        assertFalse(pattern.matcher("one/").matches());
        assertFalse(pattern.matcher("/one/two").matches());
    }

    @Test
    public void testSingleWildcardInMiddle() {
        Pattern pattern = subject.createIdPattern("one/+/three");
        assertNotNull(pattern);
        assertEquals("one/[^/]+/three", pattern.pattern());
        assertTrue(pattern.matcher("one/anything/three").matches());
        assertTrue(pattern.matcher("one/1/three").matches());
        assertTrue(pattern.matcher("one/_here_/three").matches());
        assertFalse(pattern.matcher("one/two/four/three").matches());
        assertFalse(pattern.matcher("one/three").matches());
    }

    @Test
    public void testSingleWildcardAtEnd() {
        Pattern pattern = subject.createIdPattern("one/two/+");
        assertNotNull(pattern);
        assertEquals("one/two/[^/]+$", pattern.pattern());
        assertTrue(pattern.matcher("one/two/anything").matches());
        assertTrue(pattern.matcher("one/two/3").matches());
        assertTrue(pattern.matcher("one/two/and another partition").matches());
        assertFalse(pattern.matcher("one/two/three/four").matches());
        assertFalse(pattern.matcher("one/two").matches());
    }

    @Test
    public void testMultiWildcardAtEnd() {
        Pattern pattern = subject.createIdPattern("one/two/*");
        assertNotNull(pattern);
        assertEquals("one/two(/.*)?$", pattern.pattern());
        assertTrue(pattern.matcher("one/two/anything").matches());
        assertTrue(pattern.matcher("one/two/3").matches());
        assertTrue(pattern.matcher("one/two/and another partition").matches());
        assertTrue(pattern.matcher("one/two/three/four").matches());
        assertTrue(pattern.matcher("one/two").matches());
        assertFalse(pattern.matcher("one/twothree").matches());
        assertFalse(pattern.matcher("").matches());
    }

    @Test
    public void testOnlyMultiWildcard() {
        Pattern pattern = subject.createIdPattern("*");
        assertNotNull(pattern);
        assertEquals(".+", pattern.pattern());
        assertTrue(pattern.matcher("one").matches());
        assertTrue(pattern.matcher("one/two").matches());
        assertTrue(pattern.matcher("one/two/three").matches());
        assertFalse(pattern.matcher("").matches());
    }

    @Test
    public void testPlusAsPartOfPartition() {
        List<String> faultyPlusPatternList = new ArrayList<String>();
        faultyPlusPatternList.add("+onetwo/threefour/fivesix");
        faultyPlusPatternList.add("one+two/threefour/fivesix");
        faultyPlusPatternList.add("onetwo+/threefour/fivesix");
        faultyPlusPatternList.add("onetwo/+threefour/fivesix");
        faultyPlusPatternList.add("onetwo/three+four/fivesix");
        faultyPlusPatternList.add("onetwo/threefour+/fivesix");
        faultyPlusPatternList.add("onetwo/threefour/+fivesix");
        faultyPlusPatternList.add("onetwo/threefour/five+six");
        faultyPlusPatternList.add("onetwo/threefour/fivesix+");
        faultyPlusPatternList.add("++/threefour/fivesix");
        faultyPlusPatternList.add("onetwo/++/fivesix");
        faultyPlusPatternList.add("onetwo/threefour/++");
        for (String faultyPlusPattern : faultyPlusPatternList) {
            try {
                subject.createIdPattern(faultyPlusPattern);
                fail("Pattern " + faultyPlusPattern + " should not be accepted");
            } catch (JoynrIllegalStateException e) {

            }
        }
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testAsteriskInTheMiddle() {
        subject.createIdPattern("one/*/three");
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testAsteriskInAPartition() {
        subject.createIdPattern("one/two*and a half/three");
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testAsteriskAtTheBeginning() {
        subject.createIdPattern("*one/two/three");
    }

}
