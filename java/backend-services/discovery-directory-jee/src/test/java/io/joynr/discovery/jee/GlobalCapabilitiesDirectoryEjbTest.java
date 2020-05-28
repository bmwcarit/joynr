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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.function.Function;

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

        expectedGlobalDiscoveryEntry1 = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry1);
        expectedGlobalDiscoveryEntry1_a = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry1_a);
        expectedGlobalDiscoveryEntry2 = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry2);
    }

    private static void checkDiscoveryEntry(GlobalDiscoveryEntry expected,
                                            GlobalDiscoveryEntry actual,
                                            String expectedGbid) {
        String[] expectedGbids = new String[]{ expectedGbid };
        checkDiscoveryEntry(expected, actual, expectedGbids);
    }

    private static void checkDiscoveryEntry(GlobalDiscoveryEntry expected,
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

    private static void checkDiscoveryEntryPersisted(GlobalDiscoveryEntry expected,
                                                     GlobalDiscoveryEntryPersisted actual,
                                                     String expectedGbid) {
        checkDiscoveryEntry(expected, actual, expectedGbid);
        assertEquals(expectedGbid, actual.getGbid());
        assertEquals(TOPIC_NAME, actual.getClusterControllerId());
    }

    private List<GlobalDiscoveryEntryPersisted> queryEntityManagerByDomainInterface(String domain,
                                                                                    String interfaceName) {
        String queryString = "FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.domain = :domain AND gdep.interfaceName = :interfaceName";
        List<GlobalDiscoveryEntryPersisted> result = entityManager.createQuery(queryString,
                                                                               GlobalDiscoveryEntryPersisted.class)
                                                                  .setParameter("domain", domain)
                                                                  .setParameter("interfaceName", interfaceName)
                                                                  .getResultList();
        return result;
    }

    private List<GlobalDiscoveryEntryPersisted> queryEntityManagerByParticipantId(String participantId) {
        String queryString = "FROM GlobalDiscoveryEntryPersisted gdep WHERE gdep.participantId = :participantId";
        List<GlobalDiscoveryEntryPersisted> result = entityManager.createQuery(queryString,
                                                                               GlobalDiscoveryEntryPersisted.class)
                                                                  .setParameter("participantId", participantId)
                                                                  .getResultList();
        return result;
    }

    private void checkEntryIsNotInEntityManager(String testParticipantId1) {
        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantId(testParticipantId1);
        assertEquals(0, result.size());
    }

    // Tests for add
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

    @Test
    public void add_singleDiscoveryEntry() {
        checkEntryIsNotInEntityManager(testParticipantId2);

        addEntry(testGlobalDiscoveryEntry2);

        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantId(testParticipantId2);

        assertNotNull(result);
        assertEquals(1, result.size());
        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry2, result.get(0), JOYNR_DEFAULT_GCD_GBID);
    }

    @Test
    public void add_multipleDiscoveryEntries() {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        addEntry(testGlobalDiscoveryEntry1);
        addEntry(testGlobalDiscoveryEntry2);

        List<GlobalDiscoveryEntryPersisted> result1 = queryEntityManagerByParticipantId(testParticipantId1);
        List<GlobalDiscoveryEntryPersisted> result2 = queryEntityManagerByParticipantId(testParticipantId2);

        assertNotNull(result1);
        assertEquals(1, result1.size());
        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result1.get(0), JOYNR_DEFAULT_GCD_GBID);

        assertNotNull(result2);
        assertEquals(1, result2.size());
        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry2, result2.get(0), JOYNR_DEFAULT_GCD_GBID);
    }

    @Test
    public void addWithGbids_singleDiscoveryEntry_allValidGbids() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId2);

        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);
        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantId(testParticipantId2);
        assertNotNull(result);
        assertEquals(3, result.size());

        List<Integer> found = new ArrayList<Integer>(3);
        for (GlobalDiscoveryEntryPersisted gdep : result) {
            if (validGbidsArray[0].equals(gdep.getGbid())) {
                checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry2, gdep, validGbidsArray[0]);
                found.add(1);
            } else if (validGbidsArray[1].equals(gdep.getGbid())) {
                checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry2, gdep, validGbidsArray[1]);
                found.add(2);
            } else {
                checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry2, gdep, validGbidsArray[2]);
                found.add(3);
            }
        }
        assertTrue(found.containsAll(Arrays.asList(1, 2, 3)));
    }

    @Test
    public void addWithGbids_singleDiscoveryEntry_singleGbid() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        String[] gbidsForAdd = new String[]{ validGbidsArray[1] };

        addEntry(testGlobalDiscoveryEntry1, gbidsForAdd);
        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantId(testParticipantId1);
        assertNotNull(result);
        assertEquals(1, result.size());

        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result.get(0), validGbidsArray[1]);
    }

    @Test
    public void addWithGbids_singleDiscoveryEntry_singleEmptyGbid_replacedByGcdGbid() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        String[] gbidsForAdd = new String[]{ "" };

        addEntry(testGlobalDiscoveryEntry1, gbidsForAdd);
        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantId(testParticipantId1);
        assertNotNull(result);
        assertEquals(1, result.size());

        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result.get(0), validGbidsArray[0]);
    }

    @Test
    public void addWithGbids_singleDiscoveryEntry_multipleGbidsWithEmptyGbid_emptyGbidReplaced() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        String[] gbidsForAdd = new String[]{ validGbidsArray[1], "" };

        addEntry(testGlobalDiscoveryEntry1, gbidsForAdd);
        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantId(testParticipantId1);
        assertNotNull(result);
        assertEquals(2, result.size());

        if (validGbidsArray[1].equals(result.get(0).getGbid())) {
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result.get(0), validGbidsArray[1]);
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result.get(1), validGbidsArray[0]);
        } else {
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result.get(0), validGbidsArray[0]);
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result.get(1), validGbidsArray[1]);
        }
    }

    @Test
    public void addWithGbids_singleDiscoveryEntry_emptyAndGcdGbid_emptyGbidReplaced() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        String[] gbidsForAdd = new String[]{ validGbidsArray[0], "" };

        addEntry(testGlobalDiscoveryEntry1, gbidsForAdd);
        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantId(testParticipantId1);
        assertNotNull(result);
        assertEquals(1, result.size());

        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result.get(0), validGbidsArray[0]);
    }

    @Test
    public void addWithGbids_singleDiscoveryEntry_multipleGbids() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        String[] gbidsForAdd = new String[]{ validGbidsArray[2], validGbidsArray[1] };

        addEntry(testGlobalDiscoveryEntry1, gbidsForAdd);
        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantId(testParticipantId1);
        assertNotNull(result);
        assertEquals(2, result.size());

        if (validGbidsArray[1].equals(result.get(0).getGbid())) {
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result.get(0), validGbidsArray[1]);
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result.get(1), validGbidsArray[2]);
        } else {
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result.get(1), validGbidsArray[1]);
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, result.get(0), validGbidsArray[2]);
        }
    }

    @Test
    public void addSameEntryTwice() throws Exception {
        addEntry(testGlobalDiscoveryEntry1);
        addEntry(testGlobalDiscoveryEntry1);

        List<GlobalDiscoveryEntryPersisted> persisted = queryEntityManagerByDomainInterface(domain, interfaceName1);
        assertNotNull(persisted);
        assertEquals(1, persisted.size());
        checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, persisted.get(0), JOYNR_DEFAULT_GCD_GBID);
    }

    @Test
    public void addSameEntryTwiceWithGbids() throws Exception {
        String[] gbidsForAdd1 = new String[]{ validGbidsArray[2] };
        String[] gbidsForAdd2 = new String[]{ validGbidsArray[2], validGbidsArray[1] };
        addEntry(testGlobalDiscoveryEntry1, gbidsForAdd1);
        addEntry(testGlobalDiscoveryEntry1, gbidsForAdd2);

        List<GlobalDiscoveryEntryPersisted> persisted = queryEntityManagerByDomainInterface(domain, interfaceName1);
        assertNotNull(persisted);
        assertEquals(2, persisted.size());
        if (validGbidsArray[2].equals(persisted.get(0).getGbid())) {
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, persisted.get(0), validGbidsArray[2]);
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, persisted.get(1), validGbidsArray[1]);
        } else {
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, persisted.get(1), validGbidsArray[2]);
            checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, persisted.get(0), validGbidsArray[1]);
        }
    }

    @Test
    public void addSameParticipantIdTwice() throws Exception {
        GlobalDiscoveryEntry entry2 = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry1.getProviderVersion(),
                                                               testGlobalDiscoveryEntry1.getDomain(),
                                                               testGlobalDiscoveryEntry1.getInterfaceName(),
                                                               testGlobalDiscoveryEntry1.getParticipantId(),
                                                               testGlobalDiscoveryEntry1.getQos(),
                                                               testGlobalDiscoveryEntry1.getLastSeenDateMs() + 42,
                                                               testGlobalDiscoveryEntry1.getExpiryDateMs() + 42,
                                                               testGlobalDiscoveryEntry1.getPublicKeyId(),
                                                               testGlobalDiscoveryEntry1.getAddress());
        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(entry2);
        addEntry(testGlobalDiscoveryEntry1);
        addEntry(entry2);

        List<GlobalDiscoveryEntryPersisted> persisted = queryEntityManagerByDomainInterface(domain, interfaceName1);
        assertNotNull(persisted);
        assertEquals(1, persisted.size());
        checkDiscoveryEntryPersisted(expectedEntry, persisted.get(0), JOYNR_DEFAULT_GCD_GBID);
    }

    @Test
    public void addSameParticipantIdTwiceWithGbids() throws Exception {
        String[] gbidsForAdd1 = new String[]{ validGbidsArray[2], validGbidsArray[0] };
        String[] gbidsForAdd2 = new String[]{ validGbidsArray[2], validGbidsArray[1] };
        GlobalDiscoveryEntry entry2 = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry1.getProviderVersion(),
                                                               testGlobalDiscoveryEntry1.getDomain(),
                                                               testGlobalDiscoveryEntry1.getInterfaceName(),
                                                               testGlobalDiscoveryEntry1.getParticipantId(),
                                                               testGlobalDiscoveryEntry1.getQos(),
                                                               testGlobalDiscoveryEntry1.getLastSeenDateMs() + 42,
                                                               testGlobalDiscoveryEntry1.getExpiryDateMs() + 42,
                                                               testGlobalDiscoveryEntry1.getPublicKeyId(),
                                                               testGlobalDiscoveryEntry1.getAddress());
        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(entry2);
        addEntry(testGlobalDiscoveryEntry1, gbidsForAdd1);
        addEntry(entry2, gbidsForAdd2);

        List<GlobalDiscoveryEntryPersisted> persisted = queryEntityManagerByDomainInterface(domain, interfaceName1);
        assertNotNull(persisted);
        assertEquals(3, persisted.size());
        List<Integer> found = new ArrayList<Integer>(3);
        for (GlobalDiscoveryEntryPersisted gdep : persisted) {
            if (validGbidsArray[0].equals(gdep.getGbid())) {
                checkDiscoveryEntryPersisted(expectedGlobalDiscoveryEntry1, gdep, validGbidsArray[0]);
                found.add(1);
            } else if (validGbidsArray[1].equals(gdep.getGbid())) {
                checkDiscoveryEntryPersisted(expectedEntry, gdep, validGbidsArray[1]);
                found.add(2);
            } else {
                checkDiscoveryEntryPersisted(expectedEntry, gdep, validGbidsArray[2]);
                found.add(3);
            }
        }
        assertTrue(found.containsAll(Arrays.asList(1, 2, 3)));
    }

    private void testAddWithGbids_discoveryError(String[] gbids, DiscoveryError expectedError) {
        try {
            addEntry(testGlobalDiscoveryEntry1, gbids);
            fail("expected ApplicationException");
        } catch (ApplicationException e) {
            assertEquals(expectedError, e.getError());
        }
        checkEntryIsNotInEntityManager(testParticipantId1);
    }

    @Test
    public void addWithGbids_unknownGbid() {
        final String[] invalidGbidsArray = { "unknownGbid" };
        testAddWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void addWithGbids_invalidGbid_nullGbid() {
        final String[] invalidGbidsArray = { null };
        testAddWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void addWithGbids_invalidGbid_nullGbidsArray() {
        final String[] invalidGbidsArray = null;
        testAddWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void addWithGbids_invalidGbid_emptyGbidsArray() {
        final String[] invalidGbidsArray = new String[0];
        testAddWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void addWithGbids_invalidGbid_duplicateGbid() {
        final String[] invalidGbidsArray = { validGbidsArray[2], validGbidsArray[1], validGbidsArray[2] };
        testAddWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    // Tests for lookup by participantId
    @Test
    public void lookupParticipantId_singleMatchingEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        addEntry(testGlobalDiscoveryEntry1, new String[]{ JOYNR_DEFAULT_GCD_GBID, validGbidsArray[2] });
        addEntry(testGlobalDiscoveryEntry2, new String[]{ JOYNR_DEFAULT_GCD_GBID });

        GlobalDiscoveryEntry result1 = subject.lookup(testParticipantId1);
        assertNotNull(result1);
        assertFalse(result1 instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result1, JOYNR_DEFAULT_GCD_GBID);

        GlobalDiscoveryEntry result2 = subject.lookup(testParticipantId2);
        assertNotNull(result2);
        assertFalse(result2 instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry2, result2, JOYNR_DEFAULT_GCD_GBID);
    }

    @Test
    public void lookupParticipantId_singleMatchingEntryEmptyGbidOnAdd() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        addEntry(testGlobalDiscoveryEntry1, new String[]{ "", validGbidsArray[2] });
        addEntry(testGlobalDiscoveryEntry2, new String[]{ "" });

        GlobalDiscoveryEntry result1 = subject.lookup(testParticipantId1);
        assertNotNull(result1);
        assertFalse(result1 instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result1, JOYNR_DEFAULT_GCD_GBID);

        GlobalDiscoveryEntry result2 = subject.lookup(testParticipantId2);
        assertNotNull(result2);
        assertFalse(result2 instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry2, result2, JOYNR_DEFAULT_GCD_GBID);
    }

    @Test
    public void lookupParticipantIdWithGbids_defaultGbid_singleMatchingEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ JOYNR_DEFAULT_GCD_GBID };
        String expectedGbid = JOYNR_DEFAULT_GCD_GBID;

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[2], JOYNR_DEFAULT_GCD_GBID });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, selectedGbids);
        assertNotNull(result);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, expectedGbid);
    }

    @Test
    public void lookupParticipantIdWithGbids_singleEmptyGbid_replacedByGcdGbid() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ "" };
        String expectedGbid = JOYNR_DEFAULT_GCD_GBID;

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[2], JOYNR_DEFAULT_GCD_GBID });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, selectedGbids);
        assertNotNull(result);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, expectedGbid);
    }

    @Test
    public void lookupParticipantIdWithGbids_multipleGbidsWithEmptyGbid_emptyGbidReplaced() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ validGbidsArray[2], "", validGbidsArray[1] };
        String expectedGbid = JOYNR_DEFAULT_GCD_GBID;

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[2], JOYNR_DEFAULT_GCD_GBID });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, selectedGbids);
        assertNotNull(result);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, expectedGbid);
    }

    @Test
    public void lookupParticipantIdWithGbids_emptyAndGcdGbid_emptyGbidReplaced() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ "", JOYNR_DEFAULT_GCD_GBID };
        String expectedGbid = JOYNR_DEFAULT_GCD_GBID;

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[2], JOYNR_DEFAULT_GCD_GBID });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, selectedGbids);
        assertNotNull(result);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, expectedGbid);
    }

    @Test
    public void lookupParticipantIdWithGbids_otherGbid_singleMatchingEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ validGbidsArray[1] };
        String expectedGbid = validGbidsArray[1];

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1], JOYNR_DEFAULT_GCD_GBID });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, selectedGbids);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, expectedGbid);
    }

    @Test
    public void lookupParticipantIdWithGbids_multipleGbids_singleMatchingEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ JOYNR_DEFAULT_GCD_GBID, validGbidsArray[1] };
        String expectedGbid = validGbidsArray[1];

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1] });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, selectedGbids);
        assertNotNull(result);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, expectedGbid);
    }

    @Test
    public void lookupParticipantId_multipleMatchingEntries_onlyOneReturned() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1);
        assertNotNull(result);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, JOYNR_DEFAULT_GCD_GBID);
    }

    @Test
    public void lookupParticipantIdWithGbids_defaultGbid_multipleMatchingEntries_onlyOneReturned() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        addEntry(testGlobalDiscoveryEntry1, new String[]{ JOYNR_DEFAULT_GCD_GBID, validGbidsArray[1] });
        addEntry(testGlobalDiscoveryEntry2, new String[]{ JOYNR_DEFAULT_GCD_GBID });

        String[] selectedGbids = new String[]{ JOYNR_DEFAULT_GCD_GBID };
        String expectedGbid = JOYNR_DEFAULT_GCD_GBID;

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, selectedGbids);
        assertNotNull(result);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, expectedGbid);
    }

    @Test
    public void lookupParticipantIdWithGbids_otherGbid_multipleMatchingEntries_onlyOneReturned() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        addEntry(testGlobalDiscoveryEntry1, new String[]{ JOYNR_DEFAULT_GCD_GBID, validGbidsArray[1] });
        addEntry(testGlobalDiscoveryEntry2, new String[]{ JOYNR_DEFAULT_GCD_GBID });

        String[] selectedGbids = new String[]{ validGbidsArray[1] };
        String expectedGbid = validGbidsArray[1];

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, selectedGbids);
        assertNotNull(result);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, expectedGbid);
    }

    @Test
    public void lookupParticipantIdWithGbids_multipleGbids_multipleMatchingEntries_onlyOneReturned() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ validGbidsArray[1], validGbidsArray[2] };
        String[] expectedGbids = selectedGbids.clone();

        addEntry(testGlobalDiscoveryEntry1, new String[]{ JOYNR_DEFAULT_GCD_GBID, validGbidsArray[1] });
        addEntry(testGlobalDiscoveryEntry2, selectedGbids.clone());

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId1, selectedGbids);
        assertNotNull(result);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result, expectedGbids);
    }

    private void testLookupByParticipantIdWithGbids_discoveryError(String[] gbids, DiscoveryError expectedError) {
        GlobalDiscoveryEntry result = null;
        try {
            result = subject.lookup(testParticipantId1, gbids);
            fail("Should throw ApplicationException");
        } catch (ApplicationException e) {
            assertEquals(expectedError, e.getError());
            assertNull(result);
        }
    }

    @Test
    public void lookupParticipantWithGbids_unknownGbid() {
        final String[] invalidGbidsArray = { JOYNR_DEFAULT_GCD_GBID, validGbidsArray[1], "unknowngbid" };
        testLookupByParticipantIdWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void lookupParticipantIdWithGbids_invalidGbid_nullGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { validGbidsArray[2], null };
        testLookupByParticipantIdWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupParticipantIdWithGbids_invalidGbid_nullGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = null;
        testLookupByParticipantIdWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupParticipantIdWithGbids_invalidGbid_emptyGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = new String[0];
        testLookupByParticipantIdWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupParticipantIdWithGbids_invalidGbid_duplicatedGbid() {
        final String[] invalidGbidsArray = { JOYNR_DEFAULT_GCD_GBID, validGbidsArray[1], validGbidsArray[1] };
        testLookupByParticipantIdWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupParticipantIdWithGbids_NO_ENTRY_FOR_PARTICIPANT() {
        testLookupByParticipantIdWithGbids_discoveryError(validGbidsArray, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test
    public void lookupParticipantIdWithGbids_NO_ENTRY_FOR_SELECTED_BACKENDS() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1] });

        String[] queriedValidGbids = { validGbidsArray[0], validGbidsArray[2] };
        testLookupByParticipantIdWithGbids_discoveryError(queriedValidGbids,
                                                          DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    private void testLookupByParticipantId_discoveryError(String participantId, DiscoveryError expectedError) {
        GlobalDiscoveryEntry result = null;
        try {
            result = subject.lookup(participantId);
            fail("Should throw ProviderRuntimeException");
        } catch (ProviderRuntimeException e) {
            assertTrue(e.getMessage().contains(expectedError.name()));
            assertNull(result);
        }
    }

    @Test
    public void lookupParticipantId_NO_ENTRY_FOR_PARTICIPANT() {
        testLookupByParticipantId_discoveryError(testParticipantId1, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test
    public void lookupParticipantId_NO_ENTRY_FOR_SELECTED_BACKENDS() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1] });
        testLookupByParticipantId_discoveryError(testParticipantId1, DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    // Tests for lookup by domain and interface
    @Test
    public void lookupDomainInterface_singleMatchingEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId1_a);
        checkEntryIsNotInEntityManager(testParticipantId2);

        addEntry(testGlobalDiscoveryEntry1, new String[]{ JOYNR_DEFAULT_GCD_GBID, validGbidsArray[2] });
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ validGbidsArray[1] });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1);
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], JOYNR_DEFAULT_GCD_GBID);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_defaultGbid_singleMatchingEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId1_a);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ JOYNR_DEFAULT_GCD_GBID };
        String expectedGbid = JOYNR_DEFAULT_GCD_GBID;

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[2], JOYNR_DEFAULT_GCD_GBID });
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ validGbidsArray[2], validGbidsArray[1] });
        addEntry(testGlobalDiscoveryEntry2, new String[]{ JOYNR_DEFAULT_GCD_GBID });

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, selectedGbids);
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], expectedGbid);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_otherGbid_singleMatchingEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId1_a);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ validGbidsArray[1] };
        String expectedGbid = validGbidsArray[1];

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1], JOYNR_DEFAULT_GCD_GBID });
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ validGbidsArray[0], validGbidsArray[2] });
        addEntry(testGlobalDiscoveryEntry2, new String[]{ validGbidsArray[0], validGbidsArray[1] });

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, selectedGbids);
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], expectedGbid);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_singleEmptyGbid_replacedByGcdGbid() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId1_a);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ "" };
        String expectedGbid = validGbidsArray[0];

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1], validGbidsArray[2] });
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ validGbidsArray[0], validGbidsArray[2] });
        addEntry(testGlobalDiscoveryEntry2, new String[]{ validGbidsArray[0], validGbidsArray[1] });

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, selectedGbids);
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[0], expectedGbid);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_multipleGbidsWithEmptyGbid_emptyGbidReplaced() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId1_a);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ validGbidsArray[2], validGbidsArray[1], "" };
        String expectedGbid1 = validGbidsArray[1];
        String expectedGbid2 = validGbidsArray[0];

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1] });
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ validGbidsArray[0] });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, selectedGbids.clone());
        assertNotNull(result);
        assertEquals(2, result.length);
        if (expectedGlobalDiscoveryEntry1.getParticipantId().equals(result[0].getParticipantId())) {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], expectedGbid1);
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[1], expectedGbid2);
        } else {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[1], expectedGbid1);
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[0], expectedGbid2);
        }
    }

    @Test
    public void lookupDomainInterfaceWithGbids_emptyAndGcdGbid_emptyGbidReplaced() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId1_a);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ "", validGbidsArray[0] };
        String expectedGbid = validGbidsArray[0];

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1] });
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ validGbidsArray[0] });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, selectedGbids.clone());
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[0], expectedGbid);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_multipleGbids_singleMatchingEntry() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId1_a);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ validGbidsArray[2], validGbidsArray[1] };
        String expectedGbid = validGbidsArray[1];

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1] });
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ validGbidsArray[0] });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, selectedGbids.clone());
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], expectedGbid);
    }

    @Test
    public void lookupDomainInterface_multipleMatchingEntries_onlyOneReturnedPerParticipantId() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId1_a);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] multipleValidGbids = new String[]{ validGbidsArray[1], JOYNR_DEFAULT_GCD_GBID };
        addEntry(testGlobalDiscoveryEntry1, multipleValidGbids);
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ JOYNR_DEFAULT_GCD_GBID });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1);
        assertNotNull(result);
        assertEquals(2, result.length);
        if (expectedGlobalDiscoveryEntry1.getParticipantId().equals(result[0].getParticipantId())) {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], JOYNR_DEFAULT_GCD_GBID);
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[1], JOYNR_DEFAULT_GCD_GBID);
        } else {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[1], JOYNR_DEFAULT_GCD_GBID);
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[0], JOYNR_DEFAULT_GCD_GBID);
        }
    }

    @Test
    public void lookupDomainInterfaceWithGbids_defaultGbid_multipleMatchingEntries_onlyOneReturnedPerParticipantId() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId1_a);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ JOYNR_DEFAULT_GCD_GBID };
        String expectedGbid = JOYNR_DEFAULT_GCD_GBID;

        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ JOYNR_DEFAULT_GCD_GBID });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, selectedGbids);
        assertEquals(2, result.length);
        if (expectedGlobalDiscoveryEntry1.getParticipantId().equals(result[0].getParticipantId())) {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], expectedGbid);
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[1], expectedGbid);
        } else {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[1], expectedGbid);
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[0], expectedGbid);
        }
    }

    @Test
    public void lookupDomainInterfaceWithGbids_otherGbid_multipleMatchingEntries_onlyOneReturnedPerParticipantId() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId1_a);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ validGbidsArray[1] };
        String expectedGbid = validGbidsArray[1];

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1], validGbidsArray[2] });
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ JOYNR_DEFAULT_GCD_GBID });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, selectedGbids);
        assertNotNull(result);
        assertEquals(1, result.length);
        checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], expectedGbid);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_multipleGbids_multipleMatchingEntries_onlyOneReturnedPerParticipantId() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        checkEntryIsNotInEntityManager(testParticipantId1_a);
        checkEntryIsNotInEntityManager(testParticipantId2);

        String[] selectedGbids = new String[]{ validGbidsArray[2], validGbidsArray[1] };
        String[] expectedGbids = selectedGbids.clone();

        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);
        addEntry(testGlobalDiscoveryEntry1_a, new String[]{ validGbidsArray[2] });
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, selectedGbids);
        assertNotNull(result);
        assertEquals(2, result.length);
        if (expectedGlobalDiscoveryEntry1.getParticipantId().equals(result[0].getParticipantId())) {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[0], expectedGbids);
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[1], expectedGbids);
        } else {
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1, result[1], expectedGbids);
            checkDiscoveryEntry(expectedGlobalDiscoveryEntry1_a, result[0], expectedGbids);
        }
    }

    private void testLookupDomainInterfaceWithGbids_discoveryError(String[] gbids, DiscoveryError expectedError) {
        try {
            subject.lookup(domains, interfaceName1, gbids);
            fail("expected ApplicationException");
        } catch (ApplicationException e) {
            assertEquals(expectedError, e.getError());
        }
    }

    @Test
    public void lookupDomainInterfaceWithGbids_unknownGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { "unknownGbid" };
        testLookupDomainInterfaceWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_invalidGbid_nullGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { null };
        testLookupDomainInterfaceWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_invalidGbid_nullGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = null;
        testLookupDomainInterfaceWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_invalidGbid_emptyGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = new String[0];
        testLookupDomainInterfaceWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_invalidGbid_duplicateGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { validGbidsArray[2], validGbidsArray[2], JOYNR_DEFAULT_GCD_GBID };
        testLookupDomainInterfaceWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_noMatchingEntry() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        String[] selectedGbids = new String[]{ validGbidsArray[0], validGbidsArray[1] };

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1, selectedGbids);
        assertNotNull(result);
        assertEquals(0, result.length);
    }

    @Test
    public void lookupDomainInterfaceWithGbids_NO_ENTRY_FOR_SELECTED_BACKENDS() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);

        String[] selectedGbids = new String[]{ validGbidsArray[0], validGbidsArray[2] };

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1] });

        testLookupDomainInterfaceWithGbids_discoveryError(selectedGbids, DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    @Test
    public void lookupDomainInterface_noMatchingEntry() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry2, validGbidsArray);

        GlobalDiscoveryEntry[] result = subject.lookup(domains, interfaceName1);
        assertNotNull(result);
        assertEquals(0, result.length);
    }

    @Test
    public void lookupDomainInterface_NO_ENTRY_FOR_SELECTED_BACKENDS() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);

        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1] });

        try {
            subject.lookup(domains, interfaceName1);
            fail("expected ProviderRuntimeException");
        } catch (ProviderRuntimeException e) {
            assertNotNull(e.getMessage());
            assertTrue(e.getMessage().contains(DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS.toString()));
        }
    }

    // Tests for remove
    @Test
    public void remove_entryForDefaultGbidOnly() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1, new String[]{ JOYNR_DEFAULT_GCD_GBID });

        List<GlobalDiscoveryEntryPersisted> entriesBeforeRemove = queryEntityManagerByParticipantId(testParticipantId1);
        assertEquals(1, entriesBeforeRemove.size());

        subject.remove(testParticipantId1);
        entityManager.flush();
        entityManager.clear();

        checkEntryIsNotInEntityManager(testParticipantId1);
    }

    @Test
    public void remove_entryForMultipleGbids() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1, new String[]{ validGbidsArray[1], JOYNR_DEFAULT_GCD_GBID });

        List<GlobalDiscoveryEntryPersisted> entriesBeforeRemove = queryEntityManagerByParticipantId(testParticipantId1);
        assertEquals(2, entriesBeforeRemove.size());
        assertTrue(JOYNR_DEFAULT_GCD_GBID.equals(entriesBeforeRemove.get(0).getGbid())
                ^ JOYNR_DEFAULT_GCD_GBID.equals(entriesBeforeRemove.get(1).getGbid()));
        assertTrue(validGbidsArray[1].equals(entriesBeforeRemove.get(0).getGbid())
                || validGbidsArray[1].equals(entriesBeforeRemove.get(1).getGbid()));

        subject.remove(testParticipantId1);
        entityManager.flush();
        entityManager.clear();

        List<GlobalDiscoveryEntryPersisted> entriesAfterRemove = queryEntityManagerByParticipantId(testParticipantId1);
        assertEquals(1, entriesAfterRemove.size());
        assertEquals(validGbidsArray[1], entriesAfterRemove.get(0).getGbid());
    }

    @Test
    public void testRemoveWithGbids_multipleGbids_entryForSelectedGbids() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);

        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);

        List<GlobalDiscoveryEntryPersisted> entriesBeforeRemove = queryEntityManagerByParticipantId(testParticipantId1);
        assertEquals(3, entriesBeforeRemove.size());

        subject.remove(testParticipantId1, validGbidsArray);
        entityManager.flush();
        entityManager.clear();

        checkEntryIsNotInEntityManager(testParticipantId1);
    }

    @Test
    public void testRemoveWithGbids_multipleGbidsWithEmptyGbid_entryForSelectedGbids() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);
        String[] gbidArrayWithEmptyDefault = validGbidsArray.clone();
        gbidArrayWithEmptyDefault[0] = "";

        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);

        List<GlobalDiscoveryEntryPersisted> entriesBeforeRemove = queryEntityManagerByParticipantId(testParticipantId1);
        assertEquals(3, entriesBeforeRemove.size());

        subject.remove(testParticipantId1, gbidArrayWithEmptyDefault);
        entityManager.flush();
        entityManager.clear();

        checkEntryIsNotInEntityManager(testParticipantId1);
    }

    @Test
    public void testRemoveWithGbids_multipleGbids_entryForSelectedAndOtherGbids() throws ApplicationException {
        checkEntryIsNotInEntityManager(testParticipantId1);

        addEntry(testGlobalDiscoveryEntry1, validGbidsArray);

        List<GlobalDiscoveryEntryPersisted> entriesBeforeRemove = queryEntityManagerByParticipantId(testParticipantId1);
        assertEquals(3, entriesBeforeRemove.size());

        subject.remove(testParticipantId1, new String[]{ validGbidsArray[2], validGbidsArray[0] });
        entityManager.flush();
        entityManager.clear();

        List<GlobalDiscoveryEntryPersisted> entriesAfterRemove = queryEntityManagerByParticipantId(testParticipantId1);
        assertEquals(1, entriesAfterRemove.size());
        assertEquals(validGbidsArray[1], entriesAfterRemove.get(0).getGbid());
    }

    private void testRemoveWithGbids_discoveryError(String[] invalidGbidsArray, DiscoveryError expectedError) {
        try {
            subject.remove(testParticipantId1, invalidGbidsArray);
            fail("expected ApplicationException");
        } catch (ApplicationException e) {
            assertEquals(expectedError, e.getError());
        }
    }

    @Test
    public void removeWithGbids_unknownGbid() {
        final String[] invalidGbidsArray = { JOYNR_DEFAULT_GCD_GBID, "unknownGBID" };
        testRemoveWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.UNKNOWN_GBID);
    }

    @Test
    public void removeWithGbids_invalidGbid_nullGbid() {
        final String[] invalidGbidsArray = { null };
        testRemoveWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void removeWithGbids_invalidGbid_nullGbidsArray() {
        final String[] invalidGbidsArray = null;
        testRemoveWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void removeWithGbids_invalidGbid_emptyGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = new String[0];
        testRemoveWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void removeWithGbids_invalidGbid_duplicateGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { JOYNR_DEFAULT_GCD_GBID, JOYNR_DEFAULT_GCD_GBID, validGbidsArray[2] };
        testRemoveWithGbids_discoveryError(invalidGbidsArray, DiscoveryError.INVALID_GBID);
    }

    @Test
    public void removeWithGbids_NO_ENTRY_FOR_PARTICIPANT() {
        testRemoveWithGbids_discoveryError(validGbidsArray, DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
    }

    @Test
    public void removeWithGbids_NO_ENTRY_FOR_SELECTED_BACKENDS() throws ApplicationException {
        addEntry(testGlobalDiscoveryEntry1, new String[]{ JOYNR_DEFAULT_GCD_GBID });
        testRemoveWithGbids_discoveryError(new String[]{ validGbidsArray[1], validGbidsArray[2] },
                                           DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS);
    }

    // Other test cases
    @Test
    public void touch_updatesEntries() throws InterruptedException {
        final long toleranceMs = 100;
        long initialLastSeen = testGlobalDiscoveryEntry1.getLastSeenDateMs();
        long initialExpiryDate = testGlobalDiscoveryEntry1.getExpiryDateMs();
        addEntry(testGlobalDiscoveryEntry1);

        GlobalDiscoveryEntryPersistedKey primaryKey = new GlobalDiscoveryEntryPersistedKey();
        primaryKey.setParticipantId(testGlobalDiscoveryEntry1.getParticipantId());
        primaryKey.setGbid(JOYNR_DEFAULT_GCD_GBID);
        GlobalDiscoveryEntryPersisted persisted = entityManager.find(GlobalDiscoveryEntryPersisted.class, primaryKey);
        assertNotNull(persisted);
        assertEquals((Long) initialLastSeen, persisted.getLastSeenDateMs());

        Thread.sleep(1L);

        long now = System.currentTimeMillis();
        long expectedExpiryDate = now + TestJoynrConfigurationProvider.DEFAULT_EXPIRY_INTERVAL_MS;
        subject.touch(TOPIC_NAME);
        entityManager.flush();
        entityManager.clear();

        persisted = entityManager.find(GlobalDiscoveryEntryPersisted.class, primaryKey);
        assertNotNull(persisted);
        assertTrue(initialLastSeen < persisted.getLastSeenDateMs());
        assertTrue((persisted.getLastSeenDateMs() - now) < toleranceMs);
        assertTrue(initialExpiryDate < persisted.getExpiryDateMs());
        assertTrue((persisted.getExpiryDateMs() - expectedExpiryDate) < toleranceMs);
    }

    @Test
    public void touchWithParticipantIds_updatesSelectedParticipantIds() throws Exception {
        final long toleranceMs = 100;
        Function<GlobalDiscoveryEntryPersisted, GlobalDiscoveryEntryPersisted> containsUntouched = new Function<GlobalDiscoveryEntryPersisted, GlobalDiscoveryEntryPersisted>() {
            @Override
            public GlobalDiscoveryEntryPersisted apply(GlobalDiscoveryEntryPersisted entry) {
                GlobalDiscoveryEntryPersistedKey primaryKey = new GlobalDiscoveryEntryPersistedKey();
                primaryKey.setParticipantId(entry.getParticipantId());
                primaryKey.setGbid(entry.getGbid());
                GlobalDiscoveryEntryPersisted returnedEntry = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                                 primaryKey);
                assertNotNull(returnedEntry);
                assertEquals(entry, returnedEntry);
                return returnedEntry;
            }
        };
        Function<GlobalDiscoveryEntryPersisted, GlobalDiscoveryEntryPersisted> containsTouched = new Function<GlobalDiscoveryEntryPersisted, GlobalDiscoveryEntryPersisted>() {
            @Override
            public GlobalDiscoveryEntryPersisted apply(GlobalDiscoveryEntryPersisted entry) {
                GlobalDiscoveryEntryPersistedKey primaryKey = new GlobalDiscoveryEntryPersistedKey();
                primaryKey.setParticipantId(entry.getParticipantId());
                primaryKey.setGbid(entry.getGbid());
                GlobalDiscoveryEntryPersisted returnedEntry = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                                 primaryKey);
                assertNotNull(returnedEntry);
                assertNotEquals(entry, returnedEntry);
                return returnedEntry;
            }
        };

        // prepare entries
        long initialLastSeen1 = testGlobalDiscoveryEntry1.getLastSeenDateMs();
        long initialLastSeen2 = testGlobalDiscoveryEntry2.getLastSeenDateMs();
        long initialExpiryDate1 = testGlobalDiscoveryEntry1.getExpiryDateMs();
        long initialExpiryDate2 = testGlobalDiscoveryEntry2.getExpiryDateMs();

        GlobalDiscoveryEntryPersisted testGdep1 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry1,
                                                                                    TOPIC_NAME,
                                                                                    JOYNR_DEFAULT_GCD_GBID);
        MqttAddress address1 = (MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(testGdep1);
        address1.setBrokerUri(JOYNR_DEFAULT_GCD_GBID);
        testGdep1.setAddress(CapabilityUtils.serializeAddress(address1));

        GlobalDiscoveryEntryPersisted testGdep2 = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry2,
                                                                                    TOPIC_NAME,
                                                                                    JOYNR_DEFAULT_GCD_GBID);
        MqttAddress address2 = (MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(testGdep2);
        address2.setBrokerUri(JOYNR_DEFAULT_GCD_GBID);
        testGdep2.setAddress(CapabilityUtils.serializeAddress(address2));

        // add entries
        addEntry(testGlobalDiscoveryEntry1);
        addEntry(testGlobalDiscoveryEntry2);

        Thread.sleep(1);
        // check if untouched + lastSeenDateMs < System.currentTimeMillis()
        long currentTimeMillisBefore = System.currentTimeMillis();
        assertTrue(initialLastSeen1 < currentTimeMillisBefore);
        assertTrue(initialLastSeen2 < currentTimeMillisBefore);

        GlobalDiscoveryEntryPersisted returnedEntry = containsUntouched.apply(testGdep1);
        assertEquals(initialLastSeen1, returnedEntry.getLastSeenDateMs().longValue());
        assertEquals(initialExpiryDate1, returnedEntry.getExpiryDateMs().longValue());
        returnedEntry = containsUntouched.apply(testGdep2);
        assertEquals(initialLastSeen2, returnedEntry.getLastSeenDateMs().longValue());
        assertEquals(initialExpiryDate2, returnedEntry.getExpiryDateMs().longValue());

        // call touch for clusterControllerId and participantId 2 and check lastSeenDateMs
        subject.touch(TOPIC_NAME, new String[]{ testParticipantId2 });
        entityManager.flush();
        entityManager.clear();

        Thread.sleep(1);
        long currentTimeMillisAfter1 = System.currentTimeMillis();
        long expectedExpiryDate = currentTimeMillisBefore + TestJoynrConfigurationProvider.DEFAULT_EXPIRY_INTERVAL_MS;

        returnedEntry = containsUntouched.apply(testGdep1);
        assertTrue(returnedEntry.getLastSeenDateMs() < currentTimeMillisBefore);
        assertEquals(initialLastSeen1, returnedEntry.getLastSeenDateMs().longValue());
        assertEquals(initialExpiryDate1, returnedEntry.getExpiryDateMs().longValue());

        returnedEntry = containsTouched.apply(testGdep2);
        assertTrue(returnedEntry.getLastSeenDateMs() > currentTimeMillisBefore);
        assertTrue(returnedEntry.getLastSeenDateMs() < currentTimeMillisAfter1);
        assertTrue(returnedEntry.getExpiryDateMs() - expectedExpiryDate < toleranceMs);

        // call touch for clusterControllerId and all participantIds and check lastSeenDateMs
        subject.touch(TOPIC_NAME, new String[]{ testParticipantId2, testParticipantId1 });
        entityManager.flush();
        entityManager.clear();

        Thread.sleep(1);
        long currentTimeMillisAfter2 = System.currentTimeMillis();
        expectedExpiryDate = currentTimeMillisAfter1 + TestJoynrConfigurationProvider.DEFAULT_EXPIRY_INTERVAL_MS;

        returnedEntry = containsTouched.apply(testGdep1);
        assertTrue(returnedEntry.getLastSeenDateMs() > currentTimeMillisBefore);
        assertTrue(returnedEntry.getLastSeenDateMs() > currentTimeMillisAfter1);
        assertTrue(returnedEntry.getLastSeenDateMs() < currentTimeMillisAfter2);
        assertTrue(returnedEntry.getExpiryDateMs() - expectedExpiryDate < toleranceMs);

        returnedEntry = containsTouched.apply(testGdep2);
        assertTrue(returnedEntry.getLastSeenDateMs() > currentTimeMillisBefore);
        assertTrue(returnedEntry.getLastSeenDateMs() > currentTimeMillisAfter1);
        assertTrue(returnedEntry.getLastSeenDateMs() < currentTimeMillisAfter2);
        assertTrue(returnedEntry.getExpiryDateMs() - expectedExpiryDate < toleranceMs);
    }

    @Test
    public void touchWithParticipantIds_doesNotUpdateEntriesFromOtherClusterControllers() throws Exception {
        // prepare entries
        String clusterControllerId = "testTouchClusterControllerId";
        GlobalDiscoveryEntry touchedDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(testGlobalDiscoveryEntry1.getProviderVersion(),
                                                                                             testGlobalDiscoveryEntry1.getDomain(),
                                                                                             testGlobalDiscoveryEntry1.getInterfaceName(),
                                                                                             testGlobalDiscoveryEntry1.getParticipantId(),
                                                                                             testGlobalDiscoveryEntry1.getQos(),
                                                                                             testGlobalDiscoveryEntry1.getLastSeenDateMs(),
                                                                                             testGlobalDiscoveryEntry1.getExpiryDateMs(),
                                                                                             testGlobalDiscoveryEntry1.getPublicKeyId(),
                                                                                             new MqttAddress("tcp://mqttbroker:1883",
                                                                                                             clusterControllerId));
        GlobalDiscoveryEntryPersisted touchedGdep = new GlobalDiscoveryEntryPersisted(touchedDiscoveryEntry,
                                                                                      clusterControllerId,
                                                                                      JOYNR_DEFAULT_GCD_GBID);
        MqttAddress address1 = (MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(touchedGdep);
        address1.setBrokerUri(JOYNR_DEFAULT_GCD_GBID);
        touchedGdep.setAddress(CapabilityUtils.serializeAddress(address1));

        GlobalDiscoveryEntryPersistedKey touchedKey = new GlobalDiscoveryEntryPersistedKey();
        touchedKey.setParticipantId(touchedDiscoveryEntry.getParticipantId());
        touchedKey.setGbid(touchedGdep.getGbid());

        GlobalDiscoveryEntryPersisted untouchedGdep = new GlobalDiscoveryEntryPersisted(testGlobalDiscoveryEntry2,
                                                                                        TOPIC_NAME,
                                                                                        JOYNR_DEFAULT_GCD_GBID);
        MqttAddress address2 = (MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(untouchedGdep);
        address2.setBrokerUri(JOYNR_DEFAULT_GCD_GBID);
        untouchedGdep.setAddress(CapabilityUtils.serializeAddress(address2));

        GlobalDiscoveryEntryPersistedKey unTouchedKey = new GlobalDiscoveryEntryPersistedKey();
        unTouchedKey.setParticipantId(testGlobalDiscoveryEntry2.getParticipantId());
        unTouchedKey.setGbid(untouchedGdep.getGbid());

        // add entries
        addEntry(touchedDiscoveryEntry);
        addEntry(testGlobalDiscoveryEntry2);

        // wait some time to ensure new lastSeenDateMs
        Thread.sleep(1);

        GlobalDiscoveryEntryPersisted returnedEntry = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                         touchedKey);
        assertEquals(touchedGdep, returnedEntry);
        returnedEntry = entityManager.find(GlobalDiscoveryEntryPersisted.class, unTouchedKey);
        assertEquals(untouchedGdep, returnedEntry);

        // call touch for clusterControllerId and all participantIds and check discoveryEntries
        subject.touch(clusterControllerId,
                      new String[]{ touchedGdep.getParticipantId(), untouchedGdep.getParticipantId() });
        Thread.sleep(1);

        returnedEntry = entityManager.find(GlobalDiscoveryEntryPersisted.class, touchedKey);
        assertNotEquals(touchedGdep, returnedEntry);
        returnedEntry = entityManager.find(GlobalDiscoveryEntryPersisted.class, unTouchedKey);
        assertEquals(untouchedGdep, returnedEntry);
    }

    @Test
    public void removeStale() throws ApplicationException {
        String[] gbidsForAdd1 = new String[]{ validGbidsArray[2], validGbidsArray[0] };
        String[] gbidsForAdd2 = validGbidsArray;
        testGlobalDiscoveryEntry1.setLastSeenDateMs(42l);
        long maxLastSeenDate = System.currentTimeMillis() - 1001;
        testGlobalDiscoveryEntry1_a.setLastSeenDateMs(maxLastSeenDate + 1000);
        testGlobalDiscoveryEntry2.setLastSeenDateMs(maxLastSeenDate - 1);
        addEntry(testGlobalDiscoveryEntry1, gbidsForAdd1);
        addEntry(testGlobalDiscoveryEntry1_a, gbidsForAdd1);
        addEntry(testGlobalDiscoveryEntry2, gbidsForAdd2);

        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantId(testParticipantId1);
        assertEquals(gbidsForAdd1.length, result.size());
        result = queryEntityManagerByParticipantId(testParticipantId1_a);
        assertEquals(gbidsForAdd1.length, result.size());
        result = queryEntityManagerByParticipantId(testParticipantId2);
        assertEquals(gbidsForAdd2.length, result.size());

        subject.removeStale(TOPIC_NAME, maxLastSeenDate);
        entityManager.flush();
        entityManager.clear();

        checkEntryIsNotInEntityManager(testParticipantId1);
        result = queryEntityManagerByParticipantId(testParticipantId1_a);
        assertEquals(gbidsForAdd1.length, result.size());
        checkEntryIsNotInEntityManager(testParticipantId2);
    }

    @Test
    public void addAgainAfterRemoveStale() throws ApplicationException {
        String[] gbidsForAdd1 = new String[]{ validGbidsArray[1] };
        long maxLastSeenDate = System.currentTimeMillis() - 1001;
        testGlobalDiscoveryEntry1.setLastSeenDateMs(maxLastSeenDate - 1);
        addEntry(testGlobalDiscoveryEntry1, gbidsForAdd1);

        List<GlobalDiscoveryEntryPersisted> result = queryEntityManagerByParticipantId(testParticipantId1);
        assertEquals(gbidsForAdd1.length, result.size());

        subject.removeStale(TOPIC_NAME, maxLastSeenDate);

        checkEntryIsNotInEntityManager(testParticipantId1);
        GlobalDiscoveryEntryPersistedKey key = new GlobalDiscoveryEntryPersistedKey();
        key.setGbid(gbidsForAdd1[0]);
        key.setParticipantId(testParticipantId1);
        assertNull(entityManager.find(GlobalDiscoveryEntryPersisted.class, key));

        GlobalDiscoveryEntry entry2 = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry1);
        entry2.setExpiryDateMs(testGlobalDiscoveryEntry1.getExpiryDateMs() + 1);
        entry2.setLastSeenDateMs(testGlobalDiscoveryEntry1.getLastSeenDateMs() + 1);
        addEntry(entry2, gbidsForAdd1);
        result = queryEntityManagerByParticipantId(testParticipantId1);
        assertEquals(gbidsForAdd1.length, result.size());
    }

}
