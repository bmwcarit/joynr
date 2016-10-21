package io.joynr.accesscontrol.global.jee;

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

import static java.util.stream.Collectors.toSet;

import java.util.List;
import java.util.Set;

import javax.ejb.Stateless;
import javax.inject.Inject;
import javax.persistence.EntityManager;
import javax.persistence.Query;

import com.google.common.collect.Sets;
import io.joynr.accesscontrol.global.jee.persistence.DomainRoleEntryEntity;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.Role;

@Stateless
public class DomainRoleEntryManager {

    private EntityManager entityManager;

    // Required for integration tests with Arquillian
    protected DomainRoleEntryManager() {
    }

    @Inject
    public DomainRoleEntryManager(EntityManager entityManager) {
        this.entityManager = entityManager;
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

    public DomainRoleEntryEntity createOrUpdate(DomainRoleEntry joynrType) {
        DomainRoleEntryEntity entity = findByUserIdAndRole(joynrType.getUid(), joynrType.getRole());
        if (entity != null) {
            entity.getDomains().clear();
            entity.getDomains().addAll(Sets.newHashSet(joynrType.getDomains()));
        } else {
            entity = mapJoynrTypeToEntity(joynrType);
            entityManager.persist(entity);
        }
        return entity;
    }

    public boolean removeByUserIdAndRole(String userId, Role role) {
        DomainRoleEntryEntity entity = findByUserIdAndRole(userId, role);
        if (entity != null) {
            entityManager.remove(entity);
            return true;
        }
        return false;
    }
}
