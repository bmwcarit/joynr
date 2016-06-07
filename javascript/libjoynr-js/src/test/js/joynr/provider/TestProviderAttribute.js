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
                                            "Boolean",
                                            "NOTIFYREADWRITE");
                            isOnNotifyReadOnly =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnNotifyReadOnly",
                                            "Boolean",
                                            "NOTIFYREADONLY");
                            isOnNotifyWriteOnly =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnNotifyWriteOnly",
                                            "Boolean",
                                            "NOTIFYWRITEONLY");
                            isOnNotify =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnNotify",
                                            "Boolean",
                                            "NOTIFY");
                            isOnReadWrite =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnReadWrite",
                                            "Boolean",
                                            "READWRITE");
                            isOnReadOnly =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnReadOnly",
                                            "Boolean",
                                            "READONLY");
                            isOnWriteOnly =
                                    new ProviderAttribute(
                                            provider,
                                            implementation,
                                            "isOnWriteOnly",
                                            "Boolean",
                                            "WRITEONLY");

                            isOnProviderAttributeNotifyReadWrite =
                                    new ProviderAttributeNotifyReadWrite(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeNotifyReadWrite",
                                            "Boolean");
                            isOnProviderAttributeNotifyRead =
                                    new ProviderAttributeNotifyRead(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeNotifyRead",
                                            "Boolean");
                            isOnProviderAttributeNotifyWrite =
                                    new ProviderAttributeNotifyWrite(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeNotifyWrite",
                                            "Boolean");
                            isOnProviderAttributeNotify =
                                    new ProviderAttributeNotify(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeNotify",
                                            "Boolean");
                            isOnProviderAttributeReadWrite =
                                    new ProviderAttributeReadWrite(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeReadWrite",
                                            "Boolean");
                            isOnProviderAttributeRead =
                                    new ProviderAttributeRead(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeRead",
                                            "Boolean");
                            isOnProviderAttributeWrite =
                                    new ProviderAttributeWrite(
                                            provider,
                                            implementation,
                                            "isOnProviderAttributeWrite",
                                            "Boolean");

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

                        function checkRegisteredGettersAndSetters(attribute, i) {
                            var spy;
                            var result;
                            var done = false;
                            var testParam = "myTestParameter";

                            // only check getter if the attribute is readable
                            if (attribute.get instanceof Function) {
                                spy = jasmine.createSpy("ProviderAttributeSpy");
                                spy.andReturn(testParam);
                                attribute.registerGetter(spy);
                                result = attribute.get();
                                expect(spy).toHaveBeenCalled();
                                expect(result).toEqual([ testParam
                                ]);
                            }

                            // only check the setter if the attribute is writable
                            if (attribute.set instanceof Function) {
                                runs(function() {
                                    spy = jasmine.createSpy("ProviderAttributeSpy");
                                    attribute.registerSetter(spy);
                                    attribute.set(testParam).then(function() {
                                        done = true;
                                    });
                                });

                                waitsFor(function() {
                                    return done;
                                }, i + " setter was not called", 1000);

                                runs(function() {
                                    expect(spy).toHaveBeenCalled();
                                    expect(spy).toHaveBeenCalledWith(testParam);
                                });
                            }
                        }

                        it("call[G|S]etter calls through to registered [g|s]etters", function() {
                            var i;
                            for (i = 0; i < allAttributes.length; i++) {
                                checkRegisteredGettersAndSetters(allAttributes[i], i);
                            }
                        });

                        function checkImplementationGettersAndSetters(attribute, i) {
                            var spy;
                            var result;
                            var done = false;
                            var testParam = "myTestParameter";

                            // only check getter if the attribute is readable
                            if (attribute.get instanceof Function) {
                                implementation.get.reset();
                                implementation.get.andReturn(testParam);
                                expect(allAttributes[i].get()).toEqual([ testParam
                                ]);
                                expect(implementation.get).toHaveBeenCalled();
                            }

                            // only check the setter if the attribute is writable
                            if (attribute.set instanceof Function) {
                                runs(function() {
                                    implementation.set.reset();
                                    allAttributes[i].set(testParam).then(function() {
                                        done = true;
                                    });
                                });

                                waitsFor(function() {
                                    return done;
                                }, i + " setter was not called", 1000);

                                runs(function() {
                                    expect(implementation.set).toHaveBeenCalled();
                                    expect(implementation.set).toHaveBeenCalledWith(testParam);
                                });
                            }
                        }

                        it("call[G|S]etter calls through to provided implementation", function() {
                            for (i = 0; i < allAttributes.length; ++i) {
                                checkImplementationGettersAndSetters(allAttributes[i], i);
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

                        function setNewValueCallsValueChangedObserver(attribute) {
                            var spy1, spy2, func1, func2, value, done;
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

                            // expect 2 observers to be called
                            runs(function() {
                                done = false;
                                attribute.set(value).then(function() {
                                    done = true;
                                });
                            });

                            waitsFor(function() {
                                return done;
                            }, "setter was not called", 1000);

                            runs(function() {
                                expect(spy1).toHaveBeenCalled();
                                expect(spy1).toHaveBeenCalledWith([ value
                                ]);
                                expect(spy2).toHaveBeenCalled();
                                expect(spy2).toHaveBeenCalledWith([ value
                                ]);

                                // expect one observer to be called
                                attribute.unregisterObserver(func2);

                                value = new ComplexRadioStation({
                                    name : "nameValue2",
                                    station : "stationValue2",
                                    source : Country.AUSTRIA
                                });

                                done = false;
                                attribute.set(value).then(function() {
                                    done = true;
                                });
                            });

                            waitsFor(function() {
                                return done;
                            }, "setter was not called", 1000);

                            runs(function() {
                                expect(spy1.callCount).toEqual(2);
                                expect(spy2.callCount).toEqual(1);

                                // expect no observers to be called, as none are registered
                                attribute.unregisterObserver(func1);

                                value = new ComplexRadioStation({
                                    name : "nameValue3",
                                    station : "stationValue3",
                                    source : Country.AUSTRALIA
                                });

                                done = false;
                                attribute.set(value).then(function() {
                                    done = true;
                                });
                            });

                            waitsFor(function() {
                                return done;
                            }, "setter was not called", 1000);

                            runs(function() {
                                expect(spy1.callCount).toEqual(2);
                                expect(spy2.callCount).toEqual(1);
                            });

                        }

                        it("notifies observer when calling set with new value", function() {
                            var i, attribute;

                            for (i = 0; i < allNotifyAttributes.length; ++i) {
                                attribute = allNotifyAttributes[i];
                                if (attribute.set) {
                                    setNewValueCallsValueChangedObserver(attribute);
                                }
                            }
                        });

                        function setSameValueDoesNotCallValueChangedObserver(attribute) {
                            var spy1, spy2, func1, func2, value, done;
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

                            // expect 2 observers to be called
                            runs(function() {
                                done = false;
                                attribute.set(value).then(function() {
                                    done = true;
                                });
                            });

                            waitsFor(function() {
                                return done;
                            }, "setter was not called", 1000);

                            runs(function() {
                                expect(spy1).toHaveBeenCalled();
                                expect(spy1).toHaveBeenCalledWith([ value
                                ]);
                                expect(spy2).toHaveBeenCalled();
                                expect(spy2).toHaveBeenCalledWith([ value
                                ]);
                            });

                            // expect observers not to be called, as setting same value
                            runs(function() {
                                done = false;
                                attribute.set(value).then(function() {
                                    done = true;
                                });
                            });

                            waitsFor(function() {
                                return done;
                            }, "setter was not called", 1000);

                            runs(function() {
                                expect(spy1.callCount).toEqual(1);
                                expect(spy2.callCount).toEqual(1);
                            });
                        }

                        it("doesn't notify observer when calling set with same values", function() {
                            var i, spy1, spy2, attribute, func1, func2, value;

                            for (i = 0; i < allNotifyAttributes.length; ++i) {
                                attribute = allNotifyAttributes[i];
                                if (attribute.set) {
                                    setSameValueDoesNotCallValueChangedObserver(attribute);
                                }
                            }
                        });

                        it(
                                "calls provided setter implementation with enum as operation argument",
                                function() {
                                    var done;
                                    /*jslint nomen: true */
                                    var fixture =
                                            new ProviderAttribute(
                                                    {},
                                                    implementation,
                                                    "testWithEnumAsAttributeType",
                                                    TestEnum.ZERO._typeName,
                                                    "WRITEONLY");
                                    /*jslint nomen: false */

                                    runs(function() {
                                        fixture.set("ZERO").then(function() {
                                            done = true;
                                        });
                                    });

                                    waitsFor(function() {
                                        return done;
                                    }, " setter was not called", 1000);

                                    runs(function() {
                                        expect(implementation.set).toHaveBeenCalledWith(
                                                TestEnum.ZERO);
                                    });
                                });
                    });

        }); // require
