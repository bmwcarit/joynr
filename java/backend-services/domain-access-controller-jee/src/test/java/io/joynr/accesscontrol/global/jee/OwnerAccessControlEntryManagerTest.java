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
import io.joynr.accesscontrol.global.jee.persistence.OwnerAccessControlEntryEntity;
import io.joynr.jeeintegration.api.security.JoynrCallingPrincipal;
import io.joynr.jeeintegration.context.JoynrJeeMessageContext;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

@RunWith(Arquillian.class)
@Transactional(TransactionMode.ROLLBACK)
public class OwnerAccessControlEntryManagerTest {
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
                                     OwnerAccessControlEntryEntity.class,
                                     OwnerAccessControlEntryManager.class,
                                     JoynrConfigurationProvider.class)
                         .addAsLibraries(files)
                         .addAsResource("test-persistence.xml", "META-INF/persistence.xml")
                         .addAsWebInfResource(new File("src/main/webapp/WEB-INF/beans.xml"));
    }

    @Inject
    private EntityManager entityManager;

    @Inject
    private OwnerAccessControlEntryManager subject;

    @Inject
    private JoynrCallingPrincipal joynrCallingPrincipal;

    @Test
    public void testFindByUserId() {
        String userId = "user";
        create(userId, "domain", "interfaceName", TrustLevel.HIGH, TrustLevel.HIGH, "operation", Permission.ASK);

        flushAndClear();

        OwnerAccessControlEntry[] result = subject.findByUserId(userId);

        assertNotNull(result);
        assertEquals(1, result.length);
    }

    @Test
    public void testFindByDomainAndInterfaceName() {
        String domain = "domain";
        String interfaceName = "interfaceName";
        create("user", domain, interfaceName, TrustLevel.HIGH, TrustLevel.HIGH, "operation", Permission.ASK);

        flushAndClear();

        OwnerAccessControlEntry[] result = subject.findByDomainAndInterfaceName(domain, interfaceName);

        assertNotNull(result);
        assertEquals(1, result.length);
    }

    @Test
    public void testFindByUserIdAndAreEditable() {
        String userId = "user1";
        String domain = "domain1";
        String interfaceName = "interfaceName1";
        String operation = "operation1";

        create(userId, domain, interfaceName, TrustLevel.HIGH, TrustLevel.HIGH, operation, Permission.ASK);
        create(userId, "otherdomain", "interfaceName", TrustLevel.LOW, TrustLevel.HIGH, "operation2", Permission.NO);

        DomainRoleEntryEntity domainRoleEntryEntity = new DomainRoleEntryEntity();
        domainRoleEntryEntity.setUserId(userId);
        domainRoleEntryEntity.setRole(Role.OWNER);
        domainRoleEntryEntity.setDomains(new HashSet<String>(Arrays.asList(domain)));
        entityManager.persist(domainRoleEntryEntity);

        flushAndClear();

        OwnerAccessControlEntry[] result = subject.findByUserIdThatAreEditable(userId);

        assertNotNull(result);
        assertEquals(1, result.length);
        assertEquals(interfaceName, result[0].getInterfaceName());
        assertEquals(operation, result[0].getOperation());
    }

    @Test
    public void testCreate() {
        String userId = "user";
        String domain = "domain";
        String interfaceName = "interfaceName";
        String operation = "operation";
        OwnerAccessControlEntry data = new OwnerAccessControlEntry(userId,
                                                                   domain,
                                                                   interfaceName,
                                                                   TrustLevel.HIGH,
                                                                   TrustLevel.LOW,
                                                                   operation,
                                                                   Permission.ASK);

        CreateOrUpdateResult<OwnerAccessControlEntry> result = subject.createOrUpdate(data);
        assertNotNull(result);
        assertEquals(ChangeType.ADD, result.getChangeType());
        assertNotNull(result.getEntry());
        assertEquals(userId, result.getEntry().getUid());

        flushAndClear();

        OwnerAccessControlEntry persisted = subject.findByUserId(userId)[0];
        assertEquals(userId, persisted.getUid());
        assertEquals(domain, persisted.getDomain());
        assertEquals(interfaceName, persisted.getInterfaceName());
        assertEquals(operation, persisted.getOperation());
        assertEquals(TrustLevel.HIGH, persisted.getRequiredTrustLevel());
        assertEquals(TrustLevel.LOW, persisted.getRequiredAceChangeTrustLevel());
        assertEquals(Permission.ASK, persisted.getConsumerPermission());
    }

    @Test
    public void testCreateNotAllowedWithoutOwnerRole() {
        String userId = "user";
        String domain = "domain";
        String interfaceName = "interfaceName";
        String operation = "operation";

        DomainRoleEntryEntity domainRoleEntryEntity = new DomainRoleEntryEntity();
        domainRoleEntryEntity.setUserId(userId);
        domainRoleEntryEntity.setRole(Role.MASTER);
        domainRoleEntryEntity.setDomains(new HashSet<String>(Arrays.asList(domain)));
        entityManager.persist(domainRoleEntryEntity);

        flushAndClear();

        OwnerAccessControlEntry data = new OwnerAccessControlEntry(userId,
                                                                   domain,
                                                                   interfaceName,
                                                                   TrustLevel.HIGH,
                                                                   TrustLevel.LOW,
                                                                   operation,
                                                                   Permission.ASK);

        CreateOrUpdateResult<OwnerAccessControlEntry> result = null;
        JoynrJeeMessageContext.getInstance().activate();
        try {
            joynrCallingPrincipal.setUsername(userId);
            result = subject.createOrUpdate(data);
        } finally {
            JoynrJeeMessageContext.getInstance().deactivate();
        }
        assertNull(result);

        flushAndClear();

        assertEquals(0, subject.findByUserId(userId).length);
    }

    @Test
    public void testUpdate() {
        OwnerAccessControlEntryEntity entity = create("user",
                                                      "domain",
                                                      "interfaceName",
                                                      TrustLevel.HIGH,
                                                      TrustLevel.LOW,
                                                      "operation",
                                                      Permission.ASK);
        entityManager.persist(entity);

        flushAndClear();

        OwnerAccessControlEntry updateData = new OwnerAccessControlEntry("user",
                                                                         "domain",
                                                                         "interfaceName",
                                                                         TrustLevel.MID,
                                                                         TrustLevel.HIGH,
                                                                         "operation",
                                                                         Permission.YES);
        CreateOrUpdateResult<OwnerAccessControlEntry> result = subject.createOrUpdate(updateData);
        assertNotNull(result);
        assertEquals(ChangeType.UPDATE, result.getChangeType());
        assertNotNull(result.getEntry());
        assertEquals("user", result.getEntry().getUid());

        flushAndClear();

        entity = entityManager.find(OwnerAccessControlEntryEntity.class, entity.getId());
        assertNotNull(entity);
        assertEquals(TrustLevel.MID, entity.getRequiredTrustLevel());
        assertEquals(TrustLevel.HIGH, entity.getRequiredAceChangeTrustLevel());
        assertEquals(Permission.YES, entity.getConsumerPermission());
    }

    @Test
    public void testRemove() {
        String userId = "userId";
        String domain = "domain";
        String interfaceName = "interfaceName";
        String operation = "operation";
        OwnerAccessControlEntryEntity entity = create(userId,
                                                      domain,
                                                      interfaceName,
                                                      TrustLevel.MID,
                                                      TrustLevel.HIGH,
                                                      operation,
                                                      Permission.YES);

        flushAndClear();

        assertNotNull(subject.removeByUserIdDomainInterfaceNameAndOperation(userId, domain, interfaceName, operation));

        flushAndClear();

        OwnerAccessControlEntryEntity persisted = entityManager.find(OwnerAccessControlEntryEntity.class,
                                                                     entity.getId());
        assertNull(persisted);
    }

    @Test
    public void testRemoveNotAllowed() {
        String userId = "userId";
        String domain = "domain";
        String interfaceName = "interfaceName";
        String operation = "operation";
        OwnerAccessControlEntryEntity entity = create(userId,
                                                      domain,
                                                      interfaceName,
                                                      TrustLevel.MID,
                                                      TrustLevel.HIGH,
                                                      operation,
                                                      Permission.YES);

        flushAndClear();

        OwnerAccessControlEntry result = null;
        JoynrJeeMessageContext.getInstance().activate();
        try {
            joynrCallingPrincipal.setUsername(userId);
            result = subject.removeByUserIdDomainInterfaceNameAndOperation(userId, domain, interfaceName, operation);
        } finally {
            JoynrJeeMessageContext.getInstance().deactivate();
        }

        assertNull(result);

        flushAndClear();

        OwnerAccessControlEntryEntity persisted = entityManager.find(OwnerAccessControlEntryEntity.class,
                                                                     entity.getId());
        assertNotNull(persisted);
    }

    private OwnerAccessControlEntryEntity create(String userId,
                                                 String domain,
                                                 String interfaceName,
                                                 TrustLevel requiredTrustLevel,
                                                 TrustLevel requiredAceChangeTrustLevel,
                                                 String operation,
                                                 Permission consumerPermission) {
        OwnerAccessControlEntryEntity entity = new OwnerAccessControlEntryEntity();
        entity.setUserId(userId);
        entity.setDomain(domain);
        entity.setInterfaceName(interfaceName);
        entity.setRequiredTrustLevel(requiredTrustLevel);
        entity.setRequiredAceChangeTrustLevel(requiredAceChangeTrustLevel);
        entity.setOperation(operation);
        entity.setConsumerPermission(consumerPermission);
        entityManager.persist(entity);
        return entity;
    }

    private void flushAndClear() {
        entityManager.flush();
        entityManager.clear();
    }
}
