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
import static org.junit.Assert.fail;

import java.lang.reflect.Field;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrCommunicationException;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

public class DiscoveryEntryStoreInMemoryTest {
    private static final Logger logger = LoggerFactory.getLogger(DiscoveryEntryStoreInMemoryTest.class);

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

    @Rule
    public ExpectedException thrown = ExpectedException.none();

    @Before
    public void setUp() {
        discoveryEntryStore = new DiscoveryEntryStoreInMemory<DiscoveryEntry>(maxiumNumberOfNonStickyEntries);

        localProviderQos.setScope(ProviderScope.LOCAL);
        globalProviderQos.setScope(ProviderScope.GLOBAL);

        localEntry = new DiscoveryEntry(new Version(),
                                        "TEST_DOMAIN",
                                        "interfaceName",
                                        localParticipantId,
                                        localProviderQos,
                                        LAST_SEEN_DATE_MS,
                                        EXPIRY_DATE_MS,
                                        "Public");
        globalEntry = new DiscoveryEntry(new Version(),
                                         "TEST_DOMAIN",
                                         "interfaceName",
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
        expectedLocalDiscoveryEntry.setLastSeenDateMs(newLastSeenDateMs);
        expectedLocalDiscoveryEntry.setExpiryDateMs(newExpiryDateMs);
        expectedGlobalDiscoveryEntry.setLastSeenDateMs(newLastSeenDateMs);
        expectedGlobalDiscoveryEntry.setExpiryDateMs(newExpiryDateMs);

        String[] actualParticipantIds = discoveryEntryStore.touchDiscoveryEntries(newLastSeenDateMs, newExpiryDateMs);

        actualLocalDiscoveryEntry = discoveryEntryStore.lookup(localParticipantId, Long.MAX_VALUE).get();
        actualGlobalDiscoveryEntry = discoveryEntryStore.lookup(globalParticipantId, Long.MAX_VALUE).get();

        assertEquals(expectedLocalDiscoveryEntry, actualLocalDiscoveryEntry);
        assertEquals(expectedGlobalDiscoveryEntry, actualGlobalDiscoveryEntry);

        assertEquals(1, actualParticipantIds.length);
        assertEquals(globalParticipantId, actualParticipantIds[0]);
    }

    @Test
    public void lookupGlobalEntries() {
        String[] domains = { "TEST_DOMAIN" };
        final String interfaceName = "interfaceName";
        Set<DiscoveryEntry> expectedEntries = new HashSet<DiscoveryEntry>();
        expectedEntries.add(globalEntry);
        discoveryEntryStore.add(localEntry);
        discoveryEntryStore.add(globalEntry);
        Collection<DiscoveryEntry> actualEntries = discoveryEntryStore.lookupGlobalEntries(domains, interfaceName);
        assertEquals(1, actualEntries.size());
        assertEquals(expectedEntries, actualEntries);
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

    @Test
    public void addWithNullThrows() {
        thrown.expect(JoynrCommunicationException.class);
        discoveryEntryStore.add((DiscoveryEntry) null);
    }

    @Test
    public void addWithMissingDomainThrows() {
        thrown.expect(JoynrCommunicationException.class);
        DiscoveryEntry expectedLocalDiscoveryEntry = new DiscoveryEntry(localEntry.getProviderVersion(),
                                                                        null, // domain
                                                                        localEntry.getInterfaceName(),
                                                                        localEntry.getParticipantId(),
                                                                        localEntry.getQos(),
                                                                        localEntry.getLastSeenDateMs(),
                                                                        localEntry.getExpiryDateMs(),
                                                                        localEntry.getPublicKeyId());
        discoveryEntryStore.add(expectedLocalDiscoveryEntry);
    }

    @Test
    public void addWithMissingInterfaceThrows() {
        thrown.expect(JoynrCommunicationException.class);
        DiscoveryEntry expectedLocalDiscoveryEntry = new DiscoveryEntry(localEntry.getProviderVersion(),
                                                                        localEntry.getDomain(),
                                                                        null, // interfaceName
                                                                        localEntry.getParticipantId(),
                                                                        localEntry.getQos(),
                                                                        localEntry.getLastSeenDateMs(),
                                                                        localEntry.getExpiryDateMs(),
                                                                        localEntry.getPublicKeyId());
        discoveryEntryStore.add(expectedLocalDiscoveryEntry);
    }

    @Test
    public void addWithMissingParticipantIdThrows() {
        thrown.expect(JoynrCommunicationException.class);
        DiscoveryEntry expectedLocalDiscoveryEntry = new DiscoveryEntry(localEntry.getProviderVersion(),
                                                                        localEntry.getDomain(),
                                                                        localEntry.getInterfaceName(),
                                                                        null, // participantId
                                                                        localEntry.getQos(),
                                                                        localEntry.getLastSeenDateMs(),
                                                                        localEntry.getExpiryDateMs(),
                                                                        localEntry.getPublicKeyId());
        discoveryEntryStore.add(expectedLocalDiscoveryEntry);
    }

    @Test
    public void addCollectionWithNullDoesNotThrow() {
        discoveryEntryStore.add((Collection) null);
    }

    @Test
    public void addCollectionWithEmptyListDoesNotThrow() {
        ArrayList<DiscoveryEntry> discoveryEntryList = new ArrayList<>();
        discoveryEntryStore.add(discoveryEntryList);
    }

    @Test
    public void addCollectionWithNonEmptyList() {
        DiscoveryEntry expectedLocalDiscoveryEntry = new DiscoveryEntry(localEntry);
        DiscoveryEntry expectedGlobalDiscoveryEntry = new DiscoveryEntry(globalEntry);

        ArrayList<DiscoveryEntry> discoveryEntryList = new ArrayList<>();

        discoveryEntryList.add(localEntry);
        discoveryEntryList.add(globalEntry);

        discoveryEntryStore.add(discoveryEntryList);

        DiscoveryEntry actualGlobalDiscoveryEntry = discoveryEntryStore.lookup(globalParticipantId, Long.MAX_VALUE)
                                                                       .get();
        DiscoveryEntry actualLocalDiscoveryEntry = discoveryEntryStore.lookup(localParticipantId, Long.MAX_VALUE).get();

        assertEquals(expectedLocalDiscoveryEntry, actualLocalDiscoveryEntry);
        assertEquals(expectedGlobalDiscoveryEntry, actualGlobalDiscoveryEntry);
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

        discoveryEntryStore = new DiscoveryEntryStoreInMemory<DiscoveryEntry>(storeLimit);

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

    private Field getPrivateField(Class<?> runtimeClass, String fieldName) {
        try {
            Field result = runtimeClass.getDeclaredField(fieldName);
            return result;
        } catch (Exception e) {
            return null;
        }
    }

    private String domainInterfaceKey(String domain, String interfaceName) {
        return (domain + "|" + interfaceName).toLowerCase();
    }

    @Test
    public void discoveryEntryStoreIsLimitedUpdateSameEntriesMultipleTimes() {

        final int storeLimit = 5;
        final int maxCount = 100;

        discoveryEntryStore = new DiscoveryEntryStoreInMemory<DiscoveryEntry>(storeLimit);

        for (int i = 0; i < maxCount; i++) {
            for (int j = 0; j < 2; j++) {
                String participantId = "nonStickyId" + i;
                DiscoveryEntry globalEntry2 = new DiscoveryEntry(globalEntry);
                globalEntry2.setParticipantId(participantId);
                discoveryEntryStore.add(globalEntry2);
            }
        }

        // get access to internal maps
        Field registeredCapabilitiesTimeField = getPrivateField(discoveryEntryStore.getClass(),
                                                                "registeredCapabilitiesTime");
        Field interfaceAddressToCapabilityMappingField = getPrivateField(discoveryEntryStore.getClass(),
                                                                         "interfaceAddressToCapabilityMapping");
        Field participantIdToCapabilityMappingField = getPrivateField(discoveryEntryStore.getClass(),
                                                                      "participantIdToCapabilityMapping");
        Field capabilityKeyToCapabilityMappingfield = getPrivateField(discoveryEntryStore.getClass(),
                                                                      "capabilityKeyToCapabilityMapping");
        Field queueIdToParticipantIdMappingField = getPrivateField(discoveryEntryStore.getClass(),
                                                                   "queueIdToParticipantIdMapping");
        Field participantIdToQueueIdMappingField = getPrivateField(discoveryEntryStore.getClass(),
                                                                   "participantIdToQueueIdMapping");

        registeredCapabilitiesTimeField.setAccessible(true);
        interfaceAddressToCapabilityMappingField.setAccessible(true);
        participantIdToCapabilityMappingField.setAccessible(true);
        capabilityKeyToCapabilityMappingfield.setAccessible(true);
        queueIdToParticipantIdMappingField.setAccessible(true);
        participantIdToQueueIdMappingField.setAccessible(true);

        try {
            Map<String, Long> registeredCapabilitiesTime = (Map<String, Long>) registeredCapabilitiesTimeField.get(discoveryEntryStore);
            Map<String, List<String>> interfaceAddressToCapabilityMapping = (Map<String, List<String>>) interfaceAddressToCapabilityMappingField.get(discoveryEntryStore);
            Map<String, String> participantIdToCapabilityMapping = (Map<String, String>) participantIdToCapabilityMappingField.get(discoveryEntryStore);
            Map<String, DiscoveryEntry> capabilityKeyToCapabilityMapping = (Map<String, DiscoveryEntry>) capabilityKeyToCapabilityMappingfield.get(discoveryEntryStore);
            TreeMap<Long, String> queueIdToParticipantIdMapping = (TreeMap<Long, String>) queueIdToParticipantIdMappingField.get(discoveryEntryStore);
            Map<String, Long> participantIdToQueueIdMapping = (Map<String, Long>) participantIdToQueueIdMappingField.get(discoveryEntryStore);

            assertEquals(storeLimit, registeredCapabilitiesTime.size());
            assertEquals(storeLimit, participantIdToCapabilityMapping.size());
            assertEquals(storeLimit, capabilityKeyToCapabilityMapping.size());
            assertEquals(storeLimit, queueIdToParticipantIdMapping.size());
            assertEquals(storeLimit, participantIdToQueueIdMapping.size());

            // since all entries use same domain/interfaceId there shoudl be exactly one entry
            assertEquals(1, interfaceAddressToCapabilityMapping.size());
            // the list associated with that entry should have expected number of entries
            String domainInterfaceId = domainInterfaceKey(globalEntry.getDomain(), globalEntry.getInterfaceName());
            List<String> mapping = interfaceAddressToCapabilityMapping.get(domainInterfaceId);
            if (mapping == null) {
                fail("required mapping not found in interfaceAddressToCapabilityMapping");
            }
            assertEquals(storeLimit, mapping.size());
        } catch (Exception exception) {
            fail(exception.getMessage());
        }
    }
}
