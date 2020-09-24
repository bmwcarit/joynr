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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.File;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

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
import io.joynr.jeeintegration.api.security.JoynrCallingPrincipal;
import io.joynr.jeeintegration.context.JoynrJeeMessageContext;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.Role;

@RunWith(Arquillian.class)
@Transactional(TransactionMode.ROLLBACK)
public class DomainRoleEntryManagerTest {
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
                                     TestJoynrConfigurationProvider.class)
                         .addAsLibraries(files)
                         .addAsResource("test-persistence.xml", "META-INF/persistence.xml")
                         .addAsWebInfResource(new File("src/main/webapp/WEB-INF/beans.xml"));
    }

    @Inject
    private DomainRoleEntryManager subject;

    @Inject
    private EntityManager entityManager;

    @Inject
    private JoynrCallingPrincipal joynrCallingPrincipal;

    @Test
    public void testFindByUserId() {
        String userId = "user";
        Set<String> domains1 = new HashSet<>(Arrays.asList("domain.1", "domain.2"));
        Set<String> domains2 = new HashSet<>(Arrays.asList("domain.3", "domain.4", "domain.5"));

        create(userId, domains1, Role.MASTER);
        create(userId, domains2, Role.OWNER);

        entityManager.flush();
        entityManager.clear();

        DomainRoleEntry[] result = subject.findByUserId(userId);

        assertNotNull(result);
        assertEquals(2, result.length);
        for (DomainRoleEntry entry : result) {
            assertEquals(userId, entry.getUid());
            if (Role.MASTER.equals(entry.getRole())) {
                assertEquals(domains1, new HashSet<String>(Arrays.asList(entry.getDomains())));
            } else if (Role.OWNER.equals(entry.getRole())) {
                assertEquals(domains2, new HashSet<String>(Arrays.asList(entry.getDomains())));
            } else {
                fail("Should only have master and owner roles. Got: " + entry.getRole());
            }
        }
    }

    @Test
    public void testCreate() {
        String userId = "user";

        DomainRoleEntry newEntry = new DomainRoleEntry(userId, new String[]{ "domain" }, Role.OWNER);
        CreateOrUpdateResult<DomainRoleEntry> result = subject.createOrUpdate(newEntry);

        flushAndClear();

        assertNotNull(result);
        assertEquals(ChangeType.ADD, result.getChangeType());
        assertEquals(userId, result.getEntry().getUid());
        assertEquals(Role.OWNER, result.getEntry().getRole());
        assertEquals(1, result.getEntry().getDomains().length);
        assertEquals("domain", result.getEntry().getDomains()[0]);

        DomainRoleEntry[] persisted = subject.findByUserId(userId);
        assertNotNull(persisted);
        assertEquals(1, persisted.length);
    }

    @Test
    public void testUpdateExistingEntry() {
        String userId = "user";
        create(userId, new HashSet<String>(Arrays.asList("domain1", "domain2")), Role.OWNER);

        flushAndClear();

        DomainRoleEntry updatedEntry = new DomainRoleEntry(userId, new String[]{ "domain3" }, Role.OWNER);
        CreateOrUpdateResult<DomainRoleEntry> result = subject.createOrUpdate(updatedEntry);

        flushAndClear();

        assertNotNull(result);
        assertEquals(ChangeType.UPDATE, result.getChangeType());
        assertEquals(userId, result.getEntry().getUid());
        assertEquals(Role.OWNER, result.getEntry().getRole());
        assertNotNull(result.getEntry().getRole());
        assertEquals(1, result.getEntry().getDomains().length);
        assertEquals("domain3", result.getEntry().getDomains()[0]);
    }

    @Test
    public void testRemoveExistingEntry() {
        String userId = "user";
        create(userId, new HashSet<String>(), Role.OWNER);

        flushAndClear();

        DomainRoleEntry result = subject.removeByUserIdAndRole(userId, Role.OWNER);

        flushAndClear();

        assertNotNull(result);
        DomainRoleEntry[] byUserId = subject.findByUserId(userId);
        assertNotNull(byUserId);
        assertEquals(0, byUserId.length);
    }

    @Test
    public void testCurrentUserHasRoleInDomainDefaultsToTrueWithoutCurrentUser() {
        assertTrue(subject.hasCurrentUserGotRoleForDomain(Role.MASTER, "doesn't even exist"));
    }

    @Test
    public void testCurrentUserHasRoleInDomainFalse() {
        assertFalse(testCurrentUserHasRoleInDomain(Role.OWNER));
    }

    @Test
    public void testCurrentUserHasRoleInDomainTrue() {
        assertTrue(testCurrentUserHasRoleInDomain(Role.MASTER));
    }

    @Test
    public void testCurrentUserHasRoleInDomainOtherDomain() {
        assertFalse(testCurrentUserHasRoleInDomain(Role.MASTER,
                                                   new HashSet<String>(Arrays.asList("other.domain",
                                                                                     "yet another domain"))));
    }

    private boolean testCurrentUserHasRoleInDomain(Role persistedRole) {
        return testCurrentUserHasRoleInDomain(persistedRole,
                                              new HashSet<String>(Arrays.asList("domain", "domain1", "domain2")));
    }

    private boolean testCurrentUserHasRoleInDomain(Role persistedRole, Set<String> domains) {
        String username = "current-user";
        DomainRoleEntryEntity entry = new DomainRoleEntryEntity();
        entry.setUserId(username);
        entry.setRole(persistedRole);
        entry.setDomains(domains);
        entityManager.persist(entry);

        JoynrJeeMessageContext.getInstance().activate();
        joynrCallingPrincipal.setUsername(username);
        boolean result = subject.hasCurrentUserGotRoleForDomain(Role.MASTER, "domain");
        JoynrJeeMessageContext.getInstance().deactivate();
        return result;
    }

    private DomainRoleEntryEntity create(String userId, Set<String> domains, Role role) {
        DomainRoleEntryEntity entity = new DomainRoleEntryEntity();
        entity.setUserId(userId);
        entity.setDomains(domains);
        entity.setRole(role);
        entityManager.persist(entity);
        return entity;
    }

    private void flushAndClear() {
        entityManager.flush();
        entityManager.clear();
    }

}
