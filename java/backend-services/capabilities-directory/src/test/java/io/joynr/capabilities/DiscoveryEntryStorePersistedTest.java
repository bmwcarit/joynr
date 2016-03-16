package io.joynr.capabilities;

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

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.fail;

import java.util.Arrays;
import java.util.Collection;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

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

public class DiscoveryEntryStorePersistedTest {

    private PersistService service;
    private DiscoveryEntryStore store;

    @Before
    public void setUp() throws Exception {

        Injector injector = Guice.createInjector(new JpaPersistModule("CapabilitiesDirectory"), new AbstractModule() {

            @Override
            protected void configure() {
                bind(DiscoveryEntryStore.class).to(DiscoveryEntryStorePersisted.class);
                bind(DiscoveryEntry.class).to(GlobalDiscoveryEntryPersisted.class);
                bind(CapabilitiesProvisioning.class).to(DefaultCapabilitiesProvisioning.class);
            }
        });
        service = injector.getInstance(PersistService.class);
        store = injector.getInstance(DiscoveryEntryStore.class);
    }

    @After
    public void tearDown() {
        service.stop();
    }

    @Test
    public void testAddDiscoveryEntry() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry("domain", "interfaceName", "participantId");

        store.add(discoveryEntry);
        assertContains(discoveryEntry);

    }

    @Test
    public void testAddCollectionOfDiscoveryEntry() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry1 = createDiscoveryEntry("domain1",
                                                                             "interfaceName1",
                                                                             "participantId1");

        GlobalDiscoveryEntryPersisted discoveryEntry2 = createDiscoveryEntry("domain2",
                                                                             "interfaceName2",
                                                                             "participantId2");

        store.add(Arrays.asList(discoveryEntry1, discoveryEntry2));
        assertContains(discoveryEntry1, discoveryEntry2);
    }

    @Test
    public void testRemoveByParticipantId() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry("domain", "interfaceName", "participantId");
        store.add(discoveryEntry);
        assertContains(discoveryEntry);

        store.remove(discoveryEntry.getParticipantId());
        assertNotContains(discoveryEntry);
    }

    @Test
    public void testRemoveByListOfParticipantIds() throws Exception {
        GlobalDiscoveryEntryPersisted discoveryEntry1 = createDiscoveryEntry("domain1",
                                                                             "interfaceName1",
                                                                             "participantId1");

        GlobalDiscoveryEntryPersisted discoveryEntry2 = createDiscoveryEntry("domain2",
                                                                             "interfaceName2",
                                                                             "participantId2");

        store.add(Arrays.asList(discoveryEntry1, discoveryEntry2));
        assertContains(discoveryEntry1, discoveryEntry2);

        store.remove(Arrays.asList(discoveryEntry1.getParticipantId(), discoveryEntry2.getParticipantId()));
        assertNotContains(discoveryEntry1, discoveryEntry2);

    }

    @Test
    public void testLookupDomainInterface() throws Exception {
        String domain = "domain";
        String interfaceName = "interfaceName";
        GlobalDiscoveryEntryPersisted discoveryEntry = createDiscoveryEntry(domain, interfaceName, "participantId");
        store.add(discoveryEntry);
        Collection<DiscoveryEntry> lookup = store.lookup(domain, interfaceName);
        assertTrue(lookup.contains(discoveryEntry));
    }

    @Test
    public void testLookupDomainInterfaceWithMaxCacheAge() {
        fail("Not yet implemented");
    }

    @Test
    public void testLookupParticipantIdWithMaxCacheAge() {
        fail("Not yet implemented");
    }

    @Test
    public void testGetAllDiscoveryEntries() {
        fail("Not yet implemented");
    }

    @Test
    public void testHasDiscoveryEntry() {
        fail("Not yet implemented");
    }

    private GlobalDiscoveryEntryPersisted createDiscoveryEntry(String domain, String interfaceName, String participantId)
                                                                                                                         throws Exception {
        ProviderQos qos = new ProviderQos();
        long lastSeenDateMs = 123L;
        Address address = new MqttAddress("brokerUri", "topic");
        String addressSeialized = new ObjectMapper().writeValueAsString(address);
        GlobalDiscoveryEntryPersisted discoveryEntry = new GlobalDiscoveryEntryPersisted(domain,
                                                                                         interfaceName,
                                                                                         participantId,
                                                                                         qos,
                                                                                         lastSeenDateMs,
                                                                                         addressSeialized);
        return discoveryEntry;
    }

    private void assertContains(GlobalDiscoveryEntryPersisted... discoveryEntries) {
        for (GlobalDiscoveryEntryPersisted discoveryEntry : discoveryEntries) {
            Collection<DiscoveryEntry> returnedEntries = store.lookup(discoveryEntry.getDomain(),
                                                                      discoveryEntry.getInterfaceName(),
                                                                      10000);
            assertTrue(returnedEntries.contains(discoveryEntry));
        }
    }

    private void assertNotContains(GlobalDiscoveryEntryPersisted... discoveryEntries) {
        for (GlobalDiscoveryEntryPersisted discoveryEntry : discoveryEntries) {
            Collection<DiscoveryEntry> returnedEntries = store.lookup(discoveryEntry.getDomain(),
                                                                      discoveryEntry.getInterfaceName(),
                                                                      10000);
            assertFalse(returnedEntries.contains(discoveryEntry));
        }
    }
}
