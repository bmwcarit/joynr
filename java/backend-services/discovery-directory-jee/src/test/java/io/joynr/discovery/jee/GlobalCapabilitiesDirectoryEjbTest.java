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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.lang.reflect.Field;

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
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.Timeout;
import org.junit.runner.RunWith;

import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import joynr.exceptions.ApplicationException;
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

    private GlobalDiscoveryEntry testGlobalDiscoveryEntry;
    final private String testParticipantId = "participantId";
    final private String domain = "com";
    final private String[] domains = { domain };
    final private String interfaceName = "interfaceName";
    final private String[] validGbidsArray = { "joynrdefaultgbid", "gbid2", "gbid3" };

    @Inject
    private GlobalCapabilitiesDirectorySync subject;

    @Inject
    private EntityManager entityManager;

    @Before
    public void setup() throws NoSuchFieldException, IllegalAccessException {
        Field field = CapabilityUtils.class.getDeclaredField("objectMapper");
        field.setAccessible(true);
        field.set(CapabilityUtils.class, new ObjectMapper());
        testGlobalDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                           domain,
                                                                           interfaceName,
                                                                           testParticipantId,
                                                                           new ProviderQos(),
                                                                           System.currentTimeMillis(),
                                                                           System.currentTimeMillis() + 1000L,
                                                                           "public key ID",
                                                                           new MqttAddress("tcp://mqttbroker:1883",
                                                                                           TOPIC_NAME));
    }

    private void checkDiscoveryEntry(GlobalDiscoveryEntry expected, GlobalDiscoveryEntry actual, String expectedGbid) {
        assertEquals(expected.getParticipantId(), actual.getParticipantId());
        assertEquals(expected.getInterfaceName(), actual.getInterfaceName());
        assertEquals(expected.getDomain(), actual.getDomain());
        assertEquals(expected.getExpiryDateMs(), actual.getExpiryDateMs());
        assertEquals(expected.getLastSeenDateMs(), actual.getLastSeenDateMs());
        assertEquals(expected.getProviderVersion(), actual.getProviderVersion());
        assertEquals(expected.getQos(), actual.getQos());
        assertNotEquals(expected.getAddress(), actual.getAddress());
        assertEquals(expectedGbid,
                     ((MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(actual)).getBrokerUri());
        assertEquals(TOPIC_NAME, ((MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(actual)).getTopic());
    }

    @Ignore
    @Test
    public void testAddAndLookup_byParticipantId_singleDiscoveryEntry() {
        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry);
        GlobalDiscoveryEntry resultBeforeAdd = subject.lookup(testParticipantId);
        assertNull(resultBeforeAdd);

        subject.add(testGlobalDiscoveryEntry);
        entityManager.flush();
        entityManager.clear();
        GlobalDiscoveryEntry result = subject.lookup(testParticipantId);
        assertNotNull(result);
        assertTrue(result instanceof GlobalDiscoveryEntry);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        GlobalDiscoveryEntry persisted = (GlobalDiscoveryEntry) result;

        checkDiscoveryEntry(expectedEntry, persisted, validGbidsArray[0]);
    }

    @Ignore
    @Test
    public void testAddAndLookup_byDomainInterface_singleDiscoveryEntry() throws Exception {
        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry);
        GlobalDiscoveryEntry[] resultBeforeAdd = subject.lookup(new String[]{ testGlobalDiscoveryEntry.getDomain() },
                                                                testGlobalDiscoveryEntry.getInterfaceName());
        assertNotNull(resultBeforeAdd);
        assertEquals(0, resultBeforeAdd.length);

        subject.add(testGlobalDiscoveryEntry);
        entityManager.flush();
        entityManager.clear();
        GlobalDiscoveryEntry[] result = subject.lookup(new String[]{ testGlobalDiscoveryEntry.getDomain() },
                                                       testGlobalDiscoveryEntry.getInterfaceName());
        assertNotNull(result);
        assertEquals(1, result.length);
        assertTrue(result[0] instanceof GlobalDiscoveryEntry);
        assertFalse(result[0] instanceof GlobalDiscoveryEntryPersisted);

        checkDiscoveryEntry(expectedEntry, result[0], validGbidsArray[0]);
    }

    @Ignore
    @Test
    public void testAddAndRemove_singleDiscoveryEntry() {
        GlobalDiscoveryEntry expectedEntry = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry);
        GlobalDiscoveryEntry resultBeforeAdd = subject.lookup(testParticipantId);
        assertNull(resultBeforeAdd);

        subject.add(testGlobalDiscoveryEntry);
        entityManager.flush();
        entityManager.clear();

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId);
        assertNotNull(result);
        GlobalDiscoveryEntry persisted = (GlobalDiscoveryEntry) result;
        checkDiscoveryEntry(expectedEntry, persisted, validGbidsArray[0]);

        subject.remove(testParticipantId);
        entityManager.flush();
        entityManager.clear();

        GlobalDiscoveryEntry resultAfterRemove = subject.lookup(testParticipantId);
        assertNull(resultAfterRemove);
    }

    @Ignore
    @Test
    public void testAddWithGbids_validGbid() throws ApplicationException {
        GlobalDiscoveryEntry expectedDiscoveryEntry = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry);
        GlobalDiscoveryEntry resultBeforeAdd = subject.lookup(testParticipantId);
        assertNull(resultBeforeAdd);

        subject.add(testGlobalDiscoveryEntry, validGbidsArray);
        entityManager.flush();
        entityManager.clear();

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId);
        assertNotNull(result);
        checkDiscoveryEntry(expectedDiscoveryEntry, result, validGbidsArray[0]);
    }

    @Test
    @Ignore
    public void testAddWithGbids_unknownGbid() {
        final String[] invalidGbidsArray = { "unknownGbid" };
        try {
            subject.add(testGlobalDiscoveryEntry, invalidGbidsArray);
        } catch (ApplicationException e) {
            assertEquals(DiscoveryError.UNKNOWN_GBID, e.getError());
        }
        entityManager.flush();
        entityManager.clear();

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId);
        assertNull(result);
    }

    private void testAddWithGbids_invalidGbid(String[] invalidGbidsArray) {
        try {
            subject.add(testGlobalDiscoveryEntry, invalidGbidsArray);
        } catch (ApplicationException e) {
            assertEquals(DiscoveryError.INVALID_GBID, e.getError());
        }
        entityManager.flush();
        entityManager.clear();

        GlobalDiscoveryEntry result = subject.lookup(testParticipantId);
        assertNull(result);
    }

    @Ignore
    @Test
    public void testAddWithGbids_invalidGbid_emptyGbid() {
        final String[] invalidGbidsArray = { "" };
        testAddWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Ignore
    @Test
    public void testAddWithGbids_invalidGbid_nullGbid() {
        final String[] invalidGbidsArray = { null };
        testAddWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Ignore
    @Test
    public void testAddWithGbids_invalidGbid_nullGbidsArray() {
        final String[] invalidGbidsArray = null;
        testAddWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Ignore
    @Test
    public void testAddWithGbids_invalidGbid_emptyGbidsArray() {
        final String[] invalidGbidsArray = {};
        testAddWithGbids_invalidGbid(invalidGbidsArray);
    }

    @Ignore
    @Test
    public void testLookupWithGbids_validGbid() throws ApplicationException {
        GlobalDiscoveryEntry expectedDiscoveryEntry = new GlobalDiscoveryEntry(testGlobalDiscoveryEntry);
        GlobalDiscoveryEntry resultBeforeAdd = subject.lookup(testParticipantId);
        assertNull(resultBeforeAdd);

        subject.add(testGlobalDiscoveryEntry, validGbidsArray);
        entityManager.flush();
        entityManager.clear();

        GlobalDiscoveryEntry[] globalDiscoveryEntries = subject.lookup(domains, interfaceName, validGbidsArray);
        assertEquals(1, globalDiscoveryEntries.length);
        checkDiscoveryEntry(expectedDiscoveryEntry, globalDiscoveryEntries[0], validGbidsArray[0]);
    }

    @Test
    public void testLookupWithGbids_unknownGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { "unknownGbid" };
        try {
            subject.lookup(domains, interfaceName, invalidGbidsArray);
        } catch (ApplicationException e) {
            assertEquals(DiscoveryError.UNKNOWN_GBID, e.getError());
        }
    }

    private void testLookupWithGbids_byDomainInterface_invalidGbid(String[] invalidGbidsArray) {
        try {
            subject.lookup(domains, interfaceName, invalidGbidsArray);
        } catch (ApplicationException e) {
            assertEquals(DiscoveryError.INVALID_GBID, e.getError());
        }
    }

    @Test
    public void testLookupWithGbids_byDomainInterface_invalidGbid_emptyGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { "" };
        testLookupWithGbids_byDomainInterface_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testLookupWithGbids_byDomainInterface_invalidGbid_nullGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { null };
        testLookupWithGbids_byDomainInterface_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testLookupWithGbids_byDomainInterface_invalidGbid_nullGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = null;
        testLookupWithGbids_byDomainInterface_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testLookupWithGbids_byDomainInterface_invalidGbid_emptyGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = {};
        testLookupWithGbids_byDomainInterface_invalidGbid(invalidGbidsArray);
    }

    private void testLookupWithGbids_byParticipantId_invalidGbid(String[] invalidGbidsArray) {
        try {
            subject.lookup(testParticipantId, invalidGbidsArray);
        } catch (ApplicationException e) {
            assertEquals(DiscoveryError.INVALID_GBID, e.getError());
        }
    }

    @Test
    public void testLookupWithGbids_byParticipantId_invalidGbid_emptyGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { "" };
        testLookupWithGbids_byParticipantId_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testLookupWithGbids_byParticipantId_invalidGbid_nullGbid() throws ApplicationException {
        final String[] invalidGbidsArray = { null };
        testLookupWithGbids_byParticipantId_invalidGbid(invalidGbidsArray);
    }

    @Ignore
    @Test
    public void testLookupWithGbids_byParticipantId_invalidGbid_nullGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = null;
        testLookupWithGbids_byParticipantId_invalidGbid(invalidGbidsArray);
    }

    @Test
    public void testLookupWithGbids_byParticipantId_invalidGbid_emptyGbidsArray() throws ApplicationException {
        final String[] invalidGbidsArray = {};
        testLookupWithGbids_byParticipantId_invalidGbid(invalidGbidsArray);
    }

    @Ignore
    @Test
    public void testRemoveWithGbids_validGbid() throws ApplicationException {
        GlobalDiscoveryEntry resultBeforeAdd = subject.lookup(testParticipantId);
        assertNull(resultBeforeAdd);

        subject.add(testGlobalDiscoveryEntry, validGbidsArray);
        GlobalDiscoveryEntry[] globalDiscoveryEntries = subject.lookup(domains, interfaceName, validGbidsArray);
        assertEquals(1, globalDiscoveryEntries.length);

        subject.remove(testParticipantId, validGbidsArray);

        globalDiscoveryEntries = subject.lookup(domains, interfaceName, validGbidsArray);
        assertEquals(0, globalDiscoveryEntries.length);
    }

    @Ignore
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
            subject.remove(testParticipantId, invalidGbidsArray);
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

    @Ignore
    @Test
    public void testTouch() throws InterruptedException {
        long initialLastSeen = testGlobalDiscoveryEntry.getLastSeenDateMs();
        subject.add(testGlobalDiscoveryEntry);

        entityManager.flush();
        entityManager.clear();

        GlobalDiscoveryEntryPersisted persisted = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                                                     testGlobalDiscoveryEntry.getParticipantId());
        assertNotNull(persisted);
        assertEquals((Long) initialLastSeen, persisted.getLastSeenDateMs());

        Thread.sleep(1L);

        subject.touch(TOPIC_NAME);

        entityManager.flush();
        entityManager.clear();

        persisted = entityManager.find(GlobalDiscoveryEntryPersisted.class,
                                       testGlobalDiscoveryEntry.getParticipantId());
        assertNotNull(persisted);
        assertTrue(initialLastSeen < persisted.getLastSeenDateMs());
    }

    @Test
    public void testAddTwice() throws Exception {
        subject.add(testGlobalDiscoveryEntry);
        subject.add(testGlobalDiscoveryEntry);

        GlobalDiscoveryEntry[] persisted = subject.lookup(new String[]{ testGlobalDiscoveryEntry.getDomain() },
                                                          testGlobalDiscoveryEntry.getInterfaceName());
        assertNotNull(persisted);
        assertEquals(1, persisted.length);
    }

}
