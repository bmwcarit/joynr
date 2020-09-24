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

import static io.joynr.accesscontrol.global.jee.persistence.ControlEntryType.MASTER;
import static io.joynr.accesscontrol.global.jee.persistence.ControlEntryType.MEDIATOR;
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
import io.joynr.accesscontrol.global.jee.persistence.MasterAccessControlEntryEntity;
import io.joynr.jeeintegration.api.security.JoynrCallingPrincipal;
import io.joynr.jeeintegration.context.JoynrJeeMessageContext;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

@RunWith(Arquillian.class)
@Transactional(TransactionMode.ROLLBACK)
public class MasterAccessControlEntryManagerTest {

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
                                     MasterAccessControlEntryEntity.class,
                                     MasterAccessControlEntryManager.class,
                                     TestJoynrConfigurationProvider.class)
                         .addAsLibraries(files)
                         .addAsResource("test-persistence.xml", "META-INF/persistence.xml")
                         .addAsWebInfResource(new File("src/main/webapp/WEB-INF/beans.xml"));
    }

    @Inject
    private MasterAccessControlEntryManager subject;

    @Inject
    private EntityManager entityManager;

    @Inject
    private JoynrCallingPrincipal joynrCallingPrincipal;

    @Test
    public void testFindByUserId() {
        String userId = "userA";
        create(userId,
               "domain1",
               "interface1",
               TrustLevel.HIGH,
               new TrustLevel[]{ TrustLevel.LOW },
               TrustLevel.HIGH,
               new TrustLevel[]{ TrustLevel.MID, TrustLevel.LOW },
               "operation1",
               Permission.YES,
               new Permission[]{ Permission.ASK });
        create(userId,
               "domain2",
               "interface2",
               TrustLevel.HIGH,
               new TrustLevel[]{ TrustLevel.MID },
               TrustLevel.LOW,
               new TrustLevel[]{ TrustLevel.HIGH, TrustLevel.MID },
               "operation2",
               Permission.YES,
               new Permission[]{ Permission.NO });
        create("otherUser",
               "domain3",
               "interface3",
               TrustLevel.HIGH,
               new TrustLevel[]{},
               TrustLevel.LOW,
               new TrustLevel[]{},
               "operation3",
               Permission.YES,
               new Permission[]{});

        flushAndClear();

        MasterAccessControlEntry[] result = subject.findByUserId(userId, MASTER);

        assertNotNull(result);
        assertEquals(2, result.length);
    }

    @Test
    public void testFindByUserIdAndIsEditable() {
        String userId = "user";
        create(userId,
               "domain1",
               "interface1",
               TrustLevel.HIGH,
               new TrustLevel[]{},
               TrustLevel.LOW,
               new TrustLevel[]{},
               "operation1",
               Permission.YES,
               new Permission[]{});
        create(userId,
               "domain2",
               "interface2",
               TrustLevel.HIGH,
               new TrustLevel[]{},
               TrustLevel.LOW,
               new TrustLevel[]{},
               "operation2",
               Permission.YES,
               new Permission[]{});

        DomainRoleEntryEntity role1 = new DomainRoleEntryEntity();
        role1.setUserId(userId);
        role1.setRole(Role.MASTER);
        role1.setDomains(new HashSet<String>(Arrays.asList("domain1", "domain4", "domain5")));
        entityManager.persist(role1);

        flushAndClear();

        MasterAccessControlEntry[] result = subject.findByUserIdThatAreEditable(userId, MASTER);

        assertNotNull(result);
        assertEquals(1, result.length);
        assertEquals("domain1", result[0].getDomain());
        assertEquals("interface1", result[0].getInterfaceName());
    }

    @Test
    public void testFindByDomainAndInterfaceName() {
        String domain = "domain";
        String interfaceName = "interfaceName";
        String userId = "user1";
        create(userId,
               domain,
               interfaceName,
               TrustLevel.HIGH,
               new TrustLevel[0],
               TrustLevel.LOW,
               new TrustLevel[0],
               "operation",
               Permission.YES,
               new Permission[0]);
        create("user2",
               "other domain",
               "other interface name",
               TrustLevel.HIGH,
               new TrustLevel[0],
               TrustLevel.LOW,
               new TrustLevel[0],
               "other operation",
               Permission.YES,
               new Permission[0]);

        flushAndClear();

        MasterAccessControlEntry[] result = subject.findByDomainAndInterfaceName(domain, interfaceName, MASTER);

        assertNotNull(result);
        assertEquals(1, result.length);
        assertEquals(userId, result[0].getUid());
    }

    @Test
    public void testCreate() {
        MasterAccessControlEntry newEntry = new MasterAccessControlEntry("user",
                                                                         "domain",
                                                                         "interfaceName",
                                                                         TrustLevel.HIGH,
                                                                         new TrustLevel[0],
                                                                         TrustLevel.LOW,
                                                                         new TrustLevel[0],
                                                                         "operation",
                                                                         Permission.ASK,
                                                                         new Permission[0]);

        CreateOrUpdateResult<MasterAccessControlEntry> result = subject.createOrUpdate(newEntry, MASTER);

        assertNotNull(result);
        assertEquals(ChangeType.ADD, result.getChangeType());
        assertNotNull(result.getEntry());
        assertEquals("user", result.getEntry().getUid());

        flushAndClear();

        MasterAccessControlEntry[] persisted = subject.findByUserId("user", MASTER);
        assertNotNull(persisted);
        assertEquals(1, persisted.length);
        MasterAccessControlEntry entry = persisted[0];
        assertEquals("user", entry.getUid());
        assertEquals("domain", entry.getDomain());
        assertEquals("interfaceName", entry.getInterfaceName());
        assertEquals("operation", entry.getOperation());
        assertEquals(TrustLevel.HIGH, entry.getDefaultRequiredTrustLevel());
        assertEquals(TrustLevel.LOW, entry.getDefaultRequiredControlEntryChangeTrustLevel());
        assertEquals(Permission.ASK, entry.getDefaultConsumerPermission());
        assertNotNull(entry.getPossibleRequiredTrustLevels());
        assertEquals(0, entry.getPossibleRequiredTrustLevels().length);
        assertNotNull(entry.getPossibleRequiredControlEntryChangeTrustLevels());
        assertEquals(0, entry.getPossibleRequiredControlEntryChangeTrustLevels().length);
        assertNotNull(entry.getPossibleConsumerPermissions());
        assertEquals(0, entry.getPossibleConsumerPermissions().length);
    }

    @Test
    public void testUpdate() {
        create("user",
               "domain",
               "interfaceName",
               TrustLevel.HIGH,
               new TrustLevel[0],
               TrustLevel.LOW,
               new TrustLevel[0],
               "operation",
               Permission.YES,
               new Permission[0]);
        MasterAccessControlEntry entry = subject.findByUserId("user", MASTER)[0];

        flushAndClear();

        entry.setDefaultRequiredTrustLevel(TrustLevel.LOW);
        entry.setPossibleRequiredTrustLevels(new TrustLevel[]{ TrustLevel.HIGH, TrustLevel.MID });
        entry.setDefaultConsumerPermission(Permission.ASK);

        CreateOrUpdateResult<MasterAccessControlEntry> result = subject.createOrUpdate(entry, MASTER);

        assertNotNull(result);
        assertEquals(ChangeType.UPDATE, result.getChangeType());
        assertNotNull(result.getEntry());
        assertEquals("user", result.getEntry().getUid());

        flushAndClear();

        entry = subject.findByUserId("user", MASTER)[0];

        assertEquals(TrustLevel.LOW, entry.getDefaultRequiredTrustLevel());
        assertEquals(2, entry.getPossibleRequiredTrustLevels().length);
        assertEquals(Permission.ASK, entry.getDefaultConsumerPermission());
    }

    @Test
    public void testUserNotAllowedToCreateOrUpdate() {
        String userId = "user";
        String domain = "domain";
        DomainRoleEntryEntity domainRoleEntryEntity = new DomainRoleEntryEntity();
        domainRoleEntryEntity.setUserId(userId);
        domainRoleEntryEntity.setRole(Role.OWNER);
        domainRoleEntryEntity.setDomains(new HashSet<String>(Arrays.asList(domain)));
        entityManager.persist(domainRoleEntryEntity);

        create(userId,
               domain,
               "interfaceName",
               TrustLevel.HIGH,
               new TrustLevel[0],
               TrustLevel.LOW,
               new TrustLevel[0],
               "operation",
               Permission.YES,
               new Permission[0]);

        flushAndClear();

        MasterAccessControlEntry updatedData = new MasterAccessControlEntry(userId,
                                                                            domain,
                                                                            "interfaceName",
                                                                            TrustLevel.LOW,
                                                                            new TrustLevel[]{ TrustLevel.HIGH },
                                                                            TrustLevel.HIGH,
                                                                            new TrustLevel[0],
                                                                            "operation",
                                                                            Permission.ASK,
                                                                            new Permission[]{ Permission.NO });

        CreateOrUpdateResult<MasterAccessControlEntry> result = null;
        JoynrJeeMessageContext.getInstance().activate();
        try {
            joynrCallingPrincipal.setUsername(userId);
            result = subject.createOrUpdate(updatedData, ControlEntryType.MASTER);
        } finally {
            JoynrJeeMessageContext.getInstance().deactivate();
        }

        assertNull(result);
    }

    @Test
    public void testRemove() {
        MasterAccessControlEntryEntity entity = create("user",
                                                       "domain",
                                                       "interfaceName",
                                                       TrustLevel.HIGH,
                                                       new TrustLevel[0],
                                                       TrustLevel.LOW,
                                                       new TrustLevel[0],
                                                       "operation",
                                                       Permission.YES,
                                                       new Permission[0]);

        flushAndClear();

        MasterAccessControlEntry result = subject.removeByUserIdDomainInterfaceNameAndOperation(entity.getUserId(),
                                                                                                entity.getDomain(),
                                                                                                entity.getInterfaceName(),
                                                                                                entity.getOperation(),
                                                                                                MASTER);

        assertNotNull(result);
        assertEquals("user", result.getUid());

        flushAndClear();

        MasterAccessControlEntryEntity persisted = entityManager.find(MasterAccessControlEntryEntity.class,
                                                                      entity.getId());

        assertNull(persisted);
    }

    @Test
    public void testRemoveNotAllowed() {
        String userId = "user";
        MasterAccessControlEntryEntity entity = create(userId,
                                                       "domain",
                                                       "interfaceName",
                                                       TrustLevel.HIGH,
                                                       new TrustLevel[0],
                                                       TrustLevel.LOW,
                                                       new TrustLevel[0],
                                                       "operation",
                                                       Permission.YES,
                                                       new Permission[0]);

        flushAndClear();

        MasterAccessControlEntry result = null;
        JoynrJeeMessageContext.getInstance().activate();
        try {
            joynrCallingPrincipal.setUsername(userId);
            subject.removeByUserIdDomainInterfaceNameAndOperation(entity.getUserId(),
                                                                  entity.getDomain(),
                                                                  entity.getInterfaceName(),
                                                                  entity.getOperation(),
                                                                  MASTER);
        } finally {
            JoynrJeeMessageContext.getInstance().deactivate();
        }

        assertNull(result);

        flushAndClear();

        MasterAccessControlEntryEntity persisted = entityManager.find(MasterAccessControlEntryEntity.class,
                                                                      entity.getId());

        assertNotNull(persisted);
    }

    @Test
    public void testCorrectTypeFound() {
        create("user",
               "domain",
               "interfaceName",
               TrustLevel.HIGH,
               new TrustLevel[0],
               TrustLevel.LOW,
               new TrustLevel[0],
               "operation",
               Permission.YES,
               new Permission[0],
               MASTER);
        create("user",
               "domain",
               "interfaceName",
               TrustLevel.HIGH,
               new TrustLevel[0],
               TrustLevel.LOW,
               new TrustLevel[0],
               "operation",
               Permission.YES,
               new Permission[0],
               MEDIATOR);

        flushAndClear();

        for (ControlEntryType type : new ControlEntryType[]{ MASTER, MEDIATOR }) {
            MasterAccessControlEntry[] persisted = subject.findByUserId("user", type);
            assertNotNull("Not found for user and " + type, persisted);
            assertEquals("Not found for user and " + type, 1, persisted.length);
        }
    }

    private MasterAccessControlEntryEntity create(String userId,
                                                  String domain,
                                                  String interfaceName,
                                                  TrustLevel trustLevel,
                                                  TrustLevel[] possibleTrustLevels,
                                                  TrustLevel changeTrustLevel,
                                                  TrustLevel[] possibleChangeTrustLevels,
                                                  String operation,
                                                  Permission permission,
                                                  Permission[] possiblePermissions) {
        return create(userId,
                      domain,
                      interfaceName,
                      trustLevel,
                      possibleTrustLevels,
                      changeTrustLevel,
                      possibleChangeTrustLevels,
                      operation,
                      permission,
                      possiblePermissions,
                      MASTER);
    }

    private MasterAccessControlEntryEntity create(String userId,
                                                  String domain,
                                                  String interfaceName,
                                                  TrustLevel trustLevel,
                                                  TrustLevel[] possibleTrustLevels,
                                                  TrustLevel changeTrustLevel,
                                                  TrustLevel[] possibleChangeTrustLevels,
                                                  String operation,
                                                  Permission permission,
                                                  Permission[] possiblePermissions,
                                                  ControlEntryType type) {
        MasterAccessControlEntryEntity entity = new MasterAccessControlEntryEntity();
        entity.setUserId(userId);
        entity.setDomain(domain);
        entity.setInterfaceName(interfaceName);
        entity.setDefaultRequiredTrustLevel(trustLevel);
        entity.setPossibleRequiredTrustLevels(new HashSet<TrustLevel>(Arrays.asList(possibleTrustLevels)));
        entity.setDefaultRequiredControlEntryChangeTrustLevel(changeTrustLevel);
        entity.setPossibleRequiredControlEntryChangeTrustLevels(new HashSet<TrustLevel>(Arrays.asList(possibleChangeTrustLevels)));
        entity.setOperation(operation);
        entity.setDefaultConsumerPermission(permission);
        entity.setPossibleConsumerPermissions(new HashSet<Permission>(Arrays.asList(possiblePermissions)));
        entity.setType(type);
        entityManager.persist(entity);
        return entity;
    }

    private void flushAndClear() {
        entityManager.flush();
        entityManager.clear();
    }

}
