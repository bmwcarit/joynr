/*jslint es5: true, node: true, node: true */
/*global fail: true */
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
var ProviderAttribute = require('../../../classes/joynr/provider/ProviderAttribute');
var ProviderAttributeNotifyReadWrite = require('../../../classes/joynr/provider/ProviderAttributeNotifyReadWrite');
var ProviderAttributeNotifyRead = require('../../../classes/joynr/provider/ProviderAttributeNotifyRead');
var ProviderAttributeNotifyWrite = require('../../../classes/joynr/provider/ProviderAttributeNotifyWrite');
var ProviderAttributeNotify = require('../../../classes/joynr/provider/ProviderAttributeNotify');
var ProviderAttributeReadWrite = require('../../../classes/joynr/provider/ProviderAttributeReadWrite');
var ProviderAttributeRead = require('../../../classes/joynr/provider/ProviderAttributeRead');
var ProviderAttributeWrite = require('../../../classes/joynr/provider/ProviderAttributeWrite');
var ProviderQos = require('../../../classes/joynr/types/ProviderQos');
var TestEnum = require('../../../test-classes/joynr/tests/testTypes/TestEnum');
var ComplexRadioStation = require('../../../test-classes/joynr/datatypes/exampleTypes/ComplexRadioStation');
var Country = require('../../../test-classes/joynr/datatypes/exampleTypes/Country');
var Promise = require('../../../classes/global/Promise');
var WaitsFor = require('../../../test-classes/global/WaitsFor');
module.exports = (function (ProviderAttribute, ProviderAttributeNotifyReadWrite, ProviderAttributeNotifyRead, ProviderAttributeNotifyWrite, ProviderAttributeNotify, ProviderAttributeReadWrite, ProviderAttributeRead, ProviderAttributeWrite, ProviderQos, TestEnum, ComplexRadioStation, Country, Promise, waitsFor) {

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

                        beforeEach(function(done) {
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
                            spyOn(implementation, "get").and.callThrough();
                            spyOn(implementation, "set").and.callThrough();
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
                            done();
                        });

                        it("is of correct type (ProviderAttribute)", function(done) {
                            expect(isOn).toBeDefined();
                            expect(isOn).not.toBeNull();
                            expect(typeof isOn === "object").toBeTruthy();
                            expect(isOn instanceof ProviderAttribute).toBeTruthy();
                            done();
                        });

                        it(
                                "has correct members (ProviderAttribute with NOTIFYREADWRITE)",
                                function(done) {
                                    expect(isOn.registerGetter).toBeDefined();
                                    expect(isOn.registerSetter).toBeDefined();
                                    expect(isOn.valueChanged).toBeDefined();
                                    expect(isOn.registerObserver).toBeDefined();
                                    expect(isOn.unregisterObserver).toBeDefined();
                                    done();
                                });

                        it(
                                "has correct members (ProviderAttribute with NOTIFYREADONLY)",
                                function(done) {
                                    expect(isOnNotifyReadOnly.registerGetter).toBeDefined();
                                    expect(isOnNotifyReadOnly.registerSetter).toBeUndefined();
                                    expect(isOnNotifyReadOnly.valueChanged).toBeDefined();
                                    expect(isOnNotifyReadOnly.registerObserver).toBeDefined();
                                    expect(isOnNotifyReadOnly.unregisterObserver).toBeDefined();
                                    done();
                                });

                        it(
                                "has correct members (ProviderAttribute with NOTIFYWRITEONLY)",
                                function(done) {
                                    expect(isOnNotifyWriteOnly.registerGetter).toBeUndefined();
                                    expect(isOnNotifyWriteOnly.registerSetter).toBeDefined();
                                    expect(isOnNotifyWriteOnly.valueChanged).toBeDefined();
                                    expect(isOnNotifyWriteOnly.registerObserver).toBeDefined();
                                    expect(isOnNotifyWriteOnly.unregisterObserver).toBeDefined();
                                    done();
                                });

                        it("has correct members (ProviderAttribute with NOTIFY)", function(done) {
                            expect(isOnNotify.registerGetter).toBeUndefined();
                            expect(isOnNotify.registerSetter).toBeUndefined();
                            expect(isOnNotify.valueChanged).toBeDefined();
                            expect(isOnNotify.registerObserver).toBeDefined();
                            expect(isOnNotify.unregisterObserver).toBeDefined();
                            done();
                        });

                        it("has correct members (ProviderAttribute with READWRITE)", function(done) {
                            expect(isOnReadWrite.registerGetter).toBeDefined();
                            expect(isOnReadWrite.registerSetter).toBeDefined();
                            expect(isOnReadWrite.valueChanged).toBeUndefined();
                            expect(isOnReadWrite.registerObserver).toBeUndefined();
                            expect(isOnReadWrite.unregisterObserver).toBeUndefined();
                            done();
                        });

                        it("has correct members (ProviderAttribute with READONLY)", function(done) {
                            expect(isOnReadOnly.registerGetter).toBeDefined();
                            expect(isOnReadOnly.registerSetter).toBeUndefined();
                            expect(isOnReadOnly.valueChanged).toBeUndefined();
                            expect(isOnReadOnly.registerObserver).toBeUndefined();
                            expect(isOnReadOnly.unregisterObserver).toBeUndefined();
                            done();
                        });

                        it("has correct members (ProviderAttribute with WRITEONLY)", function(done) {
                            expect(isOnWriteOnly.registerGetter).toBeUndefined();
                            expect(isOnWriteOnly.registerSetter).toBeDefined();
                            expect(isOnWriteOnly.valueChanged).toBeUndefined();
                            expect(isOnWriteOnly.registerObserver).toBeUndefined();
                            expect(isOnWriteOnly.unregisterObserver).toBeUndefined();
                            done();
                        });

                        it(
                                "has correct members (ProviderAttributeNotifyReadWrite)",
                                function(done) {
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
                                    done();
                                });

                        it(
                                "has correct members (ProviderAttributeNotifyRead)",
                                function(done) {
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
                                    done();
                                });

                        it(
                                "has correct members (ProviderAttributeNotifyWrite)",
                                function(done) {
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
                                    done();
                                });

                        it("has correct members (ProviderAttributeNotify)", function(done) {
                            expect(isOnProviderAttributeNotify instanceof ProviderAttributeNotify)
                                    .toBeTruthy();
                            expect(isOnProviderAttributeNotify.registerGetter).toBeUndefined();
                            expect(isOnProviderAttributeNotify.registerSetter).toBeUndefined();
                            expect(isOnProviderAttributeNotify.valueChanged).toBeDefined();
                            expect(isOnProviderAttributeNotify.registerObserver).toBeDefined();
                            expect(isOnProviderAttributeNotify.unregisterObserver).toBeDefined();
                            done();
                        });

                        it(
                                "has correct members (ProviderAttributeReadWrite)",
                                function(done) {
                                    expect(
                                            isOnProviderAttributeReadWrite instanceof ProviderAttributeReadWrite)
                                            .toBeTruthy();
                                    expect(isOnProviderAttributeReadWrite.registerGetter)
                                            .toBeDefined();
                                    expect(isOnProviderAttributeReadWrite.registerSetter)
                                            .toBeDefined();
                                    done();
                                });

                        it("has correct members (ProviderAttributeRead)", function(done) {
                            expect(isOnProviderAttributeRead instanceof ProviderAttributeRead)
                                    .toBeTruthy();
                            expect(isOnProviderAttributeRead.registerGetter).toBeDefined();
                            expect(isOnProviderAttributeRead.registerSetter).toBeUndefined();
                            done();
                        });

                        it("has correct members (ProviderAttributeWrite)", function(done) {
                            expect(isOnProviderAttributeWrite instanceof ProviderAttributeWrite)
                                    .toBeTruthy();
                            expect(isOnProviderAttributeWrite.registerGetter).toBeUndefined();
                            expect(isOnProviderAttributeWrite.registerSetter).toBeDefined();
                            done();
                        });

                        it("call[G|S]etter calls through to registered [g|s]etters", function(done) {
                            var result;
                            var testParam = "myTestParameter";
                            var promiseChain = Promise.resolve();

                            var createFunc = function(attribute, promiseChain) {
                                var spy = jasmine.createSpy("ProviderAttributeSpy");
                                attribute.registerSetter(spy);
                                return promiseChain.then(function() {
                                    return attribute.set(testParam);
                                }).then(function() {
                                    expect(spy).toHaveBeenCalled();
                                    expect(spy).toHaveBeenCalledWith(testParam);
                                });
                            };

                            for (i = 0; i < allAttributes.length; ++i) {
                                // only check getter if the attribute is readable
                                if (allAttributes[i].get instanceof Function) {
                                    var spy = jasmine.createSpy("ProviderAttributeSpy");
                                    spy.and.returnValue(testParam);
                                    allAttributes[i].registerGetter(spy);
                                    result = allAttributes[i].get();
                                    expect(spy).toHaveBeenCalled();
                                    expect(result).toEqual([ testParam
                                    ]);
                                }

                                if (allAttributes[i].set instanceof Function) {
                                    promiseChain = createFunc(allAttributes[i], promiseChain);
                                }
                            }
                            promiseChain.then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("call[G|S]etter calls through to provided implementation", function(done) {
                            var spy;
                            var result;
                            var testParam = "myTestParameter";
                            var promiseChain = Promise.resolve();

                            var createFunc = function(attribute, promiseChain) {
                                return promiseChain.then(function() {
                                    implementation.set.calls.reset();
                                    return attribute.set(testParam);
                                }).then(function() {
                                    expect(implementation.set).toHaveBeenCalled();
                                    expect(implementation.set).toHaveBeenCalledWith(testParam);
                                });
                            };

                            for (i = 0; i < allAttributes.length; ++i) {
                                // only check getter if the attribute is readable
                                if (allAttributes[i].get instanceof Function) {
                                    implementation.get.calls.reset();
                                    implementation.get.and.returnValue(testParam);
                                    expect(allAttributes[i].get()).toEqual([ testParam
                                    ]);
                                    expect(implementation.get).toHaveBeenCalled();
                                }

                                if (allAttributes[i].set instanceof Function) {
                                    promiseChain = createFunc(allAttributes[i], promiseChain);
                                }
                            }
                            promiseChain.then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        function buildObserver(spy) {
                            return function(value) {
                                spy(value);
                            };
                        }

                        it("implements the observer concept correctly", function(done) {
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

                                expect(spy1.calls.count()).toEqual(2);
                                expect(spy2.calls.count()).toEqual(1);

                                attribute.unregisterObserver(func1);

                                attribute.valueChanged(value);

                                expect(spy1.calls.count()).toEqual(2);
                                expect(spy2.calls.count()).toEqual(1);
                            }
                            done();
                        });

                        function setNewValueCallsValueChangedObserver(attribute, promiseChain) {
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
                            return promiseChain.then(function() {
                                return attribute.set(value);
                            }).then(function() {
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

                                return attribute.set(value);
                            }).then(function() {
                                expect(spy1.calls.count()).toEqual(2);
                                expect(spy2.calls.count()).toEqual(1);

                                // expect no observers to be called, as none are registered
                                attribute.unregisterObserver(func1);

                                value = new ComplexRadioStation({
                                    name : "nameValue3",
                                    station : "stationValue3",
                                    source : Country.AUSTRALIA
                                });

                                return attribute.set(value);
                            }).then(function() {
                                expect(spy1.calls.count()).toEqual(2);
                                expect(spy2.calls.count()).toEqual(1);
                            });
                        }

                        it("notifies observer when calling set with new value", function(done) {
                            var i, attribute;
                            var promiseChain = Promise.resolve();

                            for (i = 0; i < allNotifyAttributes.length; ++i) {
                                attribute = allNotifyAttributes[i];
                                if (attribute.set) {
                                    promiseChain = setNewValueCallsValueChangedObserver(attribute, promiseChain);
                                }
                            }
                            promiseChain.then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        function setSameValueDoesNotCallValueChangedObserver(attribute, promiseChain) {
                            var spy1, spy2, func1, func2, value;
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
                            return promiseChain.then(function() {
                                return attribute.set(value);
                            }).then(function() {
                                expect(spy1).toHaveBeenCalled();
                                expect(spy1).toHaveBeenCalledWith([ value
                                ]);
                                expect(spy2).toHaveBeenCalled();
                                expect(spy2).toHaveBeenCalledWith([ value
                                ]);
                                return attribute.set(value);
                            }).then(function() {
                                expect(spy1.calls.count()).toEqual(1);
                                expect(spy2.calls.count()).toEqual(1);
                            });
                        }

                        it("doesn't notify observer when calling set with same values", function(done) {
                            var i, spy1, spy2, attribute, func1, func2, value;
                            var promiseChain = Promise.resolve();

                            for (i = 0; i < allNotifyAttributes.length; ++i) {
                                attribute = allNotifyAttributes[i];
                                if (attribute.set) {
                                    promiseChain = setSameValueDoesNotCallValueChangedObserver(attribute, promiseChain);
                                }
                            }
                            promiseChain.then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "calls provided setter implementation with enum as operation argument",
                                function(done) {
                                    /*jslint nomen: true */
                                    var fixture =
                                            new ProviderAttribute(
                                                    {},
                                                    implementation,
                                                    "testWithEnumAsAttributeType",
                                                    TestEnum.ZERO._typeName,
                                                    "WRITEONLY");
                                    /*jslint nomen: false */

                                    fixture.set("ZERO").then(function() {
                                        expect(implementation.set).toHaveBeenCalledWith(
                                                TestEnum.ZERO);
                                        done();
                                        return null;
                                    }).catch(fail);
                        });
                    });

}(ProviderAttribute, ProviderAttributeNotifyReadWrite, ProviderAttributeNotifyRead, ProviderAttributeNotifyWrite, ProviderAttributeNotify, ProviderAttributeReadWrite, ProviderAttributeRead, ProviderAttributeWrite, ProviderQos, TestEnum, ComplexRadioStation, Country, Promise, WaitsFor));    // require
