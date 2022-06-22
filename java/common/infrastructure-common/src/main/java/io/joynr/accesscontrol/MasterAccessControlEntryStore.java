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
import java.util.Collection;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.googlecode.cqengine.ConcurrentIndexedCollection;
import com.googlecode.cqengine.IndexedCollection;
import com.googlecode.cqengine.index.hash.HashIndex;
import com.googlecode.cqengine.resultset.ResultSet;

import io.joynr.accesscontrol.datatype.MasterAccessControlEntryDB;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterAccessControlEntry;
import joynr.infrastructure.DacTypes.Role;

public class MasterAccessControlEntryStore {
    private static final String WILDCARD = "*";
    private static final Logger logger = LoggerFactory.getLogger(MasterAccessControlEntryStore.class);

    private DomainRoleEntryStore domainRoleEntryStore;
    private IndexedCollection<MasterAccessControlEntryDB> masterAclDB = new ConcurrentIndexedCollection<MasterAccessControlEntryDB>();

    public MasterAccessControlEntryStore(DomainAccessControlProvisioning domainAccessControlProvisioning,
                                         DomainRoleEntryStore domainRoleEntryStore) {
        this.domainRoleEntryStore = domainRoleEntryStore;

        masterAclDB.addIndex(HashIndex.onAttribute(MasterAccessControlEntryDB.UID));
        masterAclDB.addIndex(HashIndex.onAttribute(MasterAccessControlEntryDB.INTERFACENAME));
        masterAclDB.addIndex(HashIndex.onAttribute(MasterAccessControlEntryDB.DOMAIN));
        masterAclDB.addIndex(HashIndex.onAttribute(MasterAccessControlEntryDB.OPERATION));

        Collection<MasterAccessControlEntry> masterAccessControlEntries = domainAccessControlProvisioning.getMasterAccessControlEntries();
        for (MasterAccessControlEntry provisionedMasterAccessControlEntry : masterAccessControlEntries) {
            updateControlEntry(provisionedMasterAccessControlEntry);
        }
    }

    public List<MasterAccessControlEntry> getControlEntries(String uid) {
        List<MasterAccessControlEntry> controlEntries = new ArrayList<MasterAccessControlEntry>();
        com.googlecode.cqengine.query.Query<MasterAccessControlEntryDB> cqQuery = equal(MasterAccessControlEntryDB.UID,
                                                                                        uid);
        masterAclDB.retrieve(cqQuery).forEach(result -> controlEntries.add(result.getMasterAccessControlEntry()));
        if (controlEntries.isEmpty()) {
            com.googlecode.cqengine.query.Query<MasterAccessControlEntryDB> cqQueryWildcard = equal(MasterAccessControlEntryDB.UID,
                                                                                                    WILDCARD);
            masterAclDB.retrieve(cqQueryWildcard)
                       .forEach(result -> controlEntries.add(result.getMasterAccessControlEntry()));
        }
        return controlEntries;
    }

    public List<MasterAccessControlEntry> getEditableAces(String uid, Role role) {
        List<MasterAccessControlEntry> controlEntries = new ArrayList<MasterAccessControlEntry>();
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
            com.googlecode.cqengine.query.Query<MasterAccessControlEntryDB> cqQuery = and(equal(MasterAccessControlEntryDB.UID,
                                                                                                uid),
                                                                                          equal(MasterAccessControlEntryDB.DOMAIN,
                                                                                                domain));
            masterAclDB.retrieve(cqQuery).forEach(result -> controlEntries.add(result.getMasterAccessControlEntry()));
        }

        return controlEntries;
    }

    public List<MasterAccessControlEntry> getControlEntries(String domain, String interfaceName) {
        List<MasterAccessControlEntry> controlEntries = new ArrayList<MasterAccessControlEntry>();
        com.googlecode.cqengine.query.Query<MasterAccessControlEntryDB> cqQuery = and(equal(MasterAccessControlEntryDB.DOMAIN,
                                                                                            domain),
                                                                                      equal(MasterAccessControlEntryDB.INTERFACENAME,
                                                                                            interfaceName));
        masterAclDB.retrieve(cqQuery).forEach(result -> controlEntries.add(result.getMasterAccessControlEntry()));
        return controlEntries;
    }

    public List<MasterAccessControlEntry> getControlEntries(String uid, String domain, String interfaceName) {
        List<MasterAccessControlEntry> controlEntries = new ArrayList<MasterAccessControlEntry>();
        com.googlecode.cqengine.query.Query<MasterAccessControlEntryDB> cqQuery = and(or(equal(MasterAccessControlEntryDB.UID,
                                                                                               uid),
                                                                                         equal(MasterAccessControlEntryDB.UID,
                                                                                               WILDCARD)),
                                                                                      equal(MasterAccessControlEntryDB.DOMAIN,
                                                                                            domain),
                                                                                      equal(MasterAccessControlEntryDB.INTERFACENAME,
                                                                                            interfaceName));
        ResultSet<MasterAccessControlEntryDB> cqResult = masterAclDB.retrieve(cqQuery,
                                                                              queryOptions(orderBy(descending(MasterAccessControlEntryDB.UID))));
        String currentUid = null;
        for (MasterAccessControlEntryDB result : cqResult) {
            if (currentUid == null) {
                currentUid = result.getUid();
            } else if (!currentUid.equals(result.getUid())) {
                break;
            }

            controlEntries.add(result.getMasterAccessControlEntry());
        }

        return controlEntries;
    }

    public MasterAccessControlEntry getControlEntry(String uid, String domain, String interfaceName, String operation) {

        MasterAccessControlEntry masterAce = null;
        com.googlecode.cqengine.query.Query<MasterAccessControlEntryDB> cqQuery = and(or(equal(MasterAccessControlEntryDB.UID,
                                                                                               uid),
                                                                                         equal(MasterAccessControlEntryDB.UID,
                                                                                               WILDCARD)),
                                                                                      equal(MasterAccessControlEntryDB.DOMAIN,
                                                                                            domain),
                                                                                      equal(MasterAccessControlEntryDB.INTERFACENAME,
                                                                                            interfaceName),
                                                                                      equal(MasterAccessControlEntryDB.OPERATION,
                                                                                            operation));
        ResultSet<MasterAccessControlEntryDB> cqResult = masterAclDB.retrieve(cqQuery);
        if (!cqResult.isEmpty()) {
            masterAce = cqResult.uniqueResult().getMasterAccessControlEntry();
        } else {
            com.googlecode.cqengine.query.Query<MasterAccessControlEntryDB> cqQueryWildcard = and(or(equal(MasterAccessControlEntryDB.UID,
                                                                                                           uid),
                                                                                                     equal(MasterAccessControlEntryDB.UID,
                                                                                                           WILDCARD)),
                                                                                                  equal(MasterAccessControlEntryDB.DOMAIN,
                                                                                                        domain),
                                                                                                  equal(MasterAccessControlEntryDB.INTERFACENAME,
                                                                                                        interfaceName),
                                                                                                  equal(MasterAccessControlEntryDB.OPERATION,
                                                                                                        WILDCARD));
            cqResult = masterAclDB.retrieve(cqQueryWildcard);
            if (!cqResult.isEmpty()) {
                masterAce = cqResult.uniqueResult().getMasterAccessControlEntry();
            }
        }

        return masterAce;
    }

    public Boolean updateControlEntry(MasterAccessControlEntry updatedMasterAce) {
        com.googlecode.cqengine.query.Query<MasterAccessControlEntryDB> cqQuery = and(or(equal(MasterAccessControlEntryDB.UID,
                                                                                               updatedMasterAce.getUid()),
                                                                                         equal(MasterAccessControlEntryDB.UID,
                                                                                               WILDCARD)),
                                                                                      equal(MasterAccessControlEntryDB.DOMAIN,
                                                                                            updatedMasterAce.getDomain()),
                                                                                      equal(MasterAccessControlEntryDB.INTERFACENAME,
                                                                                            updatedMasterAce.getInterfaceName()),
                                                                                      equal(MasterAccessControlEntryDB.OPERATION,
                                                                                            updatedMasterAce.getOperation()));
        ResultSet<MasterAccessControlEntryDB> cqResult = masterAclDB.retrieve(cqQuery,
                                                                              queryOptions(orderBy(descending(MasterAccessControlEntryDB.UID))));
        if (!cqResult.isEmpty()) {
            masterAclDB.remove(cqResult.uniqueResult());
        }
        return masterAclDB.add(new MasterAccessControlEntryDB(updatedMasterAce));
    }

    public boolean removeControlEntry(String uid, String domain, String interfaceName, String operation) {
        boolean removeResult = false;
        try {
            com.googlecode.cqengine.query.Query<MasterAccessControlEntryDB> cqQuery = and(equal(MasterAccessControlEntryDB.UID,
                                                                                                uid),
                                                                                          equal(MasterAccessControlEntryDB.DOMAIN,
                                                                                                domain),
                                                                                          equal(MasterAccessControlEntryDB.INTERFACENAME,
                                                                                                interfaceName),
                                                                                          equal(MasterAccessControlEntryDB.OPERATION,
                                                                                                operation));
            removeResult = masterAclDB.update(masterAclDB.retrieve(cqQuery),
                                              new ArrayList<MasterAccessControlEntryDB>());
        } catch (IllegalArgumentException | IllegalStateException e) {
            logger.error("Remove masterAcl failed.", e);
        }

        return removeResult;
    }
}
