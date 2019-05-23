/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
/* istanbul ignore file */

import TypeRegistry from "../start/TypeRegistry";

let instance: TypeRegistry;

class TypeRegistrySingleton {
    /**
     * forward addType call to the TypeRegistry instance
     * @function TypeRegistrySingleton#addType
     */
    public static addType = TypeRegistrySingleton.getInstance().addType;

    /**
     * A singleton Implementation for the Type Registry.
     *
     * Cannot be instantiated.
     *
     * @constructor
     * @throws {Error} Can not instantiate this type
     */
    public constructor() {
        throw new Error("Can not instantiate this type");
    }

    /**
     * @returns the TypeRegistry singleton instance
     */
    public static getInstance(): TypeRegistry {
        if (instance === undefined) {
            instance = new TypeRegistry();
        }
        return instance;
    }
}

export = TypeRegistrySingleton;
