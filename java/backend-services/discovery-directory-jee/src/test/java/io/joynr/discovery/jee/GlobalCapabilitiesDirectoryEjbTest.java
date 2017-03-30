package io.joynr.discovery.jee;

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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.lang.reflect.Field;

import javax.inject.Inject;
import javax.persistence.EntityManager;

import com.fasterxml.jackson.databind.ObjectMapper;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalDiscoveryEntryPersisted;
import joynr.infrastructure.GlobalCapabilitiesDirectorySync;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;
import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.arquillian.transaction.api.annotation.TransactionMode;
import org.jboss.arquillian.transaction.api.annotation.Transactional;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.WebArchive;
import org.jboss.shrinkwrap.resolver.api.maven.Maven;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(Arquillian.class)
@Transactional(TransactionMode.ROLLBACK)
public class GlobalCapabilitiesDirectoryEjbTest {

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
                                     JoynrConfigurationProvider.class)
                         .addAsLibraries(files)
                         .addAsResource("META-INF/persistence.xml")
                         .addAsWebInfResource(new File("src/main/webapp/WEB-INF/beans.xml"));
    }

    private GlobalDiscoveryEntry testGlobalDiscoveryEntry;
    private String testParticpantId;

    @Inject
    private GlobalCapabilitiesDirectorySync subject;

    @Inject
    private EntityManager entityManager;

    @Before
    public void setup() throws NoSuchFieldException, IllegalAccessException {
        Field field = CapabilityUtils.class.getDeclaredField("objectMapper");
        field.setAccessible(true);
        field.set(CapabilityUtils.class, new ObjectMapper());
        testParticpantId = "participantId";
        testGlobalDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0, 1),
                                                                           "domain",
                                                                           "interfaceName",
                                                                           testParticpantId,
                                                                           new ProviderQos(),
                                                                           System.currentTimeMillis(),
                                                                           System.currentTimeMillis() + 1000L,
                                                                           "public key ID",
                                                                           new MqttAddress("tcp://mqttbroker:1883",
                                                                                           TOPIC_NAME));
    }

    @Test
    public void testAddAndLookupSingleDiscoveryEntryByParticipantId() {
        GlobalDiscoveryEntry resultBeforeAdd = subject.lookup(testParticpantId);
        assertNull(resultBeforeAdd);

        subject.add(testGlobalDiscoveryEntry);
        entityManager.flush();
        entityManager.clear();
        GlobalDiscoveryEntry result = subject.lookup(testParticpantId);
        assertNotNull(result);
        assertTrue(result instanceof GlobalDiscoveryEntry);
        assertFalse(result instanceof GlobalDiscoveryEntryPersisted);
        GlobalDiscoveryEntry persisted = (GlobalDiscoveryEntry) result;
        assertEquals(testGlobalDiscoveryEntry.getInterfaceName(), persisted.getInterfaceName());
        assertEquals(testGlobalDiscoveryEntry.getDomain(), persisted.getDomain());
    }

    @Test
    public void testAddAndRemoveSingleDiscoveryEntry() {
        GlobalDiscoveryEntry resultBeforeRemove = subject.lookup(testParticpantId);
        assertNull(resultBeforeRemove);

        subject.add(testGlobalDiscoveryEntry);
        entityManager.flush();
        entityManager.clear();
        GlobalDiscoveryEntry result = subject.lookup(testParticpantId);
        assertNotNull(result);
        GlobalDiscoveryEntry persisted = (GlobalDiscoveryEntry) result;
        assertEquals(testGlobalDiscoveryEntry, persisted);

        subject.remove(testParticpantId);
        entityManager.flush();
        entityManager.clear();

        GlobalDiscoveryEntry resultAfterRemove = subject.lookup(testParticpantId);
        assertNull(resultAfterRemove);
    }

    @Test
    public void testAddAndLookupMultiDomain() throws Exception {
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
        assertEquals(testGlobalDiscoveryEntry.getDomain(), ((GlobalDiscoveryEntry) result[0]).getDomain());
        assertEquals(testGlobalDiscoveryEntry.getInterfaceName(), ((GlobalDiscoveryEntry) result[0]).getInterfaceName());
    }

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

        persisted = entityManager.find(GlobalDiscoveryEntryPersisted.class, testGlobalDiscoveryEntry.getParticipantId());
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
