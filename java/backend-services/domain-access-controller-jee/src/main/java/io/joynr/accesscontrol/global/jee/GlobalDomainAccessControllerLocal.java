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

import joynr.infrastructure.DacTypes.ChangeType;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.GlobalDomainAccessControllerSync;

import javax.ejb.Local;

@Local
public interface GlobalDomainAccessControllerLocal extends GlobalDomainAccessControllerSync {
    void doFireMasterAccessControlEntryChanged(ChangeType changeType, MasterAccessControlEntry persistedAce);
    void doFireMediatorAccessControlEntryChanged(ChangeType changeType, MasterAccessControlEntry persistedAce);
    void doFireOwnerAccessControlEntryChanged(ChangeType changeType, OwnerAccessControlEntry persistedAce);
    void doFireMasterRegistrationControlEntryChanged(ChangeType changeType, MasterRegistrationControlEntry persistedAce);
    void doFireMediatorRegistrationControlEntryChanged(ChangeType changeType, MasterRegistrationControlEntry persistedAce);
    void doFireOwnerRegistrationControlEntryChanged(ChangeType changeType, OwnerRegistrationControlEntry persistedAce);
}
