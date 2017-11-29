/*jslint node: true */

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
var RadioProvider = require("../../../test-classes/joynr/vehicle/RadioProvider");
var RadioStation = require("../../../test-classes/joynr/vehicle/radiotypes/RadioStation");
var Country = require("../../../test-classes/joynr/datatypes/exampleTypes/Country");
var StringMap = require("../../../test-classes/joynr/datatypes/exampleTypes/StringMap");
var ProviderAttribute = require("../../../classes/joynr/provider/ProviderAttribute");
var ProviderOperation = require("../../../classes/joynr/provider/ProviderOperation");
var ProviderEvent = require("../../../classes/joynr/provider/ProviderEvent");
var uuid = require("../../../classes/joynr/util/uuid");
var TestWithVersionProvider = require("../../../test-classes/joynr/tests/TestWithVersionProvider");
var TestWithoutVersionProvider = require("../../../test-classes/joynr/tests/TestWithoutVersionProvider");
describe("libjoynr-js.joynr.provider.Provider", function() {
    var implementation = null;
    var dependencies = {
        ProviderAttribute: ProviderAttribute,
        ProviderOperation: ProviderOperation,
        ProviderEvent: ProviderEvent,
        uuid: uuid
    };

    beforeEach(function() {
        implementation = {
            isOn: {
                value: false,
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
                    this.value = newValue;
                }
            },
            StartWithCapitalLetter: {
                value: false,
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
                    this.value = newValue;
                }
            },
            numberOfStations: {
                value: 0,
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
                    this.value = newValue;
                }
            },
            mixedSubscriptions: {
                value: "testvalue",
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
                    this.value = newValue;
                }
            },
            enumAttribute: {
                value: Country.GERMANY,
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
                    this.value = newValue;
                }
            },
            enumArrayAttribute: {
                value: [Country.GERMANY],
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
                    this.value = newValue;
                }
            },
            byteBufferAttribute: {
                value: [],
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
                    this.value = newValue;
                }
            },
            stringMapAttribute: {
                value: {},
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
                    this.value = newValue;
                }
            },
            complexStructMapAttribute: {
                value: {},
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
                    this.value = newValue;
                }
            },
            failingSyncAttribute: {
                value: 0,
                get: function() {
                    return this.value;
                }
            },
            failingAsyncAttribute: {
                value: 0,
                get: function() {
                    return this.value;
                }
            },
            attrProvidedImpl: {
                value: "testValue2",
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
                    this.value = newValue;
                }
            },
            attributeTestingProviderInterface: {
                get: function() {
                    return undefined;
                }
            },
            typeDefForStruct: {
                value: new RadioStation({
                    name: "radioStation",
                    byteBuffer: []
                }),
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
                    this.value = newValue;
                }
            },
            typeDefForPrimitive: {
                value: 0,
                get: function() {
                    return this.value;
                },
                set: function(newValue) {
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

    it("version is set correctly", function() {
        expect(TestWithVersionProvider.MAJOR_VERSION).toBeDefined();
        expect(TestWithVersionProvider.MAJOR_VERSION).toEqual(47);
        expect(TestWithVersionProvider.MINOR_VERSION).toBeDefined();
        expect(TestWithVersionProvider.MINOR_VERSION).toEqual(11);
    });

    it("default version is set correctly", function() {
        expect(TestWithoutVersionProvider.MAJOR_VERSION).toBeDefined();
        expect(TestWithoutVersionProvider.MAJOR_VERSION).toEqual(0);
        expect(TestWithoutVersionProvider.MINOR_VERSION).toBeDefined();
        expect(TestWithoutVersionProvider.MINOR_VERSION).toEqual(0);
    });

    it("RadioProvider does not throw when instantiated with different implementations", function() {
        expect(function() {
            var radioProvider = new RadioProvider({}, dependencies);
        }).not.toThrow();

        expect(function() {
            var radioProvider = new RadioProvider(
                {
                    isOn: {}
                },
                dependencies
            );
        }).not.toThrow();

        expect(function() {
            var radioProvider = new RadioProvider(
                {
                    isOn: {
                        get: function() {
                            return false;
                        }
                    }
                },
                dependencies
            );
        }).not.toThrow();

        expect(function() {
            var radioProvider = new RadioProvider(
                {
                    isOn: {
                        set: function(value) {}
                    }
                },
                dependencies
            );
        }).not.toThrow();

        expect(function() {
            var radioProvider = new RadioProvider(
                {
                    isOn: {
                        get: function() {
                            return false;
                        },
                        set: function(value) {}
                    }
                },
                dependencies
            );
        }).not.toThrow();

        expect(function() {
            var radioProvider = new RadioProvider(
                {
                    isOn: {
                        get: function() {
                            return false;
                        },
                        set: function(value) {}
                    }
                },
                dependencies
            );
        }).not.toThrow();

        expect(function() {
            var radioProvider = new RadioProvider(
                {
                    addFavoriteStation: function(opArgs) {
                        return true;
                    }
                },
                dependencies
            );
        }).not.toThrow();
    });

    it("RadioProvider is instantiable", function() {
        var radioProvider = new RadioProvider({}, dependencies);
        expect(radioProvider).toBeDefined();
        expect(radioProvider).not.toBeNull();
        expect(typeof radioProvider === "object").toBeTruthy();
        expect(radioProvider instanceof RadioProvider).toBeTruthy();
    });

    it("RadioProvider has all members", function() {
        var radioProvider = new RadioProvider({}, dependencies);
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

    it("RadioProvider recognizes a correct implementation", function() {
        var radioProvider = new RadioProvider(implementation, dependencies);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(0);
    });

    it("RadioProvider recognizes an incorrect implementation", function() {
        delete implementation.addFavoriteStation;
        var radioProvider = new RadioProvider(implementation, dependencies);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(1);
    });

    it("RadioProvider recognizes a missing getter", function() {
        delete implementation.isOn.get;
        var radioProvider = new RadioProvider(implementation, dependencies);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(1);
    });

    it("RadioProvider recognizes a missing setter", function() {
        delete implementation.isOn.set;
        var radioProvider = new RadioProvider(implementation, dependencies);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(1);
    });

    it("RadioProvider recognizes multiple missing implementation functions", function() {
        delete implementation.isOn.get;
        delete implementation.addFavoriteStation;
        var radioProvider = new RadioProvider(implementation, dependencies);
        expect(radioProvider.checkImplementation).toBeDefined();
        expect(radioProvider.checkImplementation()).toBeTruthy();
        expect(radioProvider.checkImplementation().length).toEqual(2);
    });
});
