package io.joynr.accesscontrol.global.jee;

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

import static java.util.stream.Collectors.toSet;

import java.util.List;
import java.util.Set;

import javax.ejb.Stateless;
import javax.enterprise.context.ContextNotActiveException;
import javax.inject.Inject;
import javax.persistence.EntityManager;
import javax.persistence.Query;

import com.google.common.collect.Sets;
import io.joynr.accesscontrol.global.jee.persistence.DomainRoleEntryEntity;
import io.joynr.jeeintegration.api.security.JoynrCallingPrincipal;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.Role;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@Stateless
public class DomainRoleEntryManager {

    private static final Logger logger = LoggerFactory.getLogger(DomainRoleEntryManager.class);

    private EntityManager entityManager;

    private JoynrCallingPrincipal joynrCallingPrincipal;

    // Required for integration tests with Arquillian
    protected DomainRoleEntryManager() {
    }

    @Inject
    public DomainRoleEntryManager(EntityManager entityManager, JoynrCallingPrincipal joynrCallingPrincipal) {
        this.entityManager = entityManager;
        this.joynrCallingPrincipal = joynrCallingPrincipal;
    }

    private DomainRoleEntry mapEntityToJoynrType(DomainRoleEntryEntity entity) {
        Set<String> entityDomains = entity.getDomains();
        String[] domains = entityDomains.toArray(new String[entityDomains.size()]);
        return new DomainRoleEntry(entity.getUserId(), domains, entity.getRole());
    }

    private DomainRoleEntryEntity mapJoynrTypeToEntity(DomainRoleEntry joynrType) {
        DomainRoleEntryEntity result = new DomainRoleEntryEntity();
        result.setUserId(joynrType.getUid());
        result.setDomains(Sets.newHashSet(joynrType.getDomains()));
        result.setRole(joynrType.getRole());
        return result;
    }

    public DomainRoleEntry[] findByUserId(String userId) {
        Query query = entityManager.createQuery("select dre from DomainRoleEntryEntity dre where dre.userId = :userId", DomainRoleEntryEntity.class);
        query.setParameter("userId", userId);
        List<DomainRoleEntryEntity> entities = query.getResultList();
        Set<DomainRoleEntry> resultSet = entities.stream().map(this::mapEntityToJoynrType).collect(toSet());
        return resultSet.toArray(new DomainRoleEntry[resultSet.size()]);
    }

    private DomainRoleEntryEntity findByUserIdAndRole(String userId, Role role) {
        Query query = entityManager.createQuery("select dre from DomainRoleEntryEntity dre where dre.userId = :userId and dre.role = :role",
                                                DomainRoleEntryEntity.class);
        query.setParameter("userId", userId);
        query.setParameter("role", role);
        List<DomainRoleEntryEntity> resultList = query.getResultList();
        if (resultList.size() == 1) {
            return resultList.get(0);
        } else if (resultList.size() > 1) {
            throw new IllegalStateException("Too many entries found for DomainRoleEntryEntity with user id " + userId
                    + " and role " + role);
        }
        return null;
    }

    public CreateOrUpdateResult<DomainRoleEntry> createOrUpdate(DomainRoleEntry joynrType) {
        DomainRoleEntryEntity entity = findByUserIdAndRole(joynrType.getUid(), joynrType.getRole());
        boolean created = entity == null;
        if (!created) {
            entity.getDomains().clear();
            entity.getDomains().addAll(Sets.newHashSet(joynrType.getDomains()));
        } else {
            entity = mapJoynrTypeToEntity(joynrType);
            entityManager.persist(entity);
        }
        return new CreateOrUpdateResult<>(mapEntityToJoynrType(entity), created ? ChangeType.ADD : ChangeType.UPDATE);
    }

    public DomainRoleEntry removeByUserIdAndRole(String userId, Role role) {
        DomainRoleEntryEntity entity = findByUserIdAndRole(userId, role);
        if (entity != null) {
            entityManager.remove(entity);
            return mapEntityToJoynrType(entity);
        }
        return null;
    }

    /**
     * Determine whether the joynr calling principal, if available, has the given role for the given domain.
     * The result will default to <code>true</code> if there is no current user - that is, the call is being
     * made from without of a {@link io.joynr.jeeintegration.api.JoynrJeeMessageScoped joynr calling scope}.
     * This way, the bean functionality can be used to create an admin API which doesn't require a user to
     * be already available in the database, e.g. for initial creation of security settings or providing a
     * RESTful API which is secured by other means.
     *
     * @param role the role the user should have.
     * @param domain the domain for which the current user must have the role.
     * @return <code>true</code> if there is a current user, and that user has the specified role in the given
     * domain, <code>false</code> if there is a current user and they don't. <code>true</code> if there is no
     * current user.
     */
    public boolean hasCurrentUserGotRoleForDomain(Role role, String domain) {
        try {
            DomainRoleEntryEntity domainRoleEntry = findByUserIdAndRole(joynrCallingPrincipal.getUsername(), role);
            return domainRoleEntry != null && domainRoleEntry.getDomains().contains(domain);
        } catch (ContextNotActiveException e) {
            logger.debug("No joynr message scope context active. Defaulting to 'true'.");
        }
        return true;
    }
}
