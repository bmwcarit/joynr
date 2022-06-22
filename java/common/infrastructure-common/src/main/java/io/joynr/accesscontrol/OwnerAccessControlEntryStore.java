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

import io.joynr.accesscontrol.datatype.OwnerAccessControlEntryDB;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;
import joynr.infrastructure.DacTypes.Role;

public class OwnerAccessControlEntryStore {
    private static final String WILDCARD = "*";
    private static final Logger logger = LoggerFactory.getLogger(OwnerAccessControlEntryStore.class);

    private DomainRoleEntryStore domainRoleEntryStore;
    private IndexedCollection<OwnerAccessControlEntryDB> ownerAclDB = new ConcurrentIndexedCollection<OwnerAccessControlEntryDB>();

    public OwnerAccessControlEntryStore(DomainRoleEntryStore domainRoleEntryStore) {
        this.domainRoleEntryStore = domainRoleEntryStore;

        ownerAclDB.addIndex(HashIndex.onAttribute(OwnerAccessControlEntryDB.UID));
        ownerAclDB.addIndex(HashIndex.onAttribute(OwnerAccessControlEntryDB.INTERFACENAME));
        ownerAclDB.addIndex(HashIndex.onAttribute(OwnerAccessControlEntryDB.DOMAIN));
        ownerAclDB.addIndex(HashIndex.onAttribute(OwnerAccessControlEntryDB.OPERATION));
    }

    public List<OwnerAccessControlEntry> getControlEntries(String uid) {
        List<OwnerAccessControlEntry> controlEntries = new ArrayList<OwnerAccessControlEntry>();
        com.googlecode.cqengine.query.Query<OwnerAccessControlEntryDB> cqQuery = equal(OwnerAccessControlEntryDB.UID,
                                                                                       uid);
        ownerAclDB.retrieve(cqQuery).forEach(result -> controlEntries.add(result.getOwnerAccessControlEntry()));
        if (controlEntries.isEmpty()) {
            com.googlecode.cqengine.query.Query<OwnerAccessControlEntryDB> cqQueryWildcard = equal(OwnerAccessControlEntryDB.UID,
                                                                                                   WILDCARD);
            ownerAclDB.retrieve(cqQueryWildcard)
                      .forEach(result -> controlEntries.add(result.getOwnerAccessControlEntry()));
        }
        return controlEntries;
    }

    public List<OwnerAccessControlEntry> getEditableAces(String uid, Role role) {
        List<OwnerAccessControlEntry> controlEntries = new ArrayList<OwnerAccessControlEntry>();
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
            com.googlecode.cqengine.query.Query<OwnerAccessControlEntryDB> cqQuery = and(equal(OwnerAccessControlEntryDB.UID,
                                                                                               uid),
                                                                                         equal(OwnerAccessControlEntryDB.DOMAIN,
                                                                                               domain));
            ownerAclDB.retrieve(cqQuery).forEach(result -> controlEntries.add(result.getOwnerAccessControlEntry()));
        }

        return controlEntries;
    }

    public List<OwnerAccessControlEntry> getControlEntries(String domain, String interfaceName) {
        List<OwnerAccessControlEntry> controlEntries = new ArrayList<OwnerAccessControlEntry>();
        com.googlecode.cqengine.query.Query<OwnerAccessControlEntryDB> cqQuery = and(equal(OwnerAccessControlEntryDB.DOMAIN,
                                                                                           domain),
                                                                                     equal(OwnerAccessControlEntryDB.INTERFACENAME,
                                                                                           interfaceName));
        ownerAclDB.retrieve(cqQuery).forEach(result -> controlEntries.add(result.getOwnerAccessControlEntry()));
        return controlEntries;
    }

    public List<OwnerAccessControlEntry> getControlEntries(String uid, String domain, String interfaceName) {
        List<OwnerAccessControlEntry> controlEntries = new ArrayList<OwnerAccessControlEntry>();
        com.googlecode.cqengine.query.Query<OwnerAccessControlEntryDB> cqQuery = and(or(equal(OwnerAccessControlEntryDB.UID,
                                                                                              uid),
                                                                                        equal(OwnerAccessControlEntryDB.UID,
                                                                                              WILDCARD)),
                                                                                     equal(OwnerAccessControlEntryDB.DOMAIN,
                                                                                           domain),
                                                                                     equal(OwnerAccessControlEntryDB.INTERFACENAME,
                                                                                           interfaceName));
        ResultSet<OwnerAccessControlEntryDB> cqResult = ownerAclDB.retrieve(cqQuery,
                                                                            queryOptions(orderBy(descending(OwnerAccessControlEntryDB.UID))));
        String currentUid = null;
        for (OwnerAccessControlEntryDB result : cqResult) {
            if (currentUid == null) {
                currentUid = result.getUid();
            } else if (currentUid.equals(result.getUid())) {
                break;
            }

            controlEntries.add(result.getOwnerAccessControlEntry());
        }

        return controlEntries;
    }

    public OwnerAccessControlEntry getControlEntry(String uid, String domain, String interfaceName, String operation) {

        OwnerAccessControlEntry ownerAce = null;
        com.googlecode.cqengine.query.Query<OwnerAccessControlEntryDB> cqQuery = and(or(equal(OwnerAccessControlEntryDB.UID,
                                                                                              uid),
                                                                                        equal(OwnerAccessControlEntryDB.UID,
                                                                                              WILDCARD)),
                                                                                     equal(OwnerAccessControlEntryDB.DOMAIN,
                                                                                           domain),
                                                                                     equal(OwnerAccessControlEntryDB.INTERFACENAME,
                                                                                           interfaceName),
                                                                                     equal(OwnerAccessControlEntryDB.OPERATION,
                                                                                           operation));
        ResultSet<OwnerAccessControlEntryDB> cqResult = ownerAclDB.retrieve(cqQuery);
        if (!cqResult.isEmpty()) {
            ownerAce = cqResult.uniqueResult().getOwnerAccessControlEntry();
        } else {
            com.googlecode.cqengine.query.Query<OwnerAccessControlEntryDB> cqQueryWildcard = and(or(equal(OwnerAccessControlEntryDB.UID,
                                                                                                          uid),
                                                                                                    equal(OwnerAccessControlEntryDB.UID,
                                                                                                          WILDCARD)),
                                                                                                 equal(OwnerAccessControlEntryDB.DOMAIN,
                                                                                                       domain),
                                                                                                 equal(OwnerAccessControlEntryDB.INTERFACENAME,
                                                                                                       interfaceName),
                                                                                                 equal(OwnerAccessControlEntryDB.OPERATION,
                                                                                                       WILDCARD));
            cqResult = ownerAclDB.retrieve(cqQueryWildcard);
            if (!cqResult.isEmpty()) {
                ownerAce = cqResult.uniqueResult().getOwnerAccessControlEntry();
            }
        }

        return ownerAce;
    }

    public Boolean updateControlEntry(OwnerAccessControlEntry updatedMasterAce) {
        com.googlecode.cqengine.query.Query<OwnerAccessControlEntryDB> cqQuery = and(or(equal(OwnerAccessControlEntryDB.UID,
                                                                                              updatedMasterAce.getUid()),
                                                                                        equal(OwnerAccessControlEntryDB.UID,
                                                                                              WILDCARD)),
                                                                                     equal(OwnerAccessControlEntryDB.DOMAIN,
                                                                                           updatedMasterAce.getDomain()),
                                                                                     equal(OwnerAccessControlEntryDB.INTERFACENAME,
                                                                                           updatedMasterAce.getInterfaceName()),
                                                                                     equal(OwnerAccessControlEntryDB.OPERATION,
                                                                                           updatedMasterAce.getOperation()));
        ResultSet<OwnerAccessControlEntryDB> cqResult = ownerAclDB.retrieve(cqQuery,
                                                                            queryOptions(orderBy(descending(OwnerAccessControlEntryDB.UID))));
        if (!cqResult.isEmpty()) {
            ownerAclDB.remove(cqResult.uniqueResult());
        }
        return ownerAclDB.add(new OwnerAccessControlEntryDB(updatedMasterAce));
    }

    public boolean removeControlEntry(String uid, String domain, String interfaceName, String operation) {
        boolean removeResult = false;
        try {
            com.googlecode.cqengine.query.Query<OwnerAccessControlEntryDB> cqQuery = and(equal(OwnerAccessControlEntryDB.UID,
                                                                                               uid),
                                                                                         equal(OwnerAccessControlEntryDB.DOMAIN,
                                                                                               domain),
                                                                                         equal(OwnerAccessControlEntryDB.INTERFACENAME,
                                                                                               interfaceName),
                                                                                         equal(OwnerAccessControlEntryDB.OPERATION,
                                                                                               operation));
            removeResult = ownerAclDB.update(ownerAclDB.retrieve(cqQuery), new ArrayList<OwnerAccessControlEntryDB>());
        } catch (IllegalArgumentException | IllegalStateException e) {
            logger.error("Remove masterAcl failed.", e);
        }

        return removeResult;
    }
}
