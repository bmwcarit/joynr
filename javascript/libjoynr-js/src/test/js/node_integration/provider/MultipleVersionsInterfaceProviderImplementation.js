/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

function get(attributeName) {
    return this[`_${attributeName}`];
}

function set(attributeName, value) {
    this[`_${attributeName}`] = value;
}

function ProviderImplementation() {
    this._uInt8Attribute1 = undefined;
    this._uInt8Attribute2 = undefined;
    this.uInt8Attribute1 = {
        get: async () => {
            return await get.apply(this, ["uInt8Attribute1"]);
        },
        set: async value => {
            await set.apply(this, ["uInt8Attribute1", value]);
        }
    };
    this.uInt8Attribute2 = {
        get: async () => {
            return await get.apply(this, ["uInt8Attribute2"]);
        },
        set: async value => {
            await set.apply(this, ["uInt8Attribute2", value]);
        }
    };
    this.getVersionedStruct = () => {
        // TODO implement
        return undefined;
    };
    this.getAnonymousVersionedStruct = () => {
        // TODO implement
        return undefined;
    };
    this.getInterfaceVersionedStruct = () => {
        // TODO implement
        return undefined;
    };
}

ProviderImplementation.prototype.getTrue = () => {
    return true;
};

module.exports = ProviderImplementation;
