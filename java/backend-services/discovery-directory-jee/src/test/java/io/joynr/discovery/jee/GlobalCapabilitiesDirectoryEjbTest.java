package io.joynr.discovery.jee;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static org.junit.Assert.assertNotNull;

import java.io.File;
import java.lang.reflect.Field;

import javax.inject.Inject;
import javax.persistence.EntityManager;

import com.fasterxml.jackson.databind.ObjectMapper;
import io.joynr.capabilities.CapabilityUtils;
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
                                                                           "domain",
                                                                           "interfaceName",
                                                                           "participantId",
                                                                           new ProviderQos(),
                                                                           System.currentTimeMillis(),
                                                                           System.currentTimeMillis() + 1000L,
                                                                           "public key ID",
                                                                           new MqttAddress("tcp://mqttbroker:1883",
                                                                                           "my/topic"));
    }

    @Test
    public void testAddAndLookup() {
        subject.add(testGlobalDiscoveryEntry);
        entityManager.flush();
        entityManager.clear();
        GlobalDiscoveryEntry result = subject.lookup("participantId");
        assertNotNull(result);
    }

}
