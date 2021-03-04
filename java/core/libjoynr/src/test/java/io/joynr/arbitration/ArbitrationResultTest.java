/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import joynr.types.DiscoveryEntryWithMetaInfo;

public class ArbitrationResultTest {

    private ArbitrationResult arbitrationResult;

    @Before
    public void setUp() {
        arbitrationResult = new ArbitrationResult();
    }

    @Test
    public void initializeWithNonNullParameters() {
        DiscoveryEntryWithMetaInfo selectedDiscoverEntry = new DiscoveryEntryWithMetaInfo();
        DiscoveryEntryWithMetaInfo otherDiscoveryEntry1 = new DiscoveryEntryWithMetaInfo();
        DiscoveryEntryWithMetaInfo otherDiscoveryEntry2 = new DiscoveryEntryWithMetaInfo();

        Set<DiscoveryEntryWithMetaInfo> expectedSelectedDiscoveryEntries = new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(selectedDiscoverEntry));
        Set<DiscoveryEntryWithMetaInfo> expectedOtherDiscoveryEntries = new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(otherDiscoveryEntry1,
                                                                                                                              otherDiscoveryEntry2));
        arbitrationResult = new ArbitrationResult(expectedSelectedDiscoveryEntries, expectedOtherDiscoveryEntries);

        assertEquals(expectedSelectedDiscoveryEntries, arbitrationResult.getDiscoveryEntries());
        assertEquals(expectedOtherDiscoveryEntries, arbitrationResult.getOtherDiscoveryEntries());
    }

    @Test
    public void initializeWithNullParameters() {
        arbitrationResult = new ArbitrationResult(null, null);
        assertTrue(arbitrationResult.getDiscoveryEntries().isEmpty());
        assertTrue(arbitrationResult.getOtherDiscoveryEntries().isEmpty());
    }

    @Test
    public void setAndGetDiscoveryEntries() {
        Set<DiscoveryEntryWithMetaInfo> expectedSelectedDiscoveryEntries = new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(new DiscoveryEntryWithMetaInfo()));
        arbitrationResult.setDiscoveryEntries(expectedSelectedDiscoveryEntries);
        assertEquals(expectedSelectedDiscoveryEntries, arbitrationResult.getDiscoveryEntries());
    }

    @Test
    public void setDiscoveryEntriesWithNullParameters() {
        arbitrationResult.setDiscoveryEntries(null);
        assertTrue(arbitrationResult.getDiscoveryEntries().isEmpty());
    }

    @Test
    public void setAndGetOtherDiscoveryEntries() {
        Set<DiscoveryEntryWithMetaInfo> expectedOtherDiscoveryEntries = new HashSet<DiscoveryEntryWithMetaInfo>(Arrays.asList(new DiscoveryEntryWithMetaInfo()));
        arbitrationResult.setDiscoveryEntries(expectedOtherDiscoveryEntries);
        assertEquals(expectedOtherDiscoveryEntries, arbitrationResult.getDiscoveryEntries());
    }

    @Test
    public void setOtherDiscoveryEntriesWithNullParameters() {
        arbitrationResult.setOtherDiscoveryEntries(null);
        assertTrue(arbitrationResult.getOtherDiscoveryEntries().isEmpty());
    }
}
