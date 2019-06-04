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

import RadioProvider from "../../../generated/joynr/vehicle/RadioProvider";
import ProviderBuilder from "../../../../main/js/joynr/provider/ProviderBuilder";
import ProviderOperation from "../../../../main/js/joynr/provider/ProviderOperation";
import TypeRegistrySingleton from "../../../../main/js/joynr/types/TypeRegistrySingleton";
const typeRegistry = TypeRegistrySingleton.getInstance();

describe("libjoynr-js.joynr.provider.ProviderBuilder", () => {
    let providerBuilder: ProviderBuilder;
    let implementation: any;

    beforeEach(() => {
        providerBuilder = new ProviderBuilder({ typeRegistry });
        implementation = {
            isOn: {
                value: false,
                get() {
                    return this.value;
                },
                set(newValue: any) {
                    this.value = newValue;
                }
            },
            numberOfStations: {
                value: 0,
                get() {
                    return this.value;
                },
                set(newValue: any) {
                    this.value = newValue;
                }
            },
            mixedSubscriptions: {
                value: "testvalue",
                get() {
                    return this.value;
                },
                set(newValue: any) {
                    this.value = newValue;
                }
            },
            attrProvidedImpl: {
                value: "testValue2",
                get() {
                    return this.value;
                },
                set(newValue: any) {
                    this.value = newValue;
                }
            },
            attributeTestingProviderInterface: {
                get() {
                    return undefined;
                }
            },
            addFavoriteStation: jest.fn(),
            weakSignal: jest.fn(),
            methodProvidedImpl: jest.fn()
        };
    });

    it("is defined and of correct type", () => {
        expect(providerBuilder).toBeDefined();
        expect(typeof providerBuilder.build === "function").toBe(true);
    });

    it("returns a provider of the given type", () => {
        const radioProvider = providerBuilder.build(RadioProvider, implementation);
        expect(radioProvider).toBeDefined();
        expect(radioProvider).not.toBeNull();
        expect(radioProvider instanceof RadioProvider).toBeTruthy();
        expect(radioProvider.interfaceName).toBeDefined();
        expect(radioProvider.isOn).toBeDefined();
        expect(radioProvider.addFavoriteStation).toBeDefined();
        expect(radioProvider.addFavoriteStation instanceof ProviderOperation).toBeTruthy();
    });
});
