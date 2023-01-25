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
package io.joynr.capabilities.directory;

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import io.joynr.capabilities.CapabilityUtils;
import io.joynr.exceptions.JoynrException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.provider.PromiseKeeper;
import io.joynr.util.ObjectMapper;
import joynr.exceptions.ApplicationException;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

public class CapabilitiesDirectoryIntegrationTest {

    private static final CustomParameter[] CUSTOM_PARAMETERS = {};

    private static final long ONE_DAY_IN_MS = 24 * 60 * 60 * 1000;
    private static final int PROMISE_SETTLEMENT_TIMEOUT_MS = 1000;
    private static final String gcdGbid = "joynrGcdTestGbid";

    private static CapabilitiesDirectoryImpl capabilitiesDirectory;

    String url = "tcp://testUrl";
    String topic = "testTopic";
    Address mqttAddress = new MqttAddress(url, topic);
    String mqttAddressSerialized;
    String domain = "com";
    String interface1 = "interface1";
    String interface2 = "interface2";
    String interface3 = "Interface3";
    String interface4 = "Interface4";
    String participantId1 = "testParticipantId1_" + createUuidString();
    String participantId2 = "testParticipantId2_" + createUuidString();
    String participantId3 = "testParticipantId3_" + createUuidString();
    String participantId4 = "testParticipantId4_" + createUuidString();

    ProviderQos providerQos = new ProviderQos(CUSTOM_PARAMETERS, 1L, ProviderScope.GLOBAL, true);
    GlobalDiscoveryEntry discoveryEntry1;
    GlobalDiscoveryEntry discoveryEntry2;
    GlobalDiscoveryEntry discoveryEntry3;
    GlobalDiscoveryEntry discoveryEntry4;
    GlobalDiscoveryEntry discoveryEntry4FromAnotherNodeInCluster;
    String postFix = "" + System.currentTimeMillis();

    @BeforeClass
    public static void start() throws IOException, InterruptedException {
        Properties systemProperties = System.getProperties();
        systemProperties.setProperty(CapabilitiesDirectoryImpl.GCD_GBID, gcdGbid);
        systemProperties.setProperty(ConfigurableMessagingSettings.PROPERTY_GBIDS, gcdGbid);
        System.setProperties(systemProperties);
        capabilitiesDirectory = startCapabilitiesDirectory();
    }

    @Before
    public void setUp() throws Exception {
        mqttAddressSerialized = new ObjectMapper().writeValueAsString(mqttAddress);
        String publicKeyId = "publicKeyId";
        String publicKeyIdFromAnotherNodeInCluster = "publicKeyIdAnotherNode";

        long lastSeenDateMs = System.currentTimeMillis();
        long expiryDateMs = System.currentTimeMillis() + ONE_DAY_IN_MS;
        discoveryEntry1 = new GlobalDiscoveryEntry(new Version(47, 11),
                                                   domain,
                                                   interface1,
                                                   participantId1,
                                                   providerQos,
                                                   lastSeenDateMs,
                                                   expiryDateMs,
                                                   publicKeyId,
                                                   mqttAddressSerialized);
        discoveryEntry2 = new GlobalDiscoveryEntry(new Version(47, 11),
                                                   domain,
                                                   interface2,
                                                   participantId2,
                                                   providerQos,
                                                   lastSeenDateMs,
                                                   expiryDateMs,
                                                   publicKeyId,
                                                   mqttAddressSerialized);
        discoveryEntry3 = new GlobalDiscoveryEntry(new Version(47, 11),
                                                   domain,
                                                   interface3,
                                                   participantId3,
                                                   providerQos,
                                                   lastSeenDateMs,
                                                   expiryDateMs,
                                                   publicKeyId,
                                                   mqttAddressSerialized);
        discoveryEntry4 = new GlobalDiscoveryEntry(new Version(47, 11),
                                                   domain,
                                                   interface4,
                                                   participantId4,
                                                   providerQos,
                                                   lastSeenDateMs,
                                                   expiryDateMs,
                                                   publicKeyId,
                                                   mqttAddressSerialized);
        discoveryEntry4FromAnotherNodeInCluster = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                           domain,
                                                                           interface4,
                                                                           participantId4,
                                                                           providerQos,
                                                                           lastSeenDateMs + 5000,
                                                                           expiryDateMs + 5000,
                                                                           publicKeyIdFromAnotherNodeInCluster,
                                                                           mqttAddressSerialized);

    }

    private static CapabilitiesDirectoryImpl startCapabilitiesDirectory() throws IOException, InterruptedException {
        Properties testProperties = new Properties();
        testProperties.setProperty(CapabilitiesDirectoryLauncher.GCD_DB_NAME, "gcd-test");
        Map<String, String> jpaProperties = new HashMap<String, String>();
        // create-drop: drop the schema at the end of the session
        jpaProperties.put("hibernate.hbm2ddl.auto", "create-drop");
        testProperties.put(CapabilitiesDirectoryLauncher.GCD_JPA_PROPERTIES, jpaProperties);
        CapabilitiesDirectoryLauncher.start(testProperties);
        return CapabilitiesDirectoryLauncher.getCapabilitiesDirectory();
    }

    @After
    public void tearDown() {
        capabilitiesDirectory.remove(new String[]{ participantId1, participantId2, participantId3, participantId4 });
    }

    @AfterClass
    public static void stop() {
        CapabilitiesDirectoryLauncher.shutdown();
    }

    private void checkDiscoveryEntry(GlobalDiscoveryEntry expected, GlobalDiscoveryEntry actual, String expectedGbid) {
        assertEquals(expected.getParticipantId(), actual.getParticipantId());
        assertEquals(expected.getInterfaceName(), actual.getInterfaceName());
        assertEquals(expected.getDomain(), actual.getDomain());
        assertEquals(expected.getExpiryDateMs(), actual.getExpiryDateMs());
        assertEquals(expected.getLastSeenDateMs(), actual.getLastSeenDateMs());
        assertEquals(expected.getProviderVersion(), actual.getProviderVersion());
        assertEquals(expected.getQos(), actual.getQos());

        // The (Mqtt) address is not equal because the brokerUri is replaced with the GCD's GBID
        assertNotEquals(expected.getAddress(), actual.getAddress());
        assertEquals(expectedGbid,
                     ((MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(actual)).getBrokerUri());
        assertEquals(topic, ((MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(actual)).getTopic());
    }

    @Test
    public void addMultipleCapabilitiesAsArray() throws InterruptedException {
        GlobalDiscoveryEntry expectedDiscoveryEntry2 = new GlobalDiscoveryEntry(discoveryEntry2);
        GlobalDiscoveryEntry expectedDiscoveryEntry3 = new GlobalDiscoveryEntry(discoveryEntry3);

        GlobalDiscoveryEntry[] interfaces2And3 = { discoveryEntry2, discoveryEntry3 };
        capabilitiesDirectory.add(interfaces2And3);

        PromiseKeeper lookupCapInfo2 = new PromiseKeeper();
        capabilitiesDirectory.lookup(new String[]{ domain }, interface2).then(lookupCapInfo2);
        lookupCapInfo2.waitForSettlement(PROMISE_SETTLEMENT_TIMEOUT_MS);
        Object[] objectArray2 = lookupCapInfo2.getValues().get();
        GlobalDiscoveryEntry[] discoveredEntries2 = (GlobalDiscoveryEntry[]) objectArray2[0];
        assertEquals(1, discoveredEntries2.length);
        checkDiscoveryEntry(expectedDiscoveryEntry2, discoveredEntries2[0], gcdGbid);

        PromiseKeeper lookupCapInfo3 = new PromiseKeeper();
        capabilitiesDirectory.lookup(new String[]{ domain }, interface3).then(lookupCapInfo3);

        lookupCapInfo3.waitForSettlement(PROMISE_SETTLEMENT_TIMEOUT_MS);
        GlobalDiscoveryEntry[] discoveredEntries3 = (GlobalDiscoveryEntry[]) lookupCapInfo3.getValues().get()[0];
        assertEquals(1, discoveredEntries3.length);
        checkDiscoveryEntry(expectedDiscoveryEntry3, discoveredEntries3[0], gcdGbid);
    }

    @Test
    public void addAndLookupAndRemoveWithGbids_validGbid() throws InterruptedException {
        String[] gbids = { gcdGbid };
        GlobalDiscoveryEntry expectedDiscoveryEntry = new GlobalDiscoveryEntry(discoveryEntry1);

        PromiseKeeper addPromiseKeeper = new PromiseKeeper();
        capabilitiesDirectory.add(discoveryEntry1, gbids).then(addPromiseKeeper);
        addPromiseKeeper.waitForSettlement(PROMISE_SETTLEMENT_TIMEOUT_MS);
        assertTrue(addPromiseKeeper.isFulfilled());

        PromiseKeeper lookupPromiseKeeper1 = new PromiseKeeper();
        capabilitiesDirectory.lookup(new String[]{ domain }, interface1, gbids).then(lookupPromiseKeeper1);
        lookupPromiseKeeper1.waitForSettlement(PROMISE_SETTLEMENT_TIMEOUT_MS);
        assertTrue(lookupPromiseKeeper1.isFulfilled());
        GlobalDiscoveryEntry[] discoveredEntries = (GlobalDiscoveryEntry[]) lookupPromiseKeeper1.getValues().get()[0];
        assertEquals(1, discoveredEntries.length);
        checkDiscoveryEntry(expectedDiscoveryEntry, discoveredEntries[0], gbids[0]);

        PromiseKeeper removePromiseKeeper = new PromiseKeeper();
        capabilitiesDirectory.remove(participantId1, gbids).then(removePromiseKeeper);
        removePromiseKeeper.waitForSettlement(PROMISE_SETTLEMENT_TIMEOUT_MS);
        assertTrue(removePromiseKeeper.isFulfilled());

        PromiseKeeper lookupPromiseKeeper2 = new PromiseKeeper();
        capabilitiesDirectory.lookup(new String[]{ domain }, interface1, gbids).then(lookupPromiseKeeper2);
        lookupPromiseKeeper2.waitForSettlement(PROMISE_SETTLEMENT_TIMEOUT_MS);
        assertTrue(lookupPromiseKeeper2.isFulfilled());
        assertArrayEquals(new GlobalDiscoveryEntry[]{},
                          (GlobalDiscoveryEntry[]) lookupPromiseKeeper2.getValues().get()[0]);
    }

    @Test
    public void addAndLookupAndRemoveWithGbids_unknownGbid() throws InterruptedException {
        String gbids[] = { "unknownGbid" };

        PromiseKeeper addPromiseKeeper = new PromiseKeeper();
        capabilitiesDirectory.add(discoveryEntry1, gbids).then(addPromiseKeeper);
        addPromiseKeeper.waitForSettlement(PROMISE_SETTLEMENT_TIMEOUT_MS);
        assertTrue(addPromiseKeeper.isRejected());
        JoynrException error = addPromiseKeeper.getError().get();
        assertTrue(error instanceof ApplicationException);
        assertEquals(DiscoveryError.UNKNOWN_GBID, ((ApplicationException) error).getError());

        PromiseKeeper lookupPromiseKeeper = new PromiseKeeper();
        capabilitiesDirectory.lookup(new String[]{ domain }, interface1, gbids).then(lookupPromiseKeeper);
        lookupPromiseKeeper.waitForSettlement(PROMISE_SETTLEMENT_TIMEOUT_MS);
        assertTrue(lookupPromiseKeeper.isRejected());
        error = lookupPromiseKeeper.getError().get();
        assertTrue(error instanceof ApplicationException);
        assertEquals(DiscoveryError.UNKNOWN_GBID, ((ApplicationException) error).getError());

        PromiseKeeper removePromiseKeeper = new PromiseKeeper();
        capabilitiesDirectory.remove(participantId1, gbids).then(removePromiseKeeper);
        removePromiseKeeper.waitForSettlement(PROMISE_SETTLEMENT_TIMEOUT_MS);
        assertTrue(removePromiseKeeper.isRejected());
        error = removePromiseKeeper.getError().get();
        assertTrue(error instanceof ApplicationException);
        assertEquals(DiscoveryError.UNKNOWN_GBID, ((ApplicationException) error).getError());

    }

    @Test
    public void addAndLookup() throws Exception {
        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(discoveryEntry1);
        capabilitiesDirectory.add(discoveryEntry1);

        PromiseKeeper lookupCapInfo1 = new PromiseKeeper();
        capabilitiesDirectory.lookup(new String[]{ domain }, interface1).then(lookupCapInfo1);
        lookupCapInfo1.waitForSettlement(PROMISE_SETTLEMENT_TIMEOUT_MS);
        GlobalDiscoveryEntry[] discoveredEntries = (GlobalDiscoveryEntry[]) lookupCapInfo1.getValues().get()[0];
        assertEquals(1, discoveredEntries.length);
        checkDiscoveryEntry(expectedEntry, discoveredEntries[0], gcdGbid);
    }

    @Test
    public void registerSameProviderMultipleTimesFromClusteredApplication() throws InterruptedException {
        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(discoveryEntry4FromAnotherNodeInCluster);
        capabilitiesDirectory.add(discoveryEntry4);
        capabilitiesDirectory.add(discoveryEntry4FromAnotherNodeInCluster);

        PromiseKeeper lookupCapInfo4 = new PromiseKeeper();
        capabilitiesDirectory.lookup(new String[]{ domain }, interface4).then(lookupCapInfo4);
        lookupCapInfo4.waitForSettlement(PROMISE_SETTLEMENT_TIMEOUT_MS);
        GlobalDiscoveryEntry[] discoveredEntries = (GlobalDiscoveryEntry[]) lookupCapInfo4.getValues().get()[0];
        assertEquals(1, discoveredEntries.length);
        checkDiscoveryEntry(expectedEntry, discoveredEntries[0], gcdGbid);
    }
}
