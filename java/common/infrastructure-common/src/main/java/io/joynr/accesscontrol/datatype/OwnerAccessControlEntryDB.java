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

import joynr.infrastructure.DacTypes.OwnerAccessControlEntry;

public class OwnerAccessControlEntryDB extends OwnerAccessControlEntry {

    public static final com.googlecode.cqengine.attribute.Attribute<OwnerAccessControlEntryDB, String> UID = new SimpleAttribute<OwnerAccessControlEntryDB, String>("uid") {
        public String getValue(OwnerAccessControlEntryDB ownerAccessControlEntryDB, QueryOptions queryOptions) {
            return ownerAccessControlEntryDB.getUid();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<OwnerAccessControlEntryDB, String> DOMAIN = new SimpleAttribute<OwnerAccessControlEntryDB, String>("domain") {
        public String getValue(OwnerAccessControlEntryDB ownerAccessControlEntryDB, QueryOptions queryOptions) {
            return ownerAccessControlEntryDB.getDomain();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<OwnerAccessControlEntryDB, String> INTERFACENAME = new SimpleAttribute<OwnerAccessControlEntryDB, String>("interfaceName") {
        public String getValue(OwnerAccessControlEntryDB ownerAccessControlEntryDB, QueryOptions queryOptions) {
            return ownerAccessControlEntryDB.getInterfaceName();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<OwnerAccessControlEntryDB, String> OPERATION = new SimpleAttribute<OwnerAccessControlEntryDB, String>("operation") {
        public String getValue(OwnerAccessControlEntryDB ownerAccessControlEntryDB, QueryOptions queryOptions) {
            return ownerAccessControlEntryDB.getOperation();
        }
    };

    public OwnerAccessControlEntryDB() {
        super();
    }

    public OwnerAccessControlEntryDB(OwnerAccessControlEntry ownerRegistrationControlEntryObj) {
        super(ownerRegistrationControlEntryObj);
    }

    public OwnerAccessControlEntry getOwnerAccessControlEntry() {
        return new OwnerAccessControlEntry(this);
    }
}