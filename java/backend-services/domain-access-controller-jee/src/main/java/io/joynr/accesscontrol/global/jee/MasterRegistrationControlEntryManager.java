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

import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import javax.ejb.Stateless;
import javax.inject.Inject;
import javax.persistence.EntityManager;
import javax.persistence.Query;

import com.google.common.collect.Sets;
import io.joynr.accesscontrol.global.jee.persistence.ControlEntryType;
import io.joynr.accesscontrol.global.jee.persistence.MasterRegistrationControlEntryEntity;
import io.joynr.exceptions.JoynrIllegalStateException;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

@Stateless
public class MasterRegistrationControlEntryManager {

    private EntityManager entityManager;

    private DomainRoleEntryManager domainRoleEntryManager;

    // Only required for testing with Arquillian
    protected MasterRegistrationControlEntryManager() {
    }

    @Inject
    public MasterRegistrationControlEntryManager(EntityManager entityManager,
                                                 DomainRoleEntryManager domainRoleEntryManager) {
        this.entityManager = entityManager;
        this.domainRoleEntryManager = domainRoleEntryManager;
    }

    private MasterRegistrationControlEntry[] executeAndConvert(Query query) {
        List<MasterRegistrationControlEntryEntity> resultList = query.getResultList();
        Set<MasterRegistrationControlEntry> resultSet = resultList.stream().map(this::mapEntityToJoynrType).collect(
            Collectors.toSet());
        return resultSet.toArray(new MasterRegistrationControlEntry[resultSet.size()]);
    }

    private MasterRegistrationControlEntry mapEntityToJoynrType(MasterRegistrationControlEntryEntity entity) {
        Set<TrustLevel> possibleRequiredTrustLevels = Sets.newHashSet(entity.getPossibleRequiredTrustLevels());
        Set<TrustLevel> possibleRequiredControlEntryChangeTrustLevels = Sets.newHashSet(entity.getPossibleRequiredControlEntryChangeTrustLevels());
        Set<Permission> possibleProviderPermissions = Sets.newHashSet(entity.getDefaultProviderPermission());
        MasterRegistrationControlEntry entry = new MasterRegistrationControlEntry(entity.getUserId(),
                                                                                  entity.getDomain(),
                                                                                  entity.getInterfaceName(),
                                                                                  entity.getDefaultRequiredTrustLevel(),
                                                                                  possibleRequiredTrustLevels.toArray(new TrustLevel[possibleRequiredTrustLevels.size()]),
                                                                                  entity.getDefaultRequiredControlEntryChangeTrustLevel(),
                                                                                  possibleRequiredControlEntryChangeTrustLevels.toArray(new TrustLevel[possibleRequiredControlEntryChangeTrustLevels.size()]),
                                                                                  entity.getDefaultProviderPermission(),
                                                                                  possibleProviderPermissions.toArray(new Permission[possibleProviderPermissions.size()]));
        return entry;
    }

    public MasterRegistrationControlEntry[] findByUserIdAndType(String userId, ControlEntryType type) {
        Query query = entityManager.createQuery("select mrce from MasterRegistrationControlEntryEntity mrce where mrce.userId = :userId and mrce.type = :type",
                                                MasterRegistrationControlEntryEntity.class);
        query.setParameter("userId", userId);
        query.setParameter("type", type);
        return executeAndConvert(query);
    }

    public MasterRegistrationControlEntry[] findByUserIdAndThatAreEditable(String userId, ControlEntryType type) {
        Query query = entityManager.createQuery("select mrce from MasterRegistrationControlEntryEntity mrce, "
                + "DomainRoleEntryEntity dre, in(dre.domains) dds where mrce.userId = :userId and mrce.type = :type "
                + "and mrce.domain = dds and dre.userId = :userId and dre.role = :role");
        query.setParameter("userId", userId);
        query.setParameter("type", type);
        query.setParameter("role", Role.MASTER);
        return executeAndConvert(query);
    }

    private MasterRegistrationControlEntryEntity findByUserIdDomainInterfaceNameOperationAndType(String userId,
                                                                                                 String domain,
                                                                                                 String interfaceName,
                                                                                                 ControlEntryType type) {
        Query query = entityManager.createQuery("select mrce from MasterRegistrationControlEntryEntity mrce "
                                                        + "where mrce.userId = :userId and mrce.domain = :domain and mrce.interfaceName = :interfaceName and mrce.type = :type",
                                                MasterRegistrationControlEntryEntity.class);
        query.setParameter("userId", userId);
        query.setParameter("domain", domain);
        query.setParameter("interfaceName", interfaceName);
        query.setParameter("type", type);
        List<MasterRegistrationControlEntryEntity> resultList = query.getResultList();
        MasterRegistrationControlEntryEntity entity = null;
        if (resultList.size() == 1) {
            entity = resultList.get(0);
        } else if (resultList.size() > 1) {
            throw new JoynrIllegalStateException(String.format("Too many results found for %s with userId / domain / interfaceName / type: %s / %s / %s / %s",
                                                               MasterRegistrationControlEntryEntity.class.getSimpleName(),
                                                               userId,
                                                               domain,
                                                               interfaceName,
                                                               type));
        }
        return entity;
    }

    public CreateOrUpdateResult<MasterRegistrationControlEntry> createOrUpdate(MasterRegistrationControlEntry updatedMasterRce, ControlEntryType type) {
        if (!domainRoleEntryManager.hasCurrentUserGotRoleForDomain(Role.MASTER, updatedMasterRce.getDomain())) {
            return null;
        }
        MasterRegistrationControlEntryEntity entity = findByUserIdDomainInterfaceNameOperationAndType(updatedMasterRce.getUid(),
                                                                                                      updatedMasterRce.getDomain(),
                                                                                                      updatedMasterRce.getInterfaceName(),
                                                                                                      type);
        boolean created = entity == null;
        if (created) {
            entity = new MasterRegistrationControlEntryEntity();
            entity.setUserId(updatedMasterRce.getUid());
            entity.setDomain(updatedMasterRce.getDomain());
            entity.setInterfaceName(updatedMasterRce.getInterfaceName());
            entity.setType(type);
            entityManager.persist(entity);
        }
        entity.setDefaultRequiredTrustLevel(updatedMasterRce.getDefaultRequiredTrustLevel());
        entity.setPossibleRequiredTrustLevels(Sets.newHashSet(updatedMasterRce.getPossibleRequiredTrustLevels()));
        entity.setDefaultRequiredControlEntryChangeTrustLevel(updatedMasterRce.getDefaultRequiredControlEntryChangeTrustLevel());
        entity.setPossibleRequiredControlEntryChangeTrustLevels(Sets.newHashSet(updatedMasterRce.getPossibleRequiredControlEntryChangeTrustLevels()));
        entity.setDefaultProviderPermission(updatedMasterRce.getDefaultProviderPermission());
        entity.setPossibleProviderPermissions(Sets.newHashSet(updatedMasterRce.getPossibleProviderPermissions()));
        return new CreateOrUpdateResult<>(mapEntityToJoynrType(entity), created ? ChangeType.ADD : ChangeType.UPDATE);
    }

    public MasterRegistrationControlEntry removeByUserIdDomainInterfaceNameAndType(String userId,
                                                                                   String domain,
                                                                                   String interfaceName,
                                                                                   ControlEntryType type) {
        if (!domainRoleEntryManager.hasCurrentUserGotRoleForDomain(Role.MASTER, domain)) {
            return null;
        }
        MasterRegistrationControlEntryEntity entity = findByUserIdDomainInterfaceNameOperationAndType(userId,
                                                                                                      domain,
                                                                                                      interfaceName,
                                                                                                      type);
        if (entity != null) {
            entityManager.remove(entity);
            return mapEntityToJoynrType(entity);
        }
        return null;
    }
}
