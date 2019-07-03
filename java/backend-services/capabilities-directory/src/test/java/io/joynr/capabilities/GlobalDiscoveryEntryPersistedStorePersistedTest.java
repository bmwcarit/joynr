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
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Collection;
import java.util.List;

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
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

@Ignore
public class GlobalDiscoveryEntryPersistedStorePersistedTest {

    private static final Logger logger = LoggerFactory.getLogger(GlobalDiscoveryEntryPersistedStorePersistedTest.class);

    private PersistService service;
    private GlobalDiscoveryEntryPersistedStorePersisted store;
    private EntityManager entityManager;
    private String gbid = "joynrdefaultgbid";
    private String[] gbids = { gbid, "joynrtestgbid2" };

    @Before
    public void setUp() throws Exception {

        Injector injector = Guice.createInjector(new JpaPersistModule("CapabilitiesDirectory"), new AbstractModule() {
            @Override
            protected void configure() {
                bind(CapabilitiesProvisioning.class).to(DefaultCapabilitiesProvisioning.class);
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
    public void testAddDiscoveryEntry() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry("domain", "interfaceName", "participantId");

        store.add(discoveryEntry, gbids);
        entityManager.clear();
        assertContains(discoveryEntry);
    }

    @Test
    public void testVersionPersistedAndRetrieved() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry("domain", "interfaceName", "participantId");
        logger.info("Discovery entry: " + discoveryEntry);

        store.add(discoveryEntry, gbids);
        entityManager.clear();

        Collection<GlobalDiscoveryEntryPersisted> lookupResult = store.lookup(new String[]{
                discoveryEntry.getDomain() }, discoveryEntry.getInterfaceName());
        assertNotNull(lookupResult);
        assertEquals(1, lookupResult.size());
        DiscoveryEntry persistedEntry = lookupResult.iterator().next();
        logger.info("Persisted entry: " + persistedEntry);
        assertNotEquals(System.identityHashCode(discoveryEntry), System.identityHashCode(persistedEntry));
        assertNotNull(persistedEntry);
        assertNotNull(persistedEntry.getProviderVersion());
        assertEquals(discoveryEntry.getProviderVersion().getMajorVersion(),
                     persistedEntry.getProviderVersion().getMajorVersion());
        assertEquals(discoveryEntry.getProviderVersion().getMinorVersion(),
                     persistedEntry.getProviderVersion().getMinorVersion());
    }

    @Test
    public void testRemoveByParticipantId() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry("domain", "interfaceName", "participantId");
        store.add(discoveryEntry, gbids);
        entityManager.clear();
        assertContains(discoveryEntry);

        store.remove(discoveryEntry.getParticipantId(), gbids);
        entityManager.clear();
        assertNotContains(discoveryEntry);
    }

    @Test
    public void testLookupDomainInterface() throws Exception {
        String domain = "domain";
        String interfaceName = "interfaceName";
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry(domain, interfaceName, "participantId");
        store.add(discoveryEntry, gbids);
        entityManager.clear();
        Collection<GlobalDiscoveryEntryPersisted> lookup = store.lookup(new String[]{ domain }, interfaceName);
        assertTrue(lookup.contains(discoveryEntry));
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
        store.add(discoveryEntry1, gbids);
        GlobalDiscoveryEntryPersisted discoveryEntry2 = createDiscoveryEntry(domain + "2",
                                                                             interfaceName + "2",
                                                                             "testTouchParticipantId2");
        discoveryEntry2.setClusterControllerId(clusterControllerId);
        store.add(discoveryEntry2, gbids);

        entityManager.clear();

        Thread.sleep(1);
        // check: lastSeenDateMs < System.currentTimeMillis()
        long currentTimeMillisBefore = System.currentTimeMillis();

        List<GlobalDiscoveryEntryPersisted> returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{
                discoveryEntry1.getDomain() }, discoveryEntry1.getInterfaceName());
        assertTrue(returnedEntries.contains(discoveryEntry1));
        assertTrue(returnedEntries.size() == 1);
        assertTrue(returnedEntries.get(0).getLastSeenDateMs() < currentTimeMillisBefore);

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{
                discoveryEntry2.getDomain() }, discoveryEntry2.getInterfaceName());
        assertTrue(returnedEntries.contains(discoveryEntry2));
        assertTrue(returnedEntries.size() == 1);
        assertTrue(returnedEntries.get(0).getLastSeenDateMs() < currentTimeMillisBefore);

        // call touch for clusterControllerId and check lastSeenDateMs
        store.touch(clusterControllerId);

        entityManager.clear();

        Thread.sleep(1);
        long currentTimeMillisAfter = System.currentTimeMillis();

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{
                discoveryEntry1.getDomain() }, discoveryEntry1.getInterfaceName());
        assertFalse(returnedEntries.contains(discoveryEntry1));
        // touch should not insert additional entries
        assertTrue(returnedEntries.size() == 1);
        assertTrue(returnedEntries.get(0).getLastSeenDateMs() > currentTimeMillisBefore);
        assertTrue(returnedEntries.get(0).getLastSeenDateMs() < currentTimeMillisAfter);

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{
                discoveryEntry2.getDomain() }, discoveryEntry2.getInterfaceName());
        assertFalse(returnedEntries.contains(discoveryEntry2));
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
        store.add(touchedDiscoveryEntry, gbids);
        GlobalDiscoveryEntryPersisted discoveryEntryFromDefaultClusterController = createDiscoveryEntry(domain,
                                                                                                        interfaceName,
                                                                                                        "testTouchParticipantIdFromDefaultClusterController");
        store.add(discoveryEntryFromDefaultClusterController, gbids);
        entityManager.clear();

        // wait some time to ensure new lastSeenDateMs
        Thread.sleep(1);

        List<GlobalDiscoveryEntryPersisted> returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{
                domain }, interfaceName);
        assertTrue(returnedEntries.contains(touchedDiscoveryEntry));
        assertTrue(returnedEntries.contains(discoveryEntryFromDefaultClusterController));
        assertTrue(returnedEntries.size() == 2);

        // call touch for clusterControllerId and check discoveryEntries
        store.touch(clusterControllerId);
        Thread.sleep(1);

        returnedEntries = (List<GlobalDiscoveryEntryPersisted>) store.lookup(new String[]{ domain }, interfaceName);
        assertFalse(returnedEntries.contains(touchedDiscoveryEntry));
        assertTrue(returnedEntries.contains(discoveryEntryFromDefaultClusterController));
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
                                                                                         gbid);
        return discoveryEntry;
    }

    private void assertContains(GlobalDiscoveryEntryPersisted... discoveryEntries) {
        for (GlobalDiscoveryEntryPersisted discoveryEntry : discoveryEntries) {
            Collection<GlobalDiscoveryEntryPersisted> returnedEntries = store.lookup(new String[]{
                    discoveryEntry.getDomain() }, discoveryEntry.getInterfaceName());
            assertTrue(returnedEntries.contains(discoveryEntry));
        }
    }

    private void assertNotContains(GlobalDiscoveryEntryPersisted... discoveryEntries) {
        for (GlobalDiscoveryEntryPersisted discoveryEntry : discoveryEntries) {
            Collection<GlobalDiscoveryEntryPersisted> returnedEntries = store.lookup(new String[]{
                    discoveryEntry.getDomain() }, discoveryEntry.getInterfaceName());
            assertFalse(returnedEntries.contains(discoveryEntry));
        }
    }
}
