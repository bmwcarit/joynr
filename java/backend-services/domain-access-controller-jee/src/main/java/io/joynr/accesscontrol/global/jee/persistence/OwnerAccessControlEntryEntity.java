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

import javax.persistence.Entity;
import javax.persistence.GeneratedValue;
import javax.persistence.Id;

import joynr.infrastructure.DacTypes.Permission;
import joynr.infrastructure.DacTypes.TrustLevel;

@Entity
public class OwnerAccessControlEntryEntity {

    private long id;

    private String userId;

    private String domain;

    private String interfaceName;

    private TrustLevel requiredTrustLevel;

    private TrustLevel requiredAceChangeTrustLevel;

    private String operation;

    private Permission consumerPermission;

    @Id
    @GeneratedValue
    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
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

    public TrustLevel getRequiredTrustLevel() {
        return requiredTrustLevel;
    }

    public void setRequiredTrustLevel(TrustLevel requiredTrustLevel) {
        this.requiredTrustLevel = requiredTrustLevel;
    }

    public TrustLevel getRequiredAceChangeTrustLevel() {
        return requiredAceChangeTrustLevel;
    }

    public void setRequiredAceChangeTrustLevel(TrustLevel requiredAceChangeTrustLevel) {
        this.requiredAceChangeTrustLevel = requiredAceChangeTrustLevel;
    }

    public String getOperation() {
        return operation;
    }

    public void setOperation(String operation) {
        this.operation = operation;
    }

    public Permission getConsumerPermission() {
        return consumerPermission;
    }

    public void setConsumerPermission(Permission consumerPermission) {
        this.consumerPermission = consumerPermission;
    }
}
