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
    private GlobalDiscoveryEntry expectedGlobalDiscoveryEntry1;
    final private String testParticipantId1 = "participantId";
    final private String domain = "com";
    final private String[] domains = { domain };
    final private String interfaceName1 = "interfaceName1";
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
