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

import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;

public class GlobalDomainAccessControllerQueueJob {
    private ChangeType changeType;
    private MasterAccessControlEntry masterAccessControlEntry;
    private MasterAccessControlEntry mediatorAccessControlEntry;;
    private OwnerAccessControlEntry ownerAccessControlEntry;
    private MasterRegistrationControlEntry masterRegistrationControlEntry;
    private MasterRegistrationControlEntry mediatorRegistrationControlEntry;
    private OwnerRegistrationControlEntry ownerRegistrationControlEntry;

    public ChangeType getChangeType() {
        return changeType;
    }

    public MasterAccessControlEntry getMasterAccessControlEntry() {
        return masterAccessControlEntry;
    }

    public MasterAccessControlEntry getMediatorAccessControlEntry() {
        return mediatorAccessControlEntry;
    }

    public OwnerAccessControlEntry getOwnerAccessControlEntry() {
        return ownerAccessControlEntry;
    }

    public MasterRegistrationControlEntry getMasterRegistrationControlEntry() {
        return masterRegistrationControlEntry;
    }

    public MasterRegistrationControlEntry getMediatorRegistrationControlEntry() {
        return mediatorRegistrationControlEntry;
    }

    public OwnerRegistrationControlEntry getOwnerRegistrationControlEntry() {
        return ownerRegistrationControlEntry;
    }

    public void setChangeType(ChangeType changeType) {
        this.changeType = changeType;
    }

    public void setMasterAccessControlEntry(MasterAccessControlEntry masterAccessControlEntry) {
        this.masterAccessControlEntry = masterAccessControlEntry;
    }

    public void setMediatorAccessControlEntry(MasterAccessControlEntry mediatorAccessControlEntry) {
        this.mediatorAccessControlEntry = mediatorAccessControlEntry;
    }

    public void setOwnerAccessControlEntry(OwnerAccessControlEntry ownerAccessControlEntry) {
        this.ownerAccessControlEntry = ownerAccessControlEntry;
    }

    public void setMasterRegistrationControlEntry(MasterRegistrationControlEntry masterRegistrationControlEntry) {
        this.masterRegistrationControlEntry = masterRegistrationControlEntry;
    }

    public void setMediatorRegistrationControlEntry(MasterRegistrationControlEntry mediatorRegistrationControlEntry) {
        this.mediatorRegistrationControlEntry = mediatorRegistrationControlEntry;
    }

    public void setOwnerRegistrationControlEntry(OwnerRegistrationControlEntry ownerRegistrationControlEntry) {
        this.ownerRegistrationControlEntry = ownerRegistrationControlEntry;
    }
}
