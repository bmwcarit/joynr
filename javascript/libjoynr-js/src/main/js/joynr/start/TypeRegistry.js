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
const Promise = require("../../global/Promise");
const util = require("../util/UtilInternal");

/**
 * The <code>TypeRegistry</code> contains a mapping of type names (which are sent on the wire
 * through joynr) to the corresponding
 * JavaScript type constructor.
 *
 * This class is used during deserialization, ie mapping a received request/response to the
 * corresponding JavaScript call with the
 * correct parameters.
 *
 * @name TypeRegistry
 * @constructor
 */
function TypeRegistry() {
    const registry = {};
    const enumRegistry = {};
    const registryPromise = {};

    /**
     * Adds a typeName to constructor entry in the type registry.
     *
     * @name TypeRegistry#addType
     * @function
     *
     * @param {String}
     *            joynrTypeName - the joynr type name that is sent on the wire.
     * @param {Function}
     *            typeConstructor - the corresponding JavaScript constructor for this type.
     * @param {boolean}
     *            isEnum - optional flag if the added type is an enumeration type
     */
    this.addType = function addType(joynrTypeName, typeConstructor, isEnum) {
        if (isEnum) {
            enumRegistry[joynrTypeName] = typeConstructor;
        }
        registry[joynrTypeName] = typeConstructor;
        if (registryPromise[joynrTypeName]) {
            registryPromise[joynrTypeName].resolve(typeConstructor);
        }
        return this;
    };

    /**
     * Detects if asked joynr type is an enumeration type
     *
     * @name TypeRegistry#isEnumType
     * @function
     *
     * @param {String}
     *            joynrTypeName - the joynr type name that is sent/received on the wire.
     * @returns {boolean} true if the asked joynr type is an enumeration type
     */
    this.isEnumType = function isEnumType(joynrTypeName) {
        return enumRegistry[joynrTypeName] !== undefined;
    };

    /**
     * Retrieves the constructor for a given type.
     *
     * @name TypeRegistry#getConstructor
     * @function
     *
     * @param {String}
     *            joynrTypeName - the joynr type name that is sent/received on the wire.
     * @returns {Function} the constructor function for the specified type name
     */
    this.getConstructor = function getConstructor(joynrTypeName) {
        return registry[joynrTypeName];
    };

    /**
     * Returns an A+ promise, which is resolved once the type has been added to the typeRegistry
     *
     * @name TypeRegistry#getTypeRegisteredPromise
     * @function
     *
     * @param {String}
     *            joynrTypeName - the joynr type name that is to be resolved
     * @param {Number}
     *            timeout - if timeout exceed before required joynr type is registered,
     *            the returning promise will be rejected
     * @returns {Promise} an A+ promise object
     */
    this.getTypeRegisteredPromise = function getTypeRegisteredPromise(joynrTypeName, timeout) {
        if (registry[joynrTypeName]) {
            return Promise.resolve();
        }
        registryPromise[joynrTypeName] = util.createDeferred();
        if (timeout && timeout > 0) {
            registryPromise[joynrTypeName].timeoutTimer = setTimeout(() => {
                if (registryPromise[joynrTypeName].pending) {
                    registryPromise[joynrTypeName].reject(
                        new Error(
                            "joynr/start/TypeRegistry: " +
                                joynrTypeName +
                                " is not registered in the joynr type registry"
                        )
                    );
                }
            }, timeout);
        }
        return registryPromise[joynrTypeName].promise;
    };

    /**
     * Shutdown the type registry
     *
     * @function
     * @name TypeRegistry#shutdown
     */
    this.shutdown = function shutdown() {
        let typeName;
        for (typeName in registryPromise) {
            if (registryPromise.hasOwnProperty(typeName)) {
                if (registryPromise[typeName].timeoutTimer !== undefined) {
                    clearTimeout(registryPromise[typeName].timeoutTimer);
                }
            }
        }
    };
}

module.exports = TypeRegistry;
