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
package io.joynr.arbitration;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import joynr.types.DiscoveryEntryWithMetaInfo;
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
        Set<DiscoveryEntryWithMetaInfo> discoveryEntries = new HashSet<>();

        Set<DiscoveryEntryWithMetaInfo> result = subject.filter(callerVersion, discoveryEntries, null);

        assertNotNull(result);
        assertEquals(discoveryEntries, result);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testCallerVersionNull() {
        subject.filter(null, new HashSet<DiscoveryEntryWithMetaInfo>(), null);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testDiscoveryEntriesNull() {
        subject.filter(new Version(1, 1), null, null);
    }

    @Test
    public void testIncompatibleVersionFilteredOut() {
        Version callerVersion = new Version(1, 0);
        DiscoveryEntryWithMetaInfo discoveryEntry = mock(DiscoveryEntryWithMetaInfo.class);
        Version providerVersion = new Version(2, 0);
        when(discoveryEntry.getProviderVersion()).thenReturn(providerVersion);
        Set<DiscoveryEntryWithMetaInfo> discoveryEntries = new HashSet<>(Arrays.asList(discoveryEntry));

        Set<DiscoveryEntryWithMetaInfo> result = subject.filter(callerVersion, discoveryEntries, null);

        assertNotNull(result);
        assertTrue(result.isEmpty());
    }

    @Test
    public void testCompatibleVersionLeftIn() {
        Version callerVersion = new Version(1, 0);
        DiscoveryEntryWithMetaInfo discoveryEntry = mock(DiscoveryEntryWithMetaInfo.class);
        Version providerVersion = new Version(1, 0);
        when(discoveryEntry.getProviderVersion()).thenReturn(providerVersion);
        Set<DiscoveryEntryWithMetaInfo> discoveryEntries = new HashSet<>(Arrays.asList(discoveryEntry));
        when(versionCompatibilityChecker.check(eq(callerVersion), eq(providerVersion))).thenReturn(true);

        Set<DiscoveryEntryWithMetaInfo> result = subject.filter(callerVersion, discoveryEntries, null);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(discoveryEntries, result);
    }

    @Test
    public void testOnlyIncompatibleVersionsFilteredOut() {
        Version callerVersion = new Version(0, 0);
        DiscoveryEntryWithMetaInfo compatibleEntry = mock(DiscoveryEntryWithMetaInfo.class);
        Version compatibleVersion = new Version(0, 1);
        when(compatibleEntry.getProviderVersion()).thenReturn(compatibleVersion);
        when(versionCompatibilityChecker.check(eq(callerVersion), eq(compatibleVersion))).thenReturn(true);
        DiscoveryEntryWithMetaInfo incompatibleEntry = mock(DiscoveryEntryWithMetaInfo.class);
        Version incompatibleVersion = new Version(2, 2);
        when(incompatibleEntry.getProviderVersion()).thenReturn(incompatibleVersion);
        when(versionCompatibilityChecker.check(callerVersion, incompatibleVersion)).thenReturn(false);
        Set<DiscoveryEntryWithMetaInfo> discoveryEntries = new HashSet<>(Arrays.asList(compatibleEntry,
                                                                                       incompatibleEntry));

        Set<DiscoveryEntryWithMetaInfo> result = subject.filter(callerVersion, discoveryEntries, null);

        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(compatibleEntry, result.iterator().next());
    }

    @Test
    public void testFilteredOutVersionsCollected() {
        Version callerVersion = new Version(1, 0);
        DiscoveryEntryWithMetaInfo discoveryEntry = mock(DiscoveryEntryWithMetaInfo.class);
        Version providerVersion = new Version(2, 0);
        when(discoveryEntry.getProviderVersion()).thenReturn(providerVersion);
        when(discoveryEntry.getDomain()).thenReturn("domain");
        DiscoveryEntryWithMetaInfo otherDiscoveryEntry = mock(DiscoveryEntryWithMetaInfo.class);
        Version otherProviderVersion = new Version(4, 10);
        when(otherDiscoveryEntry.getProviderVersion()).thenReturn(otherProviderVersion);
        when(otherDiscoveryEntry.getDomain()).thenReturn("domain");
        Set<DiscoveryEntryWithMetaInfo> discoveryEntries = new HashSet<>(Arrays.asList(discoveryEntry,
                                                                                       otherDiscoveryEntry));

        Map<String, Set<Version>> filteredOutVersions = new HashMap<>();

        Set<DiscoveryEntryWithMetaInfo> result = subject.filter(callerVersion, discoveryEntries, filteredOutVersions);

        assertNotNull(result);
        assertTrue(result.isEmpty());
        assertFalse(filteredOutVersions.isEmpty());
        assertTrue(filteredOutVersions.containsKey("domain"));
        Set<Version> versions = filteredOutVersions.get("domain");
        assertTrue(versions.containsAll(new HashSet<Version>(Arrays.asList(providerVersion, otherProviderVersion))));
    }

    @Test
    public void testVersionsCollectedByDomain() {
        Version callerVersion = new Version(1, 0);
        DiscoveryEntryWithMetaInfo discoveryEntry = mock(DiscoveryEntryWithMetaInfo.class);
        Version providerVersion = new Version(2, 0);
        when(discoveryEntry.getProviderVersion()).thenReturn(providerVersion);
        when(discoveryEntry.getDomain()).thenReturn("domain-1");

        DiscoveryEntryWithMetaInfo otherDiscoveryEntry = mock(DiscoveryEntryWithMetaInfo.class);
        Version otherProviderVersion = new Version(4, 10);
        when(otherDiscoveryEntry.getProviderVersion()).thenReturn(otherProviderVersion);
        when(otherDiscoveryEntry.getDomain()).thenReturn("domain-2");
        Set<DiscoveryEntryWithMetaInfo> discoveryEntries = new HashSet<>(Arrays.asList(discoveryEntry,
                                                                                       otherDiscoveryEntry));

        Map<String, Set<Version>> filteredOutVersions = new HashMap<>();

        Set<DiscoveryEntryWithMetaInfo> result = subject.filter(callerVersion, discoveryEntries, filteredOutVersions);

        assertNotNull(result);
        assertTrue(result.isEmpty());
        assertFalse(filteredOutVersions.isEmpty());

        assertTrue(filteredOutVersions.containsKey("domain-1"));
        Set<Version> versions = filteredOutVersions.get("domain-1");
        assertNotNull(versions);
        assertTrue(versions.containsAll(new HashSet<Version>(Arrays.asList(providerVersion))));

        assertTrue(filteredOutVersions.containsKey("domain-2"));
        versions = filteredOutVersions.get("domain-2");
        assertNotNull(versions);
        assertTrue(versions.containsAll(new HashSet<Version>(Arrays.asList(otherProviderVersion))));
    }
}
