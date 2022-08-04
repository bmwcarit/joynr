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

import ApplicationException from "../exceptions/ApplicationException";
import DiscoveryException from "../exceptions/DiscoveryException";
import IllegalAccessException from "../exceptions/IllegalAccessException";
import JoynrException from "../exceptions/JoynrException";
import JoynrRuntimeException from "../exceptions/JoynrRuntimeException";
import MethodInvocationException from "../exceptions/MethodInvocationException";
import NoCompatibleProviderFoundException from "../exceptions/NoCompatibleProviderFoundException";
import ProviderRuntimeException from "../exceptions/ProviderRuntimeException";
import PublicationMissedException from "../exceptions/PublicationMissedException";
import SubscriptionException from "../exceptions/SubscriptionException";
import JoynrEnum = require("../types/JoynrEnum");

interface TypeConstructor {
    new (...args: any[]): any;
    _typeName: string;
}

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
    public constructor() {
        this.addType(ApplicationException)
            .addType(DiscoveryException)
            .addType(IllegalAccessException)
            .addType(JoynrException)
            .addType(JoynrRuntimeException)
            .addType(MethodInvocationException)
            .addType(NoCompatibleProviderFoundException)
            .addType(ProviderRuntimeException)
            .addType(PublicationMissedException)
            .addType(SubscriptionException);
    }

    /**
     * Adds a typeName to constructor entry in the type registry.
     *
     * @param typeConstructor - the corresponding JavaScript constructor for this type.
     * @returns typeRegistry for chained calls.
     */
    public addType(typeConstructor: TypeConstructor): TypeRegistry {
        const typeName = typeConstructor._typeName;
        if (typeName === undefined) {
            throw new Error(`Trying to add invalid type: ${typeConstructor}`);
        }
        if (typeConstructor.prototype instanceof JoynrEnum) {
            this.enumRegistry[typeName] = typeConstructor;
        }
        this.registry[typeName] = typeConstructor;
        if (this.registryPromise[typeName]) {
            this.registryPromise[typeName].resolve(typeConstructor);
            clearTimeout(this.registryPromise[typeName].timeoutTimer);
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
    public getConstructor(joynrTypeName: string): any {
        return this.registry[joynrTypeName];
    }

    /**
     * Shutdown the type registry
     */
    public shutdown(): void {
        let typeName;
        for (typeName in this.registryPromise) {
            if (Object.prototype.hasOwnProperty.call(this.registryPromise, typeName)) {
                if (this.registryPromise[typeName].timeoutTimer !== undefined) {
                    clearTimeout(this.registryPromise[typeName].timeoutTimer);
                }
            }
        }
    }
}

export = TypeRegistry;
