/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
var TypeRegistry = require("../start/TypeRegistry");
var instance;

/**
 * A singleton Implementation for the Type Registry.
 *
 * Cannot be instantiated.
 *
 * @name TypeRegistrySingleton
 * @constructor
 * @throws {Error} Can not instantiate this type
 */
function TypeRegistrySingleton() {
    throw new Error("Can not instantiate this type");
}

/**
 * @function TypeRegistrySingleton#getInstance
 * @returns {TypeRegistry} the TypeRegistry singleton instance
 */
TypeRegistrySingleton.getInstance = function() {
    if (instance === undefined) {
        instance = new TypeRegistry();
    }
    return instance;
};

/**
 * forward addType call to the TypeRegistry instance
 * @function TypeRegistrySingleton#addType
 */
TypeRegistrySingleton.addType = TypeRegistrySingleton.getInstance().addType;

module.exports = TypeRegistrySingleton;
