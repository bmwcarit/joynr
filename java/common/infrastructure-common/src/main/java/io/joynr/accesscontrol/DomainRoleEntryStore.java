/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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
package io.joynr.accesscontrol;

import static com.googlecode.cqengine.query.QueryFactory.and;
import static com.googlecode.cqengine.query.QueryFactory.equal;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.googlecode.cqengine.ConcurrentIndexedCollection;
import com.googlecode.cqengine.IndexedCollection;
import com.googlecode.cqengine.attribute.SimpleAttribute;
import com.googlecode.cqengine.index.hash.HashIndex;
import com.googlecode.cqengine.query.option.QueryOptions;
import com.googlecode.cqengine.resultset.ResultSet;

import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.Role;

class DomainRoleEntryDB extends DomainRoleEntry {

    public static final com.googlecode.cqengine.attribute.Attribute<DomainRoleEntryDB, String> ROLE = new SimpleAttribute<DomainRoleEntryDB, String>("role") {
        public String getValue(DomainRoleEntryDB domainRoleEntryDB, QueryOptions queryOptions) {
            return domainRoleEntryDB.getRole().toString();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<DomainRoleEntryDB, String> UID = new SimpleAttribute<DomainRoleEntryDB, String>("uid") {
        public String getValue(DomainRoleEntryDB domainRoleEntryDB, QueryOptions queryOptions) {
            return domainRoleEntryDB.getUid();
        }
    };

    public DomainRoleEntryDB() {
        super();
    }

    DomainRoleEntryDB(DomainRoleEntry domainRoleEntryObj) {
        super(domainRoleEntryObj);
    }

    public DomainRoleEntry getDomainRoleEntry() {
        return new DomainRoleEntry(this);
    }

}

public class DomainRoleEntryStore {

    private static final Logger logger = LoggerFactory.getLogger(DomainRoleEntryStore.class);

    private IndexedCollection<DomainRoleEntryDB> domainRoleDB = new ConcurrentIndexedCollection<DomainRoleEntryDB>();

    public DomainRoleEntryStore(DomainAccessControlProvisioning domainAccessControlProvisioning) {
        domainRoleDB.addIndex(HashIndex.onAttribute(DomainRoleEntryDB.ROLE));
        domainRoleDB.addIndex(HashIndex.onAttribute(DomainRoleEntryDB.UID));

        Collection<DomainRoleEntry> domainRoleEntries = domainAccessControlProvisioning.getDomainRoleEntries();
        for (DomainRoleEntry provisionedDomainRoleEntry : domainRoleEntries) {
            updateDomainRole(provisionedDomainRoleEntry);
        }
    }

    public synchronized List<DomainRoleEntry> getDomainRoles(String uid) {
        List<DomainRoleEntry> domainRoles = new ArrayList<DomainRoleEntry>();
        com.googlecode.cqengine.query.Query<DomainRoleEntryDB> cqQuery = equal(DomainRoleEntryDB.UID, uid);
        domainRoleDB.retrieve(cqQuery).forEach(result -> domainRoles.add(result.getDomainRoleEntry()));
        return domainRoles;
    }

    public synchronized DomainRoleEntry getDomainRole(String uid, Role role) {
        DomainRoleEntry domainRole = null;

        com.googlecode.cqengine.query.Query<DomainRoleEntryDB> cqQuery = and(equal(DomainRoleEntryDB.UID, uid),
                                                                             equal(DomainRoleEntryDB.ROLE,
                                                                                   role.toString()));
        ResultSet<DomainRoleEntryDB> cqResult = domainRoleDB.retrieve(cqQuery);
        if (!cqResult.isEmpty()) {
            // Note: since (uid, role) is the primary key in domain role table
            // results is either empty or contains exactly one entry
            domainRole = cqResult.uniqueResult().getDomainRoleEntry();
        }
        return domainRole;
    }

    public synchronized Boolean updateDomainRole(DomainRoleEntry updatedEntry) {
        boolean updateSuccess = false;
        try {
            domainRoleDB.add(new DomainRoleEntryDB(updatedEntry));
            updateSuccess = true;
        } catch (IllegalArgumentException | IllegalStateException e) {
            logger.error("UpdateDomainRole failed.", e);
        }

        return updateSuccess;
    }

    public synchronized Boolean removeDomainRole(String uid, Role role) {
        com.googlecode.cqengine.query.Query<DomainRoleEntryDB> cqQuery = and(equal(DomainRoleEntryDB.UID, uid),
                                                                             equal(DomainRoleEntryDB.ROLE,
                                                                                   role.toString()));
        return domainRoleDB.update(domainRoleDB.retrieve(cqQuery), new ArrayList<DomainRoleEntryDB>());
    }
}
