package io.joynr.accesscontrol.global.jee.persistence;

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

import java.util.Set;

import javax.persistence.ElementCollection;
import javax.persistence.Entity;
import javax.persistence.GeneratedValue;
import javax.persistence.Id;

import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

@Entity
public class MasterAccessControlEntryEntity {

    private long id;

    private ControlEntryType type;

    private String userId;

    private String domain;

    private String interfaceName;

    private TrustLevel defaultRequiredTrustLevel;

    private Set<TrustLevel> possibleRequiredTrustLevels;

    private TrustLevel defaultRequiredControlEntryChangeTrustLevel;

    private Set<TrustLevel> possibleRequiredControlEntryChangeTrustLevels;

    private String operation;

    private Permission defaultConsumerPermission;

    private Set<Permission> possibleConsumerPermissions;

    @Id
    @GeneratedValue
    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public ControlEntryType getType() {
        return type;
    }

    public void setType(ControlEntryType type) {
        this.type = type;
    }

    public String getUserId() {
        return userId;
    }

    public void setUserId(String userId) {
        this.userId = userId;
    }

    public String getDomain() {
        return domain;
    }

    public void setDomain(String domain) {
        this.domain = domain;
    }

    public String getInterfaceName() {
        return interfaceName;
    }

    public void setInterfaceName(String interfaceName) {
        this.interfaceName = interfaceName;
    }

    public TrustLevel getDefaultRequiredTrustLevel() {
        return defaultRequiredTrustLevel;
    }

    public void setDefaultRequiredTrustLevel(TrustLevel defaultRequiredTrustLevel) {
        this.defaultRequiredTrustLevel = defaultRequiredTrustLevel;
    }

    @ElementCollection
    public Set<TrustLevel> getPossibleRequiredTrustLevels() {
        return possibleRequiredTrustLevels;
    }

    public void setPossibleRequiredTrustLevels(Set<TrustLevel> possibleRequiredTrustLevels) {
        this.possibleRequiredTrustLevels = possibleRequiredTrustLevels;
    }

    public TrustLevel getDefaultRequiredControlEntryChangeTrustLevel() {
        return defaultRequiredControlEntryChangeTrustLevel;
    }

    public void setDefaultRequiredControlEntryChangeTrustLevel(TrustLevel defaultRequiredControlEntryChangeTrustLevel) {
        this.defaultRequiredControlEntryChangeTrustLevel = defaultRequiredControlEntryChangeTrustLevel;
    }

    @ElementCollection
    public Set<TrustLevel> getPossibleRequiredControlEntryChangeTrustLevels() {
        return possibleRequiredControlEntryChangeTrustLevels;
    }

    public void setPossibleRequiredControlEntryChangeTrustLevels(Set<TrustLevel> possibleRequiredControlEntryChangeTrustLevels) {
        this.possibleRequiredControlEntryChangeTrustLevels = possibleRequiredControlEntryChangeTrustLevels;
    }

    public String getOperation() {
        return operation;
    }

    public void setOperation(String operation) {
        this.operation = operation;
    }

    public Permission getDefaultConsumerPermission() {
        return defaultConsumerPermission;
    }

    public void setDefaultConsumerPermission(Permission defaultConsumerPermission) {
        this.defaultConsumerPermission = defaultConsumerPermission;
    }

    @ElementCollection
    public Set<Permission> getPossibleConsumerPermissions() {
        return possibleConsumerPermissions;
    }

    public void setPossibleConsumerPermissions(Set<Permission> possibleConsumerPermissions) {
        this.possibleConsumerPermissions = possibleConsumerPermissions;
    }
}
