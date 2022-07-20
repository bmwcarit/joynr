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
import static com.googlecode.cqengine.query.QueryFactory.descending;
import static com.googlecode.cqengine.query.QueryFactory.equal;
import static com.googlecode.cqengine.query.QueryFactory.or;
import static com.googlecode.cqengine.query.QueryFactory.orderBy;
import static com.googlecode.cqengine.query.QueryFactory.queryOptions;

import java.util.ArrayList;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.googlecode.cqengine.ConcurrentIndexedCollection;
import com.googlecode.cqengine.IndexedCollection;
import com.googlecode.cqengine.index.hash.HashIndex;
import com.googlecode.cqengine.resultset.ResultSet;

import io.joynr.accesscontrol.datatype.OwnerRegistrationControlEntryDB;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Role;

public class OwnerRegistrationControlEntryStore {
    private static final String WILDCARD = "*";
    private static final Logger logger = LoggerFactory.getLogger(OwnerRegistrationControlEntryStore.class);

    private DomainRoleEntryStore domainRoleEntryStore;
    private IndexedCollection<OwnerRegistrationControlEntryDB> ownerRclDB = new ConcurrentIndexedCollection<OwnerRegistrationControlEntryDB>();

    public OwnerRegistrationControlEntryStore(DomainRoleEntryStore domainRoleEntryStore) {
        this.domainRoleEntryStore = domainRoleEntryStore;

        ownerRclDB.addIndex(HashIndex.onAttribute(OwnerRegistrationControlEntryDB.UID));
        ownerRclDB.addIndex(HashIndex.onAttribute(OwnerRegistrationControlEntryDB.INTERFACENAME));
        ownerRclDB.addIndex(HashIndex.onAttribute(OwnerRegistrationControlEntryDB.DOMAIN));
    }

    public List<OwnerRegistrationControlEntry> getControlEntries(String uid) {
        List<OwnerRegistrationControlEntry> controlEntries = new ArrayList<OwnerRegistrationControlEntry>();
        com.googlecode.cqengine.query.Query<OwnerRegistrationControlEntryDB> cqQuery = equal(OwnerRegistrationControlEntryDB.UID,
                                                                                             uid);
        ownerRclDB.retrieve(cqQuery).forEach(result -> controlEntries.add(result.getOwnerRegistrationControlEntry()));
        if (controlEntries.isEmpty()) {
            com.googlecode.cqengine.query.Query<OwnerRegistrationControlEntryDB> cqQueryWildcard = equal(OwnerRegistrationControlEntryDB.UID,
                                                                                                         WILDCARD);
            ownerRclDB.retrieve(cqQueryWildcard)
                      .forEach(result -> controlEntries.add(result.getOwnerRegistrationControlEntry()));
        }
        return controlEntries;
    }

    public List<OwnerRegistrationControlEntry> getEditableAces(String uid, Role role) {
        List<OwnerRegistrationControlEntry> controlEntries = new ArrayList<OwnerRegistrationControlEntry>();
        // find out first on which domains uid has specified role
        DomainRoleEntry domainRoleEntry = domainRoleEntryStore.getDomainRole(uid, role);
        // read domains from DRE
        if (domainRoleEntry == null) {
            return controlEntries;
        }
        // if uid has no domains with specified role return empty list
        if (domainRoleEntry.getDomains().length == 0) {
            return controlEntries;
        }

        for (String domain : domainRoleEntry.getDomains()) {
            com.googlecode.cqengine.query.Query<OwnerRegistrationControlEntryDB> cqQuery = and(equal(OwnerRegistrationControlEntryDB.UID,
                                                                                                     uid),
                                                                                               equal(OwnerRegistrationControlEntryDB.DOMAIN,
                                                                                                     domain));
            ownerRclDB.retrieve(cqQuery)
                      .forEach(result -> controlEntries.add(result.getOwnerRegistrationControlEntry()));
        }

        return controlEntries;
    }

    public List<OwnerRegistrationControlEntry> getControlEntries(String domain, String interfaceName) {
        List<OwnerRegistrationControlEntry> controlEntries = new ArrayList<OwnerRegistrationControlEntry>();
        com.googlecode.cqengine.query.Query<OwnerRegistrationControlEntryDB> cqQuery = and(equal(OwnerRegistrationControlEntryDB.DOMAIN,
                                                                                                 domain),
                                                                                           equal(OwnerRegistrationControlEntryDB.INTERFACENAME,
                                                                                                 interfaceName));
        ownerRclDB.retrieve(cqQuery).forEach(result -> controlEntries.add(result.getOwnerRegistrationControlEntry()));
        return controlEntries;
    }

    public OwnerRegistrationControlEntry getControlEntry(String uid, String domain, String interfaceName) {

        OwnerRegistrationControlEntry ownerRce = null;
        com.googlecode.cqengine.query.Query<OwnerRegistrationControlEntryDB> cqQuery = and(or(equal(OwnerRegistrationControlEntryDB.UID,
                                                                                                    uid),
                                                                                              equal(OwnerRegistrationControlEntryDB.UID,
                                                                                                    WILDCARD)),
                                                                                           equal(OwnerRegistrationControlEntryDB.DOMAIN,
                                                                                                 domain),
                                                                                           equal(OwnerRegistrationControlEntryDB.INTERFACENAME,
                                                                                                 interfaceName));
        ResultSet<OwnerRegistrationControlEntryDB> cqResult = ownerRclDB.retrieve(cqQuery,
                                                                                  queryOptions(orderBy(descending(OwnerRegistrationControlEntryDB.UID))));
        if (!cqResult.isEmpty()) {
            ownerRce = cqResult.uniqueResult().getOwnerRegistrationControlEntry();
        }

        return ownerRce;
    }

    public Boolean updateControlEntry(OwnerRegistrationControlEntry updatedMasterAce) {
        com.googlecode.cqengine.query.Query<OwnerRegistrationControlEntryDB> cqQuery = and(or(equal(OwnerRegistrationControlEntryDB.UID,
                                                                                                    updatedMasterAce.getUid()),
                                                                                              equal(OwnerRegistrationControlEntryDB.UID,
                                                                                                    WILDCARD)),
                                                                                           equal(OwnerRegistrationControlEntryDB.DOMAIN,
                                                                                                 updatedMasterAce.getDomain()),
                                                                                           equal(OwnerRegistrationControlEntryDB.INTERFACENAME,
                                                                                                 updatedMasterAce.getInterfaceName()));
        ResultSet<OwnerRegistrationControlEntryDB> cqResult = ownerRclDB.retrieve(cqQuery,
                                                                                  queryOptions(orderBy(descending(OwnerRegistrationControlEntryDB.UID))));
        if (!cqResult.isEmpty()) {
            ownerRclDB.remove(cqResult.uniqueResult());
        }
        return ownerRclDB.add(new OwnerRegistrationControlEntryDB(updatedMasterAce));
    }

    public boolean removeControlEntry(String uid, String domain, String interfaceName) {
        boolean removeResult = false;
        try {
            com.googlecode.cqengine.query.Query<OwnerRegistrationControlEntryDB> cqQuery = and(equal(OwnerRegistrationControlEntryDB.UID,
                                                                                                     uid),
                                                                                               equal(OwnerRegistrationControlEntryDB.DOMAIN,
                                                                                                     domain),
                                                                                               equal(OwnerRegistrationControlEntryDB.INTERFACENAME,
                                                                                                     interfaceName));
            removeResult = ownerRclDB.update(ownerRclDB.retrieve(cqQuery),
                                             new ArrayList<OwnerRegistrationControlEntryDB>());
        } catch (IllegalArgumentException | IllegalStateException e) {
            logger.error("Remove masterAcl failed.", e);
        }

        return removeResult;
    }
}
