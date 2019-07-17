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
package io.joynr.accesscontrol.global.jee;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import java.io.File;
import java.util.Arrays;
import java.util.HashSet;

import javax.inject.Inject;
import javax.persistence.EntityManager;

import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.arquillian.transaction.api.annotation.TransactionMode;
import org.jboss.arquillian.transaction.api.annotation.Transactional;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.WebArchive;
import org.jboss.shrinkwrap.resolver.api.maven.Maven;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.Timeout;
import org.junit.runner.RunWith;

import io.joynr.accesscontrol.global.jee.persistence.DomainRoleEntryEntity;
import io.joynr.accesscontrol.global.jee.persistence.OwnerRegistrationControlEntryEntity;
import io.joynr.jeeintegration.api.security.JoynrCallingPrincipal;
import io.joynr.jeeintegration.context.JoynrJeeMessageContext;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

@RunWith(Arquillian.class)
@Transactional(TransactionMode.ROLLBACK)
public class OwnerRegistrationControlEntryManagerTest {
    @Rule
    public Timeout globalTimeout = Timeout.seconds(60);

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
                                     DomainRoleEntryEntity.class,
                                     DomainRoleEntryManager.class,
                                     OwnerRegistrationControlEntryEntity.class,
                                     OwnerRegistrationControlEntryManager.class,
                                     JoynrConfigurationProvider.class)
                         .addAsLibraries(files)
                         .addAsResource("test-persistence.xml", "META-INF/persistence.xml")
                         .addAsWebInfResource(new File("src/main/webapp/WEB-INF/beans.xml"));
    }

    @Inject
    private EntityManager entityManager;

    @Inject
    private OwnerRegistrationControlEntryManager subject;

    @Inject
    private JoynrCallingPrincipal joynrCallingPrincipal;

    @Test
    public void testFindByUserId() {
        String userId = "userId";
        create(userId, "domain", "interfaceName", TrustLevel.MID, TrustLevel.HIGH, Permission.ASK);

        flushAndClear();

        OwnerRegistrationControlEntry[] result = subject.findByUserId(userId);

        assertNotNull(result);
        assertEquals(1, result.length);
    }

    @Test
    public void testFindByUserIdAndThatAreEditable() {
        String userId = "userId";
        String domain = "domain1";
        create(userId, domain, "interfaceName", TrustLevel.MID, TrustLevel.HIGH, Permission.ASK);
        create(userId, "domain2", "interfaceName", TrustLevel.MID, TrustLevel.HIGH, Permission.ASK);

        DomainRoleEntryEntity domainRoleEntryEntity = new DomainRoleEntryEntity();
        domainRoleEntryEntity.setUserId(userId);
        domainRoleEntryEntity.setRole(Role.OWNER);
        domainRoleEntryEntity.setDomains(new HashSet<String>(Arrays.asList(domain)));
        entityManager.persist(domainRoleEntryEntity);

        flushAndClear();

        OwnerRegistrationControlEntry[] result = subject.findByUserIdAndThatIsEditable(userId);

        assertNotNull(result);
        assertEquals(1, result.length);
    }

    @Test
    public void testCreate() {
        String userId = "user";
        String domain = "domain";
        String interfaceName = "interfaceName";
        OwnerRegistrationControlEntry data = new OwnerRegistrationControlEntry(userId,
                                                                               domain,
                                                                               interfaceName,
                                                                               TrustLevel.MID,
                                                                               TrustLevel.HIGH,
                                                                               Permission.ASK);

        CreateOrUpdateResult<OwnerRegistrationControlEntry> result = subject.createOrUpdate(data);

        assertNotNull(result);
        assertEquals(ChangeType.ADD, result.getChangeType());
        assertNotNull(result.getEntry());
        assertEquals(userId, result.getEntry().getUid());

        flushAndClear();

        OwnerRegistrationControlEntry[] persisted = subject.findByUserId(userId);
        assertNotNull(persisted);
        assertEquals(1, persisted.length);
    }

    @Test
    public void testCreateNotAllowedWithoutOwnerRole() {
        String userId = "user";
        String domain = "domain";
        String interfaceName = "interfaceName";
        OwnerRegistrationControlEntry data = new OwnerRegistrationControlEntry(userId,
                                                                               domain,
                                                                               interfaceName,
                                                                               TrustLevel.MID,
                                                                               TrustLevel.HIGH,
                                                                               Permission.ASK);

        DomainRoleEntryEntity domainRoleEntryEntity = new DomainRoleEntryEntity();
        domainRoleEntryEntity.setUserId(userId);
        domainRoleEntryEntity.setRole(Role.MASTER);
        domainRoleEntryEntity.setDomains(new HashSet<String>(Arrays.asList(domain)));
        entityManager.persist(domainRoleEntryEntity);

        flushAndClear();

        CreateOrUpdateResult<OwnerRegistrationControlEntry> result = null;

        JoynrJeeMessageContext.getInstance().activate();
        try {
            joynrCallingPrincipal.setUsername(userId);
            result = subject.createOrUpdate(data);
        } finally {
            JoynrJeeMessageContext.getInstance().deactivate();
        }

        assertNull(result);

        flushAndClear();

        OwnerRegistrationControlEntry[] persisted = subject.findByUserId(userId);
        assertNotNull(persisted);
        assertEquals(0, persisted.length);
    }

    @Test
    public void testUpdate() {
        String userId = "user";
        String domain = "domain";
        String interfaceName = "interfaceName";
        OwnerRegistrationControlEntryEntity entity = create(userId,
                                                            domain,
                                                            interfaceName,
                                                            TrustLevel.MID,
                                                            TrustLevel.HIGH,
                                                            Permission.ASK);

        flushAndClear();

        OwnerRegistrationControlEntry updatedData = new OwnerRegistrationControlEntry(userId,
                                                                                      domain,
                                                                                      interfaceName,
                                                                                      TrustLevel.HIGH,
                                                                                      TrustLevel.LOW,
                                                                                      Permission.NO);

        CreateOrUpdateResult<OwnerRegistrationControlEntry> result = subject.createOrUpdate(updatedData);

        assertNotNull(result);
        assertEquals(ChangeType.UPDATE, result.getChangeType());
        assertNotNull(result.getEntry());
        assertEquals(userId, result.getEntry().getUid());

        flushAndClear();

        OwnerRegistrationControlEntryEntity persisted = entityManager.find(OwnerRegistrationControlEntryEntity.class,
                                                                           entity.getId());
        assertNotNull(persisted);
        assertEquals(TrustLevel.HIGH, persisted.getRequiredTrustLevel());
        assertEquals(TrustLevel.LOW, persisted.getRequiredAceChangeTrustLevel());
        assertEquals(Permission.NO, persisted.getProviderPermission());
    }

    @Test
    public void testRemove() {
        String userId = "user";
        String domain = "domain";
        String interfaceName = "interfaceName";
        OwnerRegistrationControlEntryEntity entity = create(userId,
                                                            domain,
                                                            interfaceName,
                                                            TrustLevel.MID,
                                                            TrustLevel.HIGH,
                                                            Permission.ASK);

        flushAndClear();

        OwnerRegistrationControlEntry removedEntry = subject.removeByUserIdDomainAndInterfaceName(userId,
                                                                                                  domain,
                                                                                                  interfaceName);

        assertNotNull(removedEntry);
        assertEquals(userId, removedEntry.getUid());

        flushAndClear();

        OwnerRegistrationControlEntryEntity persisted = entityManager.find(OwnerRegistrationControlEntryEntity.class,
                                                                           entity.getId());
        assertNull(persisted);
    }

    @Test
    public void testRemoveNotAllowedWithoutOwnerRole() {
        String userId = "user";
        String domain = "domain";
        String interfaceName = "interfaceName";
        OwnerRegistrationControlEntryEntity entity = create(userId,
                                                            domain,
                                                            interfaceName,
                                                            TrustLevel.MID,
                                                            TrustLevel.HIGH,
                                                            Permission.ASK);

        flushAndClear();

        JoynrJeeMessageContext.getInstance().activate();
        try {
            joynrCallingPrincipal.setUsername(userId);
            assertNull(subject.removeByUserIdDomainAndInterfaceName(userId, domain, interfaceName));
        } finally {
            JoynrJeeMessageContext.getInstance().deactivate();
        }

        flushAndClear();

        OwnerRegistrationControlEntryEntity persisted = entityManager.find(OwnerRegistrationControlEntryEntity.class,
                                                                           entity.getId());
        assertNotNull(persisted);
    }

    private OwnerRegistrationControlEntryEntity create(String userId,
                                                       String domain,
                                                       String interfaceName,
                                                       TrustLevel requiredTrustLevel,
                                                       TrustLevel requiredAceChangeTrustLevel,
                                                       Permission providerPermission) {
        OwnerRegistrationControlEntryEntity entity = new OwnerRegistrationControlEntryEntity();
        entity.setUserId(userId);
        entity.setDomain(domain);
        entity.setInterfaceName(interfaceName);
        entity.setRequiredTrustLevel(requiredTrustLevel);
        entity.setRequiredAceChangeTrustLevel(requiredAceChangeTrustLevel);
        entity.setProviderPermission(providerPermission);
        entityManager.persist(entity);
        return entity;
    }

    private void flushAndClear() {
        entityManager.flush();
        entityManager.clear();
    }

}
