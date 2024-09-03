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

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.Properties;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.persistence.EntityManager;

import com.google.inject.persist.jpa.JpaPersistOptions;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;
import com.google.inject.persist.PersistService;
import com.google.inject.persist.jpa.JpaPersistModule;

import io.joynr.util.ObjectMapper;
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
    private String clusterControllerId = "clusterControllerId";
    static final long DEFAULT_EXPIRY_INTERVAL_MS = 60000;

    @Before
    public void setUp() throws Exception {

        JpaPersistOptions jpaOptions = JpaPersistOptions.builder()
                                                        .setAutoBeginWorkOnEntityManagerCreation(true)
                                                        .build();
        JpaPersistModule jpaModule = new JpaPersistModule("CapabilitiesDirectory", jpaOptions);
        Properties jpaProperties = new Properties();
        jpaProperties.setProperty("javax.persistence.jdbc.url", "jdbc:postgresql://localhost:5432/gcd-test");
        // create-drop: drop the schema at the end of the session
        jpaProperties.setProperty("hibernate.hbm2ddl.auto", "create-drop");
        jpaModule.properties(jpaProperties);
        Injector injector = Guice.createInjector(jpaModule, new AbstractModule() {
            @Override
            protected void configure() {
                bind(String.class).annotatedWith(Names.named(PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS))
                                  .toInstance(String.valueOf(DEFAULT_EXPIRY_INTERVAL_MS));
                bind(CapabilitiesProvisioning.class).to(TestCapabilitiesProvisioning.class);
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
        logger.info("Discovery entry: {}", discoveryEntry);
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
        logger.info("Persisted entry: {}", persistedEntry);
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
    public void touch_updatesEntries() throws Exception {
        long toleranceMs = 100;
        assertTrue(DEFAULT_EXPIRY_INTERVAL_MS > 10 * toleranceMs);
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
        long actualLastSeen = returnedEntries.get(0).getLastSeenDateMs();
        assertTrue(actualLastSeen < currentTimeMillisBefore);

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{ expectedEntry2.getDomain() },
                                                                             expectedEntry2.getInterfaceName());
        assertTrue(returnedEntries.contains(expectedEntry2));
        assertTrue(returnedEntries.size() == 1);
        actualLastSeen = returnedEntries.get(0).getLastSeenDateMs();
        assertTrue(actualLastSeen < currentTimeMillisBefore);

        // call touch for clusterControllerId and check lastSeenDateMs
        store.touch(clusterControllerId);

        entityManager.clear();

        Thread.sleep(1);
        long currentTimeMillisAfter = System.currentTimeMillis();
        long expectedExpiryDate = currentTimeMillisBefore + DEFAULT_EXPIRY_INTERVAL_MS;

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{ expectedEntry1.getDomain() },
                                                                             expectedEntry1.getInterfaceName());
        assertFalse(returnedEntries.contains(discoveryEntry1));
        assertFalse(returnedEntries.contains(expectedEntry1));
        // touch should not insert additional entries
        assertTrue(returnedEntries.size() == 1);
        actualLastSeen = returnedEntries.get(0).getLastSeenDateMs();
        assertTrue(actualLastSeen > currentTimeMillisBefore);
        assertTrue(actualLastSeen < currentTimeMillisAfter);
        long actualExpiryDate = returnedEntries.get(0).getExpiryDateMs();
        long diff = actualExpiryDate - expectedExpiryDate;
        assertTrue("actual: " + actualExpiryDate + ", expected: " + expectedExpiryDate + ", diff: " + diff,
                   diff < toleranceMs);

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{ expectedEntry2.getDomain() },
                                                                             expectedEntry2.getInterfaceName());
        assertFalse(returnedEntries.contains(discoveryEntry2));
        assertFalse(returnedEntries.contains(expectedEntry2));
        // touch should not insert additional entries
        assertTrue(returnedEntries.size() == 1);
        actualLastSeen = returnedEntries.get(0).getLastSeenDateMs();
        assertTrue(actualLastSeen > currentTimeMillisBefore);
        assertTrue(actualLastSeen < currentTimeMillisAfter);
        actualExpiryDate = returnedEntries.get(0).getExpiryDateMs();
        diff = actualExpiryDate - expectedExpiryDate;
        assertTrue("actual: " + actualExpiryDate + ", expected: " + expectedExpiryDate + ", diff: " + diff,
                   diff < toleranceMs);
    }

    @Test
    public void touchWithParticipantIds_updatesSelectedParticipantIds() throws Exception {
        long toleranceMs = 100;
        assertTrue(DEFAULT_EXPIRY_INTERVAL_MS > 10 * toleranceMs);
        Function<GlobalDiscoveryEntryPersisted, List<GlobalDiscoveryEntryPersisted>> containsUntouched = new Function<GlobalDiscoveryEntryPersisted, List<GlobalDiscoveryEntryPersisted>>() {
            @Override
            public List<GlobalDiscoveryEntryPersisted> apply(GlobalDiscoveryEntryPersisted e) {
                List<GlobalDiscoveryEntryPersisted> returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{
                        e.getDomain() }, e.getInterfaceName());
                assertTrue(returnedEntries.contains(e));
                assertTrue(returnedEntries.size() == 1);
                return returnedEntries;
            }
        };
        Function<GlobalDiscoveryEntryPersisted, List<GlobalDiscoveryEntryPersisted>> containsTouched = new Function<GlobalDiscoveryEntryPersisted, List<GlobalDiscoveryEntryPersisted>>() {
            @Override
            public List<GlobalDiscoveryEntryPersisted> apply(GlobalDiscoveryEntryPersisted e) {
                List<GlobalDiscoveryEntryPersisted> returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{
                        e.getDomain() }, e.getInterfaceName());
                assertFalse(returnedEntries.contains(e));
                // touch should not insert additional entries
                assertTrue(returnedEntries.size() == 1);
                return returnedEntries;
            }
        };

        // prepare 2 entries
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

        // add entries
        store.add(discoveryEntry1, new String[]{ defaultGbid });
        store.add(discoveryEntry2, new String[]{ gbids[1] });
        entityManager.clear();

        Thread.sleep(1);
        // check if untouched && lastSeenDateMs < System.currentTimeMillis()
        long currentTimeMillisBefore = System.currentTimeMillis();

        List<GlobalDiscoveryEntryPersisted> returnedEntries = containsUntouched.apply(expectedEntry1);
        long actualLastSeen = returnedEntries.get(0).getLastSeenDateMs();
        assertTrue(actualLastSeen < currentTimeMillisBefore);
        returnedEntries = containsUntouched.apply(expectedEntry2);
        actualLastSeen = returnedEntries.get(0).getLastSeenDateMs();
        assertTrue(actualLastSeen < currentTimeMillisBefore);

        // call touch for clusterControllerId and participantId 2 and check lastSeenDateMs
        store.touch(clusterControllerId, new String[]{ expectedEntry2.getParticipantId() });
        entityManager.clear();

        Thread.sleep(1);
        long currentTimeMillisAfter1 = System.currentTimeMillis();
        long expectedExpiryDate = currentTimeMillisBefore + DEFAULT_EXPIRY_INTERVAL_MS;

        returnedEntries = containsUntouched.apply(expectedEntry1);
        actualLastSeen = returnedEntries.get(0).getLastSeenDateMs();
        assertTrue(actualLastSeen < currentTimeMillisBefore);
        assertTrue(actualLastSeen < currentTimeMillisAfter1);

        containsTouched.apply(discoveryEntry2);
        returnedEntries = containsTouched.apply(expectedEntry2);
        actualLastSeen = returnedEntries.get(0).getLastSeenDateMs();
        assertTrue(actualLastSeen > currentTimeMillisBefore);
        assertTrue(actualLastSeen < currentTimeMillisAfter1);
        long actualExpiryDate = returnedEntries.get(0).getExpiryDateMs();
        long diff = actualExpiryDate - expectedExpiryDate;
        assertTrue("actual: " + actualExpiryDate + ", expected: " + expectedExpiryDate + ", diff: " + diff,
                   diff < toleranceMs);

        // call touch for clusterControllerId and all participantIds and check lastSeenDateMs
        store.touch(clusterControllerId,
                    new String[]{ expectedEntry2.getParticipantId(), expectedEntry1.getParticipantId() });
        entityManager.clear();

        Thread.sleep(1);
        long currentTimeMillisAfter2 = System.currentTimeMillis();
        expectedExpiryDate = currentTimeMillisAfter1 + DEFAULT_EXPIRY_INTERVAL_MS;

        containsTouched.apply(discoveryEntry1);
        returnedEntries = containsTouched.apply(expectedEntry1);
        actualLastSeen = returnedEntries.get(0).getLastSeenDateMs();
        assertTrue(actualLastSeen > currentTimeMillisBefore);
        assertTrue(actualLastSeen > currentTimeMillisAfter1);
        assertTrue(actualLastSeen < currentTimeMillisAfter2);
        actualExpiryDate = returnedEntries.get(0).getExpiryDateMs();
        diff = actualExpiryDate - expectedExpiryDate;
        assertTrue("actual: " + actualExpiryDate + ", expected: " + expectedExpiryDate + ", diff: " + diff,
                   diff < toleranceMs);

        containsTouched.apply(discoveryEntry2);
        returnedEntries = containsTouched.apply(expectedEntry2);
        actualLastSeen = returnedEntries.get(0).getLastSeenDateMs();
        assertTrue(actualLastSeen > currentTimeMillisBefore);
        assertTrue(actualLastSeen > currentTimeMillisAfter1);
        assertTrue(actualLastSeen < currentTimeMillisAfter2);
        actualExpiryDate = returnedEntries.get(0).getExpiryDateMs();
        diff = actualExpiryDate - expectedExpiryDate;
        assertTrue("actual: " + actualExpiryDate + ", expected: " + expectedExpiryDate + ", diff: " + diff,
                   diff < toleranceMs);
    }

    @Test
    public void touchWithParticipantIds_noParticipantIds_doesNotThrow() throws Exception {
        // prepare entry
        String domain = "testTouchDomain";
        String interfaceName = "testTouchInterfaceName";
        String clusterControllerId = "testTouchClusterControllerId";

        GlobalDiscoveryEntryPersisted discoveryEntry1 = createDiscoveryEntry(domain + "1",
                                                                             interfaceName + "1",
                                                                             "testTouchParticipantId1");
        discoveryEntry1.setClusterControllerId(clusterControllerId);
        discoveryEntry1.setExpiryDateMs(System.currentTimeMillis() + DEFAULT_EXPIRY_INTERVAL_MS);

        GlobalDiscoveryEntryPersisted expectedEntry1 = new GlobalDiscoveryEntryPersisted(discoveryEntry1,
                                                                                         discoveryEntry1.getClusterControllerId(),
                                                                                         defaultGbid);
        Address expectedAddress = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(expectedEntry1);
        ((MqttAddress) expectedAddress).setBrokerUri(defaultGbid);
        expectedEntry1.setAddress(CapabilityUtils.serializeAddress(expectedAddress));

        // add entry
        store.add(discoveryEntry1, new String[]{ defaultGbid });
        entityManager.clear();

        // call touch for clusterControllerId and no participantId
        Thread.sleep(1);
        long currentTimeMillisBefore = System.currentTimeMillis();

        store.touch(clusterControllerId, new String[0]);
        entityManager.clear();

        List<GlobalDiscoveryEntryPersisted> returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{
                expectedEntry1.getDomain() }, expectedEntry1.getInterfaceName());
        assertTrue(returnedEntries.contains(expectedEntry1));
        assertTrue(returnedEntries.size() == 1);
        long actualLastSeen = returnedEntries.get(0).getLastSeenDateMs();
        long actualExpiryDate = returnedEntries.get(0).getExpiryDateMs();
        assertTrue(actualLastSeen < currentTimeMillisBefore);
        assertTrue(actualExpiryDate < currentTimeMillisBefore + DEFAULT_EXPIRY_INTERVAL_MS);
    }

    @Test
    public void touch_doesNotUpdateEntriesFromOtherClusterControllers() throws Exception {
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

    @Test
    public void touchWithParticipantIds_doesNotUpdateEntriesFromOtherClusterControllers() throws Exception {
        // prepare entries
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

        GlobalDiscoveryEntryPersisted discoveryEntryFromDefaultClusterController = createDiscoveryEntry(domain,
                                                                                                        interfaceName,
                                                                                                        "testTouchParticipantIdFromDefaultClusterController");

        GlobalDiscoveryEntryPersisted expectedEntryFromOtherCc = new GlobalDiscoveryEntryPersisted(discoveryEntryFromDefaultClusterController,
                                                                                                   discoveryEntryFromDefaultClusterController.getClusterControllerId(),
                                                                                                   defaultGbid);
        expectedAddress = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(expectedEntryFromOtherCc);
        ((MqttAddress) expectedAddress).setBrokerUri(defaultGbid);
        expectedEntryFromOtherCc.setAddress(CapabilityUtils.serializeAddress(expectedAddress));

        // add entries
        store.add(touchedDiscoveryEntry, new String[]{ defaultGbid });
        store.add(discoveryEntryFromDefaultClusterController, new String[]{ defaultGbid });
        entityManager.clear();

        // wait some time to ensure new lastSeenDateMs
        Thread.sleep(1);

        List<GlobalDiscoveryEntryPersisted> returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{
                domain }, interfaceName);
        assertTrue(returnedEntries.contains(expectedTouchedEntry));
        assertTrue(returnedEntries.contains(expectedEntryFromOtherCc));
        assertTrue(returnedEntries.size() == 2);

        // call touch for clusterControllerId and all participantIds and check discoveryEntries
        store.touch(clusterControllerId,
                    new String[]{ expectedTouchedEntry.getParticipantId(),
                            expectedEntryFromOtherCc.getParticipantId() });
        Thread.sleep(1);

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{ domain }, interfaceName);
        assertFalse(returnedEntries.contains(expectedTouchedEntry));
        assertTrue(returnedEntries.contains(expectedEntryFromOtherCc));
        // touch should not insert additional entries
        assertTrue(returnedEntries.size() == 2);
        assertTrue(returnedEntries.stream().anyMatch((entry) -> expectedTouchedEntry.getParticipantId()
                                                                                    .equals(entry.getParticipantId())));
    }

    @Test
    public void removeStale() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry1 = createDiscoveryEntry("domain1",
                                                                             "interfaceName1",
                                                                             "participantId1");
        discoveryEntry1.setLastSeenDateMs(42l);
        GlobalDiscoveryEntryPersisted discoveryEntry2 = createDiscoveryEntry("domain1",
                                                                             "interfaceName2",
                                                                             "participantId2");
        long maxLastSeenDate = System.currentTimeMillis() - 1001;
        discoveryEntry2.setLastSeenDateMs(maxLastSeenDate + 1000);
        GlobalDiscoveryEntryPersisted discoveryEntry3 = createDiscoveryEntry("domain2",
                                                                             "interfaceName3",
                                                                             "participantId3");
        discoveryEntry3.setLastSeenDateMs(maxLastSeenDate - 1);

        store.add(discoveryEntry1, gbids);
        store.add(discoveryEntry2, gbids);
        store.add(discoveryEntry3, gbids);
        entityManager.clear();
        assertContains(discoveryEntry1, gbids);
        assertContains(discoveryEntry2, gbids);
        assertContains(discoveryEntry3, gbids);

        int deletedCount = store.removeStale(clusterControllerId, maxLastSeenDate);
        assertEquals(gbids.length * 2, deletedCount);
        entityManager.clear();
        assertContains(discoveryEntry2, gbids);
        assertNotContains(discoveryEntry1, gbids);
        assertNotContains(discoveryEntry3, gbids);
    }

    @Test
    public void addAndRemoveAndAddAgain() throws Exception {
        // https://www.objectdb.com/java/jpa/query/jpql/delete
        GlobalDiscoveryEntryPersisted discoveryEntry1 = createDiscoveryEntry("domain1",
                                                                             "interfaceName1",
                                                                             "participantId1");

        store.add(discoveryEntry1, new String[]{ defaultGbid });
        assertContains(discoveryEntry1, new String[]{ defaultGbid });

        int deletedCount = store.remove(discoveryEntry1.getParticipantId(), new String[]{ defaultGbid });
        assertEquals(1, deletedCount);
        assertNotContains(discoveryEntry1, gbids);
        GlobalDiscoveryEntryPersistedKey key = new GlobalDiscoveryEntryPersistedKey();
        key.setGbid(defaultGbid);
        key.setParticipantId(discoveryEntry1.getParticipantId());
        assertNull(entityManager.find(GlobalDiscoveryEntryPersisted.class, key));

        GlobalDiscoveryEntryPersisted discoveryEntry2 = createDiscoveryEntry("domain1",
                                                                             "interfaceName1",
                                                                             "participantId1");
        store.add(discoveryEntry2, new String[]{ defaultGbid });
        assertContains(discoveryEntry2, new String[]{ defaultGbid });
    }

    @Test
    public void removeThenAdd() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry1 = createDiscoveryEntry("domain1",
                                                                             "interfaceName1",
                                                                             "participantId1");
        assertNotContains(discoveryEntry1, gbids);

        int deletedCount = store.remove(discoveryEntry1.getParticipantId(), new String[]{ defaultGbid });
        assertEquals(0, deletedCount);
        assertNotContains(discoveryEntry1, gbids);

        store.add(discoveryEntry1, new String[]{ defaultGbid });
        entityManager.clear();
        assertContains(discoveryEntry1, new String[]{ defaultGbid });
    }

    @Test
    public void addAgainAfterRemoveStale() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry1 = createDiscoveryEntry("domain1",
                                                                             "interfaceName1",
                                                                             "participantId1");
        long maxLastSeenDate = System.currentTimeMillis() - 1001;
        discoveryEntry1.setLastSeenDateMs(maxLastSeenDate - 1);

        store.add(discoveryEntry1, new String[]{ defaultGbid });
        assertContains(discoveryEntry1, new String[]{ defaultGbid });

        int deletedCount = store.removeStale(clusterControllerId, maxLastSeenDate);
        assertEquals(1, deletedCount);
        assertNotContains(discoveryEntry1, gbids);
        GlobalDiscoveryEntryPersistedKey key = new GlobalDiscoveryEntryPersistedKey();
        key.setGbid(defaultGbid);
        key.setParticipantId(discoveryEntry1.getParticipantId());
        assertNull(entityManager.find(GlobalDiscoveryEntryPersisted.class, key));

        GlobalDiscoveryEntryPersisted discoveryEntry2 = createDiscoveryEntry("domain1",
                                                                             "interfaceName1",
                                                                             "participantId1");
        discoveryEntry2.setLastSeenDateMs(System.currentTimeMillis());
        store.add(discoveryEntry2, new String[]{ defaultGbid });
        entityManager.clear();
        assertContains(discoveryEntry2, new String[]{ defaultGbid });
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
                                                                                         clusterControllerId,
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
