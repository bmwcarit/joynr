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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

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

    private final int maxiumNumberOfNonStickyEntries = 1000;

    @Before
    public void setUp() {
        discoveryEntryStore = new DiscoveryEntryStoreInMemory<DiscoveryEntry>(null, maxiumNumberOfNonStickyEntries);

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

    private DiscoveryEntry getClonedDiscoveryEntry(int index, boolean isSticky) {
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(globalEntry);
        if (isSticky) {
            discoveryEntry.setExpiryDateMs(Long.MAX_VALUE);
            discoveryEntry.setParticipantId("stickyId" + index);
        } else {
            discoveryEntry.setParticipantId("nonStickyId" + index);
        }
        return discoveryEntry;
    }

    private void createStoreWithDiscoveryEntries(int storeLimit, int maxCount) {
        assertTrue(storeLimit < maxCount);
        assertTrue(globalEntry.getExpiryDateMs() != Long.MAX_VALUE);

        discoveryEntryStore = new DiscoveryEntryStoreInMemory<DiscoveryEntry>(null, storeLimit);

        for (int i = 0; i < storeLimit; i++) {
            final boolean isSticky = true;
            discoveryEntryStore.add(getClonedDiscoveryEntry(i, isSticky));
        }

        for (int i = 0; i < maxCount; i++) {
            final boolean isSticky = false;
            discoveryEntryStore.add(getClonedDiscoveryEntry(i, isSticky));
        }

        for (int i = storeLimit; i < maxCount; i++) {
            final boolean isSticky = true;
            discoveryEntryStore.add(getClonedDiscoveryEntry(i, isSticky));
        }
    }

    @Test
    public void discoveryEntryStoreIsLimitedIfLimitationIsEnabled() {
        final int storeLimit = 5;
        final int maxCount = 10;

        createStoreWithDiscoveryEntries(storeLimit, maxCount);

        // nonSticky entries 0 - (storeLimit-1) should have been purged when inserting entries storeLimit - (maxCount-1)
        for (int i = 0; i < storeLimit; i++) {
            String participantId = "nonStickyId" + i;
            assertFalse(discoveryEntryStore.remove(participantId));
        }

        // nonSticky entries storeLimit - (maxCount-1) should be still available in store
        for (int i = storeLimit; i < maxCount; i++) {
            String participantId = "nonStickyId" + i;
            assertTrue(discoveryEntryStore.remove(participantId));
        }

        // all of the sticky entries should still be available in store
        for (int i = 0; i < maxCount; i++) {
            String participantId = "stickyId" + i;
            assertTrue(discoveryEntryStore.remove(participantId));
        }
    }

    @Test
    public void discoveryEntryStoreIsNotLimitedIfLimitationIsDisabled() {
        final int storeLimit = 0;
        final int maxCount = 10;

        createStoreWithDiscoveryEntries(storeLimit, maxCount);

        // nonSticky entries 0 - (maxCount - 1) should be still available in store
        for (int i = 0; i < maxCount; i++) {
            String participantId = "nonStickyId" + i;
            assertTrue(discoveryEntryStore.remove(participantId));
        }

        // all of the sticky entries should still be available in store
        for (int i = 0; i < maxCount; i++) {
            String participantId = "stickyId" + i;
            assertTrue(discoveryEntryStore.remove(participantId));
        }
    }
}
