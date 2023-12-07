/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.test.gcd;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Arrays;
import java.util.Map;
import java.util.Map.Entry;
import java.util.TreeMap;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.util.ObjectMapper;
import io.joynr.messaging.MessagingQos;

import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;

import joynr.exceptions.ApplicationException;
import joynr.Message;
import joynr.system.RoutingTypes.Address;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

public class GcdConsumerSyncMethodTest extends GcdConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(GcdConsumerSyncMethodTest.class);

    @Rule
    public GcdTestLoggingRule joynrTestRule = new GcdTestLoggingRule(logger);

    private static final long ONE_DAY_IN_MS = 1 * 24 * 60 * 60 * 1000;
    private static String localChannelId;
    private static String[] gbidArray; // gbids of all brokers
    private static String[] gbidArray1; // gbids of only 1st broker
    private static String[] gbidArray2; // gbids of only 2nd broker
    private static String[] gbidArrayUnknown; // gbids of 1st and unknown broker
    private static String[] brokerUriArray;
    private static String brokerUris;
    private static boolean multipleBrokers;
    private static String domain1 = "domain1";
    private static String domain2 = "domain2";
    private static String interfaceName1 = "interfaceName1";
    private static String interfaceName2 = "interfaceName2";
    private static String participantId1 = "participantId1";
    private static String participantId2 = "participantId2";
    private static String participantId3 = "participantId3";
    private static String participantId4 = "participantId4";
    private static String participantId5 = "participantId5";
    private static String participantId6 = "participantId6";
    private static Version versionV10 = new Version(1, 0);
    private static Version versionV11 = new Version(1, 1);
    private static Version versionV20 = new Version(2, 0);
    private static GlobalDiscoveryEntry discoveryEntry_V10_D1_I1_P1_G1;
    private static GlobalDiscoveryEntry discoveryEntry_V10_D1_I1_P1_G2;
    private static GlobalDiscoveryEntry discoveryEntry_V10_D1_I1_P2;
    private static GlobalDiscoveryEntry discoveryEntry_V11_D1_I1_P3;
    private static GlobalDiscoveryEntry discoveryEntry_V20_D1_I1_P4;
    private static GlobalDiscoveryEntry discoveryEntry_V10_D2_I1_P5;
    private static GlobalDiscoveryEntry discoveryEntry_V10_D1_I2_P6;
    private static boolean expectSuccess = true;
    private static boolean expectFailure = false;
    private static DiscoveryError noError = null;

    private static void createGlobalDiscoveryEntries() {
        GlobalDiscoveryEntry discoveryEntry1;

        CustomParameter[] CUSTOM_PARAMETERS = {};
        ProviderQos providerQos = new ProviderQos(CUSTOM_PARAMETERS, 1L, ProviderScope.GLOBAL, true);

        long lastSeenDateMs1 = System.currentTimeMillis();
        long expiryDateMs1 = System.currentTimeMillis() + ONE_DAY_IN_MS;
        String publicKeyId1 = "publicKeyId";

        // always use default GBID for MqttAddress
        String url1 = gbidArray[0];
        String url2 = (gbidArray.length > 1) ? gbidArray[1] : "";
        String topic = localChannelId;
        Address mqttAddress1 = new MqttAddress(url1, topic);
        Address mqttAddress2 = new MqttAddress(url2, topic);
        String mqttAddressSerialized1;
        String mqttAddressSerialized2;
        try {
            mqttAddressSerialized1 = new ObjectMapper().writeValueAsString(mqttAddress1);
            mqttAddressSerialized2 = new ObjectMapper().writeValueAsString(mqttAddress2);
        } catch (Exception e) {
            fail("ObjectMapper failed to serialize mqttAddress1/mqtAddress2");
        }

        // V1.0
        discoveryEntry_V10_D1_I1_P1_G1 = new GlobalDiscoveryEntry(versionV10,
                                                                  domain1,
                                                                  interfaceName1,
                                                                  participantId1,
                                                                  providerQos,
                                                                  lastSeenDateMs1,
                                                                  expiryDateMs1,
                                                                  publicKeyId1,
                                                                  mqttAddressSerialized1);

        // V1.0 with gbid2, same participantId1
        discoveryEntry_V10_D1_I1_P1_G2 = new GlobalDiscoveryEntry(versionV10,
                                                                  domain1,
                                                                  interfaceName1,
                                                                  participantId1,
                                                                  providerQos,
                                                                  lastSeenDateMs1,
                                                                  expiryDateMs1,
                                                                  publicKeyId1,
                                                                  mqttAddressSerialized2);

        // V1.0, different participantId2
        discoveryEntry_V10_D1_I1_P2 = new GlobalDiscoveryEntry(versionV10,
                                                               domain1,
                                                               interfaceName1,
                                                               participantId2,
                                                               providerQos,
                                                               lastSeenDateMs1,
                                                               expiryDateMs1,
                                                               publicKeyId1,
                                                               mqttAddressSerialized1);

        // V1.1, different participantId3
        discoveryEntry_V11_D1_I1_P3 = new GlobalDiscoveryEntry(versionV11,
                                                               domain1,
                                                               interfaceName1,
                                                               participantId3,
                                                               providerQos,
                                                               lastSeenDateMs1,
                                                               expiryDateMs1,
                                                               publicKeyId1,
                                                               mqttAddressSerialized1);

        // V2.0, different participantId4
        discoveryEntry_V20_D1_I1_P4 = new GlobalDiscoveryEntry(versionV20,
                                                               domain1,
                                                               interfaceName1,
                                                               participantId4,
                                                               providerQos,
                                                               lastSeenDateMs1,
                                                               expiryDateMs1,
                                                               publicKeyId1,
                                                               mqttAddressSerialized1);

        // different domain2
        discoveryEntry_V10_D2_I1_P5 = new GlobalDiscoveryEntry(versionV10,
                                                               domain2,
                                                               interfaceName1,
                                                               participantId5,
                                                               providerQos,
                                                               lastSeenDateMs1,
                                                               expiryDateMs1,
                                                               publicKeyId1,
                                                               mqttAddressSerialized1);

        // different interface2
        discoveryEntry_V10_D1_I2_P6 = new GlobalDiscoveryEntry(versionV10,
                                                               domain1,
                                                               interfaceName2,
                                                               participantId6,
                                                               providerQos,
                                                               lastSeenDateMs1,
                                                               expiryDateMs1,
                                                               publicKeyId1,
                                                               mqttAddressSerialized1);
    }

    private static String[] getDomainsFromGlobalDiscoveryEntry(GlobalDiscoveryEntry globalDiscoveryEntry) {
        String[] domains = new String[1];
        domains[0] = globalDiscoveryEntry.getDomain();
        return domains;
    }

    @BeforeClass
    public static void setUp() throws Exception {
        logger.info("setUp: Entering");
        setupConsumerRuntime(false);

        // get CHANNELID; this also serves as clusterControllerId
        // see GlobalCapabilitiesDirectoryClient.java
        localChannelId = System.getenv("GCD_TEST_CHANNELID");

        // get some data from env, the setupConsumerRuntime has already checked for presence
        brokerUris = System.getenv("GCD_TEST_BROKERURIS");
        brokerUriArray = Arrays.stream(brokerUris.split(",")).map(a -> a.trim()).toArray(String[]::new);

        String gbids = System.getenv("GCD_TEST_GBIDS");
        // all brokers
        gbidArray = Arrays.stream(gbids.split(",")).map(a -> a.trim()).toArray(String[]::new);
        // only first broker
        gbidArray1 = new String[1];
        gbidArray1[0] = gbidArray[0];

        if (gbidArray.length > 1) {
            multipleBrokers = true;

            // only second broker
            gbidArray2 = new String[1];
            gbidArray2[0] = gbidArray[1];

            // unknownGbis
            gbidArrayUnknown = new String[2];
            gbidArrayUnknown[0] = gbidArray[1];
            gbidArrayUnknown[1] = new String("unknownGbid");
        }

        cleanupProvider();
        createGlobalDiscoveryEntries();

        logger.info("setUp: Leaving");
    }

    @After
    public void cleanup() throws InterruptedException {
        logger.info("cleanup: Entering");
        cleanupProvider();
        logger.info("cleanup: Leaving");
    }

    @AfterClass
    public static void tearDown() throws InterruptedException {
        logger.info("tearDown: Entering");
        cleanupProvider();
        generalTearDown();
        logger.info("tearDown: Leaving");
    }

    private static void checkDiscoveryEntryWithoutTimestamps(GlobalDiscoveryEntry expected,
                                                             GlobalDiscoveryEntry actual) {
        assertNotNull(actual);
        assertEquals(expected.getParticipantId(), actual.getParticipantId());
        assertEquals(expected.getInterfaceName(), actual.getInterfaceName());
        assertEquals(expected.getDomain(), actual.getDomain());
        assertEquals(expected.getProviderVersion(), actual.getProviderVersion());
        assertEquals(expected.getQos(), actual.getQos());
        assertEquals(expected.getAddress(), actual.getAddress());
    }

    private static void checkDiscoveryEntry(GlobalDiscoveryEntry expected, GlobalDiscoveryEntry actual) {
        assertNotNull(actual);
        checkDiscoveryEntryWithoutTimestamps(expected, actual);
        assertEquals(expected.getExpiryDateMs(), actual.getExpiryDateMs());
        assertEquals(expected.getLastSeenDateMs(), actual.getLastSeenDateMs());
    }

    private static void checkDiscoveryEntryTimestampsIncreased(GlobalDiscoveryEntry old, GlobalDiscoveryEntry actual) {
        assertNotNull(actual);
        assertTrue("old.expiryDateMs " + old.getExpiryDateMs() + " < " + "actual.expiryDateMs "
                + actual.getExpiryDateMs(), old.getExpiryDateMs() < actual.getExpiryDateMs());
        assertTrue("old.lastSeenDateMs " + old.getLastSeenDateMs() + " < " + "actual.lastSeenDateMs "
                + actual.getLastSeenDateMs(), old.getLastSeenDateMs() < actual.getLastSeenDateMs());
    }

    private MessagingQos getMessagingQosFromGbids(String[] gbids) {
        MessagingQos messagingQos = new MessagingQos();
        // if there is only a single gbid which is non-default, setup extra custom header
        //if (gbids.length == 1 && !gbids[0].equals(gbidArray[0])) {
        messagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, gbids[0]);
        //}
        return messagingQos;
    }

    private void addProvider(GlobalDiscoveryEntry globalDiscoveryEntry,
                             String[] gbids,
                             boolean expectSuccess,
                             DiscoveryError expectedDiscoveryError) {
        try {
            logger.info("{} - calling add of {}", name.getMethodName(), globalDiscoveryEntry.toString());
            MessagingQos messagingsQos = getMessagingQosFromGbids(gbids);
            globalCapabilitiesDirectoryProxy.add(globalDiscoveryEntry, gbids, messagingsQos);
            logger.info("{} - add returned OK", name.getMethodName());
            if (!expectSuccess) {
                String msg = String.format("%s - add of %s succeeded unexpectedly",
                                           name.getMethodName(),
                                           globalDiscoveryEntry.toString());
                logger.error(msg);
                fail(msg);
            }
        } catch (Exception e) {
            if (expectSuccess) {
                String msg = String.format("%s - add of %s failed with unexpected exception %s",
                                           name.getMethodName(),
                                           globalDiscoveryEntry.toString(),
                                           e.toString());
                logger.error(msg, e);
                fail(msg);
            } else {
                if (e instanceof ApplicationException) {
                    ApplicationException applicatioException = (ApplicationException) e;
                    DiscoveryError discoveryError = (DiscoveryError) applicatioException.getError();
                    assertEquals(expectedDiscoveryError, discoveryError);
                    String msg = String.format("%s - add of %s failed with expected ApplicationException with correct DiscoveryError %s",
                                               name.getMethodName(),
                                               globalDiscoveryEntry.toString(),
                                               e.toString());
                    logger.info(msg, e);
                } else {
                    String msg = String.format("%s - add of %s failed with unexpected Exception %s",
                                               name.getMethodName(),
                                               globalDiscoveryEntry.toString(),
                                               e.toString());
                    logger.error(msg, e);
                    fail(msg);
                }
            }
        }
    }

    private void removeProvider(String participantId,
                                String[] gbids,
                                boolean expectSuccess,
                                DiscoveryError expectedDiscoveryError) {
        try {
            logger.info("{} - Calling remove participantId {}", name.getMethodName(), participantId);
            MessagingQos messagingsQos = getMessagingQosFromGbids(gbids);
            globalCapabilitiesDirectoryProxy.remove(participantId, gbids, messagingsQos);
            logger.info("{} - remove participantId {} returned OK", name.getMethodName(), participantId);
            if (!expectSuccess) {
                String msg = String.format("%s - remove of %s succeeded unexpectedly",
                                           name.getMethodName(),
                                           participantId);
                logger.error(msg);
                fail(msg);
            }
        } catch (Exception e) {
            if (expectSuccess) {
                String msg = String.format("%s - remove of %s failed with unexpected exception %s",
                                           name.getMethodName(),
                                           participantId,
                                           e.toString());
                logger.error(msg);
                fail(msg);
            } else {
                if (e instanceof ApplicationException) {
                    ApplicationException applicatioException = (ApplicationException) e;
                    DiscoveryError discoveryError = (DiscoveryError) applicatioException.getError();
                    assertEquals(expectedDiscoveryError, discoveryError);
                    String msg = String.format("%s - remove of %s failed with expected ApplicationException with correct DiscoveryError %s",
                                               name.getMethodName(),
                                               participantId,
                                               e.toString());
                    logger.info(msg, e);
                } else {
                    String msg = String.format("%s - remove of %s failed with unexpected exception %s",
                                               name.getMethodName(),
                                               participantId,
                                               e.toString());
                    logger.error(msg, e);
                    fail(msg);
                }
            }
        }
    }

    private void removeProvider(GlobalDiscoveryEntry globalDiscoveryEntry,
                                String[] gbids,
                                boolean expectSuccess,
                                DiscoveryError expectedDiscoveryError) {
        removeProvider(globalDiscoveryEntry.getParticipantId(), gbids, expectSuccess, expectedDiscoveryError);
    }

    private static void cleanupProvider() {
        // try to clean up GCD after testing by using removeStale
        try {
            Thread.sleep(1000);
        } catch (Exception e) {
            // ignore
        }
        // for now use the current time; this requires that no faked entries with lastSeenDateMs
        // set into the future have been added.
        Long now = System.currentTimeMillis();
        try {
            MessagingQos messagingQos = new MessagingQos();
            messagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, gbidArray[0]);
            logger.info("cleanupProvider: calling removeStale({}, [])", localChannelId, now);
            globalCapabilitiesDirectoryProxy.removeStale(localChannelId, now);
        } catch (Exception e) {
            logger.error("cleanupProvider: removeStale has thrown exception", e);
        }
    }

    private boolean checkEntries(GlobalDiscoveryEntry[] gcdEntries, GlobalDiscoveryEntry[] expectedEntries) {
        // simplified check, just compare for same participantIds ignoring ordering
        if (gcdEntries.length != expectedEntries.length) {
            return false;
        }

        // put gcdEntries into gcdEntriesMap sorted by participantIds
        TreeMap<String, GlobalDiscoveryEntry> gcdEntriesMap = new TreeMap<String, GlobalDiscoveryEntry>();
        for (GlobalDiscoveryEntry entry : gcdEntries) {
            gcdEntriesMap.put(entry.getParticipantId(), entry);
        }

        // put expectedEntries into expectedEntriesMap sorted by participantIds
        TreeMap<String, GlobalDiscoveryEntry> expectedEntriesMap = new TreeMap<String, GlobalDiscoveryEntry>();
        for (GlobalDiscoveryEntry entry : expectedEntries) {
            expectedEntriesMap.put(entry.getParticipantId(), entry);
        }

        assertEquals(expectedEntriesMap.size(), gcdEntriesMap.size());

        // compare Maps
        for (Map.Entry<String, GlobalDiscoveryEntry> entry : gcdEntriesMap.entrySet()) {
            GlobalDiscoveryEntry globalDiscoveryEntry1 = entry.getValue();
            GlobalDiscoveryEntry globalDiscoveryEntry2 = expectedEntriesMap.get(entry.getKey());
            if (globalDiscoveryEntry2 == null) {
                assertNotNull(globalDiscoveryEntry2);
                return false;
            }
            logger.info("{} - comparing expected {} against actual {}",
                        name.getMethodName(),
                        globalDiscoveryEntry2.toString(),
                        globalDiscoveryEntry1.toString());
            checkDiscoveryEntryWithoutTimestamps(globalDiscoveryEntry2, globalDiscoveryEntry1);
        }
        return true;
    }

    private void logEntries(GlobalDiscoveryEntry[] gcdEntries, GlobalDiscoveryEntry[] expectedEntries) {
        for (GlobalDiscoveryEntry entry : gcdEntries) {
            logger.error("received: {}", entry.toString());
        }
        logger.error("expectedEntries:");
        for (GlobalDiscoveryEntry entry : expectedEntries) {
            logger.error("expected: {}", entry.toString());
        }
    }

    private GlobalDiscoveryEntry[] lookupProviderByDomainAndInterfaceName(String[] domains,
                                                                          String interfaceName,
                                                                          String[] gbids,
                                                                          boolean expectSuccess,
                                                                          GlobalDiscoveryEntry[] expectedEntries,
                                                                          DiscoveryError expectedDiscoveryError) {
        GlobalDiscoveryEntry[] gcdEntries = null;
        try {
            logger.info("{} - Calling lookup for domain [{}], interfaceName {}, gbids [{}]",
                        name.getMethodName(),
                        String.join(",", domains),
                        interfaceName,
                        String.join(",", gbids));
            MessagingQos messagingsQos = getMessagingQosFromGbids(gbids);
            gcdEntries = globalCapabilitiesDirectoryProxy.lookup(domains, interfaceName, gbids, messagingsQos);
            logger.info("{} - lookup returned OK", name.getMethodName());
            if (!expectSuccess) {
                String msg = String.format("%s - lookup for domains [%s], interfaceName %s, gbids [%s] succeeded unexpectedly",
                                           name.getMethodName(),
                                           String.join(",", domains),
                                           interfaceName,
                                           String.join(",", gbids));
                logger.error(msg);
                logEntries(gcdEntries, expectedEntries);
                fail(msg);
            }
            if (!checkEntries(gcdEntries, expectedEntries)) {
                String msg = String.format("%s - lookup for domains [%s], interfaceName %s, gbids [%s] delivered unexpected results",
                                           name.getMethodName(),
                                           String.join(",", domains),
                                           interfaceName,
                                           String.join(",", gbids));
                logger.error(msg);
                logEntries(gcdEntries, expectedEntries);
                fail(msg);
            }
        } catch (Exception e) {
            if (expectSuccess) {
                String msg = String.format("%s - lookup for domains [%s], interfaceName %s, gbids [%s] failed with unexpected exception %s",
                                           name.getMethodName(),
                                           String.join(",", domains),
                                           interfaceName,
                                           String.join(",", gbids),
                                           e.toString());
                logger.error(msg, e);
                fail(msg);
            } else {
                if (e instanceof ApplicationException) {
                    ApplicationException applicatioException = (ApplicationException) e;
                    DiscoveryError discoveryError = (DiscoveryError) applicatioException.getError();
                    assertEquals(expectedDiscoveryError, discoveryError);
                    String msg = String.format("%s lookup for domains [%s], interfaceName %s, gbids [%s] failed with expected ApplicationException with correct DiscoveryError %s",
                                               name.getMethodName(),
                                               String.join(",", domains),
                                               interfaceName,
                                               String.join(",", gbids),
                                               e.toString());
                    logger.error(msg, e);
                } else {
                    String msg = String.format("%s - lookup for domains [%s], interfaceName %s, gbids [%s] failed with unexpected exception %s",
                                               name.getMethodName(),
                                               String.join(",", domains),
                                               interfaceName,
                                               String.join(",", gbids),
                                               e.toString());
                    logger.error(msg, e);
                    fail(msg);
                }
            }
        }
        return gcdEntries;
    }

    private GlobalDiscoveryEntry lookupProviderByParticipantId(String participantId,
                                                               String[] gbids,
                                                               boolean expectSuccess,
                                                               GlobalDiscoveryEntry[] expectedEntries,
                                                               DiscoveryError expectedDiscoveryError) {
        GlobalDiscoveryEntry gcdEntry = null;
        try {
            logger.info("{} - Calling lookup for participantId {}, gbids [{}]",
                        name.getMethodName(),
                        participantId,
                        String.join(",", gbids));
            MessagingQos messagingsQos = getMessagingQosFromGbids(gbids);
            gcdEntry = globalCapabilitiesDirectoryProxy.lookup(participantId, gbids, messagingsQos);
            logger.info("{} - lookup returned OK", name.getMethodName());
            GlobalDiscoveryEntry[] gcdEntries = new GlobalDiscoveryEntry[1];
            gcdEntries[0] = gcdEntry;
            if (!expectSuccess) {
                String msg = String.format("%s - lookup for participantId [%s], gbids [%s] succeeded unexpectedly",
                                           name.getMethodName(),
                                           participantId,
                                           String.join(",", gbids));
                logger.error(msg);
                logEntries(gcdEntries, expectedEntries);
                fail(msg);
            }
            if (!checkEntries(gcdEntries, expectedEntries)) {
                String msg = String.format("%s - lookup for participantId [%s], gbids [%s] delivered unexpected results",
                                           name.getMethodName(),
                                           participantId,
                                           String.join(",", gbids));
                logger.error(msg);
                logEntries(gcdEntries, expectedEntries);
                fail(msg);
            }
        } catch (Exception e) {
            if (expectSuccess) {
                String msg = String.format("%s - lookup for participantId [%s], gbids [%s] failed with unexpected exception %s",
                                           name.getMethodName(),
                                           participantId,
                                           String.join(",", gbids),
                                           e.toString());
                logger.error(msg, e);
                fail(msg);
            } else {
                if (e instanceof ApplicationException) {
                    ApplicationException applicatioException = (ApplicationException) e;
                    DiscoveryError discoveryError = (DiscoveryError) applicatioException.getError();
                    assertEquals(expectedDiscoveryError, discoveryError);
                    String msg = String.format("%s lookup for participantId [%s], gbids [%s] failed with expected ApplicationException with correct DiscoveryError %s",
                                               name.getMethodName(),
                                               participantId,
                                               String.join(",", gbids),
                                               e.toString());
                    logger.error(msg, e);
                } else {
                    String msg = String.format("%s - lookup for participantId [%s], gbids [%s] failed with unexpected exception %s",
                                               name.getMethodName(),
                                               participantId,
                                               String.join(",", gbids),
                                               e.toString());
                    logger.error(msg, e);
                    fail(msg);
                }
            }
        }
        return gcdEntry;
    }

    @Test
    public void addProviderGbid1Only() {
        // add provider for gbid1 only
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);

        // lookup for gbid1 should succeed
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        if (multipleBrokers) {
            // lookup for gbid2 should fail
            lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                                   discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                                   gbidArray2,
                                                   expectFailure,
                                                   expectedEntries,
                                                   DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);

            // lookup for both gbids should succeed
            expectedEntries = new GlobalDiscoveryEntry[1];
            expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
            lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                                   discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                                   gbidArray,
                                                   expectSuccess,
                                                   expectedEntries,
                                                   noError);
        }

        // cleanup
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);
    }

    @Test
    public void addProviderGbid2Only() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }

        // add provider gbid2 only
        addProvider(discoveryEntry_V10_D1_I1_P1_G2, gbidArray2, expectSuccess, noError);

        // lookup for only gbid1 should fail
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectFailure,
                                               expectedEntries,
                                               DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);

        // lookup for only gbid2 should succeed, returning G2 entry
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G2;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray2,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // lookup for both gbids should succeed, returning G2 entry
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G2;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G2.getInterfaceName(),
                                               gbidArray,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // cleanup
        removeProvider(discoveryEntry_V10_D1_I1_P1_G2, gbidArray2, expectSuccess, noError);
    }

    @Test
    public void addProviderBothGbids() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }

        // add provider for both gbids
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray, expectSuccess, noError);

        // lookup for only gbid1 should succeed with G1 entry
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // lookup for gbid2 should succeed with G2 entry
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G2;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray2,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // lookup for both gbids should succeed with G1 entry
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // cleanup
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray, expectSuccess, noError);
    }

    @Test
    public void addProvidersWithDifferentMajorVersionGbid1Only() {
        assertEquals(discoveryEntry_V10_D1_I1_P1_G1.getDomain(), discoveryEntry_V20_D1_I1_P4.getDomain());
        assertEquals(discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(), discoveryEntry_V20_D1_I1_P4.getInterfaceName());
        assertTrue(discoveryEntry_V10_D1_I1_P1_G1.getProviderVersion()
                                                 .getMajorVersion() != discoveryEntry_V20_D1_I1_P4.getProviderVersion()
                                                                                                  .getMajorVersion());

        // add both provider
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);
        addProvider(discoveryEntry_V20_D1_I1_P4, gbidArray1, expectSuccess, noError);

        // loookup for domain / interface
        // both providers should be delivered
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[2];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        expectedEntries[1] = discoveryEntry_V20_D1_I1_P4;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // cleanup
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);
        removeProvider(discoveryEntry_V20_D1_I1_P4, gbidArray1, expectSuccess, noError);
    }

    @Test
    public void addProvidersWithDifferentMinorSameMajorVerionGbid1Only() {
        assertEquals(discoveryEntry_V10_D1_I1_P1_G1.getDomain(), discoveryEntry_V11_D1_I1_P3.getDomain());
        assertEquals(discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(), discoveryEntry_V11_D1_I1_P3.getInterfaceName());
        assertEquals(discoveryEntry_V10_D1_I1_P1_G1.getProviderVersion().getMajorVersion(),
                     discoveryEntry_V11_D1_I1_P3.getProviderVersion().getMajorVersion());
        assertTrue(discoveryEntry_V10_D1_I1_P1_G1.getProviderVersion()
                                                 .getMinorVersion() != discoveryEntry_V11_D1_I1_P3.getProviderVersion()
                                                                                                  .getMinorVersion());

        // add provider v1.0 and v1.1
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);
        addProvider(discoveryEntry_V11_D1_I1_P3, gbidArray1, expectSuccess, noError);

        // loookup should deliver both providers
        // WARNING: real GCD will overwrite 1st entry when 2nd entry is added due to special rule
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[2];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        expectedEntries[1] = discoveryEntry_V11_D1_I1_P3;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // clenaup
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);
        removeProvider(discoveryEntry_V11_D1_I1_P3, gbidArray1, expectSuccess, noError);
    }

    @Test
    public void addProviderTwiceGbid1Only() throws Exception {
        // add provider to gbid1
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);

        // clone original entry, since we will modify it
        GlobalDiscoveryEntry updatedEntry = new GlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1);

        // increase expiryDateMs by some secs and let real time pass as well
        Thread.sleep(5000);
        updatedEntry.setExpiryDateMs(discoveryEntry_V10_D1_I1_P1_G1.getExpiryDateMs() + 5000);
        updatedEntry.setLastSeenDateMs(discoveryEntry_V10_D1_I1_P1_G1.getLastSeenDateMs() + 5000);

        // add provider again to gbid1
        addProvider(updatedEntry, gbidArray1, expectSuccess, noError);

        // lookup should succeed
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = updatedEntry;
        GlobalDiscoveryEntry[] gcdEntries = lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(updatedEntry),
                                                                                   updatedEntry.getInterfaceName(),
                                                                                   gbidArray1,
                                                                                   expectSuccess,
                                                                                   expectedEntries,
                                                                                   noError);

        assertTrue(gcdEntries.length == 1);
        checkDiscoveryEntryTimestampsIncreased(discoveryEntry_V10_D1_I1_P1_G1, gcdEntries[0]);

        // cleanup
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);
    }

    @Test
    public void addProviderWithUnknownGbid() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }
        // add provider with unknown gbid should fail
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArrayUnknown, expectFailure, DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void removeNonExistentProviderGbid1Only() {
        // removing nonexistent provider should fail
        removeProvider("nonexistentParticipantId", gbidArray, expectFailure, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test
    public void removeProviderGbid1Only() {
        GlobalDiscoveryEntry[] expectedEntries = null;

        // pre-check
        // lookup for domain / interface both gbids returns empty
        expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // add provider gbid1 succeeds
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);

        // lookup for gbid1 succeeds
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);
        // remove provider gbid1 succeeds
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);

        // lookup for domain / interface both gbids now returns empty
        expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // lookup for domain / interface gbid1 now returns empty
        expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);
    }

    @Test
    public void removeProviderGbid2Only() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }
        // add provider v1.0 gbid2
        addProvider(discoveryEntry_V10_D1_I1_P1_G2, gbidArray2, expectSuccess, noError);

        // lookup for only gbid2 should succeed, returning G2 entry
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G2;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray2,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // remove provider gbid2 succeeds
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray2, expectSuccess, noError);

        // lookup for domain / interface gbid2 returns empty
        expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray2,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);
    }

    @Test
    public void removeProviderBothGbids() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }

        // add provider v1.0 both gbids
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray, expectSuccess, noError);

        // lookup for both gbids should succeed with G1 entry
        // detailed checks from all gbid combinations are not required here, since this is already verified
        // in other tests.
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // remove provider bothGbids succeeds
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray, expectSuccess, noError);

        // lookup for domain / interface both gbids returns empty result
        expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);
    }

    @Test
    public void removeProviderPartiallyGbid1FromBothGbids() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }

        // add provider v1.0 both gbids
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray, expectSuccess, noError);

        // lookup for domain / interface both gbids succeeds
        // detailed checks from all gbid combinations are not required here, since this is already verified
        // in other tests.
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // remove provider gbid1 succeeds
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);

        // lookup for domain / interface both gbids still succeeds
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G2;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // lookup for domain / interface gbid1 fails
        expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectFailure,
                                               expectedEntries,
                                               DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);

        // lookup for domain / interface gbid2 still succeeds
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G2;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray2,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // cleanup

        // remove provider gbid2 succeeds
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray2, expectSuccess, noError);
    }

    @Test
    public void removeProviderPartiallyGbid2FromBothGbids() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }

        // add provider v1.0 both gbids
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray, expectSuccess, noError);

        // lookup for domain / interface both gbids succeeds
        // detailed checks from all gbid combinations are not required here, since this is already verified
        // in other tests.
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // remove provider gbid2 succeeds
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray2, expectSuccess, noError);

        // lookup for domain / interface both gbids still succeeds
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // lookup for domain / interface gbid1 still succeeds
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // lookup for domain / interface gbid2 fails
        expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray2,
                                               expectFailure,
                                               expectedEntries,
                                               DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
        // remove provider gbid1 succeeds
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);

        // lookup both gbids should now deliver empty result
        expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // lookup for domain / interface gbid1  should now deliver empty result
        expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // lookup for domain / interface gbid2 should now deliver empty result
        expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray2,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);
    }

    @Test
    public void removeProviderWithUnknownGbid() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArrayUnknown, expectFailure, DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void lookupByDomainInterfaceNameWithUnknownGbid() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArrayUnknown,
                                               expectFailure,
                                               expectedEntries,
                                               DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void lookupNonExistentParticipantIdGbid1Only() {
        // lookup for participantId gbid1 fails with
        // DiscoveryError.NO_ENTRY_FOR_PARTICIPANT
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                      gbidArray1,
                                      expectFailure,
                                      expectedEntries,
                                      DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test
    public void lookupNonExistentParticipantIdGbid2Only() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }
        // lookup for participantId gbid1 fails with
        // DiscoveryError.NO_ENTRY_FOR_PARTICIPANT
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                      gbidArray2,
                                      expectFailure,
                                      expectedEntries,
                                      DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test
    public void lookupNonExistentParticipantIdBothGbids() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }
        // lookup for participantId gbid1 fails with
        // DiscoveryError.NO_ENTRY_FOR_PARTICIPANT
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                      gbidArray,
                                      expectFailure,
                                      expectedEntries,
                                      DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test
    public void lookupParticipantIdUnknownGbid() {
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                      gbidArrayUnknown,
                                      expectFailure,
                                      expectedEntries,
                                      DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void lookupParticipantIdGbid1Only() {
        // add provider for gbid1 only
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);

        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                      gbidArray1,
                                      expectSuccess,
                                      expectedEntries,
                                      DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
        if (multipleBrokers) {
            // lookup for gbid2 should fail
            expectedEntries = new GlobalDiscoveryEntry[0];
            lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                          gbidArray2,
                                          expectFailure,
                                          expectedEntries,
                                          DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);

            // lookup for both gbids should succeed
            expectedEntries = new GlobalDiscoveryEntry[1];
            expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
            lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                          gbidArray,
                                          expectSuccess,
                                          expectedEntries,
                                          noError);
        }

        // cleanup
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);
    }

    @Test
    public void lookupParticipantIdGbid2Only() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }

        // add provider v1.0 participantId1 gbid2
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray2, expectSuccess, noError);

        // lookup for gbid1 should fail
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[0];
        lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                      gbidArray1,
                                      expectFailure,
                                      expectedEntries,
                                      DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);

        // lookup for gbid1 should succeed
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G2;
        lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                      gbidArray2,
                                      expectSuccess,
                                      expectedEntries,
                                      DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);

        // lookup for both gbids should succeed
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G2;
        lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                      gbidArray,
                                      expectSuccess,
                                      expectedEntries,
                                      noError);

        // cleanup
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray2, expectSuccess, noError);
    }

    @Test
    public void lookupParticipantIdBothGbids() {
        if (!multipleBrokers) {
            logger.info("{} - Skipped (multiple brokers only)", name.getMethodName());
            return;
        }
        // add provider v1.0 both gbids
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray, expectSuccess, noError);

        // lookup for gbid1 should fail
        GlobalDiscoveryEntry[] expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                      gbidArray1,
                                      expectSuccess,
                                      expectedEntries,
                                      DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);

        // lookup for gbid2 should succeed
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G2;
        lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                      gbidArray2,
                                      expectSuccess,
                                      expectedEntries,
                                      DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);

        // lookup for both gbids should succeed
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        lookupProviderByParticipantId(discoveryEntry_V10_D1_I1_P1_G1.getParticipantId(),
                                      gbidArray,
                                      expectSuccess,
                                      expectedEntries,
                                      noError);

        // cleanup
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray, expectSuccess, noError);
    }

    @Test
    public void touchUpdatesExpiryDateAndLasteSeenMsGbid1Only() {
        GlobalDiscoveryEntry[] gcdEntries = null;
        GlobalDiscoveryEntry[] expectedEntries = null;

        // add provider v1.0 participantId1 gbid1, will be touched
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);

        // add provider v2.0 participantId4 gbid1, will be touched
        addProvider(discoveryEntry_V20_D1_I1_P4, gbidArray1, expectSuccess, noError);

        // add provider v1.0 participantId5 with domain2, will NOT be touched
        addProvider(discoveryEntry_V10_D2_I1_P5, gbidArray1, expectSuccess, noError);

        try {
            // wait 5 secs
            Thread.sleep(5000);
        } catch (Exception e) {
            // ignore
        }

        // check first
        // lookup domain / interfaceName returns provider with domain2
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D2_I1_P5;
        gcdEntries = lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D2_I1_P5),
                                                            discoveryEntry_V10_D2_I1_P5.getInterfaceName(),
                                                            gbidArray1,
                                                            expectSuccess,
                                                            expectedEntries,
                                                            noError);
        assertEquals(1, gcdEntries.length);
        checkDiscoveryEntry(discoveryEntry_V10_D2_I1_P5, gcdEntries[0]);

        String[] participantsToTouch = new String[2];
        participantsToTouch[0] = discoveryEntry_V10_D1_I1_P1_G1.getParticipantId();
        participantsToTouch[1] = discoveryEntry_V20_D1_I1_P4.getParticipantId();

        try {
            // touch participantId1, participantId4
            MessagingQos messagingQos = new MessagingQos();
            messagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, gbidArray[0]);
            globalCapabilitiesDirectoryProxy.touch(localChannelId, participantsToTouch);
        } catch (Exception e) {
            // ignore
        }

        // lookup domain / interfaceName returns provider with domain2
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V10_D2_I1_P5;
        gcdEntries = lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D2_I1_P5),
                                                            discoveryEntry_V10_D2_I1_P5.getInterfaceName(),
                                                            gbidArray1,
                                                            expectSuccess,
                                                            expectedEntries,
                                                            noError);

        assertEquals(1, gcdEntries.length);
        // not touched entry should keep same time as before
        checkDiscoveryEntry(discoveryEntry_V10_D2_I1_P5, gcdEntries[0]);

        // lookup domain / interfaceName returns provider with domain2
        expectedEntries = new GlobalDiscoveryEntry[2];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        expectedEntries[1] = discoveryEntry_V20_D1_I1_P4;
        gcdEntries = lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                                            discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                                            gbidArray1,
                                                            expectSuccess,
                                                            expectedEntries,
                                                            noError);

        assertEquals(2, gcdEntries.length);
        // touched entries should have increased time
        // order is unknown, so check for participantId first
        if (discoveryEntry_V10_D1_I1_P1_G1.getParticipantId() == gcdEntries[0].getParticipantId()) {
            checkDiscoveryEntryTimestampsIncreased(discoveryEntry_V10_D1_I1_P1_G1, gcdEntries[0]);
            checkDiscoveryEntryTimestampsIncreased(discoveryEntry_V20_D1_I1_P4, gcdEntries[1]);
        } else {
            checkDiscoveryEntryTimestampsIncreased(discoveryEntry_V20_D1_I1_P4, gcdEntries[0]);
            checkDiscoveryEntryTimestampsIncreased(discoveryEntry_V10_D1_I1_P1_G1, gcdEntries[1]);
        }

        // cleanup
        removeProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);
        removeProvider(discoveryEntry_V20_D1_I1_P4, gbidArray1, expectSuccess, noError);
        removeProvider(discoveryEntry_V10_D2_I1_P5, gbidArray1, expectSuccess, noError);
    }

    @Test
    public void removeStaleWorksGbid1() {
        GlobalDiscoveryEntry[] expectedEntries = null;

        // add provider v1.0 participantId1 gbid1
        addProvider(discoveryEntry_V10_D1_I1_P1_G1, gbidArray1, expectSuccess, noError);
        try {
            // wait 5 secs
            Thread.sleep(5000);
        } catch (Exception e) {
            // ignore
        }

        // store time
        long now = System.currentTimeMillis();
        try {
            // wait 5 secs
            Thread.sleep(5000);
        } catch (Exception e) {
            // ignore
        }
        long later = System.currentTimeMillis();
        discoveryEntry_V20_D1_I1_P4.setExpiryDateMs(later);
        discoveryEntry_V20_D1_I1_P4.setLastSeenDateMs(later);

        // add provider v2.0 participantId2 gbid1
        addProvider(discoveryEntry_V20_D1_I1_P4, gbidArray1, expectSuccess, noError);

        // lookup domain / interfaceName returns both providers
        expectedEntries = new GlobalDiscoveryEntry[2];
        expectedEntries[0] = discoveryEntry_V10_D1_I1_P1_G1;
        expectedEntries[1] = discoveryEntry_V20_D1_I1_P4;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // invoke removeStale with stored time
        try {
            MessagingQos messagingQos = new MessagingQos();
            messagingQos.putCustomMessageHeader(Message.CUSTOM_HEADER_GBID_KEY, gbidArray[0]);
            logger.info("{} - calling removeStale({}, {})", name.getMethodName(), localChannelId, now);
            globalCapabilitiesDirectoryProxy.removeStale(localChannelId, now);
        } catch (Exception e) {
            logger.error("{} - removeStale has thrown exception", name.getMethodName(), e);
        }

        // lookup domain / interfaceName returns only non-stale providers
        expectedEntries = new GlobalDiscoveryEntry[1];
        expectedEntries[0] = discoveryEntry_V20_D1_I1_P4;
        lookupProviderByDomainAndInterfaceName(getDomainsFromGlobalDiscoveryEntry(discoveryEntry_V10_D1_I1_P1_G1),
                                               discoveryEntry_V10_D1_I1_P1_G1.getInterfaceName(),
                                               gbidArray1,
                                               expectSuccess,
                                               expectedEntries,
                                               noError);

        // cleanup
        removeProvider(discoveryEntry_V20_D1_I1_P4, gbidArray1, expectSuccess, noError);
    }
}
