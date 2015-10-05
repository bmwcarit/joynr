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

joynrTestRequire(
        "joynr/provider/TestProviderAttribute",
        [
            "joynr/TypesEnum",
            "joynr/provider/ProviderAttribute",
            "joynr/provider/ProviderAttributeNotifyReadWrite",
            "joynr/provider/ProviderAttributeNotifyRead",
            "joynr/provider/ProviderAttributeNotifyWrite",
            "joynr/provider/ProviderAttributeNotify",
            "joynr/provider/ProviderAttributeReadWrite",
            "joynr/provider/ProviderAttributeRead",
            "joynr/provider/ProviderAttributeWrite",
            "joynr/types/ProviderQos",
            "joynr/tests/testTypes/TestEnum",
            "joynr/datatypes/exampleTypes/ComplexRadioStation",
            "joynr/datatypes/exampleTypes/Country"
        ],
        function(
                TypesEnum,
                ProviderAttribute,
                ProviderAttributeNotifyReadWrite,
                ProviderAttributeNotifyRead,
                ProviderAttributeNotifyWrite,
                ProviderAttributeNotify,
                ProviderAttributeReadWrite,
                ProviderAttributeRead,
                ProviderAttributeWrite,
                ProviderQos,
                TestEnum,
                ComplexRadioStation,
                Country) {

            var safetyTimeoutDelta = 100;

            describe(
                    "libjoynr-js.joynr.provider.ProviderAttribute",
                    function() {

                        var i, implementation, isOn, isOnNotifyReadOnly, isOnNotifyWriteOnly;
                        var isOnNotify, isOnReadWrite, isOnReadOnly, isOnWriteOnly;
                        var isOnProviderAttributeNotifyReadWrite, isOnProviderAttributeNotifyRead;
                        var isOnProviderAttributeNotifyWrite, isOnProviderAttributeNotify;
                        var isOnProviderAttributeReadWrite, isOnProviderAttributeRead;
                        var isOnProviderAttributeWrite, allAttributes, allNotifyAttributes;

                        beforeEach(function() {
                            implementation = {
                                value : {
                                    key : "value",
                                    1 : 0,
                                    object : {}
                                },
                                get : function() {
                                    return implementation.value;
                                },
                                set : function(newValue) {
                                    implementation.value = newValue;
                                }
                            };
                            spyOn(implementation, "get").andCallThrough();
                            spyOn(implementation, "set").andCallThrough();
                            var provider = {};

                            isOn =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOn",
                                            TypesEnum.BOOL,
                                            "NOTIFYREADWRITE");
                            isOnNotifyReadOnly =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnNotifyReadOnly",
                                            TypesEnum.BOOL,
                                            "NOTIFYREADONLY");
                            isOnNotifyWriteOnly =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnNotifyWriteOnly",
                                            TypesEnum.BOOL,
                                            "NOTIFYWRITEONLY");
                            isOnNotify =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnNotify",
                                            TypesEnum.BOOL,
                                            "NOTIFY");
                            isOnReadWrite =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnReadWrite",
                                            TypesEnum.BOOL,
                                            "READWRITE");
                            isOnReadOnly =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnReadOnly",
                                            TypesEnum.BOOL,
                                            "READONLY");
                            isOnWriteOnly =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnWriteOnly",
                                            TypesEnum.BOOL,
                                            "WRITEONLY");

                            isOnProviderAttributeNotifyReadWrite =
                                    new ProviderAttributeNotifyReadWrite(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeNotifyReadWrite",
                                            TypesEnum.BOOL);
                            isOnProviderAttributeNotifyRead =
                                    new ProviderAttributeNotifyRead(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeNotifyRead",
                                            TypesEnum.BOOL);
                            isOnProviderAttributeNotifyWrite =
                                    new ProviderAttributeNotifyWrite(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeNotifyWrite",
                                            TypesEnum.BOOL);
                            isOnProviderAttributeNotify =
                                    new ProviderAttributeNotify(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeNotify",
                                            TypesEnum.BOOL);
                            isOnProviderAttributeReadWrite =
                                    new ProviderAttributeReadWrite(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeReadWrite",
                                            TypesEnum.BOOL);
                            isOnProviderAttributeRead =
                                    new ProviderAttributeRead(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeRead",
                                            TypesEnum.BOOL);
                            isOnProviderAttributeWrite =
                                    new ProviderAttributeWrite(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeWrite",
                                            TypesEnum.BOOL);

                            allAttributes = [
                                isOn,
                                isOnNotifyReadOnly,
                                isOnNotifyWriteOnly,
                                isOnNotify,
                                isOnReadWrite,
                                isOnReadOnly,
                                isOnWriteOnly,
                                isOnProviderAttributeNotifyReadWrite,
                                isOnProviderAttributeNotifyRead,
                                isOnProviderAttributeNotifyWrite,
                                isOnProviderAttributeNotify,
                                isOnProviderAttributeReadWrite,
                                isOnProviderAttributeRead,
                                isOnProviderAttributeWrite
                            ];

                            allNotifyAttributes = [
                                isOn,
                                isOnNotifyReadOnly,
                                isOnNotifyWriteOnly,
                                isOnNotify,
                                isOnProviderAttributeNotifyReadWrite,
                                isOnProviderAttributeNotifyRead,
                                isOnProviderAttributeNotifyWrite,
                                isOnProviderAttributeNotify
                            ];
                        });

                        it("is of correct type (ProviderAttribute)", function() {
                            expect(isOn).toBeDefined();
                            expect(isOn).not.toBeNull();
                            expect(typeof isOn === "object").toBeTruthy();
                            expect(isOn instanceof ProviderAttribute).toBeTruthy();
                        });

                        it(
                                "has correct members (ProviderAttribute with NOTIFYREADWRITE)",
                                function() {
                                    expect(isOn.registerGetter).toBeDefined();
                                    expect(isOn.registerSetter).toBeDefined();
                                    expect(isOn.valueChanged).toBeDefined();
                                    expect(isOn.registerObserver).toBeDefined();
                                    expect(isOn.unregisterObserver).toBeDefined();
                                });

                        it(
                                "has correct members (ProviderAttribute with NOTIFYREADONLY)",
                                function() {
                                    expect(isOnNotifyReadOnly.registerGetter).toBeDefined();
                                    expect(isOnNotifyReadOnly.registerSetter).toBeUndefined();
                                    expect(isOnNotifyReadOnly.valueChanged).toBeDefined();
                                    expect(isOnNotifyReadOnly.registerObserver).toBeDefined();
                                    expect(isOnNotifyReadOnly.unregisterObserver).toBeDefined();
                                });

                        it(
                                "has correct members (ProviderAttribute with NOTIFYWRITEONLY)",
                                function() {
                                    expect(isOnNotifyWriteOnly.registerGetter).toBeUndefined();
                                    expect(isOnNotifyWriteOnly.registerSetter).toBeDefined();
                                    expect(isOnNotifyWriteOnly.valueChanged).toBeDefined();
                                    expect(isOnNotifyWriteOnly.registerObserver).toBeDefined();
                                    expect(isOnNotifyWriteOnly.unregisterObserver).toBeDefined();
                                });

                        it("has correct members (ProviderAttribute with NOTIFY)", function() {
                            expect(isOnNotify.registerGetter).toBeUndefined();
                            expect(isOnNotify.registerSetter).toBeUndefined();
                            expect(isOnNotify.valueChanged).toBeDefined();
                            expect(isOnNotify.registerObserver).toBeDefined();
                            expect(isOnNotify.unregisterObserver).toBeDefined();
                        });

                        it("has correct members (ProviderAttribute with READWRITE)", function() {
                            expect(isOnReadWrite.registerGetter).toBeDefined();
                            expect(isOnReadWrite.registerSetter).toBeDefined();
                            expect(isOnReadWrite.valueChanged).toBeUndefined();
                            expect(isOnReadWrite.registerObserver).toBeUndefined();
                            expect(isOnReadWrite.unregisterObserver).toBeUndefined();
                        });

                        it("has correct members (ProviderAttribute with READONLY)", function() {
                            expect(isOnReadOnly.registerGetter).toBeDefined();
                            expect(isOnReadOnly.registerSetter).toBeUndefined();
                            expect(isOnReadOnly.valueChanged).toBeUndefined();
                            expect(isOnReadOnly.registerObserver).toBeUndefined();
                            expect(isOnReadOnly.unregisterObserver).toBeUndefined();
                        });

                        it("has correct members (ProviderAttribute with WRITEONLY)", function() {
                            expect(isOnWriteOnly.registerGetter).toBeUndefined();
                            expect(isOnWriteOnly.registerSetter).toBeDefined();
                            expect(isOnWriteOnly.valueChanged).toBeUndefined();
                            expect(isOnWriteOnly.registerObserver).toBeUndefined();
                            expect(isOnWriteOnly.unregisterObserver).toBeUndefined();
                        });

                        it(
                                "has correct members (ProviderAttributeNotifyReadWrite)",
                                function() {
                                    expect(
                                            isOnProviderAttributeNotifyReadWrite instanceof ProviderAttributeNotifyReadWrite)
                                            .toBeTruthy();
                                    expect(isOnProviderAttributeNotifyReadWrite.registerGetter)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeNotifyReadWrite.registerSetter)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeNotifyReadWrite.valueChanged)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeNotifyReadWrite.registerObserver)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeNotifyReadWrite.unregisterObserver)
                                            .toBeDefined();
                                });

                        it(
                                "has correct members (ProviderAttributeNotifyRead)",
                                function() {
                                    expect(
                                            isOnProviderAttributeNotifyRead instanceof ProviderAttributeNotifyRead)
                                            .toBeTruthy();
                                    expect(isOnProviderAttributeNotifyRead.registerGetter)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeNotifyRead.registerSetter)
                                            .toBeUndefined();
                                    expect(isOnProviderAttributeNotifyRead.valueChanged)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeNotifyRead.registerObserver)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeNotifyRead.unregisterObserver)
                                            .toBeDefined();
                                });

                        it(
                                "has correct members (ProviderAttributeNotifyWrite)",
                                function() {
                                    expect(
                                            isOnProviderAttributeNotifyWrite instanceof ProviderAttributeNotifyWrite)
                                            .toBeTruthy();
                                    expect(isOnProviderAttributeNotifyWrite.registerGetter)
                                            .toBeUndefined();
                                    expect(isOnProviderAttributeNotifyWrite.registerSetter)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeNotifyWrite.valueChanged)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeNotifyWrite.registerObserver)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeNotifyWrite.unregisterObserver)
                                            .toBeDefined();
                                });

                        it("has correct members (ProviderAttributeNotify)", function() {
                            expect(isOnProviderAttributeNotify instanceof ProviderAttributeNotify)
                                    .toBeTruthy();
                            expect(isOnProviderAttributeNotify.registerGetter).toBeUndefined();
                            expect(isOnProviderAttributeNotify.registerSetter).toBeUndefined();
                            expect(isOnProviderAttributeNotify.valueChanged).toBeDefined();
                            expect(isOnProviderAttributeNotify.registerObserver).toBeDefined();
                            expect(isOnProviderAttributeNotify.unregisterObserver).toBeDefined();
                        });

                        it(
                                "has correct members (ProviderAttributeReadWrite)",
                                function() {
                                    expect(
                                            isOnProviderAttributeReadWrite instanceof ProviderAttributeReadWrite)
                                            .toBeTruthy();
                                    expect(isOnProviderAttributeReadWrite.registerGetter)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeReadWrite.registerSetter)
                                            .toBeDefined();
                                });

                        it("has correct members (ProviderAttributeRead)", function() {
                            expect(isOnProviderAttributeRead instanceof ProviderAttributeRead)
                                    .toBeTruthy();
                            expect(isOnProviderAttributeRead.registerGetter).toBeDefined();
                            expect(isOnProviderAttributeRead.registerSetter).toBeUndefined();
                        });

                        it("has correct members (ProviderAttributeWrite)", function() {
                            expect(isOnProviderAttributeWrite instanceof ProviderAttributeWrite)
                                    .toBeTruthy();
                            expect(isOnProviderAttributeWrite.registerGetter).toBeUndefined();
                            expect(isOnProviderAttributeWrite.registerSetter).toBeDefined();
                        });

                        it("call[G|S]etter calls through to registered [g|s]etters", function() {
                            var spy, nrGetters = 0, nrSetters = 0, testParam = "myTestParameter";

                            for (i = 0; i < allAttributes.length; ++i) {
                                if (allAttributes[i].get) {
                                    spy = jasmine.createSpy("ProviderAttributeSpy");
                                    spy.andReturn(testParam);
                                    allAttributes[i].registerGetter(spy);
                                    var result = allAttributes[i].get();
                                    expect(spy).toHaveBeenCalled();
                                    expect(result).toEqual([ testParam
                                    ]);
                                    ++nrGetters;
                                }
                                if (allAttributes[i].set) {
                                    spy = jasmine.createSpy("ProviderAttributeSpy");
                                    allAttributes[i].registerSetter(spy);
                                    allAttributes[i].set(testParam);
                                    expect(spy).toHaveBeenCalled();
                                    expect(spy).toHaveBeenCalledWith(testParam);
                                    ++nrSetters;
                                }
                            }

                            // must be 4 combinations of getters and setters in 2 variations each => 4 * 2
                            expect(nrGetters).toBe(8);
                            expect(nrSetters).toBe(8);

                            // expect provided implementation not to have been called
                            expect(implementation.get.callCount).toBe(8);
                            expect(implementation.set).not.toHaveBeenCalled();
                        });

                        it("call[G|S]etter calls through to provided implementation", function() {
                            var spy, nrGetters = 0, nrSetters = 0, testParam = "myTestParameter";

                            for (i = 0; i < allAttributes.length; ++i) {
                                if (allAttributes[i].get) {
                                    implementation.get.reset();
                                    implementation.get.andReturn(testParam);
                                    expect(allAttributes[i].get()).toEqual([ testParam
                                    ]);
                                    expect(implementation.get).toHaveBeenCalled();
                                }
                                if (allAttributes[i].set) {
                                    implementation.set.reset();
                                    allAttributes[i].set(testParam);
                                    expect(implementation.set).toHaveBeenCalledWith(testParam);
                                }
                            }
                        });

                        function buildObserver(spy) {
                            return function(value) {
                                spy(value);
                            };
                        }

                        it("implements the observer concept correctly", function() {
                            var i, spy1, spy2, attribute, func1, func2, value = {
                                key : "value",
                                1 : 2,
                                object : {}
                            };

                            for (i = 0; i < allNotifyAttributes.length; ++i) {
                                attribute = allNotifyAttributes[i];

                                spy1 = jasmine.createSpy("spy1");
                                spy2 = jasmine.createSpy("spy2");

                                func1 = buildObserver(spy1);
                                func2 = buildObserver(spy2);

                                attribute.registerObserver(func1);
                                attribute.registerObserver(func2);

                                expect(spy1).not.toHaveBeenCalled();
                                expect(spy2).not.toHaveBeenCalled();

                                attribute.valueChanged(value);

                                expect(spy1).toHaveBeenCalled();
                                expect(spy1).toHaveBeenCalledWith([ value
                                ]);
                                expect(spy2).toHaveBeenCalled();
                                expect(spy2).toHaveBeenCalledWith([ value
                                ]);

                                attribute.unregisterObserver(func2);

                                attribute.valueChanged(value);

                                expect(spy1.calls.length).toEqual(2);
                                expect(spy2.calls.length).toEqual(1);

                                attribute.unregisterObserver(func1);

                                attribute.valueChanged(value);

                                expect(spy1.calls.length).toEqual(2);
                                expect(spy2.calls.length).toEqual(1);
                            }
                        });

                        it("notifies observer when calling set with new value", function() {
                            var i, spy1, spy2, attribute, func1, func2, value;

                            for (i = 0; i < allNotifyAttributes.length; ++i) {
                                attribute = allNotifyAttributes[i];
                                if (attribute.set) {
                                    spy1 = jasmine.createSpy("spy1");
                                    spy2 = jasmine.createSpy("spy2");

                                    func1 = buildObserver(spy1);
                                    func2 = buildObserver(spy2);

                                    attribute.registerObserver(func1);
                                    attribute.registerObserver(func2);

                                    expect(spy1).not.toHaveBeenCalled();
                                    expect(spy2).not.toHaveBeenCalled();

                                    value = new ComplexRadioStation({
                                        name : "nameValue",
                                        station : "stationValue",
                                        source : Country.GERMANY
                                    });
                                    attribute.set(value);

                                    expect(spy1).toHaveBeenCalled();
                                    expect(spy1).toHaveBeenCalledWith([ value
                                    ]);
                                    expect(spy2).toHaveBeenCalled();
                                    expect(spy2).toHaveBeenCalledWith([ value
                                    ]);

                                    attribute.unregisterObserver(func2);

                                    value = new ComplexRadioStation({
                                        name : "nameValue2",
                                        station : "stationValue2",
                                        source : Country.AUSTRIA
                                    });

                                    attribute.set(value);

                                    expect(spy1.callCount).toEqual(2);
                                    expect(spy2.callCount).toEqual(1);

                                    attribute.unregisterObserver(func1);

                                    value = new ComplexRadioStation({
                                        name : "nameValue3",
                                        station : "stationValue3",
                                        source : Country.AUSTRALIA
                                    });
                                    attribute.set(value);

                                    expect(spy1.callCount).toEqual(2);
                                    expect(spy2.callCount).toEqual(1);
                                }
                            }
                        });

                        it("doesn't notify observer when calling set with same values", function() {
                            var i, spy1, spy2, attribute, func1, func2, value;

                            for (i = 0; i < allNotifyAttributes.length; ++i) {
                                attribute = allNotifyAttributes[i];
                                if (attribute.set) {
                                    spy1 = jasmine.createSpy("spy1");
                                    spy2 = jasmine.createSpy("spy2");

                                    func1 = buildObserver(spy1);
                                    func2 = buildObserver(spy2);

                                    attribute.registerObserver(func1);
                                    attribute.registerObserver(func2);

                                    expect(spy1).not.toHaveBeenCalled();
                                    expect(spy2).not.toHaveBeenCalled();

                                    value = new ComplexRadioStation({
                                        name : "nameValue",
                                        station : "stationValue1",
                                        source : Country.AUSTRIA
                                    });
                                    attribute.set(value);

                                    expect(spy1).toHaveBeenCalled();
                                    expect(spy1).toHaveBeenCalledWith([ value
                                    ]);
                                    expect(spy2).toHaveBeenCalled();
                                    expect(spy2).toHaveBeenCalledWith([ value
                                    ]);

                                    attribute.unregisterObserver(func2);

                                    attribute.set(value);

                                    expect(spy1.callCount).toEqual(1);
                                    expect(spy2.callCount).toEqual(1);

                                    attribute.unregisterObserver(func1);
                                }
                            }
                        });

                        it(
                                "calls provided setter implementation with enum as operation argument",
                                function() {
                                    /*jslint nomen: true */
                                    var fixture =
                                            new ProviderAttribute(
                                                    {},
                                                    implementation,
                                                    "testWithEnumAsAttributeType",
                                                    TestEnum.ZERO._typeName,
                                                    "WRITEONLY");
                                    /*jslint nomen: false */

                                    fixture.set("ZERO");
                                    expect(implementation.set).toHaveBeenCalledWith(TestEnum.ZERO);
                                });

                    });

        }); // require
