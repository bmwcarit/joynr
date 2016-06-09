package io.joynr.arbitration;

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
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.util.HashSet;
import java.util.Set;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.common.collect.Sets;

import joynr.types.DiscoveryEntry;
import joynr.types.Version;

/**
 * Unit tests for {@link DiscoveryEntryVersionFilter}.
 */
@RunWith(MockitoJUnitRunner.class)
public class DiscoveryEntryVersionFilterTest {

    @Mock
    private VersionCompatibilityChecker versionCompatibilityChecker;

    private DiscoveryEntryVersionFilter subject;

    @Before
    public void setup() {
        subject = new DiscoveryEntryVersionFilter(versionCompatibilityChecker);
    }

    @Test
    public void testEmptySetUnchanged() {
        Version callerVersion = new Version(0, 0);
        Set<DiscoveryEntry> discoveryEntries = new HashSet<>();

        Set<DiscoveryEntry> result = subject.filter(callerVersion, discoveryEntries);

        assertNotNull(result);
        assertEquals(discoveryEntries, result);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testCallerVersionNull() {
        subject.filter(null, new HashSet<DiscoveryEntry>());
    }

    @Test(expected = IllegalArgumentException.class)
    public void testDiscoveryEntriesNull() {
        subject.filter(new Version(1, 1), null);
    }

    @Test
    public void testIncompatibleVersionFilteredOut() {
        Version callerVersion = new Version(1, 0);
        DiscoveryEntry discoveryEntry = mock(DiscoveryEntry.class);
        Version providerVersion = new Version(2, 0);
        when(discoveryEntry.getProviderVersion()).thenReturn(providerVersion);
        Set<DiscoveryEntry> discoveryEntries = Sets.newHashSet(discoveryEntry);

        Set<DiscoveryEntry> result = subject.filter(callerVersion, discoveryEntries);

        assertNotNull(result);
        assertTrue(result.isEmpty());
    }

    @Test
    public void testCompatibleVersionLeftIn() {
        Version callerVersion = new Version(1, 0);
        DiscoveryEntry discoveryEntry = mock(DiscoveryEntry.class);
        Version providerVersion = new Version(1, 0);
        when(discoveryEntry.getProviderVersion()).thenReturn(providerVersion);
        Set<DiscoveryEntry> discoveryEntries = Sets.newHashSet(discoveryEntry);
        when(versionCompatibilityChecker.check(eq(callerVersion), eq(providerVersion))).thenReturn(true);

        Set<DiscoveryEntry> result = subject.filter(callerVersion, discoveryEntries);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(discoveryEntries, result);
    }

    @Test
    public void testOnlyIncompatibleVersionsFilteredOut() {
        Version callerVersion = new Version(0, 0);
        DiscoveryEntry compatibleEntry = mock(DiscoveryEntry.class);
        Version compatibleVersion = new Version(0, 1);
        when(compatibleEntry.getProviderVersion()).thenReturn(compatibleVersion);
        when(versionCompatibilityChecker.check(eq(callerVersion), eq(compatibleVersion))).thenReturn(true);
        DiscoveryEntry incompatibleEntry = mock(DiscoveryEntry.class);
        Version incompatibleVersion = new Version(2, 2);
        when(incompatibleEntry.getProviderVersion()).thenReturn(incompatibleVersion);
        when(versionCompatibilityChecker.check(callerVersion, incompatibleVersion)).thenReturn(false);
        Set<DiscoveryEntry> discoveryEntries = Sets.newHashSet(compatibleEntry, incompatibleEntry);

        Set<DiscoveryEntry> result = subject.filter(callerVersion, discoveryEntries);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(compatibleEntry, result.iterator().next());
    }

}
