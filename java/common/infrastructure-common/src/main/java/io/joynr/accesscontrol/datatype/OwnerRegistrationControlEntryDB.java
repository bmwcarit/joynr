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

import joynr.infrastructure.DacTypes.OwnerRegistrationControlEntry;

public class OwnerRegistrationControlEntryDB extends OwnerRegistrationControlEntry {

    public static final com.googlecode.cqengine.attribute.Attribute<OwnerRegistrationControlEntryDB, String> UID = new SimpleAttribute<OwnerRegistrationControlEntryDB, String>("uid") {
        public String getValue(OwnerRegistrationControlEntryDB ownerRegistrationControlEntryObj,
                               QueryOptions queryOptions) {
            return ownerRegistrationControlEntryObj.getUid();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<OwnerRegistrationControlEntryDB, String> DOMAIN = new SimpleAttribute<OwnerRegistrationControlEntryDB, String>("domain") {
        public String getValue(OwnerRegistrationControlEntryDB ownerRegistrationControlEntryObj,
                               QueryOptions queryOptions) {
            return ownerRegistrationControlEntryObj.getDomain();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<OwnerRegistrationControlEntryDB, String> INTERFACENAME = new SimpleAttribute<OwnerRegistrationControlEntryDB, String>("interfaceName") {
        public String getValue(OwnerRegistrationControlEntryDB ownerRegistrationControlEntryObj,
                               QueryOptions queryOptions) {
            return ownerRegistrationControlEntryObj.getInterfaceName();
        }
    };

    public OwnerRegistrationControlEntryDB() {
        super();
    }

    public OwnerRegistrationControlEntryDB(OwnerRegistrationControlEntry ownerRegistrationControlEntryObj) {
        super(ownerRegistrationControlEntryObj);
    }

    public OwnerRegistrationControlEntry getOwnerRegistrationControlEntry() {
        return new OwnerRegistrationControlEntry(this);
    }
}