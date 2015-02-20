package io.joynr.capabilities;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.endpoints.EndpointAddressBase;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;

import joynr.types.CustomParameter;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.vehicle.Gps;
import joynr.vehicle.GpsAsync;
import joynr.vehicle.NavigationAsync;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import com.google.common.collect.Lists;

/**
 * Tests the interaction of the dispatcher and communication manager.
 */
public class CapabilitiesStoreTest {

    @Before
    public void setUp() {

    }

    @After
    public void tearDown() {

    }

    @Test
    public void testInvalidEntry() throws Exception {
        CapabilitiesStore store = new CapabilitiesStoreImpl();
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        CapabilityEntry capabilityEntry = new CapabilityEntry(null, GpsAsync.class, providerQos, "");
        boolean thrown = false;
        DiscoveryQos discoveryQos = new DiscoveryQos(1000, ArbitrationStrategy.NotSet, 1000);
        try {
            store.add(capabilityEntry);
            store.lookup("hello", GpsAsync.INTERFACE_NAME, discoveryQos.getCacheMaxAge());
        } catch (Exception e) {
            thrown = true;
        }
        Assert.assertTrue(thrown);

    }

    @Test
    public void testMultipleCapEntryRegistrations() {
        CapabilitiesStore store = new CapabilitiesStoreImpl();
        String domain = "testDomain";
        String participantId = "testparticipantId";

        ProviderQos providerQos = new ProviderQos(new ArrayList<CustomParameter>(), 1, 0L, ProviderScope.GLOBAL, true);
        EndpointAddressBase endpointAddress = new JoynrMessagingEndpointAddress("testChannel");
        CapabilityEntry capabilityEntry1 = new CapabilityEntry(domain,
                                                               GpsAsync.class,
                                                               providerQos,
                                                               endpointAddress,
                                                               participantId);

        CapabilityEntry capabilityEntry2 = new CapabilityEntry(domain,
                                                               NavigationAsync.class,
                                                               providerQos,
                                                               endpointAddress,
                                                               participantId);
        store.add(Lists.newArrayList(capabilityEntry1, capabilityEntry2));
        Assert.assertEquals(2, store.getAllCapabilities().size());

    }

    @Test
    public void testCapabilitiesStoreEndPointAddressQuery() {
        testCapabilitiesStoreEndPointAddressQueryInternal(CapabilityScope.LOCALONLY);
        testCapabilitiesStoreEndPointAddressQueryInternal(CapabilityScope.LOCALGLOBAL);
        testCapabilitiesStoreEndPointAddressQueryInternal(CapabilityScope.REMOTE);
    }

    private void testCapabilitiesStoreEndPointAddressQueryInternal(CapabilityScope scope) {
        CapabilitiesStore store = new CapabilitiesStoreImpl();
        HashSet<CapabilityEntry> capabilities = store.getAllCapabilities();
        Assert.assertEquals(0, capabilities.size());

        String domain = "testDomain";
        String participantId = "testparticipantId";
        ProviderQos providerQos = new ProviderQos(new ArrayList<CustomParameter>(), 1, 0L, ProviderScope.GLOBAL, true);
        EndpointAddressBase endpointAddress = new JoynrMessagingEndpointAddress("testChannel");
        CapabilityEntry capabilityEntry1 = new CapabilityEntry(domain,
                                                               GpsAsync.class,
                                                               providerQos,
                                                               endpointAddress,
                                                               participantId + "1");
        store.add(capabilityEntry1);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        DiscoveryQos discoveryQos = DiscoveryQos.NO_FILTER;

        Collection<CapabilityEntry> newlyEnteredCaps = store.findCapabilitiesForEndpointAddress(endpointAddress,
                                                                                                discoveryQos.getCacheMaxAge());

        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry1, newlyEnteredCaps.iterator().next());

        CapabilityEntry capEntryFake = new CapabilityEntry(domain, GpsAsync.class, providerQos, participantId + "Fake");
        capEntryFake.addEndpoint(new JoynrMessagingEndpointAddress("testChannelFake"));
        store.add(capEntryFake);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(2, capabilities.size());

        newlyEnteredCaps = store.findCapabilitiesForEndpointAddress(endpointAddress, discoveryQos.getCacheMaxAge());

        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry1, newlyEnteredCaps.iterator().next());

        store.remove(capEntryFake.getParticipantId());

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        CapabilityEntry capabilityEntry2 = new CapabilityEntry(domain,
                                                               NavigationAsync.class,
                                                               providerQos,
                                                               endpointAddress,
                                                               participantId + "2");
        store.add(capabilityEntry2);

        // check if newly created Entry overrides old one
        capabilities = store.getAllCapabilities();
        Assert.assertEquals(2, capabilities.size());

        newlyEnteredCaps = store.findCapabilitiesForEndpointAddress(endpointAddress, discoveryQos.getCacheMaxAge());
        Assert.assertEquals(2, newlyEnteredCaps.size());
        Assert.assertTrue(newlyEnteredCaps.contains(capabilityEntry1));
        Assert.assertTrue(newlyEnteredCaps.contains(capabilityEntry2));

        store.remove(Lists.newArrayList(capabilityEntry1.getParticipantId(), capabilityEntry2.getParticipantId()));
        Assert.assertEquals(0, store.getAllCapabilities().size());
        Assert.assertEquals(0, store.findCapabilitiesForEndpointAddress(endpointAddress, discoveryQos.getCacheMaxAge())
                                    .size());
    }

    @Test
    public void testCapabilitiesStoreInterfaceAddressQuery() {
        testCapabilitiesStoreInterfaceAddressQueryInternal(ProviderScope.LOCAL);
        testCapabilitiesStoreInterfaceAddressQueryInternal(ProviderScope.GLOBAL);
    }

    @Test
    public void testCapabilitiesStoreCleanRegisterAndUnregister() {
        CapabilitiesStoreImpl store = new CapabilitiesStoreImpl();
        HashSet<CapabilityEntry> capabilities = store.getAllCapabilities();
        Assert.assertEquals(0, capabilities.size());

        String domain = "testDomain";
        String participantId = "testparticipantId";
        ProviderQos providerQos = new ProviderQos(new ArrayList<CustomParameter>(), 1, 0L, ProviderScope.LOCAL, true);
        EndpointAddressBase endpointAddress = new JoynrMessagingEndpointAddress("testChannel");
        CapabilityEntry capabilityEntry1 = new CapabilityEntry(domain,
                                                               GpsAsync.class,
                                                               providerQos,
                                                               endpointAddress,
                                                               participantId);
        store.add(capabilityEntry1);
        String key1 = store.getInterfaceAddressParticipantKeyForCapability(domain,
                                                                           GpsAsync.INTERFACE_NAME,
                                                                           participantId);
        String key2 = store.getInterfaceAddressKeyForCapability(domain, GpsAsync.INTERFACE_NAME);
        Assert.assertEquals(1, store.capabilityKeyToCapabilityMapping.size());
        Assert.assertTrue(store.capabilityKeyToCapabilityMapping.containsKey(key1));
        Assert.assertTrue(store.capabilityKeyToCapabilityMapping.containsValue(capabilityEntry1));
        Assert.assertEquals(1, store.capabilityKeyToEndPointAddressMapping.size());
        Assert.assertTrue(store.capabilityKeyToEndPointAddressMapping.containsKey(key1));
        Assert.assertEquals(1, store.capabilityKeyToEndPointAddressMapping.get(key1).size());
        Assert.assertTrue(store.capabilityKeyToEndPointAddressMapping.get(key1).contains(endpointAddress));
        Assert.assertEquals(1, store.endPointAddressToCapabilityMapping.size());
        Assert.assertTrue(store.endPointAddressToCapabilityMapping.containsKey(endpointAddress));
        Assert.assertEquals(1, store.endPointAddressToCapabilityMapping.get(endpointAddress).size());
        Assert.assertTrue(store.endPointAddressToCapabilityMapping.get(endpointAddress).contains(key1));
        Assert.assertEquals(1, store.interfaceAddressToCapabilityMapping.size());
        Assert.assertTrue(store.interfaceAddressToCapabilityMapping.containsKey(key2));
        Assert.assertEquals(1, store.interfaceAddressToCapabilityMapping.get(key2).size());
        Assert.assertTrue(store.interfaceAddressToCapabilityMapping.get(key2).contains(key1));
        Assert.assertEquals(1, store.participantIdToCapabilityMapping.size());
        Assert.assertTrue(store.participantIdToCapabilityMapping.containsKey(participantId));
        Assert.assertNotNull(store.participantIdToCapabilityMapping.get(participantId));
        Assert.assertEquals(key1, store.participantIdToCapabilityMapping.get(participantId));
        Assert.assertTrue(store.registeredCapabilitiesTime.containsKey(key1));
        Assert.assertEquals(1, store.registeredCapabilitiesTime.size());

        store.remove(capabilityEntry1.getParticipantId());
        Assert.assertTrue(store.capabilityKeyToCapabilityMapping.isEmpty());
        Assert.assertTrue(store.capabilityKeyToEndPointAddressMapping.isEmpty());
        Assert.assertTrue(store.endPointAddressToCapabilityMapping.isEmpty());
        Assert.assertTrue(store.interfaceAddressToCapabilityMapping.isEmpty());
        Assert.assertTrue(store.participantIdToCapabilityMapping.isEmpty());
        Assert.assertTrue(store.registeredCapabilitiesTime.isEmpty());
    }

    private void testCapabilitiesStoreInterfaceAddressQueryInternal(ProviderScope scope) {
        CapabilitiesStore store = new CapabilitiesStoreImpl();
        HashSet<CapabilityEntry> capabilities = store.getAllCapabilities();
        Assert.assertEquals(0, capabilities.size());

        String domain = "testDomain";
        String participantId = "testparticipantId";
        ProviderQos providerQos = new ProviderQos(new ArrayList<CustomParameter>(), 1, 0L, scope, true);
        EndpointAddressBase endpointAddress = new JoynrMessagingEndpointAddress("testChannel");
        CapabilityEntry capabilityEntry1 = new CapabilityEntry(domain,
                                                               GpsAsync.class,
                                                               providerQos,
                                                               endpointAddress,
                                                               participantId + "1");
        store.add(capabilityEntry1);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        DiscoveryQos discoveryQos = DiscoveryQos.NO_FILTER;

        Collection<CapabilityEntry> newlyEnteredCaps = store.lookup(domain,
                                                                    GpsAsync.INTERFACE_NAME,
                                                                    discoveryQos.getCacheMaxAge());

        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry1, newlyEnteredCaps.iterator().next());

        CapabilityEntry capEntryFake = new CapabilityEntry(domain,
                                                           NavigationAsync.class,
                                                           providerQos,
                                                           endpointAddress,
                                                           participantId);

        store.add(capEntryFake);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(2, capabilities.size());

        newlyEnteredCaps = store.lookup(domain, GpsAsync.INTERFACE_NAME, discoveryQos.getCacheMaxAge());

        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry1, newlyEnteredCaps.iterator().next());

        store.remove(capEntryFake.getParticipantId());

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        CapabilityEntry capabilityEntry2 = new CapabilityEntry(domain, GpsAsync.class, providerQos, participantId + "2");
        capabilityEntry2.addEndpoint(new JoynrMessagingEndpointAddress("testChannel2"));

        store.add(capabilityEntry2);

        // check if newly created Entry overrides old one
        capabilities = store.getAllCapabilities();
        Assert.assertEquals(2, capabilities.size());

        newlyEnteredCaps = store.lookup(domain, GpsAsync.INTERFACE_NAME, discoveryQos.getCacheMaxAge());
        Assert.assertEquals(2, newlyEnteredCaps.size());
        Assert.assertTrue(newlyEnteredCaps.contains(capabilityEntry1));
        Assert.assertTrue(newlyEnteredCaps.contains(capabilityEntry2));

        store.remove(Lists.newArrayList(capabilityEntry1.getParticipantId(), capabilityEntry2.getParticipantId()));
        Assert.assertEquals(0, store.getAllCapabilities().size());
        Assert.assertEquals(0, store.findCapabilitiesForEndpointAddress(endpointAddress, discoveryQos.getCacheMaxAge())
                                    .size());
    }

    @Test
    public void testAddCapabilityEntryWithSameDomainInterfaceNameParticipantIdremovesOldEntry() {
        testAddCapabilityEntryWithSameDomainInterfaceNameParticipantIdremovesOldEntryInternal(ProviderScope.LOCAL);
        testAddCapabilityEntryWithSameDomainInterfaceNameParticipantIdremovesOldEntryInternal(ProviderScope.GLOBAL);
    }

    private void testAddCapabilityEntryWithSameDomainInterfaceNameParticipantIdremovesOldEntryInternal(ProviderScope scope) {
        CapabilitiesStore store = new CapabilitiesStoreImpl();
        HashSet<CapabilityEntry> capabilities = store.getAllCapabilities();
        Assert.assertEquals(0, capabilities.size());

        String domain = "testDomain";
        String participantId = "testparticipantId";
        ProviderQos providerQos = new ProviderQos(new ArrayList<CustomParameter>(), 1, 0L, scope, true);
        EndpointAddressBase endpointAddress = new JoynrMessagingEndpointAddress("testChannel");
        CapabilityEntry capabilityEntry1 = new CapabilityEntry(domain,
                                                               GpsAsync.class,
                                                               providerQos,
                                                               endpointAddress,
                                                               participantId);
        store.add(capabilityEntry1);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        DiscoveryQos discoveryQos = DiscoveryQos.NO_FILTER;

        Collection<CapabilityEntry> newlyEnteredCaps = store.lookup(domain,
                                                                    Gps.INTERFACE_NAME,
                                                                    discoveryQos.getCacheMaxAge());
        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry1, newlyEnteredCaps.iterator().next());

        CapabilityEntry newEnteredCapability = store.lookup(participantId, discoveryQos);
        Assert.assertEquals(capabilityEntry1, newEnteredCapability);

        CapabilityEntry capEntryFake = new CapabilityEntry(domain, GpsAsync.class, providerQos, participantId + "Fake");
        capEntryFake.addEndpoint(new JoynrMessagingEndpointAddress("testChannelFake"));
        store.add(capEntryFake);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(2, capabilities.size());

        store.remove(capEntryFake.getParticipantId());

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        EndpointAddressBase endpointAddress2 = new JoynrMessagingEndpointAddress("testChannelOverride");
        CapabilityEntry capabilityEntry2 = new CapabilityEntry(domain,
                                                               GpsAsync.class,
                                                               providerQos,
                                                               endpointAddress2,
                                                               participantId);
        store.add(capabilityEntry2);

        // check if newly created Entry overrides old one
        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        newlyEnteredCaps = store.lookup(domain, Gps.INTERFACE_NAME, discoveryQos.getCacheMaxAge());
        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry2, newlyEnteredCaps.iterator().next());

        CapabilityEntry newlyEnteredCapability = store.lookup(participantId, discoveryQos);
        Assert.assertEquals(capabilityEntry2, newlyEnteredCapability);
    }
}
