/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.stream.Collectors;

import javax.persistence.EntityManager;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.persist.PersistService;
import com.google.inject.persist.jpa.JpaPersistModule;

import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

public class GlobalDiscoveryEntryPersistedStorePersistedTest {

    private static final Logger logger = LoggerFactory.getLogger(GlobalDiscoveryEntryPersistedStorePersistedTest.class);

    private PersistService service;
    private GlobalDiscoveryEntryPersistedStorePersisted store;
    private EntityManager entityManager;
    private String defaultGbid = "joynrdefaultgbid";
    private String[] gbids = { defaultGbid, "joynrtestgbid2" };

    @Before
    public void setUp() throws Exception {

        Injector injector = Guice.createInjector(new JpaPersistModule("CapabilitiesDirectory"), new AbstractModule() {
            @Override
            protected void configure() {
                bind(CapabilitiesProvisioning.class).to(DefaultCapabilitiesProvisioning.class);
                requestStaticInjection(CapabilityUtils.class, RoutingTypesUtil.class);
            }
        });
        service = injector.getInstance(PersistService.class);
        store = injector.getInstance(GlobalDiscoveryEntryPersistedStorePersisted.class);
        entityManager = injector.getInstance(EntityManager.class);
    }

    @After
    public void tearDown() {
        service.stop();
    }

    @Test
    public void add_singleGbid() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry("domain", "interfaceName", "participantId");
        GlobalDiscoveryEntryPersisted expectedEntry = new GlobalDiscoveryEntryPersisted(discoveryEntry,
                                                                                        discoveryEntry.getClusterControllerId(),
                                                                                        discoveryEntry.getGbid());

        store.add(discoveryEntry, new String[]{ gbids[1] });
        entityManager.clear();

        assertContains(expectedEntry, new String[]{ gbids[1] });
        assertNotContainsDomainInterface(discoveryEntry); // invalid address, replaced in store.add
    }

    @Test
    public void add_multipleGbids() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry("domain", "interfaceName", "participantId");
        GlobalDiscoveryEntryPersisted expectedEntry = new GlobalDiscoveryEntryPersisted(discoveryEntry,
                                                                                        discoveryEntry.getClusterControllerId(),
                                                                                        discoveryEntry.getGbid());
        String[] expectedGbids = gbids.clone();

        store.add(discoveryEntry, gbids);
        entityManager.clear();

        assertContains(expectedEntry, expectedGbids);
        assertNotContainsDomainInterface(discoveryEntry); // invalid address, replaced in store.add
    }

    @Test
    public void add_duplicateGbid_addsOnlyOnce() throws Exception {
        // due to empty string replacement, the default/own GBID of GCD might be duplicated
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry("domain", "interfaceName", "participantId");
        GlobalDiscoveryEntryPersisted expectedEntry = new GlobalDiscoveryEntryPersisted(discoveryEntry,
                                                                                        discoveryEntry.getClusterControllerId(),
                                                                                        discoveryEntry.getGbid());
        String[] selectedGbids = new String[]{ defaultGbid, gbids[1], defaultGbid };
        String[] expectedGbids = new String[]{ defaultGbid, gbids[1] };

        store.add(discoveryEntry, selectedGbids);
        entityManager.clear();

        assertContains(expectedEntry, expectedGbids);
        assertNotContainsDomainInterface(discoveryEntry); // invalid address, replaced in store.add
    }

    @Test
    public void testVersionPersistedAndRetrieved() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry("domain", "interfaceName", "participantId");
        logger.info("Discovery entry: " + discoveryEntry);
        GlobalDiscoveryEntryPersisted expectedDiscoveryEntry = new GlobalDiscoveryEntryPersisted(discoveryEntry,
                                                                                                 discoveryEntry.getClusterControllerId(),
                                                                                                 discoveryEntry.getGbid());

        store.add(discoveryEntry, new String[]{ defaultGbid });
        entityManager.clear();

        Collection<GlobalDiscoveryEntryPersisted> lookupResult = store.lookup(new String[]{
                expectedDiscoveryEntry.getDomain() }, expectedDiscoveryEntry.getInterfaceName());
        assertNotNull(lookupResult);
        assertEquals(1, lookupResult.size());
        DiscoveryEntry persistedEntry = lookupResult.iterator().next();
        logger.info("Persisted entry: " + persistedEntry);
        assertNotEquals(System.identityHashCode(discoveryEntry), System.identityHashCode(persistedEntry));
        assertNotEquals(System.identityHashCode(expectedDiscoveryEntry), System.identityHashCode(persistedEntry));
        assertNotNull(persistedEntry);
        assertNotNull(persistedEntry.getProviderVersion());
        assertEquals(expectedDiscoveryEntry.getProviderVersion().getMajorVersion(),
                     persistedEntry.getProviderVersion().getMajorVersion());
        assertEquals(expectedDiscoveryEntry.getProviderVersion().getMinorVersion(),
                     persistedEntry.getProviderVersion().getMinorVersion());
    }

    private void testRemove(String[] selectedGbids, String[] removedGbids) throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry("domain", "interfaceName", "participantId");
        String[] remainingGbids = Arrays.stream(gbids)
                                        .filter(gbid -> !Arrays.asList(selectedGbids).contains(gbid))
                                        .toArray(String[]::new);

        store.add(discoveryEntry, gbids);
        entityManager.clear();
        assertContains(discoveryEntry, gbids);

        store.remove(discoveryEntry.getParticipantId(), selectedGbids);
        entityManager.clear();
        assertContains(discoveryEntry, remainingGbids);
        assertNotContains(discoveryEntry, removedGbids);
    }

    @Test
    public void remove_singleGbid() throws Exception {
        String[] selectedGbids = new String[]{ gbids[1] };
        String[] removedGbids = selectedGbids.clone();

        testRemove(selectedGbids, removedGbids);
    }

    @Test
    public void remove_multipleGbids() throws Exception {
        String[] selectedGbids = gbids.clone();
        String[] removedGbids = selectedGbids.clone();

        testRemove(selectedGbids, removedGbids);
    }

    @Test
    public void remove_duplicateGbid() throws Exception {
        // due to empty string replacement, the default/own GBID of GCD might be duplicated
        String[] selectedGbids = new String[]{ defaultGbid, defaultGbid };
        String[] removedGbids = new String[]{ defaultGbid };

        testRemove(selectedGbids, removedGbids);
    }

    @Test
    public void lookupDomainInterface() throws Exception {
        String domain = "domain";
        String interfaceName = "interfaceName";
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry(domain, interfaceName, "participantId");

        store.add(discoveryEntry, gbids);
        entityManager.clear();

        assertContainsDomainInterface(discoveryEntry, gbids);
    }

    @Test
    @Ignore
    public void testLookupDomainInterfaceWithMaxCacheAge() {
        fail("Not yet implemented");
    }

    @Test
    @Ignore
    public void testLookupParticipantIdWithMaxCacheAge() {
        fail("Not yet implemented");
    }

    @Test
    @Ignore
    public void testGetAllDiscoveryEntries() {
        fail("Not yet implemented");
    }

    @Test
    @Ignore
    public void testHasDiscoveryEntry() {
        fail("Not yet implemented");
    }

    @Test
    public void testTouchUpdatesLastSeenDateMs() throws Exception {
        String domain = "testTouchDomain";
        String interfaceName = "testTouchInterfaceName";
        String clusterControllerId = "testTouchClusterControllerId";

        GlobalDiscoveryEntryPersisted discoveryEntry1 = createDiscoveryEntry(domain + "1",
                                                                             interfaceName + "1",
                                                                             "testTouchParticipantId1");
        discoveryEntry1.setClusterControllerId(clusterControllerId);

        GlobalDiscoveryEntryPersisted expectedEntry1 = new GlobalDiscoveryEntryPersisted(discoveryEntry1,
                                                                                         discoveryEntry1.getClusterControllerId(),
                                                                                         defaultGbid);
        Address expectedAddress = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(expectedEntry1);
        ((MqttAddress) expectedAddress).setBrokerUri(defaultGbid);
        expectedEntry1.setAddress(CapabilityUtils.serializeAddress(expectedAddress));

        store.add(discoveryEntry1, new String[]{ defaultGbid });

        GlobalDiscoveryEntryPersisted discoveryEntry2 = createDiscoveryEntry(domain + "2",
                                                                             interfaceName + "2",
                                                                             "testTouchParticipantId2");
        discoveryEntry2.setClusterControllerId(clusterControllerId);

        GlobalDiscoveryEntryPersisted expectedEntry2 = new GlobalDiscoveryEntryPersisted(discoveryEntry2,
                                                                                         discoveryEntry2.getClusterControllerId(),
                                                                                         gbids[1]);
        expectedAddress = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(expectedEntry2);
        ((MqttAddress) expectedAddress).setBrokerUri(gbids[1]);
        expectedEntry2.setAddress(CapabilityUtils.serializeAddress(expectedAddress));

        store.add(discoveryEntry2, new String[]{ gbids[1] });

        entityManager.clear();

        Thread.sleep(1);
        // check: lastSeenDateMs < System.currentTimeMillis()
        long currentTimeMillisBefore = System.currentTimeMillis();

        List<GlobalDiscoveryEntryPersisted> returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{
                expectedEntry1.getDomain() }, expectedEntry1.getInterfaceName());
        assertTrue(returnedEntries.contains(expectedEntry1));
        assertTrue(returnedEntries.size() == 1);
        assertTrue(returnedEntries.get(0).getLastSeenDateMs() < currentTimeMillisBefore);

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{ expectedEntry2.getDomain() },
                                                                             expectedEntry2.getInterfaceName());
        assertTrue(returnedEntries.contains(expectedEntry2));
        assertTrue(returnedEntries.size() == 1);
        assertTrue(returnedEntries.get(0).getLastSeenDateMs() < currentTimeMillisBefore);

        // call touch for clusterControllerId and check lastSeenDateMs
        store.touch(clusterControllerId);

        entityManager.clear();

        Thread.sleep(1);
        long currentTimeMillisAfter = System.currentTimeMillis();

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{ expectedEntry1.getDomain() },
                                                                             expectedEntry1.getInterfaceName());
        assertFalse(returnedEntries.contains(discoveryEntry1));
        assertFalse(returnedEntries.contains(expectedEntry1));
        // touch should not insert additional entries
        assertTrue(returnedEntries.size() == 1);
        assertTrue(returnedEntries.get(0).getLastSeenDateMs() > currentTimeMillisBefore);
        assertTrue(returnedEntries.get(0).getLastSeenDateMs() < currentTimeMillisAfter);

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{ expectedEntry2.getDomain() },
                                                                             expectedEntry2.getInterfaceName());
        assertFalse(returnedEntries.contains(discoveryEntry2));
        assertFalse(returnedEntries.contains(expectedEntry2));
        // touch should not insert additional entries
        assertTrue(returnedEntries.size() == 1);
        assertTrue(returnedEntries.get(0).getLastSeenDateMs() > currentTimeMillisBefore);
        assertTrue(returnedEntries.get(0).getLastSeenDateMs() < currentTimeMillisAfter);
    }

    @Test
    public void testTouchDoesNotUpdateEntriesFromOtherClusterControllers() throws Exception {
        String domain = "testTouchDomain";
        String interfaceName = "testTouchInterfaceName";
        String clusterControllerId = "testTouchClusterControllerId";
        GlobalDiscoveryEntryPersisted touchedDiscoveryEntry = createDiscoveryEntry(domain,
                                                                                   interfaceName,
                                                                                   "testTouchParticipantId1");
        touchedDiscoveryEntry.setClusterControllerId(clusterControllerId);

        GlobalDiscoveryEntryPersisted expectedTouchedEntry = new GlobalDiscoveryEntryPersisted(touchedDiscoveryEntry,
                                                                                               touchedDiscoveryEntry.getClusterControllerId(),
                                                                                               defaultGbid);
        Address expectedAddress = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(expectedTouchedEntry);
        ((MqttAddress) expectedAddress).setBrokerUri(defaultGbid);
        expectedTouchedEntry.setAddress(CapabilityUtils.serializeAddress(expectedAddress));

        store.add(touchedDiscoveryEntry, new String[]{ defaultGbid });

        GlobalDiscoveryEntryPersisted discoveryEntryFromDefaultClusterController = createDiscoveryEntry(domain,
                                                                                                        interfaceName,
                                                                                                        "testTouchParticipantIdFromDefaultClusterController");

        GlobalDiscoveryEntryPersisted expectedEntryFromOtherCc = new GlobalDiscoveryEntryPersisted(discoveryEntryFromDefaultClusterController,
                                                                                                   discoveryEntryFromDefaultClusterController.getClusterControllerId(),
                                                                                                   defaultGbid);
        expectedAddress = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(expectedEntryFromOtherCc);
        ((MqttAddress) expectedAddress).setBrokerUri(defaultGbid);
        expectedEntryFromOtherCc.setAddress(CapabilityUtils.serializeAddress(expectedAddress));

        store.add(discoveryEntryFromDefaultClusterController, new String[]{ defaultGbid });
        entityManager.clear();

        // wait some time to ensure new lastSeenDateMs
        Thread.sleep(1);

        List<GlobalDiscoveryEntryPersisted> returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{
                domain }, interfaceName);
        assertTrue(returnedEntries.contains(expectedTouchedEntry));
        assertTrue(returnedEntries.contains(expectedEntryFromOtherCc));
        assertTrue(returnedEntries.size() == 2);

        // call touch for clusterControllerId and check discoveryEntries
        store.touch(clusterControllerId);
        Thread.sleep(1);

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{ domain }, interfaceName);
        assertFalse(returnedEntries.contains(expectedTouchedEntry));
        assertTrue(returnedEntries.contains(expectedEntryFromOtherCc));
        // touch should not insert additional entries
        assertTrue(returnedEntries.size() == 2);
    }

    private GlobalDiscoveryEntryPersisted createDiscoveryEntry(String domain,
                                                               String interfaceName,
                                                               String participantId) throws Exception {
        ProviderQos qos = new ProviderQos();
        long lastSeenDateMs = 123L;
        long expiryDateMs = Long.MAX_VALUE;
        String publicKeyId = "publicKeyId";
        Address address = new MqttAddress("brokerUri", "topic");
        String addressSerialized = new ObjectMapper().writeValueAsString(address);
        GlobalDiscoveryEntryPersisted discoveryEntry = new GlobalDiscoveryEntryPersisted(new Version(47, 11),
                                                                                         domain,
                                                                                         interfaceName,
                                                                                         participantId,
                                                                                         qos,
                                                                                         lastSeenDateMs,
                                                                                         expiryDateMs,
                                                                                         publicKeyId,
                                                                                         addressSerialized,
                                                                                         "clusterControllerId",
                                                                                         defaultGbid);
        return discoveryEntry;
    }

    private void assertContains(GlobalDiscoveryEntryPersisted discoveryEntry, String[] gbids) {
        Collection<GlobalDiscoveryEntryPersisted> returnedEntries = store.lookup(discoveryEntry.getParticipantId())
                                                                         .get();
        assertEquals(gbids.length, returnedEntries.size());

        for (String gbid : gbids) {
            GlobalDiscoveryEntryPersisted expectedEntry = new GlobalDiscoveryEntryPersisted(discoveryEntry,
                                                                                            discoveryEntry.getClusterControllerId(),
                                                                                            gbid);
            Address expectedAddress = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(expectedEntry);
            ((MqttAddress) expectedAddress).setBrokerUri(gbid);
            expectedEntry.setAddress(CapabilityUtils.serializeAddress(expectedAddress));
            assertTrue(returnedEntries.contains(expectedEntry));
        }
    }

    private void assertContainsDomainInterface(GlobalDiscoveryEntryPersisted discoveryEntry, String[] gbids) {
        Collection<GlobalDiscoveryEntryPersisted> returnedEntries = store.lookup(new String[]{
                discoveryEntry.getDomain() }, discoveryEntry.getInterfaceName());
        assertEquals(gbids.length, returnedEntries.size());

        for (String gbid : gbids) {
            GlobalDiscoveryEntryPersisted expectedEntry = new GlobalDiscoveryEntryPersisted(discoveryEntry,
                                                                                            discoveryEntry.getClusterControllerId(),
                                                                                            gbid);
            Address expectedAddress = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(expectedEntry);
            ((MqttAddress) expectedAddress).setBrokerUri(gbid);
            expectedEntry.setAddress(CapabilityUtils.serializeAddress(expectedAddress));
            assertTrue(returnedEntries.contains(expectedEntry));
        }
    }

    private void assertNotContains(GlobalDiscoveryEntryPersisted discoveryEntry, String[] gbids) {
        Collection<GlobalDiscoveryEntryPersisted> returnedEntries = store.lookup(discoveryEntry.getParticipantId())
                                                                         .get();
        Collection<GlobalDiscoveryEntryPersisted> filteredEntries = returnedEntries.stream()
                                                                                   .filter(e -> Arrays.asList(gbids)
                                                                                                      .contains(e.getGbid()))
                                                                                   .collect(Collectors.toList());
        assertEquals(0, filteredEntries.size());
    }

    private void assertNotContainsDomainInterface(GlobalDiscoveryEntryPersisted... discoveryEntries) {
        for (GlobalDiscoveryEntryPersisted discoveryEntry : discoveryEntries) {
            Collection<GlobalDiscoveryEntryPersisted> returnedEntries = store.lookup(new String[]{
                    discoveryEntry.getDomain() }, discoveryEntry.getInterfaceName());
            assertFalse(returnedEntries.contains(discoveryEntry));
        }
    }
}
