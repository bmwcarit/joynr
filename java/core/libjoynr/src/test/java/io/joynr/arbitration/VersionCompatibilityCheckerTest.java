package io.joynr.arbitration;

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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;

import joynr.types.Version;

/**
 * Unit tests for {@link VersionCompatibilityChecker}.
 */
public class VersionCompatibilityCheckerTest {

    private VersionCompatibilityChecker subject;

    @Before
    public void setup() {
        subject = new VersionCompatibilityChecker();
    }

    @Test
    public void testEqualVersionsAreCompatible() {
        Version caller = new Version(1, 1);
        Version provider = new Version(1, 1);

        boolean result = subject.check(caller, provider);

        assertTrue(result);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testCallerIsNull() {
        Version caller = null;
        Version provider = new Version(0, 0);

        subject.check(caller, provider);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testProviderIsNull() {
        Version caller = new Version(0, 0);
        Version provider = null;

        subject.check(caller, provider);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testBothNull() {
        subject.check(null, null);
    }

    @Test
    public void testProviderWithHigherMinorCompatible() {
        Version caller = new Version(1, 1);
        Version provider = new Version(1, 2);

        boolean result = subject.check(caller, provider);

        assertTrue(result);
    }

    @Test
    public void testDifferentMajorVersionsIncompatible() {
        Version caller = new Version(2, 0);
        Version provider = new Version(3, 0);

        boolean result = subject.check(caller, provider);

        assertFalse(result);
    }

    @Test
    public void testCallerWithHigherMinorVersionIncompatible() {
        Version caller = new Version(1, 2);
        Version provider = new Version(1, 0);

        boolean result = subject.check(caller, provider);

        assertFalse(result);
    }

}
