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
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import org.junit.Before;
import org.junit.Test;

import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;

import io.joynr.util.ObjectMapper;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

/**
 * Unit tests for {@link CapabilitiesUtils}.
 */
public class CapabilitiesUtilsTest {

    @Before
    public void setUp() throws Exception {
        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
        Field objectMapperField = CapabilityUtils.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(CapabilityUtils.class, objectMapper);
    }

    @Test
    public void testCreateNewGlobalDiscoveryEntry() {
        Version version = new Version(0, 0);
        String domain = "domain";
        String interfaceName = "interfaceName";
        String participantId = "participantId";
        String publicKeyId = "publicKeyId";
        Address mqttAddress = new MqttAddress("tcp://broker:1883", "topic");
        ProviderQos providerQos = new ProviderQos();

        GlobalDiscoveryEntry result = CapabilityUtils.newGlobalDiscoveryEntry(version,
                                                                              domain,
                                                                              interfaceName,
                                                                              participantId,
                                                                              providerQos,
                                                                              0L,
                                                                              0L,
                                                                              publicKeyId,
                                                                              mqttAddress);

        assertNotNull(result);
        assertEquals(version, result.getProviderVersion());
        assertEquals(domain, result.getDomain());
        assertEquals(interfaceName, result.getInterfaceName());
        assertEquals(participantId, result.getParticipantId());
        assertEquals(providerQos, result.getQos());
        assertEquals((Long) 0L, result.getLastSeenDateMs());
        assertEquals((Long) 0L, result.getExpiryDateMs());
        assertEquals(publicKeyId, result.getPublicKeyId());
        assertEquals("{\"_typeName\":\"joynr.system.RoutingTypes.MqttAddress\",\"brokerUri\":\"tcp://broker:1883\",\"topic\":\"topic\"}",
                     result.getAddress());
    }

    @Test
    public void testGetMqttAddressFromGlobalDiscoveryEntry() {
        GlobalDiscoveryEntry globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(0, 0),
                                                                             "domain",
                                                                             "interfaceName",
                                                                             "participantId",
                                                                             new ProviderQos(),
                                                                             0L,
                                                                             0L,
                                                                             "publicKeyId",
                                                                             "{\"_typeName\":\"joynr.system.RoutingTypes.MqttAddress\",\"brokerUri\":\"tcp://broker:1883\",\"topic\":\"topic\"}");
        Address result = CapabilityUtils.getAddressFromGlobalDiscoveryEntry(globalDiscoveryEntry);

        assertTrue(result instanceof MqttAddress);
        assertEquals("tcp://broker:1883", ((MqttAddress) result).getBrokerUri());
        assertEquals("topic", ((MqttAddress) result).getTopic());
    }

    private void compareDiscoveryEntries(boolean isLocal, DiscoveryEntry de, DiscoveryEntryWithMetaInfo dewmi) {
        assertTrue(de.getProviderVersion().equals(dewmi.getProviderVersion())
                && de.getDomain().equals(dewmi.getDomain()) && de.getInterfaceName().equals(dewmi.getInterfaceName())
                && de.getParticipantId().equals(dewmi.getParticipantId()) && de.getQos().equals(dewmi.getQos())
                && de.getLastSeenDateMs().equals(dewmi.getLastSeenDateMs())
                && de.getExpiryDateMs().equals(dewmi.getExpiryDateMs())
                && de.getPublicKeyId().equals(dewmi.getPublicKeyId()) && isLocal == dewmi.getIsLocal());
    }

    @Test
    public void testConvertLocalDiscoveryEntryToDiscoveryEntryWithMetaInfo() {
        boolean isLocal = true;
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(42, 23),
                                                           "testDomain",
                                                           "testInterfaceName",
                                                           "testParticipantId",
                                                           new ProviderQos(),
                                                           4711l,
                                                           4712l,
                                                           "testPublicKeyId");
        DiscoveryEntryWithMetaInfo convertedDiscoveryEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(isLocal,
                                                                                                                 discoveryEntry);
        compareDiscoveryEntries(isLocal, discoveryEntry, convertedDiscoveryEntry);
    }

    @Test
    public void testConvertGlobalDiscoveryEntryToDiscoveryEntryWithMetaInfo() {
        boolean isLocal = false;
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(42, 23),
                                                           "testDomain",
                                                           "testInterfaceName",
                                                           "testParticipantId",
                                                           new ProviderQos(),
                                                           4711l,
                                                           4712l,
                                                           "testPublicKeyId");
        GlobalDiscoveryEntry globalDiscoveryEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry,
                                                                                                        new MqttAddress());
        DiscoveryEntryWithMetaInfo convertedEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(isLocal,
                                                                                                        discoveryEntry);
        compareDiscoveryEntries(isLocal, discoveryEntry, convertedEntry);
        DiscoveryEntryWithMetaInfo convertedGlobalEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(isLocal,
                                                                                                              globalDiscoveryEntry);
        compareDiscoveryEntries(isLocal, globalDiscoveryEntry, convertedGlobalEntry);
    }

    private Collection<DiscoveryEntry> createCollectionOfDiscoveryEntries() {
        return createCollectionOfDiscoveryEntriesWithMetaInfo().stream()
                                                               .map(entryWithMetaInfo -> new DiscoveryEntry(entryWithMetaInfo))
                                                               .collect(Collectors.toList());
    }

    private void compareCollectionOfDiscoveryEntriesWithMetaInfo(boolean isLocal,
                                                                 Collection<DiscoveryEntry> discoveryEntries,
                                                                 Collection<DiscoveryEntryWithMetaInfo> convertedEntries) {
        int numberOfValidConversions = 0;
        assertEquals(discoveryEntries.size(), convertedEntries.size());
        for (DiscoveryEntry discoveryEntry : discoveryEntries) {
            for (DiscoveryEntryWithMetaInfo convertedEntry : convertedEntries) {
                if (convertedEntry.getParticipantId().equals(discoveryEntry.getParticipantId())) {
                    assertTrue(DiscoveryEntryWithMetaInfo.class.equals(convertedEntry.getClass()));
                    compareDiscoveryEntries(isLocal, discoveryEntry, convertedEntry);
                    numberOfValidConversions++;
                    break;
                }
                continue;
            }
        }
        assertEquals(discoveryEntries.size(), numberOfValidConversions);
    }

    @Test
    public void testConvertCollectionOfLocalDiscoveryEntriesToListOfDiscoveryEntriesWithMetaInfo() {
        boolean isLocal = true;
        Collection<DiscoveryEntry> discoveryEntries = createCollectionOfDiscoveryEntries();
        List<DiscoveryEntryWithMetaInfo> convertedEntries = CapabilityUtils.convertToDiscoveryEntryWithMetaInfoList(isLocal,
                                                                                                                    discoveryEntries);
        compareCollectionOfDiscoveryEntriesWithMetaInfo(isLocal, discoveryEntries, convertedEntries);
    }

    @Test
    public void testConvertCollectionOfGlobalDiscoveryEntriesToListOfDiscoveryEntriesWithMetaInfo() {
        boolean isLocal = false;
        Collection<DiscoveryEntry> discoveryEntries = createCollectionOfDiscoveryEntries();
        List<DiscoveryEntryWithMetaInfo> convertedEntries = CapabilityUtils.convertToDiscoveryEntryWithMetaInfoList(isLocal,
                                                                                                                    discoveryEntries);
        compareCollectionOfDiscoveryEntriesWithMetaInfo(isLocal, discoveryEntries, convertedEntries);
    }

    @Test
    public void testConvertCollectionOfLocalDiscoveryEntriesToSetOfDiscoveryEntriesWithMetaInfo() {
        boolean isLocal = true;
        Collection<DiscoveryEntry> discoveryEntries = createCollectionOfDiscoveryEntries();
        Set<DiscoveryEntryWithMetaInfo> convertedEntries = CapabilityUtils.convertToDiscoveryEntryWithMetaInfoSet(isLocal,
                                                                                                                  discoveryEntries);
        compareCollectionOfDiscoveryEntriesWithMetaInfo(isLocal, discoveryEntries, convertedEntries);
    }

    @Test
    public void testConvertCollectionOfGlobalDiscoveryEntriesToSetOfDiscoveryEntriesWithMetaInfo() {
        boolean isLocal = false;
        Collection<DiscoveryEntry> discoveryEntries = createCollectionOfDiscoveryEntries();
        Set<DiscoveryEntryWithMetaInfo> convertedEntries = CapabilityUtils.convertToDiscoveryEntryWithMetaInfoSet(isLocal,
                                                                                                                  discoveryEntries);
        compareCollectionOfDiscoveryEntriesWithMetaInfo(isLocal, discoveryEntries, convertedEntries);
    }

    @Test
    public void testConvertDiscoveryEntryWithMetaInfoToDiscoveryEntry() {
        boolean isLocal = true;
        DiscoveryEntryWithMetaInfo discoveryEntry = new DiscoveryEntryWithMetaInfo(new Version(42, 23),
                                                                                   "testDomain",
                                                                                   "testInterfaceName",
                                                                                   "testParticipantId",
                                                                                   new ProviderQos(),
                                                                                   4711l,
                                                                                   4712l,
                                                                                   "testPublicKeyId",
                                                                                   isLocal);
        DiscoveryEntry convertedEntry = CapabilityUtils.convertToDiscoveryEntry(discoveryEntry);
        assertTrue(DiscoveryEntry.class.equals(convertedEntry.getClass()));
        compareDiscoveryEntries(isLocal, convertedEntry, discoveryEntry);
    }

    private Collection<DiscoveryEntryWithMetaInfo> createCollectionOfDiscoveryEntriesWithMetaInfo() {
        Collection<DiscoveryEntryWithMetaInfo> discoveryEntries = new ArrayList<>(2);
        discoveryEntries.add(new DiscoveryEntryWithMetaInfo(new Version(42, 23),
                                                            "testDomain1",
                                                            "testInterfaceName",
                                                            "testParticipantId1",
                                                            new ProviderQos(),
                                                            4711l,
                                                            4712l,
                                                            "testPublicKeyId1",
                                                            true));
        discoveryEntries.add(new DiscoveryEntryWithMetaInfo(new Version(42, 23),
                                                            "testDomain2",
                                                            "testInterfaceName",
                                                            "testParticipantId2",
                                                            new ProviderQos(),
                                                            4721l,
                                                            4722l,
                                                            "testPublicKeyId2",
                                                            false));
        return discoveryEntries;
    }

    private void compareCollectionOfDiscoveryEntries(Collection<DiscoveryEntry> convertedEntries,
                                                     Collection<DiscoveryEntryWithMetaInfo> discoveryEntries) {
        int numberOfValidConversions = 0;
        assertEquals(discoveryEntries.size(), convertedEntries.size());
        for (DiscoveryEntryWithMetaInfo discoveryEntry : discoveryEntries) {
            for (DiscoveryEntry convertedEntry : convertedEntries) {
                if (convertedEntry.getParticipantId().equals(discoveryEntry.getParticipantId())) {
                    assertTrue(DiscoveryEntry.class.equals(convertedEntry.getClass()));
                    compareDiscoveryEntries(discoveryEntry.getIsLocal(), convertedEntry, discoveryEntry);
                    numberOfValidConversions++;
                    break;
                }
                continue;
            }
        }
        assertEquals(discoveryEntries.size(), numberOfValidConversions);
    }

    @Test
    public void testConvertCollectionOfDiscoveryEntriesWithMetaInfoToListOfDiscoveryEntries() {
        Collection<DiscoveryEntryWithMetaInfo> discoveryEntries = createCollectionOfDiscoveryEntriesWithMetaInfo();
        List<DiscoveryEntry> convertedEntries = CapabilityUtils.convertToDiscoveryEntryList(discoveryEntries);
        compareCollectionOfDiscoveryEntries(convertedEntries, discoveryEntries);
    }

    @Test
    public void testConvertCollectionOfDiscoveryEntriesWithMetaInfoToSetOfDiscoveryEntries() {
        Collection<DiscoveryEntryWithMetaInfo> discoveryEntries = createCollectionOfDiscoveryEntriesWithMetaInfo();
        Set<DiscoveryEntry> convertedEntries = CapabilityUtils.convertToDiscoveryEntrySet(discoveryEntries);
        compareCollectionOfDiscoveryEntries(convertedEntries, discoveryEntries);
    }
}
