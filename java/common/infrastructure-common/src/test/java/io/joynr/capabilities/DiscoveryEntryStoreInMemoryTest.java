/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.capabilities;

import static org.junit.Assert.assertEquals;

import org.junit.Before;
import org.junit.Test;

import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

public class DiscoveryEntryStoreInMemoryTest {
    private DiscoveryEntryStoreInMemory<DiscoveryEntry> discoveryEntryStore;

    private DiscoveryEntry localEntry;
    private DiscoveryEntry globalEntry;

    private final String localParticipantId = "localId";
    private final String globalParticipantId = "globalId";

    private final ProviderQos localProviderQos = new ProviderQos();
    private final ProviderQos globalProviderQos = new ProviderQos();

    private final long LAST_SEEN_DATE_MS = System.currentTimeMillis();
    private final long EXPIRY_DATE_MS = LAST_SEEN_DATE_MS + 10000;

    @Before
    public void setUp() {
        discoveryEntryStore = new DiscoveryEntryStoreInMemory<DiscoveryEntry>(null);

        localProviderQos.setScope(ProviderScope.LOCAL);
        globalProviderQos.setScope(ProviderScope.GLOBAL);

        localEntry = new DiscoveryEntry(new Version(),
                                        "TEST_DOMAIN",
                                        "",
                                        localParticipantId,
                                        localProviderQos,
                                        LAST_SEEN_DATE_MS,
                                        EXPIRY_DATE_MS,
                                        "Public");
        globalEntry = new DiscoveryEntry(new Version(),
                                         "TEST_DOMAIN",
                                         "",
                                         globalParticipantId,
                                         globalProviderQos,
                                         LAST_SEEN_DATE_MS,
                                         EXPIRY_DATE_MS,
                                         "Public");
    }

    @Test
    public void touchDiscoveryEntriesWithGlobalScope() {
        final long dateDiffMs = 42l;

        DiscoveryEntry expectedLocalDiscoveryEntry = new DiscoveryEntry(localEntry);
        DiscoveryEntry expectedGlobalDiscoveryEntry = new DiscoveryEntry(globalEntry);

        discoveryEntryStore.add(localEntry);
        discoveryEntryStore.add(globalEntry);

        DiscoveryEntry actualGlobalDiscoveryEntry = discoveryEntryStore.lookup(globalParticipantId, Long.MAX_VALUE)
                                                                       .get();
        DiscoveryEntry actualLocalDiscoveryEntry = discoveryEntryStore.lookup(localParticipantId, Long.MAX_VALUE).get();

        assertEquals(expectedLocalDiscoveryEntry, actualLocalDiscoveryEntry);
        assertEquals(expectedGlobalDiscoveryEntry, actualGlobalDiscoveryEntry);

        final long newLastSeenDateMs = LAST_SEEN_DATE_MS + dateDiffMs;
        final long newExpiryDateMs = EXPIRY_DATE_MS + dateDiffMs;
        expectedGlobalDiscoveryEntry.setLastSeenDateMs(newLastSeenDateMs);
        expectedGlobalDiscoveryEntry.setExpiryDateMs(newExpiryDateMs);

        String[] actualParticipantIds = discoveryEntryStore.touchGlobalDiscoveryEntries(newLastSeenDateMs,
                                                                                        newExpiryDateMs);

        actualLocalDiscoveryEntry = discoveryEntryStore.lookup(localParticipantId, Long.MAX_VALUE).get();
        actualGlobalDiscoveryEntry = discoveryEntryStore.lookup(globalParticipantId, Long.MAX_VALUE).get();

        assertEquals(expectedLocalDiscoveryEntry, actualLocalDiscoveryEntry);
        assertEquals(expectedGlobalDiscoveryEntry, actualGlobalDiscoveryEntry);

        assertEquals(1, actualParticipantIds.length);
        assertEquals(globalParticipantId, actualParticipantIds[0]);
    }

    @Test
    public void touchDiscoveryEntries() {
        final long dateDiffMs = 42l;

        DiscoveryEntry expectedLocalDiscoveryEntry = new DiscoveryEntry(localEntry);
        DiscoveryEntry expectedGlobalDiscoveryEntry = new DiscoveryEntry(globalEntry);

        String globalParticipantId2 = "globalId2";
        DiscoveryEntry globalEntry2 = new DiscoveryEntry(globalEntry);
        globalEntry2.setParticipantId(globalParticipantId2);
        DiscoveryEntry expectedGlobalDiscoveryEntry2 = new DiscoveryEntry(globalEntry2);

        String[] participantIds = new String[]{ localParticipantId, globalParticipantId };

        discoveryEntryStore.add(localEntry);
        discoveryEntryStore.add(globalEntry);
        discoveryEntryStore.add(globalEntry2);

        final long newLastSeenDateMs = LAST_SEEN_DATE_MS + dateDiffMs;
        final long newExpiryDateMs = EXPIRY_DATE_MS + dateDiffMs;

        expectedLocalDiscoveryEntry.setLastSeenDateMs(newLastSeenDateMs);
        expectedLocalDiscoveryEntry.setExpiryDateMs(newExpiryDateMs);
        expectedGlobalDiscoveryEntry.setLastSeenDateMs(newLastSeenDateMs);
        expectedGlobalDiscoveryEntry.setExpiryDateMs(newExpiryDateMs);

        discoveryEntryStore.touchDiscoveryEntries(participantIds, newLastSeenDateMs, newExpiryDateMs);

        // Perform lookups
        DiscoveryEntry actualLocalDiscoveryEntry = discoveryEntryStore.lookup(localParticipantId, Long.MAX_VALUE).get();
        DiscoveryEntry actualGlobalDiscoveryEntry = discoveryEntryStore.lookup(globalParticipantId, Long.MAX_VALUE)
                                                                       .get();
        DiscoveryEntry actualGlobalDiscoveryEntry2 = discoveryEntryStore.lookup(globalParticipantId2, Long.MAX_VALUE)
                                                                        .get();

        // compare actual and expected values
        assertEquals(expectedLocalDiscoveryEntry, actualLocalDiscoveryEntry);
        assertEquals(expectedGlobalDiscoveryEntry, actualGlobalDiscoveryEntry);
        assertEquals(expectedGlobalDiscoveryEntry2, actualGlobalDiscoveryEntry2);
    }
}
