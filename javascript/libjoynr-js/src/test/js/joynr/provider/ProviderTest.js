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
require("../../node-unit-test-helper");
const RadioProvider = require("../../../generated/joynr/vehicle/RadioProvider");
const RadioStation = require("../../../generated/joynr/vehicle/radiotypes/RadioStation");
const Country = require("../../../generated/joynr/datatypes/exampleTypes/Country");
const StringMap = require("../../../generated/joynr/datatypes/exampleTypes/StringMap");
const ProviderAttribute = require("../../../../main/js/joynr/provider/ProviderAttribute");
const ProviderOperation = require("../../../../main/js/joynr/provider/ProviderOperation");
const ProviderEvent = require("../../../../main/js/joynr/provider/ProviderEvent");
const uuid = require("../../../../main/js/joynr/util/uuid");
const TestWithVersionProvider = require("../../../generated/joynr/tests/TestWithVersionProvider");
const TestWithoutVersionProvider = require("../../../generated/joynr/tests/TestWithoutVersionProvider");
describe("libjoynr-js.joynr.provider.Provider", () => {
    let implementation = null;
    const dependencies = {
        ProviderAttribute,
        ProviderOperation,
        ProviderEvent,
        uuid
    };

    beforeEach(() => {
        implementation = {
            isOn: {
                value: false,
                get() {
                    return this.value;
                },
                set(newValue) {
                    this.value = newValue;
                }
            },
            StartWithCapitalLetter: {
                value: false,
                get() {
                    return this.value;
                },
                set(newValue) {
                    this.value = newValue;
                }
            },
            numberOfStations: {
                value: 0,
                get() {
                    return this.value;
                },
                set(newValue) {
                    this.value = newValue;
                }
            },
            mixedSubscriptions: {
                value: "testvalue",
                get() {
                    return this.value;
                },
                set(newValue) {
                    this.value = newValue;
                }
            },
            enumAttribute: {
                value: Country.GERMANY,
                get() {
                    return this.value;
                },
                set(newValue) {
                    this.value = newValue;
                }
            },
            enumArrayAttribute: {
                value: [Country.GERMANY],
                get() {
                    return this.value;
                },
                set(newValue) {
                    this.value = newValue;
                }
            },
            byteBufferAttribute: {
                value: [],
                get() {
                    return this.value;
                },
                set(newValue) {
                    this.value = newValue;
                }
            },
            stringMapAttribute: {
                value: {},
                get() {
                    return this.value;
                },
                set(newValue) {
                    this.value = newValue;
                }
            },
            complexStructMapAttribute: {
                value: {},
                get() {
                    return this.value;
                },
                set(newValue) {
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
                set(newValue) {
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
                set(newValue) {
                    this.value = newValue;
                }
            },
            typeDefForPrimitive: {
                value: 0,
                get() {
                    return this.value;
                },
                set(newValue) {
                    this.value = newValue;
                }
            },
            addFavoriteStation: jasmine.createSpy("addFavoriteStation"),
            methodFireAndForgetWithoutParams: jasmine.createSpy("methodFireAndForgetWithoutParams"),
            methodFireAndForget: jasmine.createSpy("methodFireAndForget"),
            weakSignal: jasmine.createSpy("weakSignal"),
            triggerBroadcasts: jasmine.createSpy("triggerBroadcasts"),
            triggerBroadcastsWithPartitions: jasmine.createSpy("triggerBroadcastsWithPartitions"),
            methodProvidedImpl: jasmine.createSpy("methodProvidedImpl"),
            methodWithByteBuffer: jasmine.createSpy("methodWithByteBuffer"),
            methodWithTypeDef: jasmine.createSpy("methodWithTypeDef"),
            methodWithComplexMap: jasmine.createSpy("methodWithComplexMap"),
            operationWithEnumsAsInputAndOutput: jasmine.createSpy("operationWithEnumsAsInputAndOutput"),
            operationWithMultipleOutputParameters: jasmine.createSpy("operationWithMultipleOutputParameters"),
            operationWithEnumsAsInputAndEnumArrayAsOutput: jasmine.createSpy(
                "operationWithEnumsAsInputAndEnumArrayAsOutput"
            ),
            methodWithSingleArrayParameters: jasmine.createSpy("methodWithSingleArrayParameters"),
            broadcastWithEnum: jasmine.createSpy("broadcastWithEnum")
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
        expect(() => {
            const radioProvider = new RadioProvider({}, dependencies);
        }).not.toThrow();

        expect(() => {
            const radioProvider = new RadioProvider(
                {
                    isOn: {}
                },
                dependencies
            );
        }).not.toThrow();

        expect(() => {
            const radioProvider = new RadioProvider(
                {
                    isOn: {
                        get() {
                            return false;
                        }
                    }
                },
                dependencies
            );
        }).not.toThrow();

        expect(() => {
            const radioProvider = new RadioProvider(
                {
                    isOn: {
                        set(value) {}
                    }
                },
                dependencies
            );
        }).not.toThrow();

        expect(() => {
            const radioProvider = new RadioProvider(
                {
                    isOn: {
                        get() {
                            return false;
                        },
                        set(value) {}
                    }
                },
                dependencies
            );
        }).not.toThrow();

        expect(() => {
            const radioProvider = new RadioProvider(
                {
                    isOn: {
                        get() {
                            return false;
                        },
                        set(value) {}
                    }
                },
                dependencies
            );
        }).not.toThrow();

        expect(() => {
            const radioProvider = new RadioProvider(
                {
                    addFavoriteStation(opArgs) {
                        return true;
                    }
                },
                dependencies
            );
        }).not.toThrow();
    });

    it("RadioProvider is instantiable", () => {
        const radioProvider = new RadioProvider({}, dependencies);
        expect(radioProvider).toBeDefined();
        expect(radioProvider).not.toBeNull();
        expect(typeof radioProvider === "object").toBeTruthy();
        expect(radioProvider instanceof RadioProvider).toBeTruthy();
    });

    it("RadioProvider has all members", () => {
        const radioProvider = new RadioProvider({}, dependencies);
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
        const radioProvider = new RadioProvider(implementation, dependencies);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(0);
    });

    it("RadioProvider recognizes an incorrect implementation", () => {
        delete implementation.addFavoriteStation;
        const radioProvider = new RadioProvider(implementation, dependencies);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(1);
    });

    it("RadioProvider recognizes a missing getter", () => {
        delete implementation.isOn.get;
        const radioProvider = new RadioProvider(implementation, dependencies);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(1);
    });

    it("RadioProvider recognizes a missing setter", () => {
        delete implementation.isOn.set;
        const radioProvider = new RadioProvider(implementation, dependencies);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(1);
    });

    it("RadioProvider recognizes multiple missing implementation functions", () => {
        delete implementation.isOn.get;
        delete implementation.addFavoriteStation;
        const radioProvider = new RadioProvider(implementation, dependencies);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(2);
    });
});
