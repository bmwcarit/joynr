/*global joynrTestRequire: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

joynrTestRequire("joynr/provider/TestProvider", [
    "joynr/vehicle/RadioProvider",
    "joynr/datatypes/exampleTypes/Country",
    "joynr/provider/ProviderAttributeNotifyReadWrite",
    "joynr/provider/ProviderAttributeNotifyRead",
    "joynr/provider/ProviderAttributeNotifyWrite",
    "joynr/provider/ProviderAttributeNotify",
    "joynr/provider/ProviderAttributeReadWrite",
    "joynr/provider/ProviderAttributeRead",
    "joynr/provider/ProviderAttributeWrite",
    "joynr/provider/ProviderOperation",
    "joynr/provider/ProviderEvent",
    "joynr/TypesEnum",
    "joynr/util/uuid"
], function(
        RadioProvider,
        Country,
        ProviderAttributeNotifyReadWrite,
        ProviderAttributeNotifyRead,
        ProviderAttributeNotifyWrite,
        ProviderAttributeNotify,
        ProviderAttributeReadWrite,
        ProviderAttributeRead,
        ProviderAttributeWrite,
        ProviderOperation,
        ProviderEvent,
        TypesEnum,
        uuid) {
    describe("libjoynr-js.joynr.provider.Provider", function() {
        var implementation = null;
        var dependencies = {
            ProviderAttributeNotifyReadWrite : ProviderAttributeNotifyReadWrite,
            ProviderAttributeNotifyRead : ProviderAttributeNotifyRead,
            ProviderAttributeNotifyWrite : ProviderAttributeNotifyWrite,
            ProviderAttributeNotify : ProviderAttributeNotify,
            ProviderAttributeReadWrite : ProviderAttributeReadWrite,
            ProviderAttributeRead : ProviderAttributeRead,
            ProviderAttributeWrite : ProviderAttributeWrite,
            ProviderOperation : ProviderOperation,
            ProviderEvent : ProviderEvent,
            TypesEnum : TypesEnum,
            uuid : uuid
        };

        beforeEach(function() {
            implementation =
                    {
                        isOn : {
                            value : false,
                            get : function() {
                                return this.value;
                            },
                            set : function(newValue) {
                                this.value = newValue;
                            }
                        },
                        numberOfStations : {
                            value : 0,
                            get : function() {
                                return this.value;
                            },
                            set : function(newValue) {
                                this.value = newValue;
                            }
                        },
                        mixedSubscriptions : {
                            value : "testvalue",
                            get : function() {
                                return this.value;
                            },
                            set : function(newValue) {
                                this.value = newValue;
                            }
                        },
                        enumAttribute : {
                            value : Country.GERMANY,
                            get : function() {
                                return this.value;
                            },
                            set : function(newValue) {
                                this.value = newValue;
                            }
                        },
                        enumArrayAttribute : {
                            value : [ Country.GERMANY
                            ],
                            get : function() {
                                return this.value;
                            },
                            set : function(newValue) {
                                this.value = newValue;
                            }
                        },
                        attrProvidedImpl : {
                            value : "testValue2",
                            get : function() {
                                return this.value;
                            },
                            set : function(newValue) {
                                this.value = newValue;
                            }
                        },
                        addFavoriteStation : jasmine.createSpy("addFavoriteStation"),
                        weakSignal : jasmine.createSpy("weakSignal"),
                        methodProvidedImpl : jasmine.createSpy("methodProvidedImpl"),
                        operationWithEnumsAsInputAndOutput : jasmine
                                .createSpy("operationWithEnumsAsInputAndOutput")
                    };
        });

        it(
                "RadioProvider does not throw when instantiated with different implementations",
                function() {
                    expect(function() {
                        var radioProvider = new RadioProvider({}, dependencies);
                    }).not.toThrow();

                    expect(function() {
                        var radioProvider = new RadioProvider({
                            isOn : {}
                        }, dependencies);
                    }).not.toThrow();

                    expect(function() {
                        var radioProvider = new RadioProvider({
                            isOn : {
                                get : function() {
                                    return false;
                                }
                            }
                        }, dependencies);
                    }).not.toThrow();

                    expect(function() {
                        var radioProvider = new RadioProvider({
                            isOn : {
                                set : function(value) {}
                            }
                        }, dependencies);
                    }).not.toThrow();

                    expect(function() {
                        var radioProvider = new RadioProvider({
                            isOn : {
                                get : function() {
                                    return false;
                                },
                                set : function(value) {}
                            }
                        }, dependencies);
                    }).not.toThrow();

                    expect(function() {
                        var radioProvider = new RadioProvider({
                            isOn : {
                                get : function() {
                                    return false;
                                },
                                set : function(value) {}
                            }
                        }, dependencies);
                    }).not.toThrow();

                    expect(function() {
                        var radioProvider = new RadioProvider({
                            addFavoriteStation : function(opArgs) {
                                return true;
                            }
                        }, dependencies);
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
            expect(radioProvider.isOn instanceof ProviderAttributeNotifyReadWrite).toBeTruthy();
            expect(radioProvider.enumAttribute).toBeDefined();
            expect(radioProvider.enumAttribute instanceof ProviderAttributeNotifyReadWrite)
                    .toBeTruthy();
            expect(radioProvider.enumArrayAttribute).toBeDefined();
            expect(radioProvider.enumArrayAttribute instanceof ProviderAttributeNotifyReadWrite)
                    .toBeTruthy();
            expect(radioProvider.addFavoriteStation).toBeDefined();
            expect(radioProvider.addFavoriteStation instanceof ProviderOperation).toBeTruthy();
            expect(radioProvider.operationWithEnumsAsInputAndOutput).toBeDefined();
            expect(radioProvider.operationWithEnumsAsInputAndOutput instanceof ProviderOperation)
                    .toBeTruthy();
            expect(radioProvider.weakSignal).toBeDefined();
            expect(radioProvider.weakSignal instanceof ProviderEvent).toBeTruthy();
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

}); // require
