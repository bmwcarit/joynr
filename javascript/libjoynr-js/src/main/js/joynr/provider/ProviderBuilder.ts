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

import TypeRegistry from "../start/TypeRegistry";

interface JoynrProvider {
    new (...args: any[]): any;
    getUsedJoynrtypes(): any[];
}

class ProviderBuilder {
    /**
     * TypeRegistry used to register the joynr types.
     */
    private typeRegistry: TypeRegistry;

    public constructor({ typeRegistry }: { typeRegistry: TypeRegistry }) {
        this.typeRegistry = typeRegistry;
    }

    /**
     * @param ProviderConstructor - the constructor function of the generated Provider that
     *            creates a new provider instance
     * @param implementation - an object containing the same fields and public functions as
     *            exposed int he provider that implements the actual functionaltiy of the
     *            provider
     * @returns a provider of the given type
     * @throws {Error} if correct implementation was not provided
     */
    public build<T extends JoynrProvider, Impl>(ProviderConstructor: T, implementation: Impl): any {
        ProviderConstructor.getUsedJoynrtypes().forEach((type: any) => {
            this.typeRegistry.addType(type);
        });
        return new ProviderConstructor(implementation);
    }
}

export = ProviderBuilder;
