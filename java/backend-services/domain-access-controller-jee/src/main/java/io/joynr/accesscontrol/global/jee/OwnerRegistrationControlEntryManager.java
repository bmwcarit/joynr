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

import static java.lang.String.format;
import static java.util.stream.Collectors.toSet;

import java.util.List;
import java.util.Set;

import javax.ejb.Stateless;
import javax.inject.Inject;
import javax.persistence.EntityManager;
import javax.persistence.Query;

import io.joynr.accesscontrol.global.jee.persistence.OwnerRegistrationControlEntryEntity;
import io.joynr.exceptions.JoynrIllegalStateException;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Role;

@Stateless
public class OwnerRegistrationControlEntryManager {

    private EntityManager entityManager;

    private DomainRoleEntryManager domainRoleEntryManager;

    // Only required for testing with Arquillian
    protected OwnerRegistrationControlEntryManager() {
    }

    @Inject
    public OwnerRegistrationControlEntryManager(EntityManager entityManager,
                                                DomainRoleEntryManager domainRoleEntryManager) {
        this.entityManager = entityManager;
        this.domainRoleEntryManager = domainRoleEntryManager;
    }

    private OwnerRegistrationControlEntry[] executeAndCovert(Query query) {
        List<OwnerRegistrationControlEntryEntity> resultList = query.getResultList();
        Set<OwnerRegistrationControlEntry> resultSet = resultList.stream().map(this::mapEntityToJoynrType).collect(
            toSet());
        return resultSet.toArray(new OwnerRegistrationControlEntry[resultSet.size()]);
    }

    private OwnerRegistrationControlEntry mapEntityToJoynrType(OwnerRegistrationControlEntryEntity entity) {
        return new OwnerRegistrationControlEntry(entity.getUserId(),
                                                 entity.getDomain(),
                                                 entity.getInterfaceName(),
                                                 entity.getRequiredTrustLevel(),
                                                 entity.getRequiredAceChangeTrustLevel(),
                                                 entity.getProviderPermission());
    }

    public OwnerRegistrationControlEntry[] findByUserId(String userId) {
        Query query = entityManager.createQuery("select orce from OwnerRegistrationControlEntryEntity orce "
                + "where orce.userId = :userId", OwnerRegistrationControlEntryEntity.class);
        query.setParameter("userId", userId);
        return executeAndCovert(query);
    }

    public OwnerRegistrationControlEntry[] findByUserIdAndThatIsEditable(String userId) {
        Query query = entityManager.createQuery("select orce from OwnerRegistrationControlEntryEntity orce, "
                                                        + "DomainRoleEntryEntity dre, in(dre.domains) dds where orce.userId = :userId "
                                                        + "and dre.userId = :userId and dre.role = :role and orce.domain = dds",
                                                OwnerRegistrationControlEntryEntity.class);
        query.setParameter("userId", userId);
        query.setParameter("role", Role.OWNER);
        return executeAndCovert(query);
    }

    private OwnerRegistrationControlEntryEntity findByUserIdDomainAndInterfaceName(String userId,
                                                                                   String domain,
                                                                                   String interfaceName) {
        Query query = entityManager.createQuery("select orce from OwnerRegistrationControlEntryEntity orce "
                                                        + "where orce.userId = :userId and orce.domain = :domain and orce.interfaceName = :interfaceName",
                                                OwnerRegistrationControlEntryEntity.class);
        query.setParameter("userId", userId);
        query.setParameter("domain", domain);
        query.setParameter("interfaceName", interfaceName);
        List<OwnerRegistrationControlEntryEntity> resultList = query.getResultList();
        OwnerRegistrationControlEntryEntity entity = null;
        if (resultList.size() == 1) {
            entity = resultList.get(0);
        } else if (resultList.size() > 1) {
            throw new JoynrIllegalStateException(format("Too many results found for %s for userId / domain / interfaceName / operation: %s / %s / %s",
                                                        OwnerRegistrationControlEntryEntity.class.getSimpleName(),
                                                        userId,
                                                        domain,
                                                        interfaceName));
        }
        return entity;
    }

    public CreateOrUpdateResult<OwnerRegistrationControlEntry> createOrUpdate(OwnerRegistrationControlEntry updatedOwnerRce) {
        if (!domainRoleEntryManager.hasCurrentUserGotRoleForDomain(Role.OWNER, updatedOwnerRce.getDomain())) {
            return null;
        }
        OwnerRegistrationControlEntryEntity entity = findByUserIdDomainAndInterfaceName(updatedOwnerRce.getUid(),
                                                                                        updatedOwnerRce.getDomain(),
                                                                                        updatedOwnerRce.getInterfaceName());
        boolean created = entity == null;
        if (created) {
            entity = new OwnerRegistrationControlEntryEntity();
            entity.setUserId(updatedOwnerRce.getUid());
            entity.setDomain(updatedOwnerRce.getDomain());
            entity.setInterfaceName(updatedOwnerRce.getInterfaceName());
            entityManager.persist(entity);
        }
        entity.setRequiredTrustLevel(updatedOwnerRce.getRequiredTrustLevel());
        entity.setRequiredAceChangeTrustLevel(updatedOwnerRce.getRequiredAceChangeTrustLevel());
        entity.setProviderPermission(updatedOwnerRce.getProviderPermission());
        return new CreateOrUpdateResult<>(mapEntityToJoynrType(entity), created ? ChangeType.ADD : ChangeType.UPDATE);
    }

    public OwnerRegistrationControlEntry removeByUserIdDomainAndInterfaceName(String userId,
                                                                              String domain,
                                                                              String interfaceName) {
        if (!domainRoleEntryManager.hasCurrentUserGotRoleForDomain(Role.OWNER, domain)) {
            return null;
        }
        OwnerRegistrationControlEntryEntity entity = findByUserIdDomainAndInterfaceName(userId, domain, interfaceName);
        if (entity != null) {
            entityManager.remove(entity);
            return mapEntityToJoynrType(entity);
        }
        return null;
    }
}
