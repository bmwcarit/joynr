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

import * as UtilInternal from "../util/UtilInternal";

class TypeRegistry {
    private registryPromise: Record<string, any> = {};
    private enumRegistry: Record<string, any> = {};
    private registry: Record<string, any> = {};
    /**
     * The <code>TypeRegistry</code> contains a mapping of type names (which are sent on the wire
     * through joynr) to the corresponding
     * JavaScript type constructor.
     *
     * This class is used during deserialization, ie mapping a received request/response to the
     * corresponding JavaScript call with the
     * correct parameters.
     *
     * @constructor
     */
    public constructor() {}

    /**
     * Adds a typeName to constructor entry in the type registry.
     *
     * @param joynrTypeName - the joynr type name that is sent on the wire.
     * @param typeConstructor - the corresponding JavaScript constructor for this type.
     * @param isEnum - optional flag if the added type is an enumeration type
     * @returns typeRegistry for chained calls.
     */
    public addType(joynrTypeName: string, typeConstructor: Function, isEnum?: boolean): TypeRegistry {
        if (isEnum) {
            this.enumRegistry[joynrTypeName] = typeConstructor;
        }
        this.registry[joynrTypeName] = typeConstructor;
        if (this.registryPromise[joynrTypeName]) {
            this.registryPromise[joynrTypeName].resolve(typeConstructor);
            clearTimeout(this.registryPromise[joynrTypeName].timeoutTimer);
        }
        return this;
    }

    /**
     * Detects if asked joynr type is an enumeration type
     *
     * @param joynrTypeName - the joynr type name that is sent/received on the wire.
     * @returns true if the asked joynr type is an enumeration type
     */
    public isEnumType(joynrTypeName: string): boolean {
        return this.enumRegistry[joynrTypeName] !== undefined;
    }

    /**
     * Retrieves the constructor for a given type.
     *
     * @param joynrTypeName - the joynr type name that is sent/received on the wire.
     * @returns the constructor function for the specified type name
     */
    public getConstructor(joynrTypeName: string): Function {
        return this.registry[joynrTypeName];
    }

    /**
     * Returns an A+ promise, which is resolved once the type has been added to the typeRegistry
     *
     * @param joynrTypeName - the joynr type name that is to be resolved
     * @param timeout - if timeout exceed before required joynr type is registered,
     *            the returning promise will be rejected
     * @returns an A+ promise object
     */
    public getTypeRegisteredPromise(joynrTypeName: string, timeout: number): Promise<any> {
        if (this.registry[joynrTypeName]) {
            return Promise.resolve();
        }
        this.registryPromise[joynrTypeName] = UtilInternal.createDeferred();
        if (timeout && timeout > 0) {
            this.registryPromise[joynrTypeName].timeoutTimer = setTimeout((): void => {
                this.registryPromise[joynrTypeName].reject(
                    new Error(`joynr/start/TypeRegistry: ${joynrTypeName} is not registered in the joynr type registry`)
                );
            }, timeout);
        }
        return this.registryPromise[joynrTypeName].promise;
    }

    /**
     * Shutdown the type registry
     */
    public shutdown(): void {
        let typeName;
        for (typeName in this.registryPromise) {
            if (this.registryPromise.hasOwnProperty(typeName)) {
                if (this.registryPromise[typeName].timeoutTimer !== undefined) {
                    clearTimeout(this.registryPromise[typeName].timeoutTimer);
                }
            }
        }
    }
}

export = TypeRegistry;
