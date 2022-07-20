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

import io.joynr.accesscontrol.datatype.MasterRegistrationControlEntryDB;
import joynr.infrastructure.DacTypes.DomainRoleEntry;
import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;
import joynr.infrastructure.DacTypes.Role;

public class MasterRegistrationControlEntryStore {
    private static final String WILDCARD = "*";
    private static final Logger logger = LoggerFactory.getLogger(MasterRegistrationControlEntryStore.class);

    private DomainRoleEntryStore domainRoleEntryStore;
    private IndexedCollection<MasterRegistrationControlEntryDB> masterRclDB = new ConcurrentIndexedCollection<MasterRegistrationControlEntryDB>();

    public MasterRegistrationControlEntryStore(DomainRoleEntryStore domainRoleEntryStore) {
        this.domainRoleEntryStore = domainRoleEntryStore;

        masterRclDB.addIndex(HashIndex.onAttribute(MasterRegistrationControlEntryDB.UID));
        masterRclDB.addIndex(HashIndex.onAttribute(MasterRegistrationControlEntryDB.INTERFACENAME));
        masterRclDB.addIndex(HashIndex.onAttribute(MasterRegistrationControlEntryDB.DOMAIN));
    }

    public List<MasterRegistrationControlEntry> getControlEntries(String uid) {
        List<MasterRegistrationControlEntry> controlEntries = new ArrayList<MasterRegistrationControlEntry>();
        com.googlecode.cqengine.query.Query<MasterRegistrationControlEntryDB> cqQuery = equal(MasterRegistrationControlEntryDB.UID,
                                                                                              uid);
        masterRclDB.retrieve(cqQuery).forEach(result -> controlEntries.add(result.getMasterRegistrationControlEntry()));
        if (controlEntries.isEmpty()) {
            com.googlecode.cqengine.query.Query<MasterRegistrationControlEntryDB> cqQueryWildcard = equal(MasterRegistrationControlEntryDB.UID,
                                                                                                          WILDCARD);
            masterRclDB.retrieve(cqQueryWildcard)
                       .forEach(result -> controlEntries.add(result.getMasterRegistrationControlEntry()));
        }
        return controlEntries;
    }

    public List<MasterRegistrationControlEntry> getEditableAces(String uid, Role role) {
        List<MasterRegistrationControlEntry> controlEntries = new ArrayList<MasterRegistrationControlEntry>();
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
            com.googlecode.cqengine.query.Query<MasterRegistrationControlEntryDB> cqQuery = and(equal(MasterRegistrationControlEntryDB.UID,
                                                                                                      uid),
                                                                                                equal(MasterRegistrationControlEntryDB.DOMAIN,
                                                                                                      domain));
            masterRclDB.retrieve(cqQuery)
                       .forEach(result -> controlEntries.add(result.getMasterRegistrationControlEntry()));
        }

        return controlEntries;
    }

    public List<MasterRegistrationControlEntry> getControlEntries(String domain, String interfaceName) {
        List<MasterRegistrationControlEntry> controlEntries = new ArrayList<MasterRegistrationControlEntry>();
        com.googlecode.cqengine.query.Query<MasterRegistrationControlEntryDB> cqQuery = and(equal(MasterRegistrationControlEntryDB.DOMAIN,
                                                                                                  domain),
                                                                                            equal(MasterRegistrationControlEntryDB.INTERFACENAME,
                                                                                                  interfaceName));
        masterRclDB.retrieve(cqQuery).forEach(result -> controlEntries.add(result.getMasterRegistrationControlEntry()));
        return controlEntries;
    }

    public MasterRegistrationControlEntry getControlEntry(String uid, String domain, String interfaceName) {

        MasterRegistrationControlEntry masterRce = null;
        com.googlecode.cqengine.query.Query<MasterRegistrationControlEntryDB> cqQuery = and(or(equal(MasterRegistrationControlEntryDB.UID,
                                                                                                     uid),
                                                                                               equal(MasterRegistrationControlEntryDB.UID,
                                                                                                     WILDCARD)),
                                                                                            equal(MasterRegistrationControlEntryDB.DOMAIN,
                                                                                                  domain),
                                                                                            equal(MasterRegistrationControlEntryDB.INTERFACENAME,
                                                                                                  interfaceName));
        ResultSet<MasterRegistrationControlEntryDB> cqResult = masterRclDB.retrieve(cqQuery,
                                                                                    queryOptions(orderBy(descending(MasterRegistrationControlEntryDB.UID))));
        if (!cqResult.isEmpty()) {
            masterRce = cqResult.uniqueResult().getMasterRegistrationControlEntry();
        }

        return masterRce;
    }

    public Boolean updateControlEntry(MasterRegistrationControlEntry updatedMasterAce) {
        com.googlecode.cqengine.query.Query<MasterRegistrationControlEntryDB> cqQuery = and(or(equal(MasterRegistrationControlEntryDB.UID,
                                                                                                     updatedMasterAce.getUid()),
                                                                                               equal(MasterRegistrationControlEntryDB.UID,
                                                                                                     WILDCARD)),
                                                                                            equal(MasterRegistrationControlEntryDB.DOMAIN,
                                                                                                  updatedMasterAce.getDomain()),
                                                                                            equal(MasterRegistrationControlEntryDB.INTERFACENAME,
                                                                                                  updatedMasterAce.getInterfaceName()));
        ResultSet<MasterRegistrationControlEntryDB> cqResult = masterRclDB.retrieve(cqQuery,
                                                                                    queryOptions(orderBy(descending(MasterRegistrationControlEntryDB.UID))));
        if (!cqResult.isEmpty()) {
            masterRclDB.remove(cqResult.uniqueResult());
        }
        return masterRclDB.add(new MasterRegistrationControlEntryDB(updatedMasterAce));
    }

    public boolean removeControlEntry(String uid, String domain, String interfaceName) {
        boolean removeResult = false;
        try {
            com.googlecode.cqengine.query.Query<MasterRegistrationControlEntryDB> cqQuery = and(equal(MasterRegistrationControlEntryDB.UID,
                                                                                                      uid),
                                                                                                equal(MasterRegistrationControlEntryDB.DOMAIN,
                                                                                                      domain),
                                                                                                equal(MasterRegistrationControlEntryDB.INTERFACENAME,
                                                                                                      interfaceName));
            removeResult = masterRclDB.update(masterRclDB.retrieve(cqQuery),
                                              new ArrayList<MasterRegistrationControlEntryDB>());
        } catch (IllegalArgumentException | IllegalStateException e) {
            logger.error("Remove masterAcl failed.", e);
        }

        return removeResult;
    }
}
