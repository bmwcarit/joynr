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

class ProviderImplementation {
    private _uInt8Attribute1: any;
    private _uInt8Attribute2: any;
    public uInt8Attribute1 = {
        get: async () => {
            return this._uInt8Attribute1;
        },
        set: async (value: any) => {
            this._uInt8Attribute1 = value;
        }
    };
    public uInt8Attribute2 = {
        get: async () => {
            return this._uInt8Attribute2;
        },
        set: async (value: any) => {
            this._uInt8Attribute2 = value;
        }
    };
    public getVersionedStruct = (): any => {
        // TODO implement
    };
    public getAnonymousVersionedStruct = (): any => {
        // TODO implement
    };
    public getInterfaceVersionedStruct = (): any => {
        // TODO implement
    };

    public getTrue(): boolean {
        return true;
    }
}

export = ProviderImplementation;
