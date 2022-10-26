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
package io.joynr.accesscontrol.datatype;

import com.googlecode.cqengine.attribute.SimpleAttribute;
import com.googlecode.cqengine.query.option.QueryOptions;

import joynr.infrastructure.DacTypes.MasterAccessControlEntry;

public class MasterAccessControlEntryDB extends MasterAccessControlEntry {

    public static final String WILDCARD = "*";

    public static final com.googlecode.cqengine.attribute.Attribute<MasterAccessControlEntryDB, String> UID = new SimpleAttribute<MasterAccessControlEntryDB, String>("uid") {
        public String getValue(MasterAccessControlEntryDB masterAccessControlEntryDB, QueryOptions queryOptions) {
            return masterAccessControlEntryDB.getUid();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<MasterAccessControlEntryDB, String> DOMAIN = new SimpleAttribute<MasterAccessControlEntryDB, String>("domain") {
        public String getValue(MasterAccessControlEntryDB masterAccessControlEntryDB, QueryOptions queryOptions) {
            return masterAccessControlEntryDB.getDomain();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<MasterAccessControlEntryDB, String> INTERFACENAME = new SimpleAttribute<MasterAccessControlEntryDB, String>("interfaceName") {
        public String getValue(MasterAccessControlEntryDB masterAccessControlEntryDB, QueryOptions queryOptions) {
            return masterAccessControlEntryDB.getInterfaceName();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<MasterAccessControlEntryDB, String> OPERATION = new SimpleAttribute<MasterAccessControlEntryDB, String>("operation") {
        public String getValue(MasterAccessControlEntryDB masterAccessControlEntryDB, QueryOptions queryOptions) {
            return masterAccessControlEntryDB.getOperation();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<MasterAccessControlEntryDB, Boolean> WILDCARDDOMAIN = new SimpleAttribute<MasterAccessControlEntryDB, Boolean>("wildcardDomain") {
        public Boolean getValue(MasterAccessControlEntryDB masterAccessControlEntryDB, QueryOptions queryOptions) {
            return masterAccessControlEntryDB.getDomain().endsWith(WILDCARD);
        }
    };

    public MasterAccessControlEntryDB() {
        super();
    }

    public MasterAccessControlEntryDB(MasterAccessControlEntry masterRegistrationControlEntryObj) {
        super(masterRegistrationControlEntryObj);
    }

    public MasterAccessControlEntry getMasterAccessControlEntry() {
        return new MasterAccessControlEntry(this);
    }
}