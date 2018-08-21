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
const ProviderAttribute = require("../../../../main/js/joynr/provider/ProviderAttribute");
const TestEnum = require("../../../generated/joynr/tests/testTypes/TestEnum");
const ComplexRadioStation = require("../../../generated/joynr/datatypes/exampleTypes/ComplexRadioStation");
const Country = require("../../../generated/joynr/datatypes/exampleTypes/Country");

describe("libjoynr-js.joynr.provider.ProviderAttribute", () => {
    let i, implementation, isOn, isOnNotifyReadOnly, isOnNotifyWriteOnly;
    let isOnNotify, isOnReadWrite, isOnReadOnly, isOnWriteOnly;
    let allAttributes, allNotifyAttributes;

    beforeEach(done => {
        implementation = {
            value: {
                key: "value",
                1: 0,
                object: {}
            },
            get() {
                return implementation.value;
            },
            set(newValue) {
                implementation.value = newValue;
            }
        };
        spyOn(implementation, "get").and.callThrough();
        spyOn(implementation, "set").and.callThrough();
        const provider = {};

        isOn = new ProviderAttribute(provider, implementation, "isOn", "Boolean", "NOTIFYREADWRITE");
        isOnNotifyReadOnly = new ProviderAttribute(
            provider,
            implementation,
            "isOnNotifyReadOnly",
            "Boolean",
            "NOTIFYREADONLY"
        );
        isOnNotifyWriteOnly = new ProviderAttribute(
            provider,
            implementation,
            "isOnNotifyWriteOnly",
            "Boolean",
            "NOTIFYWRITEONLY"
        );
        isOnNotify = new ProviderAttribute(provider, implementation, "isOnNotify", "Boolean", "NOTIFY");
        isOnReadWrite = new ProviderAttribute(provider, implementation, "isOnReadWrite", "Boolean", "READWRITE");
        isOnReadOnly = new ProviderAttribute(provider, implementation, "isOnReadOnly", "Boolean", "READONLY");
        isOnWriteOnly = new ProviderAttribute(provider, implementation, "isOnWriteOnly", "Boolean", "WRITEONLY");

        allAttributes = [
            isOn,
            isOnNotifyReadOnly,
            isOnNotifyWriteOnly,
            isOnNotify,
            isOnReadWrite,
            isOnReadOnly,
            isOnWriteOnly
        ];

        allNotifyAttributes = [isOn, isOnNotifyReadOnly, isOnNotifyWriteOnly, isOnNotify];
        done();
    });

    it("got initialized", done => {
        expect(isOn).toBeDefined();
        expect(isOn).not.toBeNull();
        expect(typeof isOn === "object").toBeTruthy();
        done();
    });

    it("has correct members (ProviderAttribute with NOTIFYREADWRITE)", done => {
        expect(isOn.registerGetter).toBeDefined();
        expect(isOn.registerSetter).toBeDefined();
        expect(isOn.valueChanged).toBeDefined();
        expect(isOn.registerObserver).toBeDefined();
        expect(isOn.unregisterObserver).toBeDefined();
        done();
    });

    it("has correct members (ProviderAttribute with NOTIFYREADONLY)", done => {
        expect(isOnNotifyReadOnly.registerGetter).toBeDefined();
        expect(isOnNotifyReadOnly.registerSetter).toBeUndefined();
        expect(isOnNotifyReadOnly.valueChanged).toBeDefined();
        expect(isOnNotifyReadOnly.registerObserver).toBeDefined();
        expect(isOnNotifyReadOnly.unregisterObserver).toBeDefined();
        done();
    });

    it("has correct members (ProviderAttribute with NOTIFYWRITEONLY)", done => {
        expect(isOnNotifyWriteOnly.registerGetter).toBeUndefined();
        expect(isOnNotifyWriteOnly.registerSetter).toBeDefined();
        expect(isOnNotifyWriteOnly.valueChanged).toBeDefined();
        expect(isOnNotifyWriteOnly.registerObserver).toBeDefined();
        expect(isOnNotifyWriteOnly.unregisterObserver).toBeDefined();
        done();
    });

    it("has correct members (ProviderAttribute with NOTIFY)", done => {
        expect(isOnNotify.registerGetter).toBeUndefined();
        expect(isOnNotify.registerSetter).toBeUndefined();
        expect(isOnNotify.valueChanged).toBeDefined();
        expect(isOnNotify.registerObserver).toBeDefined();
        expect(isOnNotify.unregisterObserver).toBeDefined();
        done();
    });

    it("has correct members (ProviderAttribute with READWRITE)", done => {
        expect(isOnReadWrite.registerGetter).toBeDefined();
        expect(isOnReadWrite.registerSetter).toBeDefined();
        expect(isOnReadWrite.valueChanged).toBeUndefined();
        expect(isOnReadWrite.registerObserver).toBeUndefined();
        expect(isOnReadWrite.unregisterObserver).toBeUndefined();
        done();
    });

    it("has correct members (ProviderAttribute with READONLY)", done => {
        expect(isOnReadOnly.registerGetter).toBeDefined();
        expect(isOnReadOnly.registerSetter).toBeUndefined();
        expect(isOnReadOnly.valueChanged).toBeUndefined();
        expect(isOnReadOnly.registerObserver).toBeUndefined();
        expect(isOnReadOnly.unregisterObserver).toBeUndefined();
        done();
    });

    it("has correct members (ProviderAttribute with WRITEONLY)", done => {
        expect(isOnWriteOnly.registerGetter).toBeUndefined();
        expect(isOnWriteOnly.registerSetter).toBeDefined();
        expect(isOnWriteOnly.valueChanged).toBeUndefined();
        expect(isOnWriteOnly.registerObserver).toBeUndefined();
        expect(isOnWriteOnly.unregisterObserver).toBeUndefined();
        done();
    });

    it("call[G|S]etter calls through to registered [g|s]etters", done => {
        const testParam = "myTestParameter";
        let promiseChain;

        const createFunc = function(attribute, promiseChain) {
            const spy = jasmine.createSpy("ProviderAttributeSpy");
            attribute.registerSetter(spy);
            return promiseChain
                .then(() => {
                    return attribute.set(testParam);
                })
                .then(() => {
                    expect(spy).toHaveBeenCalled();
                    expect(spy).toHaveBeenCalledWith(testParam);
                });
        };

        promiseChain = Promise.all(
            allAttributes.map(attribute => {
                if (attribute.get instanceof Function) {
                    const spy = jasmine.createSpy("ProviderAttributeSpy");
                    spy.and.returnValue(testParam);
                    attribute.registerGetter(spy);
                    return attribute.get().then(result => {
                        expect(spy).toHaveBeenCalled();
                        expect(result).toEqual([testParam]);
                    });
                }
            })
        );

        for (i = 0; i < allAttributes.length; ++i) {
            if (allAttributes[i].set instanceof Function) {
                promiseChain = createFunc(allAttributes[i], promiseChain);
            }
        }
        promiseChain
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("call[G|S]etter calls through to provided implementation", done => {
        const testParam = "myTestParameter";
        let promiseChain = Promise.resolve();

        const createFunc = function(attribute, promiseChain) {
            return promiseChain
                .then(() => {
                    implementation.set.calls.reset();
                    return attribute.set(testParam);
                })
                .then(() => {
                    expect(implementation.set).toHaveBeenCalled();
                    expect(implementation.set).toHaveBeenCalledWith(testParam);
                });
        };

        promiseChain = Promise.all(
            allAttributes.map(attribute => {
                // only check getter if the attribute is readable
                if (attribute.get instanceof Function) {
                    implementation.get.calls.reset();
                    implementation.get.and.returnValue(testParam);

                    return attribute.get().then(result => {
                        expect(implementation.get).toHaveBeenCalled();
                        expect(result).toEqual([testParam]);
                    });
                }
            })
        );

        for (i = 0; i < allAttributes.length; ++i) {
            if (allAttributes[i].set instanceof Function) {
                promiseChain = createFunc(allAttributes[i], promiseChain);
            }
        }
        promiseChain
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    function buildObserver(spy) {
        return function(value) {
            spy(value);
        };
    }

    it("implements the observer concept correctly", done => {
        let i, spy1, spy2, attribute, func1, func2;
        const value = {
            key: "value",
            1: 2,
            object: {}
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
            expect(spy1).toHaveBeenCalledWith([value]);
            expect(spy2).toHaveBeenCalled();
            expect(spy2).toHaveBeenCalledWith([value]);

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
        const spy1 = jasmine.createSpy("spy1");
        const spy2 = jasmine.createSpy("spy2");

        const func1 = buildObserver(spy1);
        const func2 = buildObserver(spy2);

        attribute.registerObserver(func1);
        attribute.registerObserver(func2);

        expect(spy1).not.toHaveBeenCalled();
        expect(spy2).not.toHaveBeenCalled();

        let value = new ComplexRadioStation({
            name: "nameValue",
            station: "stationValue",
            source: Country.GERMANY
        });

        // expect 2 observers to be called
        return promiseChain
            .then(() => {
                return attribute.set(value);
            })
            .then(() => {
                expect(spy1).toHaveBeenCalled();
                expect(spy1).toHaveBeenCalledWith([value]);
                expect(spy2).toHaveBeenCalled();
                expect(spy2).toHaveBeenCalledWith([value]);

                // expect one observer to be called
                attribute.unregisterObserver(func2);

                value = new ComplexRadioStation({
                    name: "nameValue2",
                    station: "stationValue2",
                    source: Country.AUSTRIA
                });

                return attribute.set(value);
            })
            .then(() => {
                expect(spy1.calls.count()).toEqual(2);
                expect(spy2.calls.count()).toEqual(1);

                // expect no observers to be called, as none are registered
                attribute.unregisterObserver(func1);

                value = new ComplexRadioStation({
                    name: "nameValue3",
                    station: "stationValue3",
                    source: Country.AUSTRALIA
                });

                return attribute.set(value);
            })
            .then(() => {
                expect(spy1.calls.count()).toEqual(2);
                expect(spy2.calls.count()).toEqual(1);
            });
    }

    it("notifies observer when calling set with new value", done => {
        let i, attribute;
        let promiseChain = Promise.resolve();

        for (i = 0; i < allNotifyAttributes.length; ++i) {
            attribute = allNotifyAttributes[i];
            if (attribute.set) {
                promiseChain = setNewValueCallsValueChangedObserver(attribute, promiseChain);
            }
        }
        promiseChain
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    async function setSameValueDoesNotCallValueChangedObserver(attribute) {
        const spy1 = jasmine.createSpy("spy1");
        const spy2 = jasmine.createSpy("spy2");

        const func1 = buildObserver(spy1);
        const func2 = buildObserver(spy2);

        attribute.registerObserver(func1);
        attribute.registerObserver(func2);

        expect(spy1).not.toHaveBeenCalled();
        expect(spy2).not.toHaveBeenCalled();

        const value = new ComplexRadioStation({
            name: "nameValue",
            station: "stationValue",
            source: Country.GERMANY
        });

        // expect 2 observers to be called

        await attribute.set(value);

        expect(spy1).toHaveBeenCalled();
        expect(spy1).toHaveBeenCalledWith([value]);
        expect(spy2).toHaveBeenCalled();
        expect(spy2).toHaveBeenCalledWith([value]);
        await attribute.set(value);

        expect(spy1.calls.count()).toEqual(1);
        expect(spy2.calls.count()).toEqual(1);
    }

    it("doesn't notify observer when calling set with same values", async () => {
        let i, attribute;

        for (i = 0; i < allNotifyAttributes.length; ++i) {
            attribute = allNotifyAttributes[i];
            if (attribute.set) {
                await setSameValueDoesNotCallValueChangedObserver(attribute);
            }
        }
    });

    it("calls provided setter implementation with enum as operation argument", done => {
        const fixture = new ProviderAttribute(
            {},
            implementation,
            "testWithEnumAsAttributeType",
            TestEnum.ZERO._typeName,
            "WRITEONLY"
        );

        fixture
            .set("ZERO")
            .then(() => {
                expect(implementation.set).toHaveBeenCalledWith(TestEnum.ZERO);
                done();
                return null;
            })
            .catch(fail);
    });
});
