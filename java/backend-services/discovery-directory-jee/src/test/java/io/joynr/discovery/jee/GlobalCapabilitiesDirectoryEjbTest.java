/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
package io.joynr.discovery.jee;

import static io.joynr.discovery.jee.TestJoynrConfigurationProvider.JOYNR_DEFAULT_GCD_GBID;
import static io.joynr.discovery.jee.TestJoynrConfigurationProvider.VALID_GBIDS_ARRAY;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.File;
import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.List;

import javax.inject.Inject;
import javax.persistence.EntityManager;

import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.arquillian.transaction.api.annotation.TransactionMode;
import org.jboss.arquillian.transaction.api.annotation.Transactional;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.WebArchive;
import org.jboss.shrinkwrap.resolver.api.maven.Maven;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.Timeout;
import org.junit.runner.RunWith;

import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import io.joynr.capabilities.GlobalDiscoveryEntryPersistedKey;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.infrastructure.GlobalCapabilitiesDirectorySync;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

@RunWith(Arquillian.class)
@Transactional(TransactionMode.ROLLBACK)
public class GlobalCapabilitiesDirectoryEjbTest {
    @Rule
    public Timeout globalTimeout = Timeout.seconds(60);

    private static final String TOPIC_NAME = "my/topic";

    @Deployment
    public static WebArchive createArchive() {
        File[] files = Maven.resolver()
                            .loadPomFromFile("pom.xml")
                            .importRuntimeDependencies()
                            .resolve()
                            .withTransitivity()
                            .asFile();
        return ShrinkWrap.create(WebArchive.class)
                         .addClasses(EntityManagerProducer.class,
                                     GlobalCapabilitiesDirectoryEjb.class,
                                     TestJoynrConfigurationProvider.class)
                         .addAsLibraries(files)
                         .addAsResource("META-INF/persistence.xml")
                         .addAsWebInfResource(new File("src/main/webapp/WEB-INF/beans.xml"));
    }

    private GlobalDiscoveryEntry testGlobalDiscoveryEntry1;
    private GlobalDiscoveryEntry testGlobalDiscoveryEntry1_a;
    private GlobalDiscoveryEntry testGlobalDiscoveryEntry2;
    private GlobalDiscoveryEntry expectedGlobalDiscoveryEntry1;
    private GlobalDiscoveryEntry expectedGlobalDiscoveryEntry1_a;
    private GlobalDiscoveryEntry expectedGlobalDiscoveryEntry2;
    final private String testParticipantId1 = "participantId";
    final private String testParticipantId1_a = "participantId_a";
    final private String testParticipantId2 = "testAnotherParticipantId";
    final private String domain = "com";
    final private String[] domains = { domain };
    final private String interfaceName1 = "interfaceName1";
    final private String interfaceName2 = "interfaceName2";
    final private String joynrdefaultgbid = JOYNR_DEFAULT_GCD_GBID;
    final private String[] validGbidsArray = VALID_GBIDS_ARRAY.split(",");

    @Inject
    private GlobalCapabilitiesDirectorySync subject;

    @Inject
    private EntityManager entityManager;

    @Before
    public void setup() throws NoSuchFieldException, IllegalAccessException {
        Field field = CapabilityUtils.class.getDeclaredField("objectMapper");
        field.setAccessible(true);
        field.set(CapabilityUtils.class, new ObjectMapper());
        testGlobalDiscoveryEntry1 = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                            domain,
                                                                            interfaceName1,
                                                                            testParticipantId1,
                                                                            new ProviderQos(),
                                                                            System.currentTimeMillis(),
                                                                            System.currentTimeMillis() + 1000L,
                                                                            "public key ID",
                                                                            new MqttAddress("tcp://mqttbroker:1883",
                                                                                            TOPIC_NAME));

        testGlobalDiscoveryEntry1_a = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                              domain,
                                                                              interfaceName1,
                                                                              testParticipantId1_a,
                                                                              new ProviderQos(),
                                                                              System.currentTimeMillis(),
                                                                              System.currentTimeMillis() + 1000L,
                                                                              "public key ID",
                                                                              new MqttAddress("tcp://mqttbroker:1883",
                                                                                              TOPIC_NAME));

        testGlobalDiscoveryEntry2 = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                            domain,
                                                                            interfaceName2,
                                                                            testParticipantId2,
                                                                            new ProviderQos(),
                                                                            System.currentTimeMillis(),
                                                                            System.currentTimeMillis() + 1000L,
                                                                            "public key ID",
                                                                            new MqttAddress("tcp://mqttbroker:1883",
                                                                                            TOPIC_NAME));

        expectedGlobalDiscoveryEntry1 = CapabilityUtils.newGlobalDiscoveryEntry(testGlobalDiscoveryEntry1.getProviderVersion(),
                                                                                testGlobalDiscoveryEntry1.getDomain(),
                                                                                testGlobalDiscoveryEntry1.getInterfaceName(),
                                                                                testGlobalDiscoveryEntry1.getParticipantId(),
                                                                                new ProviderQos(testGlobalDiscoveryEntry1.getQos()),
                                                                                testGlobalDiscoveryEntry1.getLastSeenDateMs(),
                                                                                testGlobalDiscoveryEntry1.getExpiryDateMs(),
                                                                                testGlobalDiscoveryEntry1.getPublicKeyId(),
                                                                                new MqttAddress("tcp://mqttbroker:1883",
                                                                                                TOPIC_NAME));

        expectedGlobalDiscoveryEntry1_a = CapabilityUtils.newGlobalDiscoveryEntry(testGlobalDiscoveryEntry1_a.getProviderVersion(),
                                                                                  testGlobalDiscoveryEntry1_a.getDomain(),
                                                                                  testGlobalDiscoveryEntry1_a.getInterfaceName(),
                                                                                  testGlobalDiscoveryEntry1_a.getParticipantId(),
                                                                                  new ProviderQos(testGlobalDiscoveryEntry1_a.getQos()),
                                                                                  testGlobalDiscoveryEntry1_a.getLastSeenDateMs(),
                                                                                  testGlobalDiscoveryEntry1_a.getExpiryDateMs(),
                                                                                  testGlobalDiscoveryEntry1_a.getPublicKeyId(),
                                                                                  new MqttAddress("tcp://mqttbroker:1883",
                                                                                                  TOPIC_NAME));

        expectedGlobalDiscoveryEntry2 = CapabilityUtils.newGlobalDiscoveryEntry(testGlobalDiscoveryEntry2.getProviderVersion(),
                                                                                testGlobalDiscoveryEntry2.getDomain(),
                                                                                testGlobalDiscoveryEntry2.getInterfaceName(),
                                                                                testGlobalDiscoveryEntry2.getParticipantId(),
                                                                                new ProviderQos(testGlobalDiscoveryEntry2.getQos()),
                                                                                testGlobalDiscoveryEntry2.getLastSeenDateMs(),
                                                                                testGlobalDiscoveryEntry2.getExpiryDateMs(),
                                                                                testGlobalDiscoveryEntry2.getPublicKeyId(),
                                                                                new MqttAddress("tcp://mqttbroker:1883",
                                                                                                TOPIC_NAME));
    }

    private void checkDiscoveryEntry(GlobalDiscoveryEntry expected, GlobalDiscoveryEntry actual, String expectedGbid) {
        String[] expectedGbids = new String[]{ expectedGbid };
        checkDiscoveryEntry(expected, actual, expectedGbids);
    }

    private void checkDiscoveryEntry(GlobalDiscoveryEntry expected,
                                     GlobalDiscoveryEntry actual,
                                     String[] expectedGbids) {
        assertNotNull(actual);
        assertEquals(expected.getParticipantId(), actual.getParticipantId());
        assertEquals(expected.getInterfaceName(), actual.getInterfaceName());
        assertEquals(expected.getDomain(), actual.getDomain());
        assertEquals(expected.getExpiryDateMs(), actual.getExpiryDateMs());
        assertEquals(expected.getLastSeenDateMs(), actual.getLastSeenDateMs());
        assertEquals(expected.getProviderVersion(), actual.getProviderVersion());
        assertEquals(expected.getQos(), actual.getQos());
        assertNotEquals(expected.getAddress(), actual.getAddress());

        String actualGbid = ((MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(actual)).getBrokerUri();
        if (!Arrays.asList(expectedGbids).contains(actualGbid)) {
            fail("Actual Gbid: " + actualGbid + " is not in the expected gbids: " + Arrays.toString(expectedGbids));
        }

        assertEquals(TOPIC_NAME, ((MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(actual)).getTopic());
    }

    @Test
    public void testAddAndLookup_byParticipantId_singleDiscoveryEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);

        addEntry(testGlobalDiscoveryEntry1);

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, validGbidsArray);
        assertNotNull(result);
        assertTrue(result instanceof GlobalDiscoveryEntry);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        GlobalDiscoveryEntry persisted = (GlobalDiscoveryEntry) result;

        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, persisted, joynrdefaultgbid);
    }

    @Test
    public void testAddAndLookup_byDomainInterface_singleDiscoveryEntry() throws Exception {
        checkEntryIsNotInEntityManager(testParticipantId1);

        addEntry(testGlobalDiscoveryEntry1);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1);
        assertNotNull(result);
        assertEquals(1, result.length);
        assertTrue(result[0] instanceof GlobalDiscoveryEntry);
        assertFalse(result[0] instanceof GlobalDiscoveryEntryPersisted);

        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], joynrdefaultgbid);
    }

    @Test
    public void testAddAndRemove_singleDiscoveryEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);

        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);

        subject.remove(testParticipantId1, validGbidsArray);
        entityManager.flush();
        entityManager.clear();

        checkEntryIsNotInEntityManager(testParticipantId1);
    }

    // Tests for add
    @Test
    public void testGlobalDiscoveryEntry2() {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        addEntry(testGlobalDiscoveryEntry1);
        addEntry(testGlobalDiscoveryEntry2);
        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantId(testParticipantId1);

        assertNotNull(result);
        assertEquals(1, result.size());
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result.get(0), joynrdefaultgbid);
        assertEquals(joynrdefaultgbid, result.get(0).getGbid());
        assertEquals(TOPIC_NAME, result.get(0).getClusterControllerId());
    }

    @Test
    public void addGlobalDiscoveryEntryWithGbids() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId2);

        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);
        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantIdAndGbids(testParticipantId2,
                                                                                               validGbidsArray);
        assertNotNull(result);
        assertEquals(3, result.size());

        checkDiscoveryEntry(expectedGlobalDiscoveryEntry2, result.get(0), joynrdefaultgbid);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry2, result.get(1), validGbidsArray[1]);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry2, result.get(2), validGbidsArray[2]);

        assertEquals(TOPIC_NAME, result.get(0).getClusterControllerId());
    }

    private void addEntry(GlobalDiscoveryEntry entry) {
        subject.add(entry);
        entityManager.flush();
        entityManager.clear();
    }

    private void addEntry(GlobalDiscoveryEntry entry, String[] gbids) throws ApplicationException {
        subject.add(entry, gbids);
        entityManager.flush();
        entityManager.clear();
    }

    private List<GlobalDiscoveryEntryPersisted> queryEntityManagerByParticipantId(String participantId) {
        String queryString = "FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.participantId = :participantId";
        List<GlobalDiscoveryEntryPersisted> actualResult = entityManager.createQuery(queryString,
                                                                                     GlobalDiscoveryEntryPersisted.class)
                                                                        .setParameter("participantId", participantId)
                                                                        .getResultList();

        return actualResult;
    }

    private List<GlobalDiscoveryEntryPersisted> queryEntityManagerByParticipantIdAndGbids(String participantId,
                                                                                          String[] gbids) {
        String queryString = "FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.participantId = :participantId AND gdep.gbid IN :gbids";
        List<GlobalDiscoveryEntryPersisted> actualResult = entityManager.createQuery(queryString,
                                                                                     GlobalDiscoveryEntryPersisted.class)
                                                                        .setParameter("participantId", participantId)
                                                                        .setParameter("gbids", Arrays.asList(gbids))
                                                                        .getResultList();

        return actualResult;
    }

    @Test
    public void testAddWithGbids_validGbid() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);
        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1);
        assertNotNull(result);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, joynrdefaultgbid);
    }

    @Test
    public void testAddWithGbids_unknownGbid() {
        checkEntryIsNotInEntityManager(testParticipantId1);

        final String[] invalidGbidsArray = { "unknownGbid" };
        try {
            addEntry(testGlobalDiscoveryEntry1, invalidGbidsArray);
        } catch (ApplicationException e) {
            assertEquals(DiscoveryError.UNKNOWN_GBID, e.getError());
        }
    }

    public void checkEntryIsNotInEntityManager(String testParticipantId1) {
        List<GlobalDiscoveryEntryPersisted> resultBeforeAddingAnEntry = queryEntityManagerByParticipantIdAndGbids(testParticipantId1,
                                                                                                                  validGbidsArray);
        assertEquals(0, resultBeforeAddingAnEntry.size());
    }

    private void testAddWithGbids_invalidGbid(String[] invalidGbidsArray) {
        checkEntryIsNotInEntityManager(testParticipantId1);

        try {
            addEntry(testGlobalDiscoveryEntry1, invalidGbidsArray);
        } catch (ApplicationException e) {
            assertEquals(DiscoveryError.INVALID_GBID, e.getError());
        }
    }

    @Test
    public void testAddWithGbids_invalidGbid_emptyGbid() {
        checkEntryIsNotInEntityManager(testParticipantId1);

        final String[] invalidGbidsArray = { "" };
        testAddWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testAddWithGbids_invalidGbid_nullGbid() {
        final String[] invalidGbidsArray = { null };
        testAddWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testAddWithGbids_invalidGbid_nullGbidsArray() {
        final String[] invalidGbidsArray = null;
        testAddWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testAddWithGbids_invalidGbid_emptyGbidsArray() {
        final String[] invalidGbidsArray = {};
        testAddWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testAddSameEntryTwice() throws Exception {
        addEntry(testGlobalDiscoveryEntry1);
        addEntry(testGlobalDiscoveryEntry1);

        GlobalDiscoveryEntry[] persisted = subject.lookup(domains, interfaceName1);
        assertNotNull(persisted);
        assertEquals(1, persisted.length);
    }

    // Tests for lookupParticipantId
    @Test
    public void testLookupParticipantIdWithGbids_invalidGbid_emptyGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { "" };
        checkLookupByParticipantIdWithGbids(testParticipantId1, invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void testLookupParticipantIdWithGbids_invalidGbid_nullGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { null };
        checkLookupByParticipantIdWithGbids(testParticipantId1, invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void testLookupParticipantIdWithGbids_invalidGbid_nullGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = null;
        checkLookupByParticipantIdWithGbids(testParticipantId1, invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void testLookupParticipantIdWithGbids_invalidGbid_emptyGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = {};
        checkLookupByParticipantIdWithGbids(testParticipantId1, invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void testLookupParticipantIdWithGbids_invalidGbid_duplicatedGbid() {
        final String[] invalidGbidsArray = { joynrdefaultgbid, "testGbid2", "testGbid2" };
        checkLookupByParticipantIdWithGbids(testParticipantId1, invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void testLookupParticipantWithGbids_unknownGbid() {
        final String[] invalidGbidsArray = { joynrdefaultgbid, "testGbid2", "unknowngbid" };
        checkLookupByParticipantIdWithGbids(testParticipantId1, invalidGbidsArray, DiscoveryError.UNKNOWN_GBID);
    }

    private void checkLookupByParticipantId(String participantId, DiscoveryError expectedError) {
        GlobalDiscoveryEntry result = null;
        try {
            result = subject.lookup(participantId);
            fail("Should throw ProviderRuntimeException");
        } catch (ProviderRuntimeException e) {
            assertTrue(e.getMessage().contains(expectedError.name()));
            assertNull(result);
        }
    }

    private void checkLookupByParticipantIdWithGbids(String participantId,
                                                     String[] gbids,
                                                     DiscoveryError expectedError) {
        GlobalDiscoveryEntry result = null;
        try {
            result = subject.lookup(participantId, gbids);
            fail("Should throw ApplicationException");
        } catch (ApplicationException e) {
            assertEquals(expectedError, e.getError());
            assertNull(result);
        }
    }

    @Test
    public void testLookupParticipantId_NO_ENTRY_FOR_PARTICIPANT() {
        checkLookupByParticipantId(testParticipantId1, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test
    public void testLookupParticipantIdWithGbids_NO_ENTRY_FOR_PARTICIPANT() {
        checkLookupByParticipantIdWithGbids(testParticipantId1,
                                            validGbidsArray,
                                            DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test
    public void testLookupParticipantId_NO_ENTRY_FOR_SELECTED_BACKENDS() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1] });
        checkLookupByParticipantId(testParticipantId1, DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    @Test
    public void testLookupParticipantIdWithGbids_NO_ENTRY_FOR_SELECTED_BACKENDS() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1] });

        String[] queriedValidGbids = { validGbidsArray[0], validGbidsArray[2] };
        checkLookupByParticipantIdWithGbids(testParticipantId1,
                                            queriedValidGbids,
                                            DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    @Test
    public void testLookupParticipantId_singleMatchingEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        addEntry(testGlobalDiscoveryEntry1);

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1);
        assertNotNull(result);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, joynrdefaultgbid);
    }

    @Test
    public void testLookupParticipantId_multipleMatchingEntries_onlyOneReturned() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1);
        assertNotNull(result);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, joynrdefaultgbid);
    }

    @Test
    public void testLookupParticipantIdWithGbids_defaultGbid_singleMatchingEntry() {
        addEntry(testGlobalDiscoveryEntry1);
        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1);
        assertNotNull(result);

        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, new String[]{ joynrdefaultgbid });
    }

    @Test
    public void testLookupParticipantIdWithGbids_otherGbid_singleMatchingEntry() throws ApplicationException {
        String[] selectedGbids = new String[]{ validGbidsArray[1] };
        String[] expectedGbids = selectedGbids.clone();

        addEntry(testGlobalDiscoveryEntry1, selectedGbids);

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, selectedGbids);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, expectedGbids);
    }

    @Test
    public void testLookupParticipantIdWithGbids_multipleGbids_singleMatchingEntry() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1] });

        String[] selectedGbids = new String[]{ joynrdefaultgbid, validGbidsArray[1] };
        String[] expectedGbids = selectedGbids.clone();

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, selectedGbids);

        assertNotNull(result);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, expectedGbids);
    }

    @Test
    public void testLookupParticipantIdWithGbids_defaultGbid_multipleMatchingEntries_onlyOneReturned() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1, new String[]{ joynrdefaultgbid, validGbidsArray[1] });

        GlobalDiscoveryEntry result = null;
        String[] defaultGbid = new String[]{ joynrdefaultgbid };

        result = subject.lookup(testParticipantId1, defaultGbid);
        assertNotNull(result);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, defaultGbid);
    }

    @Test
    public void testLookupParticipantIdWithGbids_otherGbid_multipleMatchingEntries_onlyOneReturned() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1, new String[]{ joynrdefaultgbid, validGbidsArray[1] });

        String[] otherGbid = new String[]{ validGbidsArray[1] };
        String[] defaultGbid = new String[]{ joynrdefaultgbid };
        GlobalDiscoveryEntry result1 = null;
        GlobalDiscoveryEntry result2 = null;

        result1 = subject.lookup(testParticipantId1, otherGbid);
        assertNotNull(result1);
        result2 = subject.lookup(testParticipantId1, defaultGbid);
        assertNotNull(result2);

        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result1, otherGbid);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result2, defaultGbid);
    }

    @Test
    public void testLookupParticipantIdWithGbids_multipleGbids_multipleMatchingEntries_onlyOneReturned() throws ApplicationException {
        String[] selectedGbids = new String[]{ validGbidsArray[1], validGbidsArray[2] };
        String[] expectedGbids = selectedGbids.clone();

        addEntry(testGlobalDiscoveryEntry1, selectedGbids);

        GlobalDiscoveryEntry result1 = subject.lookup(testParticipantId1, selectedGbids);
        assertNotNull(result1);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result1, expectedGbids);
    }

    // Tests for lookupDomainInterface
    @Test
    public void testLookupDomainInterface_singleMatchingEntry() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1);
        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1);
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], joynrdefaultgbid);
    }

    @Test
    public void testLookupDomainInterface_multipleMatchingEntries_onlyOneReturned() throws ApplicationException {
        String[] multipleValidGbids = new String[]{ validGbidsArray[1], joynrdefaultgbid };

        addEntry(testGlobalDiscoveryEntry1, multipleValidGbids);
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ joynrdefaultgbid });

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1);
        assertNotNull(result);
        assertEquals(2, result.length);
        if (expectedGlobalDiscoveryEntry1.getParticipantId().equals(result[0].getParticipantId())) {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], joynrdefaultgbid);
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[1], joynrdefaultgbid);
        } else {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[1], joynrdefaultgbid);
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[0], joynrdefaultgbid);
        }
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_defaultGbid_singleMatchingEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);

        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1);
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], joynrdefaultgbid);
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_otherGbid_singleMatchingEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);

        String[] validOtherGbid = new String[]{ validGbidsArray[1] };

        addEntry(testGlobalDiscoveryEntry1, validOtherGbid);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, validOtherGbid);
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], validOtherGbid);
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_multipleGbids_singleMatchingEntry() throws ApplicationException {
        String[] multipleGbids = new String[]{ validGbidsArray[1], validGbidsArray[2] };
        addEntry(testGlobalDiscoveryEntry1, multipleGbids);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, multipleGbids);
        assertNotNull(result);
        assertEquals(1, result.length);

        if (expectedGlobalDiscoveryEntry1.getAddress().equals(result[0].getAddress())) {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], validGbidsArray[1]);
        } else {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], validGbidsArray[2]);
        }
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_defaultGbid_multipleMatchingEntriesButWithDifferentGbid_onlyOneReturned() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);

        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);

        String[] defaultGbid = new String[]{ joynrdefaultgbid };
        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, new String[]{ joynrdefaultgbid });
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], defaultGbid);
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_otherGbid_multipleMatchingEntries_onlyOneReturned() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        String[] validMultipleGbids = new String[]{ validGbidsArray[1], validGbidsArray[2] };

        addEntry(testGlobalDiscoveryEntry1, validMultipleGbids);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, new String[]{ validGbidsArray[1] });
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], validGbidsArray[1]);
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_multipleGbids_multipleMatchingEntries_onlyOneReturned() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);

        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);

        String[] multipleValidGbids = new String[]{ validGbidsArray[1], validGbidsArray[2] };
        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, multipleValidGbids);
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], multipleValidGbids);
    }

    private void testLookupDomainInterfaceWithGbids_invalidGbid(String[] invalidGbidsArray) {
        try {
            subject.lookup(domains, interfaceName1, invalidGbidsArray);
        } catch (ApplicationException e) {
            assertEquals(DiscoveryError.INVALID_GBID, e.getError());
        }
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_invalidGbid_emptyGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { "" };
        testLookupDomainInterfaceWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_invalidGbid_nullGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { null };
        testLookupDomainInterfaceWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_invalidGbid_nullGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = null;
        testLookupDomainInterfaceWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_invalidGbid_emptyGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = {};
        testLookupDomainInterfaceWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_unknownGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { "unknownGbid" };
        try {
            subject.lookup(domains, interfaceName1, invalidGbidsArray);
        } catch (ApplicationException e) {
            assertEquals(DiscoveryError.UNKNOWN_GBID, e.getError());
        }
    }

    @Test
    public void testLookupDomainInterfaceWithGbids_validGbids() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);
        GlobalDiscoveryEntry[] globalDiscoveryEntries = subject.lookup(domains, interfaceName1, validGbidsArray);
        assertEquals(1, globalDiscoveryEntries.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, globalDiscoveryEntries[0], joynrdefaultgbid);
    }

    // Tests for remove
    @Test
    public void testRemoveWithGbids_validGbid() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);

        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);

        GlobalDiscoveryEntry resultBeforeAdd = subject.lookup(testParticipantId1);
        assertNotNull(resultBeforeAdd);

        GlobalDiscoveryEntry[] globalDiscoveryEntries = subject.lookup(domains, interfaceName1, validGbidsArray);
        assertEquals(1, globalDiscoveryEntries.length);

        subject.remove(testParticipantId1, validGbidsArray);

        checkEntryIsNotInEntityManager(testParticipantId1);
    }

    @Test
    public void testRemoveWithGbids_unknownParticipantId() throws ApplicationException {
        try {
            subject.remove("unknownParticipantId", validGbidsArray);
        } catch (ApplicationException e) {
            assertEquals(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT, e.getError());
        }
    }

    private void testRemoveWithGbids_invalidGbid(String[] invalidGbidsArray) {
        try {
            subject.remove(testParticipantId1, invalidGbidsArray);
        } catch (ApplicationException e) {
            assertEquals(DiscoveryError.INVALID_GBID, e.getError());
        }
    }

    @Test
    public void testRemoveWithGbids_invalidGbid_emptyGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { "" };
        testRemoveWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testRemoveWithGbids_invalidGbid_nullGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { null };
        testRemoveWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testRemoveWithGbids_invalidGbid_nullGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = null;
        testRemoveWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testRemoveWithGbids_invalidGbid_emptyGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = {};
        testRemoveWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testTouch() throws InterruptedException {
        long initialLastSeen = testGlobalDiscoveryEntry1.getLastSeenDateMs();
        addEntry(testGlobalDiscoveryEntry1);

        GlobalDiscoveryEntryPersistedKey primaryKey = new GlobalDiscoveryEntryPersistedKey();
        primaryKey.setParticipantId(testGlobalDiscoveryEntry1.getParticipantId());
        primaryKey.setGbid(joynrdefaultgbid);
        GlobalDiscoveryEntryPersisted persisted = entityManager.find(GlobalDiscoveryEntryPersisted.class, primaryKey);
        assertNotNull(persisted);
        assertEquals((Long) initialLastSeen, persisted.getLastSeenDateMs());

        Thread.sleep(1L);

        subject.touch(TOPIC_NAME);
        entityManager.flush();
        entityManager.clear();

        persisted = entityManager.find(GlobalDiscoveryEntryPersisted.class, primaryKey);
        assertNotNull(persisted);
        assertTrue(initialLastSeen < persisted.getLastSeenDateMs());
    }

}
