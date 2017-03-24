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

import io.joynr.accesscontrol.global.jee.persistence.OwnerAccessControlEntryEntity;
import io.joynr.exceptions.JoynrIllegalStateException;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.Role;

@Stateless
public class OwnerAccessControlEntryManager {

    private EntityManager entityManager;

    private DomainRoleEntryManager domainRoleEntryManager;

    // Only required for testing with Arquillian
    protected OwnerAccessControlEntryManager() {
    }

    @Inject
    public OwnerAccessControlEntryManager(EntityManager entityManager, DomainRoleEntryManager domainRoleEntryManager) {
        this.entityManager = entityManager;
        this.domainRoleEntryManager = domainRoleEntryManager;
    }

    private OwnerAccessControlEntry mapEntityToJoynrType(OwnerAccessControlEntryEntity entity) {
        OwnerAccessControlEntry result = new OwnerAccessControlEntry(entity.getUserId(),
                                                                     entity.getDomain(),
                                                                     entity.getInterfaceName(),
                                                                     entity.getRequiredTrustLevel(),
                                                                     entity.getRequiredAceChangeTrustLevel(),
                                                                     entity.getOperation(),
                                                                     entity.getConsumerPermission());
        return result;
    }

    private OwnerAccessControlEntry[] executeAndConvert(Query query) {
        List<OwnerAccessControlEntryEntity> resultList = query.getResultList();
        Set<OwnerAccessControlEntry> resultSet = resultList.stream().map(this::mapEntityToJoynrType).collect(toSet());
        return resultSet.toArray(new OwnerAccessControlEntry[resultSet.size()]);
    }

    public OwnerAccessControlEntry[] findByUserId(String userId) {
        Query query = entityManager.createQuery("select oace from OwnerAccessControlEntryEntity oace where oace.userId = :userId",
                                                OwnerAccessControlEntryEntity.class);
        query.setParameter("userId", userId);
        return executeAndConvert(query);
    }

    public OwnerAccessControlEntry[] findByDomainAndInterfaceName(String domain, String interfaceName) {
        Query query = entityManager.createQuery("select oace from OwnerAccessControlEntryEntity oace where oace.domain = :domain and oace.interfaceName = :interfaceName",
                                                OwnerAccessControlEntryEntity.class);
        query.setParameter("domain", domain);
        query.setParameter("interfaceName", interfaceName);
        return executeAndConvert(query);
    }

    public OwnerAccessControlEntry[] findByUserIdThatAreEditable(String userId) {
        Query query = entityManager.createQuery("select oace from OwnerAccessControlEntryEntity oace, "
                + "DomainRoleEntryEntity dre, in(dre.domains) dds where oace.userId = :userId "
                + "and oace.domain = dds and dre.userId = :userId and dre.role = :role");
        query.setParameter("userId", userId);
        query.setParameter("role", Role.OWNER);
        return executeAndConvert(query);
    }

    public CreateOrUpdateResult<OwnerAccessControlEntry> createOrUpdate(OwnerAccessControlEntry updatedOwnerAce) {
        if (!domainRoleEntryManager.hasCurrentUserGotRoleForDomain(Role.OWNER, updatedOwnerAce.getDomain())) {
            return null;
        }
        OwnerAccessControlEntryEntity entity = findByUserIdDomainInterfaceNameAndOperation(updatedOwnerAce.getUid(),
                                                                                           updatedOwnerAce.getDomain(),
                                                                                           updatedOwnerAce.getInterfaceName(),
                                                                                           updatedOwnerAce.getOperation());
        boolean created = entity == null;
        if (created) {
            entity = new OwnerAccessControlEntryEntity();
            entity.setUserId(updatedOwnerAce.getUid());
            entity.setDomain(updatedOwnerAce.getDomain());
            entity.setInterfaceName(updatedOwnerAce.getInterfaceName());
            entity.setOperation(updatedOwnerAce.getOperation());
            entityManager.persist(entity);
        }
        entity.setRequiredTrustLevel(updatedOwnerAce.getRequiredTrustLevel());
        entity.setRequiredAceChangeTrustLevel(updatedOwnerAce.getRequiredAceChangeTrustLevel());
        entity.setConsumerPermission(updatedOwnerAce.getConsumerPermission());
        return new CreateOrUpdateResult<>(mapEntityToJoynrType(entity), created ? ChangeType.ADD : ChangeType.UPDATE);
    }

    private OwnerAccessControlEntryEntity findByUserIdDomainInterfaceNameAndOperation(String userId,
                                                                                      String domain,
                                                                                      String interfaceName,
                                                                                      String operation) {
        OwnerAccessControlEntryEntity entity = null;
        Query query = entityManager.createQuery("select oace from OwnerAccessControlEntryEntity oace where "
                + "oace.userId = :userId and oace.domain = :domain and oace.interfaceName = :interfaceName "
                + "and oace.operation = :operation", OwnerAccessControlEntryEntity.class);
        query.setParameter("userId", userId);
        query.setParameter("domain", domain);
        query.setParameter("interfaceName", interfaceName);
        query.setParameter("operation", operation);
        List<OwnerAccessControlEntryEntity> resultList = query.getResultList();
        if (resultList.size() == 1) {
            entity = resultList.get(0);
        } else if (resultList.size() > 1) {
            throw new JoynrIllegalStateException(format("Too many results found for %s for userId / domain / interfaceName / operation: %s / %s / %s / %s",
                                                        OwnerAccessControlEntryEntity.class.getSimpleName(),
                                                        userId,
                                                        domain,
                                                        interfaceName,
                                                        operation));
        }
        return entity;
    }

    public OwnerAccessControlEntry removeByUserIdDomainInterfaceNameAndOperation(String userId,
                                                                                 String domain,
                                                                                 String interfaceName,
                                                                                 String operation) {
        if (!domainRoleEntryManager.hasCurrentUserGotRoleForDomain(Role.MASTER, domain)) {
            return null;
        }
        OwnerAccessControlEntryEntity entity = findByUserIdDomainInterfaceNameAndOperation(userId,
                                                                                           domain,
                                                                                           interfaceName,
                                                                                           operation);
        OwnerAccessControlEntry removedEntry = null;
        if (entity != null) {
            entityManager.remove(entity);
            removedEntry = mapEntityToJoynrType(entity);
        }
        return removedEntry;
    }
}
