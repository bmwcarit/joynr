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
import RadioStation from "../../../generated/joynr/vehicle/radiotypes/RadioStation";
import Country from "../../../generated/joynr/datatypes/exampleTypes/Country";
import ProviderOperation from "../../../../main/js/joynr/provider/ProviderOperation";
import ProviderEvent from "../../../../main/js/joynr/provider/ProviderEvent";
import TestWithVersionProvider from "../../../generated/joynr/tests/TestWithVersionProvider";
import TestWithoutVersionProvider from "../../../generated/joynr/tests/TestWithoutVersionProvider";
describe("libjoynr-js.joynr.provider.Provider", () => {
    let implementation: any = null;

    beforeEach(() => {
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
            StartWithCapitalLetter: {
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
            enumAttribute: {
                value: Country.GERMANY,
                get() {
                    return this.value;
                },
                set(newValue: any) {
                    this.value = newValue;
                }
            },
            enumArrayAttribute: {
                value: [Country.GERMANY],
                get() {
                    return this.value;
                },
                set(newValue: any) {
                    this.value = newValue;
                }
            },
            byteBufferAttribute: {
                value: [],
                get() {
                    return this.value;
                },
                set(newValue: any) {
                    this.value = newValue;
                }
            },
            stringMapAttribute: {
                value: {},
                get() {
                    return this.value;
                },
                set(newValue: any) {
                    this.value = newValue;
                }
            },
            complexStructMapAttribute: {
                value: {},
                get() {
                    return this.value;
                },
                set(newValue: any) {
                    this.value = newValue;
                }
            },
            failingSyncAttribute: {
                value: 0,
                get() {
                    return this.value;
                }
            },
            failingAsyncAttribute: {
                value: 0,
                get() {
                    return this.value;
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
            typeDefForStruct: {
                value: new RadioStation({
                    name: "radioStation",
                    byteBuffer: []
                }),
                get() {
                    return this.value;
                },
                set(newValue: any) {
                    this.value = newValue;
                }
            },
            typeDefForPrimitive: {
                value: 0,
                get() {
                    return this.value;
                },
                set(newValue: any) {
                    this.value = newValue;
                }
            },
            addFavoriteStation: jest.fn(),
            methodFireAndForgetWithoutParams: jest.fn(),
            methodFireAndForget: jest.fn(),
            weakSignal: jest.fn(),
            triggerBroadcasts: jest.fn(),
            triggerBroadcastsWithPartitions: jest.fn(),
            methodProvidedImpl: jest.fn(),
            methodWithByteBuffer: jest.fn(),
            methodWithTypeDef: jest.fn(),
            methodWithComplexMap: jest.fn(),
            operationWithEnumsAsInputAndOutput: jest.fn(),
            operationWithMultipleOutputParameters: jest.fn(),
            operationWithEnumsAsInputAndEnumArrayAsOutput: jest.fn(),
            methodWithSingleArrayParameters: jest.fn(),
            broadcastWithEnum: jest.fn()
        };
    });

    it("version is set correctly", () => {
        expect(TestWithVersionProvider.MAJOR_VERSION).toBeDefined();
        expect(TestWithVersionProvider.MAJOR_VERSION).toEqual(47);
        expect(TestWithVersionProvider.MINOR_VERSION).toBeDefined();
        expect(TestWithVersionProvider.MINOR_VERSION).toEqual(11);
    });

    it("default version is set correctly", () => {
        expect(TestWithoutVersionProvider.MAJOR_VERSION).toBeDefined();
        expect(TestWithoutVersionProvider.MAJOR_VERSION).toEqual(0);
        expect(TestWithoutVersionProvider.MINOR_VERSION).toBeDefined();
        expect(TestWithoutVersionProvider.MINOR_VERSION).toEqual(0);
    });

    it("RadioProvider does not throw when instantiated with different implementations", () => {
        expect(() => new RadioProvider({} as any)).not.toThrow();
        expect(() => new RadioProvider({ isOn: {} } as any)).not.toThrow();

        expect(() => new RadioProvider({ isOn: { get: () => false } } as any)).not.toThrow();
        expect(
            () =>
                new RadioProvider({
                    isOn: {
                        set: () => {
                            // do nothing
                        }
                    }
                } as any)
        ).not.toThrow();
        expect(
            () =>
                new RadioProvider({
                    isOn: {
                        get: () => false,
                        set: () => {
                            // do nothing
                        }
                    }
                } as any)
        ).not.toThrow();
        expect(() => new RadioProvider({ addFavoriteStation: () => true } as any)).not.toThrow();
    });

    it("RadioProvider is instantiable", () => {
        const radioProvider = new RadioProvider({} as any);
        expect(radioProvider).toBeDefined();
        expect(radioProvider).not.toBeNull();
        expect(typeof radioProvider === "object").toBeTruthy();
        expect(radioProvider instanceof RadioProvider).toBeTruthy();
    });

    it("RadioProvider has all members", () => {
        const radioProvider = new RadioProvider({} as any);
        expect(radioProvider.isOn).toBeDefined();
        expect(radioProvider.enumAttribute).toBeDefined();
        expect(radioProvider.enumArrayAttribute).toBeDefined();
        expect(radioProvider.addFavoriteStation).toBeDefined();
        expect(radioProvider.addFavoriteStation instanceof ProviderOperation).toBeTruthy();
        expect(radioProvider.operationWithEnumsAsInputAndOutput).toBeDefined();
        expect(radioProvider.operationWithEnumsAsInputAndOutput instanceof ProviderOperation).toBeTruthy();
        expect(radioProvider.operationWithEnumsAsInputAndEnumArrayAsOutput).toBeDefined();
        expect(radioProvider.operationWithEnumsAsInputAndEnumArrayAsOutput instanceof ProviderOperation).toBeTruthy();
        expect(radioProvider.weakSignal).toBeDefined();
        expect(radioProvider.weakSignal instanceof ProviderEvent).toBeTruthy();
        expect(radioProvider.broadcastWithEnum).toBeDefined();
        expect(radioProvider.broadcastWithEnum instanceof ProviderEvent).toBeTruthy();
        expect(radioProvider.interfaceName).toBeDefined();
        expect(typeof radioProvider.interfaceName).toEqual("string");
    });

    it("RadioProvider recognizes a correct implementation", () => {
        const radioProvider = new RadioProvider(implementation);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(0);
    });

    it("RadioProvider recognizes an incorrect implementation", () => {
        delete implementation.addFavoriteStation;
        const radioProvider = new RadioProvider(implementation);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(1);
    });

    it("RadioProvider recognizes a missing getter", () => {
        delete implementation.isOn.get;
        const radioProvider = new RadioProvider(implementation);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(1);
    });

    it("RadioProvider recognizes a missing setter", () => {
        delete implementation.isOn.set;
        const radioProvider = new RadioProvider(implementation);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(1);
    });

    it("RadioProvider recognizes multiple missing implementation functions", () => {
        delete implementation.isOn.get;
        delete implementation.addFavoriteStation;
        const radioProvider = new RadioProvider(implementation);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(2);
    });
});
