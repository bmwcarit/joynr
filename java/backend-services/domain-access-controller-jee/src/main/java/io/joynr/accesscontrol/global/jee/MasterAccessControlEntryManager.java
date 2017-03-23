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

import com.google.common.collect.Sets;
import io.joynr.accesscontrol.global.jee.persistence.ControlEntryType;
import io.joynr.accesscontrol.global.jee.persistence.MasterAccessControlEntryEntity;
import io.joynr.exceptions.JoynrIllegalStateException;
import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.Role;
import joynr.infrastructure.DacTypes.TrustLevel;

@Stateless
public class MasterAccessControlEntryManager {

    private EntityManager entityManager;

    private DomainRoleEntryManager domainRoleEntryManager;

    // Only required for testing with Arquillian
    protected MasterAccessControlEntryManager() {
    }

    @Inject
    public MasterAccessControlEntryManager(EntityManager entityManager, DomainRoleEntryManager domainRoleEntryManager) {
        this.entityManager = entityManager;
        this.domainRoleEntryManager = domainRoleEntryManager;
    }

    private MasterAccessControlEntry mapEntityToJoynrType(MasterAccessControlEntryEntity entity) {
        MasterAccessControlEntry result = new MasterAccessControlEntry();
        result.setUid(entity.getUserId());
        result.setDomain(entity.getDomain());
        result.setInterfaceName(entity.getInterfaceName());
        result.setDefaultRequiredTrustLevel(entity.getDefaultRequiredTrustLevel());
        result.setPossibleRequiredTrustLevels(entity.getPossibleRequiredTrustLevels()
                                                    .toArray(new TrustLevel[entity.getPossibleRequiredTrustLevels()
                                                                                  .size()]));
        result.setDefaultRequiredControlEntryChangeTrustLevel(entity.getDefaultRequiredControlEntryChangeTrustLevel());
        result.setPossibleRequiredControlEntryChangeTrustLevels(entity.getPossibleRequiredControlEntryChangeTrustLevels()
                                                                      .toArray(new TrustLevel[entity.getPossibleRequiredControlEntryChangeTrustLevels()
                                                                                                    .size()]));
        result.setOperation(entity.getOperation());
        result.setDefaultConsumerPermission(entity.getDefaultConsumerPermission());
        result.setPossibleConsumerPermissions(entity.getPossibleConsumerPermissions()
                                                    .toArray(new Permission[entity.getPossibleConsumerPermissions()
                                                                                  .size()]));
        return result;
    }

    public MasterAccessControlEntry[] findByUserId(String userId, ControlEntryType type) {
        Query query = entityManager.createQuery("select mace from MasterAccessControlEntryEntity mace where mace.userId = :userId and mace.type = :type",
            MasterAccessControlEntryEntity.class);
        query.setParameter("userId", userId);
        query.setParameter("type", type);
        List<MasterAccessControlEntryEntity> resultList = query.getResultList();
        Set<MasterAccessControlEntry> resultSet = resultList.stream().map(this::mapEntityToJoynrType).collect(
            toSet());
        return resultSet.toArray(new MasterAccessControlEntry[resultSet.size()]);
    }

    public MasterAccessControlEntry[] findByUserIdThatAreEditable(String userId, ControlEntryType type) {
        Query query = entityManager.createQuery("select mace from MasterAccessControlEntryEntity mace, "
            + "DomainRoleEntryEntity dre, in(dre.domains) dds where mace.userId = :userId and mace.type = :type "
            + "and mace.domain = dds and dre.userId = :userId and dre.role = :role");
        query.setParameter("userId", userId);
        query.setParameter("type", type);
        query.setParameter("role", Role.MASTER);
        List<MasterAccessControlEntryEntity> resultList = query.getResultList();
        Set<MasterAccessControlEntry> resultSet = resultList.stream().map(this::mapEntityToJoynrType).collect(toSet());
        return resultSet.toArray(new MasterAccessControlEntry[resultSet.size()]);
    }

    public MasterAccessControlEntry[] findByDomainAndInterfaceName(String domain, String interfaceName, ControlEntryType type) {
        Query query = entityManager.createQuery("select mace from MasterAccessControlEntryEntity mace "
            + "where mace.domain = :domain and mace.interfaceName = :interfaceName and mace.type = :type", MasterAccessControlEntryEntity.class);
        query.setParameter("domain", domain);
        query.setParameter("interfaceName", interfaceName);
        query.setParameter("type", type);
        List<MasterAccessControlEntryEntity> resultList = query.getResultList();
        Set<MasterAccessControlEntry> resultSet = resultList.stream().map(this::mapEntityToJoynrType).collect(toSet());
        return resultSet.toArray(new MasterAccessControlEntry[resultSet.size()]);
    }

    private MasterAccessControlEntryEntity findByUserIdDomainInterfaceNameOperationAndType(String userId,
                                                                                           String domain,
                                                                                           String interfaceName,
                                                                                           String operation,
                                                                                           ControlEntryType type) {
        Query query = entityManager.createQuery("select mace from MasterAccessControlEntryEntity mace "
                + "where mace.userId = :userId and mace.domain = :domain and mace.interfaceName = :interfaceName "
                + "and mace.operation = :operation and mace.type = :type", MasterAccessControlEntryEntity.class);
        query.setParameter("userId", userId);
        query.setParameter("domain", domain);
        query.setParameter("interfaceName", interfaceName);
        query.setParameter("operation", operation);
        query.setParameter("type", type);
        List<MasterAccessControlEntryEntity> resultList = query.getResultList();
        MasterAccessControlEntryEntity entity = null;
        if (resultList.size() == 1) {
            entity = resultList.get(0);
        } else if (resultList.size() > 1) {
            throw new JoynrIllegalStateException(format("Too many master access control entries for unique key uid / domain / interfaceName /operation: %s / %s / %s / %s",
                                                        userId,
                                                        domain,
                                                        interfaceName,
                                                        operation));
        }
        return entity;
    }

    public CreateOrUpdateResult<MasterAccessControlEntry> createOrUpdate(MasterAccessControlEntry updatedMasterAce, ControlEntryType type) {
        MasterAccessControlEntryEntity entity = findByUserIdDomainInterfaceNameOperationAndType(updatedMasterAce.getUid(),
                                                                                                updatedMasterAce.getDomain(),
                                                                                                updatedMasterAce.getInterfaceName(),
                                                                                                updatedMasterAce.getOperation(),
                                                                                                type);
        if (!domainRoleEntryManager.hasCurrentUserGotRoleForDomain(Role.MASTER, updatedMasterAce.getDomain())) {
            return null;
        }
        boolean created = entity == null;
        if (created) {
            entity = new MasterAccessControlEntryEntity();
            entity.setUserId(updatedMasterAce.getUid());
            entity.setDomain(updatedMasterAce.getDomain());
            entity.setInterfaceName(updatedMasterAce.getInterfaceName());
            entity.setOperation(updatedMasterAce.getOperation());
            entity.setType(type);
            entityManager.persist(entity);
        }
        entity.setDefaultRequiredTrustLevel(updatedMasterAce.getDefaultRequiredTrustLevel());
        entity.setPossibleRequiredTrustLevels(Sets.newHashSet(updatedMasterAce.getPossibleRequiredTrustLevels()));
        entity.setDefaultRequiredControlEntryChangeTrustLevel(updatedMasterAce.getDefaultRequiredControlEntryChangeTrustLevel());
        entity.setPossibleRequiredControlEntryChangeTrustLevels(Sets.newHashSet(updatedMasterAce.getPossibleRequiredControlEntryChangeTrustLevels()));
        entity.setDefaultConsumerPermission(updatedMasterAce.getDefaultConsumerPermission());
        entity.setPossibleConsumerPermissions(Sets.newHashSet(updatedMasterAce.getPossibleConsumerPermissions()));
        return new CreateOrUpdateResult<>(mapEntityToJoynrType(entity), created ? ChangeType.ADD : ChangeType.UPDATE);
    }

    public MasterAccessControlEntry removeByUserIdDomainInterfaceNameAndOperation(String uid,
                                                                                  String domain,
                                                                                  String interfaceName,
                                                                                  String operation,
                                                                                  ControlEntryType type) {
        if (!domainRoleEntryManager.hasCurrentUserGotRoleForDomain(Role.MASTER, domain)) {
            return null;
        }
        MasterAccessControlEntryEntity entity = findByUserIdDomainInterfaceNameOperationAndType(uid,
                                                                                                domain,
                                                                                                interfaceName,
                                                                                                operation,
                                                                                                type);
        if (entity != null) {
            entityManager.remove(entity);
            return mapEntityToJoynrType(entity);
        }
        return null;
    }
}
