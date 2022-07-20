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

import joynr.infrastructure.DacTypes.MasterRegistrationControlEntry;

public class MasterRegistrationControlEntryDB extends MasterRegistrationControlEntry {

    public static final com.googlecode.cqengine.attribute.Attribute<MasterRegistrationControlEntryDB, String> UID = new SimpleAttribute<MasterRegistrationControlEntryDB, String>("uid") {
        public String getValue(MasterRegistrationControlEntryDB masterRegistrationControlEntryDB,
                               QueryOptions queryOptions) {
            return masterRegistrationControlEntryDB.getUid();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<MasterRegistrationControlEntryDB, String> DOMAIN = new SimpleAttribute<MasterRegistrationControlEntryDB, String>("domain") {
        public String getValue(MasterRegistrationControlEntryDB masterRegistrationControlEntryDB,
                               QueryOptions queryOptions) {
            return masterRegistrationControlEntryDB.getDomain();
        }
    };
    public static final com.googlecode.cqengine.attribute.Attribute<MasterRegistrationControlEntryDB, String> INTERFACENAME = new SimpleAttribute<MasterRegistrationControlEntryDB, String>("interfaceName") {
        public String getValue(MasterRegistrationControlEntryDB masterRegistrationControlEntryDB,
                               QueryOptions queryOptions) {
            return masterRegistrationControlEntryDB.getInterfaceName();
        }
    };

    public MasterRegistrationControlEntryDB() {
        super();
    }

    public MasterRegistrationControlEntryDB(MasterRegistrationControlEntry masterRegistrationControlEntryObj) {
        super(masterRegistrationControlEntryObj);
    }

    public MasterRegistrationControlEntry getMasterRegistrationControlEntry() {
        return new MasterRegistrationControlEntry(this);
    }
}