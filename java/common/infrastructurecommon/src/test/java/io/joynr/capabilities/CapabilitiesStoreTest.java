package io.joynr.capabilities;

/*
 * #%L
 * joynr::java::core::libjoynr
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import joynr.types.ProviderQosRequirements;
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
        CapabilityEntry capabilityEntry = new CapabilityEntry(null, GpsAsync.class, "", CapabilityScope.LOCALONLY);
        boolean thrown = false;
        DiscoveryQos discoveryQos = new DiscoveryQos(1000, ArbitrationStrategy.NotSet, 1000);
        try {
            store.registerCapability(capabilityEntry);
            store.findCapabilitiesForInterfaceAddress("hello",
                                                      GpsAsync.INTERFACE_NAME,
                                                      new ProviderQosRequirements(),
                                                      discoveryQos);
        } catch (Exception e) {
            thrown = true;
        }
        Assert.assertTrue(thrown);

    }

    @Test
    public void testRemoveEntryWithReconstructedInfo() {
        CapabilitiesStore store = new CapabilitiesStoreImpl();
        String domain = "testDomain";
        String participantId = "testparticipantId";
        CapabilityEntry capabilityEntry = new CapabilityEntry(domain,
                                                              GpsAsync.class,
                                                              participantId,
                                                              CapabilityScope.LOCALONLY);
        capabilityEntry.addEndpoint(new JoynrMessagingEndpointAddress("testChannel"));
        store.registerCapability(capabilityEntry);

        CapabilityEntry capEntryReconstr = new CapabilityEntry(domain,
                                                               GpsAsync.class,
                                                               participantId,
                                                               CapabilityScope.LOCALONLY);
        Assert.assertTrue(store.removeCapability(capEntryReconstr));

    }

    @Test
    public void testMultipleCapEntryRegistrations() {
        CapabilitiesStore store = new CapabilitiesStoreImpl();
        String domain = "testDomain";
        String participantId = "testparticipantId";

        ProviderQos providerQos = new ProviderQos(new ArrayList<CustomParameter>(), 1, 0L, ProviderScope.GLOBAL, true);
        EndpointAddressBase endpointAddress = new JoynrMessagingEndpointAddress("testChannel");
        CapabilityScope scope = CapabilityScope.REMOTE;
        CapabilityEntry capabilityEntry1 = new CapabilityEntry(domain,
                                                               GpsAsync.class,
                                                               providerQos,
                                                               endpointAddress,
                                                               participantId,
                                                               scope);

        CapabilityEntry capabilityEntry2 = new CapabilityEntry(domain,
                                                               NavigationAsync.class,
                                                               providerQos,
                                                               endpointAddress,
                                                               participantId,
                                                               scope);
        store.registerCapabilities(Lists.newArrayList(capabilityEntry1, capabilityEntry2));
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
                                                               participantId,
                                                               scope);
        store.registerCapability(capabilityEntry1);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        DiscoveryQos discoveryQos = DiscoveryQos.NO_FILTER;

        Collection<CapabilityEntry> newlyEnteredCaps = store.findCapabilitiesForEndpointAddress(endpointAddress,
                                                                                                discoveryQos);

        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry1, newlyEnteredCaps.iterator().next());

        CapabilityEntry capEntryFake = new CapabilityEntry(domain, GpsAsync.class, participantId + "Fake", scope);
        capEntryFake.addEndpoint(new JoynrMessagingEndpointAddress("testChannelFake"));
        store.registerCapability(capEntryFake);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(2, capabilities.size());

        newlyEnteredCaps = store.findCapabilitiesForEndpointAddress(endpointAddress, discoveryQos);

        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry1, newlyEnteredCaps.iterator().next());

        store.removeCapability(capEntryFake);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        CapabilityEntry capabilityEntry2 = new CapabilityEntry(domain,
                                                               NavigationAsync.class,
                                                               providerQos,
                                                               endpointAddress,
                                                               participantId,
                                                               scope);
        store.registerCapability(capabilityEntry2);

        // check if newly created Entry overrides old one
        capabilities = store.getAllCapabilities();
        Assert.assertEquals(2, capabilities.size());

        newlyEnteredCaps = store.findCapabilitiesForEndpointAddress(endpointAddress, discoveryQos);
        Assert.assertEquals(2, newlyEnteredCaps.size());
        Assert.assertTrue(newlyEnteredCaps.contains(capabilityEntry1));
        Assert.assertTrue(newlyEnteredCaps.contains(capabilityEntry2));

        store.removeCapabilities(Lists.newArrayList(capabilityEntry1, capabilityEntry2));
        Assert.assertEquals(0, store.getAllCapabilities().size());
        Assert.assertEquals(0, store.findCapabilitiesForEndpointAddress(endpointAddress, discoveryQos).size());
    }

    @Test
    public void testCapabilitiesStoreInterfaceAddressQuery() {
        testCapabilitiesStoreInterfaceAddressQueryInternal(CapabilityScope.LOCALONLY);
        testCapabilitiesStoreInterfaceAddressQueryInternal(CapabilityScope.LOCALGLOBAL);
        testCapabilitiesStoreInterfaceAddressQueryInternal(CapabilityScope.REMOTE);
    }

    @Test
    public void testCapabilitiesStoreCleanRegisterAndUnregister() {
        CapabilitiesStoreImpl store = new CapabilitiesStoreImpl();
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
                                                               participantId,
                                                               CapabilityScope.LOCALONLY);
        store.registerCapability(capabilityEntry1);
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
        Assert.assertEquals(1, store.participantIdToCapabilityMapping.get(participantId).size());
        Assert.assertTrue(store.participantIdToCapabilityMapping.get(participantId).contains(key1));
        Assert.assertTrue(store.registeredCapabilitiesTime.containsKey(key1));
        Assert.assertEquals(1, store.registeredCapabilitiesTime.size());

        store.removeCapability(capabilityEntry1);
        Assert.assertTrue(store.capabilityKeyToCapabilityMapping.isEmpty());
        Assert.assertTrue(store.capabilityKeyToEndPointAddressMapping.isEmpty());
        Assert.assertTrue(store.endPointAddressToCapabilityMapping.isEmpty());
        Assert.assertTrue(store.interfaceAddressToCapabilityMapping.isEmpty());
        Assert.assertTrue(store.participantIdToCapabilityMapping.isEmpty());
        Assert.assertTrue(store.registeredCapabilitiesTime.isEmpty());
    }

    private void testCapabilitiesStoreInterfaceAddressQueryInternal(CapabilityScope scope) {
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
                                                               participantId,
                                                               scope);
        store.registerCapability(capabilityEntry1);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        ProviderQosRequirements requestedQos = new ProviderQosRequirements();
        DiscoveryQos discoveryQos = DiscoveryQos.NO_FILTER;

        Collection<CapabilityEntry> newlyEnteredCaps = store.findCapabilitiesForInterfaceAddress(domain,
                                                                                                 GpsAsync.INTERFACE_NAME,
                                                                                                 requestedQos,
                                                                                                 discoveryQos);

        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry1, newlyEnteredCaps.iterator().next());

        CapabilityEntry capEntryFake = new CapabilityEntry(domain,
                                                           NavigationAsync.class,
                                                           providerQos,
                                                           endpointAddress,
                                                           participantId,
                                                           scope);

        store.registerCapability(capEntryFake);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(2, capabilities.size());

        newlyEnteredCaps = store.findCapabilitiesForInterfaceAddress(domain,
                                                                     GpsAsync.INTERFACE_NAME,
                                                                     requestedQos,
                                                                     discoveryQos);

        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry1, newlyEnteredCaps.iterator().next());

        store.removeCapability(capEntryFake);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        CapabilityEntry capabilityEntry2 = new CapabilityEntry(domain, GpsAsync.class, participantId + "2", scope);
        capabilityEntry2.addEndpoint(new JoynrMessagingEndpointAddress("testChannel2"));

        store.registerCapability(capabilityEntry2);

        // check if newly created Entry overrides old one
        capabilities = store.getAllCapabilities();
        Assert.assertEquals(2, capabilities.size());

        newlyEnteredCaps = store.findCapabilitiesForInterfaceAddress(domain,
                                                                     GpsAsync.INTERFACE_NAME,
                                                                     requestedQos,
                                                                     discoveryQos);
        Assert.assertEquals(2, newlyEnteredCaps.size());
        Assert.assertTrue(newlyEnteredCaps.contains(capabilityEntry1));
        Assert.assertTrue(newlyEnteredCaps.contains(capabilityEntry2));

        store.removeCapabilities(Lists.newArrayList(capabilityEntry1, capabilityEntry2));
        Assert.assertEquals(0, store.getAllCapabilities().size());
        Assert.assertEquals(0, store.findCapabilitiesForEndpointAddress(endpointAddress, discoveryQos).size());
    }

    @Test
    public void testAddCapabilityEntryWithSameDomainInterfaceNameParticipantIdremovesOldEntry() {
        testAddCapabilityEntryWithSameDomainInterfaceNameParticipantIdremovesOldEntryInternal(CapabilityScope.LOCALONLY);
        testAddCapabilityEntryWithSameDomainInterfaceNameParticipantIdremovesOldEntryInternal(CapabilityScope.LOCALGLOBAL);
        testAddCapabilityEntryWithSameDomainInterfaceNameParticipantIdremovesOldEntryInternal(CapabilityScope.REMOTE);
    }

    private void testAddCapabilityEntryWithSameDomainInterfaceNameParticipantIdremovesOldEntryInternal(CapabilityScope scope) {
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
                                                               participantId,
                                                               scope);
        store.registerCapability(capabilityEntry1);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        ProviderQosRequirements requestedQos = new ProviderQosRequirements();
        DiscoveryQos discoveryQos = DiscoveryQos.NO_FILTER;

        Collection<CapabilityEntry> newlyEnteredCaps = store.findCapabilitiesForInterfaceAddress(domain,
                                                                                                 Gps.INTERFACE_NAME,
                                                                                                 requestedQos,
                                                                                                 discoveryQos);
        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry1, newlyEnteredCaps.iterator().next());

        newlyEnteredCaps = store.findCapabilitiesForParticipantId(participantId, discoveryQos);
        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry1, newlyEnteredCaps.iterator().next());

        CapabilityEntry capEntryFake = new CapabilityEntry(domain, GpsAsync.class, participantId + "Fake", scope);
        capEntryFake.addEndpoint(new JoynrMessagingEndpointAddress("testChannelFake"));
        store.registerCapability(capEntryFake);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(2, capabilities.size());

        store.removeCapability(capEntryFake);

        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        EndpointAddressBase endpointAddress2 = new JoynrMessagingEndpointAddress("testChannelOverride");
        CapabilityEntry capabilityEntry2 = new CapabilityEntry(domain,
                                                               GpsAsync.class,
                                                               providerQos,
                                                               endpointAddress2,
                                                               participantId,
                                                               scope);
        store.registerCapability(capabilityEntry2);

        // check if newly created Entry overrides old one
        capabilities = store.getAllCapabilities();
        Assert.assertEquals(1, capabilities.size());

        newlyEnteredCaps = store.findCapabilitiesForInterfaceAddress(domain,
                                                                     Gps.INTERFACE_NAME,
                                                                     requestedQos,
                                                                     discoveryQos);
        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry2, newlyEnteredCaps.iterator().next());

        newlyEnteredCaps = store.findCapabilitiesForParticipantId(participantId, discoveryQos);
        Assert.assertEquals(1, newlyEnteredCaps.size());
        Assert.assertEquals(capabilityEntry2, newlyEnteredCaps.iterator().next());
    }
}
