package io.joynr.accesscontrol.global;

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

import io.joynr.accesscontrol.DomainAccessControlStore;

import java.util.List;

import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;

/**
 * Interface to ACL store used for admin tasks. Used by the GDAC web interface.
 */
//TODO  this interface is being used until the "getEditable???Entries()" can be used. The refactoring will be done with JIRA-47
public interface GlobalDomainAccessStoreAdmin extends DomainAccessControlStore {

    /**
     * Query all master ACL entries from the underlying storage
     * @return a list of MasterAccessControlEntry or an empty list if there are no entries
     */
    public List<MasterAccessControlEntry> getAllMasterAclEntries();

    /**
     * Query all owner ACL entries from the underlying storage
     * @return a list of OwnerAccessControlEntry or an empty list if there are no entries
     */
    public List<OwnerAccessControlEntry> getAllOwnerAclEntries();

    /**
     * Query all domain role table entries from the underlying storage
     * @return a list of DomainRoleEntry or an empty list if there are no entries
     */
    public List<DomainRoleEntry> getAllDomainRoleEntries();

}
