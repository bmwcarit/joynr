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

import io.joynr.accesscontrol.global.jee.persistence.ControlEntryType;
import io.joynr.accesscontrol.global.jee.persistence.DomainRoleEntryEntity;
import io.joynr.accesscontrol.global.jee.persistence.MasterRegistrationControlEntryEntity;
import io.joynr.jeeintegration.api.security.JoynrCallingPrincipal;
import io.joynr.jeeintegration.context.JoynrJeeMessageContext;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

@RunWith(Arquillian.class)
@Transactional(TransactionMode.ROLLBACK)
public class MasterRegistrationControlEntryManagerTest {
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
                                     MasterRegistrationControlEntryEntity.class,
                                     MasterRegistrationControlEntryManager.class,
                                     JoynrConfigurationProvider.class)
                         .addAsLibraries(files)
                         .addAsResource("test-persistence.xml", "META-INF/persistence.xml")
                         .addAsWebInfResource(new File("src/main/webapp/WEB-INF/beans.xml"))
                         .addAsWebInfResource(new File("src/test/resources/glassfish-web.xml"));
    }

    @Inject
    private EntityManager entityManager;

    @Inject
    private MasterRegistrationControlEntryManager subject;

    @Inject
    private JoynrCallingPrincipal joynrCallingPrincipal;

    @Test
    public void testFindByUserId() {
        String userId = "user";
        create(userId,
               "domain",
               "interfaceName1",
               TrustLevel.HIGH,
               new TrustLevel[0],
               TrustLevel.LOW,
               new TrustLevel[0],
               Permission.YES,
               new Permission[0],
               ControlEntryType.MASTER);
        create(userId,
               "domain",
               "interfaceName2",
               TrustLevel.HIGH,
               new TrustLevel[0],
               TrustLevel.LOW,
               new TrustLevel[0],
               Permission.YES,
               new Permission[0],
               ControlEntryType.MEDIATOR);

        flushAndClear();

        MasterRegistrationControlEntry[] result = subject.findByUserIdAndType(userId, ControlEntryType.MASTER);

        assertNotNull(result);
        assertEquals(1, result.length);
        assertEquals("interfaceName1", result[0].getInterfaceName());
    }

    @Test
    public void testFindByUserIdAndThatAreEditable() {
        String userId = "user";
        String domain = "domain1";
        create(userId,
               domain,
               "interfaceName1",
               TrustLevel.HIGH,
               new TrustLevel[0],
               TrustLevel.LOW,
               new TrustLevel[0],
               Permission.YES,
               new Permission[0],
               ControlEntryType.MASTER);
        create(userId,
               domain,
               "interfaceName1",
               TrustLevel.HIGH,
               new TrustLevel[0],
               TrustLevel.LOW,
               new TrustLevel[0],
               Permission.YES,
               new Permission[0],
               ControlEntryType.MEDIATOR);
        create(userId,
               "domain2",
               "interfaceName2",
               TrustLevel.HIGH,
               new TrustLevel[0],
               TrustLevel.LOW,
               new TrustLevel[0],
               Permission.YES,
               new Permission[0],
               ControlEntryType.MASTER);

        DomainRoleEntryEntity domainRoleEntryEntity = new DomainRoleEntryEntity();
        domainRoleEntryEntity.setUserId(userId);
        domainRoleEntryEntity.setRole(Role.MASTER);
        domainRoleEntryEntity.setDomains(new HashSet<String>(Arrays.asList(domain)));
        entityManager.persist(domainRoleEntryEntity);

        flushAndClear();

        MasterRegistrationControlEntry[] result = subject.findByUserIdAndThatAreEditable(userId,
                                                                                         ControlEntryType.MASTER);

        assertNotNull(result);
        assertEquals(1, result.length);
        assertEquals(domain, result[0].getDomain());
    }

    @Test
    public void testCreate() {
        String userId = "userId";
        String domain = "domain";
        String interfaceName = "interfaceName";
        MasterRegistrationControlEntry data = new MasterRegistrationControlEntry(userId,
                                                                                 domain,
                                                                                 interfaceName,
                                                                                 TrustLevel.LOW,
                                                                                 new TrustLevel[0],
                                                                                 TrustLevel.LOW,
                                                                                 new TrustLevel[0],
                                                                                 Permission.ASK,
                                                                                 new Permission[0]);

        CreateOrUpdateResult<MasterRegistrationControlEntry> result = subject.createOrUpdate(data,
                                                                                             ControlEntryType.MASTER);

        assertNotNull(result);
        assertEquals(ChangeType.ADD, result.getChangeType());
        assertNotNull(result.getEntry());
        assertEquals(userId, result.getEntry().getUid());

        flushAndClear();

        MasterRegistrationControlEntry[] persisted = subject.findByUserIdAndType(userId, ControlEntryType.MASTER);
        assertNotNull(persisted);
        assertEquals(1, persisted.length);
        assertEquals(domain, persisted[0].getDomain());
        assertEquals(interfaceName, persisted[0].getInterfaceName());
        assertEquals(TrustLevel.LOW, persisted[0].getDefaultRequiredTrustLevel());
    }

    @Test
    public void testCreateNotAllowedWithoutMasterRole() {
        String userId = "userId";
        String domain = "domain";
        String interfaceName = "interfaceName";
        MasterRegistrationControlEntry data = new MasterRegistrationControlEntry(userId,
                                                                                 domain,
                                                                                 interfaceName,
                                                                                 TrustLevel.LOW,
                                                                                 new TrustLevel[0],
                                                                                 TrustLevel.LOW,
                                                                                 new TrustLevel[0],
                                                                                 Permission.ASK,
                                                                                 new Permission[0]);

        DomainRoleEntryEntity domainRoleEntryEntity = new DomainRoleEntryEntity();
        domainRoleEntryEntity.setUserId(userId);
        domainRoleEntryEntity.setRole(Role.OWNER);
        domainRoleEntryEntity.setDomains(new HashSet<String>(Arrays.asList(domain)));
        entityManager.persist(domainRoleEntryEntity);

        flushAndClear();

        JoynrJeeMessageContext.getInstance().activate();
        CreateOrUpdateResult<MasterRegistrationControlEntry> result = null;
        try {
            joynrCallingPrincipal.setUsername(userId);
            result = subject.createOrUpdate(data, ControlEntryType.MASTER);
        } finally {
            JoynrJeeMessageContext.getInstance().deactivate();
        }

        assertNull(result);

        flushAndClear();

        MasterRegistrationControlEntry[] persisted = subject.findByUserIdAndType(userId, ControlEntryType.MASTER);
        assertNotNull(persisted);
        assertEquals(0, persisted.length);
    }

    @Test
    public void testUpdate() {
        String userId = "user";
        String domain = "domain1";
        String interfaceName = "interfaceName1";
        MasterRegistrationControlEntryEntity entity = create(userId,
                                                             domain,
                                                             interfaceName,
                                                             TrustLevel.HIGH,
                                                             new TrustLevel[0],
                                                             TrustLevel.LOW,
                                                             new TrustLevel[0],
                                                             Permission.YES,
                                                             new Permission[0],
                                                             ControlEntryType.MASTER);

        flushAndClear();

        MasterRegistrationControlEntry updatedData = new MasterRegistrationControlEntry(userId,
                                                                                        domain,
                                                                                        interfaceName,
                                                                                        TrustLevel.LOW,
                                                                                        new TrustLevel[]{
                                                                                                TrustLevel.MID,
                                                                                                TrustLevel.HIGH },
                                                                                        TrustLevel.HIGH,
                                                                                        new TrustLevel[0],
                                                                                        Permission.ASK,
                                                                                        new Permission[]{
                                                                                                Permission.NO });

        CreateOrUpdateResult<MasterRegistrationControlEntry> result = subject.createOrUpdate(updatedData,
                                                                                             ControlEntryType.MASTER);

        assertNotNull(result);
        assertEquals(ChangeType.UPDATE, result.getChangeType());
        assertNotNull(result.getEntry());
        assertEquals(userId, result.getEntry().getUid());

        flushAndClear();

        MasterRegistrationControlEntryEntity persisted = entityManager.find(MasterRegistrationControlEntryEntity.class,
                                                                            entity.getId());
        assertNotNull(persisted);
        assertEquals(TrustLevel.LOW, persisted.getDefaultRequiredTrustLevel());
        assertEquals(2, persisted.getPossibleRequiredTrustLevels().size());
        assertEquals(TrustLevel.HIGH, persisted.getDefaultRequiredControlEntryChangeTrustLevel());
        assertEquals(Permission.ASK, persisted.getDefaultProviderPermission());
    }

    @Test
    public void testRemove() {
        String userId = "userId";
        String domain = "domain";
        String interfaceName = "interfaceName";
        MasterRegistrationControlEntryEntity entity = create(userId,
                                                             domain,
                                                             interfaceName,
                                                             TrustLevel.HIGH,
                                                             new TrustLevel[0],
                                                             TrustLevel.LOW,
                                                             new TrustLevel[0],
                                                             Permission.ASK,
                                                             new Permission[0],
                                                             ControlEntryType.MASTER);

        flushAndClear();

        assertNotNull(subject.removeByUserIdDomainInterfaceNameAndType(userId,
                                                                       domain,
                                                                       interfaceName,
                                                                       ControlEntryType.MASTER));

        flushAndClear();

        MasterRegistrationControlEntryEntity persisted = entityManager.find(MasterRegistrationControlEntryEntity.class,
                                                                            entity.getId());
        assertNull(persisted);
    }

    @Test
    public void testRemoveNotAllowedWithoutMasterRole() {
        String userId = "userId";
        String domain = "domain";
        String interfaceName = "interfaceName";
        MasterRegistrationControlEntryEntity entity = create(userId,
                                                             domain,
                                                             interfaceName,
                                                             TrustLevel.HIGH,
                                                             new TrustLevel[0],
                                                             TrustLevel.LOW,
                                                             new TrustLevel[0],
                                                             Permission.ASK,
                                                             new Permission[0],
                                                             ControlEntryType.MASTER);

        flushAndClear();

        JoynrJeeMessageContext.getInstance().activate();
        try {
            joynrCallingPrincipal.setUsername(userId);
            assertNull(subject.removeByUserIdDomainInterfaceNameAndType(userId,
                                                                        domain,
                                                                        interfaceName,
                                                                        ControlEntryType.MASTER));
        } finally {
            JoynrJeeMessageContext.getInstance().deactivate();
        }

        flushAndClear();

        MasterRegistrationControlEntryEntity persisted = entityManager.find(MasterRegistrationControlEntryEntity.class,
                                                                            entity.getId());
        assertNotNull(persisted);
    }

    private MasterRegistrationControlEntryEntity create(String userId,
                                                        String domain,
                                                        String interfaceName,
                                                        TrustLevel trustLevel,
                                                        TrustLevel[] possibleTrustLevels,
                                                        TrustLevel changeTrustLevel,
                                                        TrustLevel[] possibleChangeTrustLevels,
                                                        Permission permission,
                                                        Permission[] possiblePermissions,
                                                        ControlEntryType type) {
        MasterRegistrationControlEntryEntity entity = new MasterRegistrationControlEntryEntity();
        entity.setUserId(userId);
        entity.setDomain(domain);
        entity.setInterfaceName(interfaceName);
        entity.setDefaultRequiredTrustLevel(trustLevel);
        entity.setPossibleRequiredTrustLevels(new HashSet<TrustLevel>(Arrays.asList(possibleTrustLevels)));
        entity.setDefaultRequiredControlEntryChangeTrustLevel(changeTrustLevel);
        entity.setPossibleRequiredControlEntryChangeTrustLevels(new HashSet<TrustLevel>(Arrays.asList(possibleChangeTrustLevels)));
        entity.setDefaultProviderPermission(permission);
        entity.setPossibleProviderPermissions(new HashSet<Permission>(Arrays.asList(possiblePermissions)));
        entity.setType(type);
        entityManager.persist(entity);
        return entity;
    }

    private void flushAndClear() {
        entityManager.flush();
        entityManager.clear();
    }
}
