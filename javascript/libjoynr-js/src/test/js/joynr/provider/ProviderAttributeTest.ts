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

import {
    ProviderReadAttribute,
    ProviderReadWriteNotifyAttribute,
    ProviderReadNotifyAttribute,
    ProviderReadWriteAttribute
} from "../../../../main/js/joynr/provider/ProviderAttribute";
import TestEnum from "../../../generated/joynr/tests/testTypes/TestEnum";
import ComplexRadioStation from "../../../generated/joynr/datatypes/exampleTypes/ComplexRadioStation";
import Country from "../../../generated/joynr/datatypes/exampleTypes/Country";
import TypeRegistrySingleton from "../../../../main/js/joynr/types/TypeRegistrySingleton";

describe("libjoynr-js.joynr.provider.ProviderAttribute", () => {
    let implementation: any;
    let isOn: ProviderReadWriteNotifyAttribute<boolean>;
    let isOnNotifyReadOnly: ProviderReadNotifyAttribute<boolean>;
    let isOnReadWrite: ProviderReadWriteAttribute<boolean>;
    let isOnReadOnly: ProviderReadAttribute<boolean>;
    let allAttributes: any[], allNotifyAttributes: any[];

    beforeEach(() => {
        implementation = {
            value: {
                key: "value",
                1: 0,
                object: {}
            },
            get: jest.fn().mockImplementation(() => {
                return implementation.value;
            }),
            set: jest.fn().mockImplementation((newValue: any) => {
                implementation.value = newValue;
            })
        };
        const provider = {};

        isOn = new ProviderReadWriteNotifyAttribute(provider, implementation, "isOn", "Boolean");
        isOnNotifyReadOnly = new ProviderReadNotifyAttribute<boolean>(
            provider,
            implementation,
            "isOnNotifyReadOnly",
            "Boolean"
        );
        isOnReadWrite = new ProviderReadWriteAttribute(provider, implementation, "isOnReadWrite", "Boolean");
        isOnReadOnly = new ProviderReadAttribute(provider, implementation, "isOnReadOnly", "Boolean");

        allAttributes = [isOn, isOnNotifyReadOnly, isOnReadWrite, isOnReadOnly];

        allNotifyAttributes = [isOn, isOnNotifyReadOnly];
        TypeRegistrySingleton.getInstance().addType(TestEnum);
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
        // @ts-ignore
        expect(isOnNotifyReadOnly.registerSetter).toBeUndefined();
        expect(isOnNotifyReadOnly.valueChanged).toBeDefined();
        expect(isOnNotifyReadOnly.registerObserver).toBeDefined();
        expect(isOnNotifyReadOnly.unregisterObserver).toBeDefined();
        done();
    });

    it("has correct members (ProviderAttribute with READWRITE)", done => {
        expect(isOnReadWrite.registerGetter).toBeDefined();
        expect(isOnReadWrite.registerSetter).toBeDefined();
        // @ts-ignore
        expect(isOnReadWrite.valueChanged).toBeUndefined();
        // @ts-ignore
        expect(isOnReadWrite.registerObserver).toBeUndefined();
        // @ts-ignore
        expect(isOnReadWrite.unregisterObserver).toBeUndefined();
        done();
    });

    it("has correct members (ProviderAttribute with READONLY)", done => {
        expect(isOnReadOnly.registerGetter).toBeDefined();
        // @ts-ignore
        expect(isOnReadOnly.registerSetter).toBeUndefined();
        // @ts-ignore
        expect(isOnReadOnly.valueChanged).toBeUndefined();
        // @ts-ignore
        expect(isOnReadOnly.registerObserver).toBeUndefined();
        // @ts-ignore
        expect(isOnReadOnly.unregisterObserver).toBeUndefined();
        done();
    });

    it("call[G|S]etter calls through to registered [g|s]etters", done => {
        const testParam = "myTestParameter";
        let promiseChain: any;

        const createFunc = function(attribute: any, promiseChain: Promise<any>) {
            const spy = jest.fn();
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
            allAttributes.map((attribute: any) => {
                if (attribute.get instanceof Function) {
                    const spy = jest.fn();
                    spy.mockReturnValue(testParam);
                    attribute.registerGetter(spy);
                    return attribute.get().then((result: any) => {
                        expect(spy).toHaveBeenCalled();
                        expect(result).toEqual([testParam]);
                    });
                }
            })
        );

        for (let i = 0; i < allAttributes.length; ++i) {
            if (allAttributes[i].set instanceof Function) {
                promiseChain = createFunc(allAttributes[i], promiseChain);
            }
        }
        promiseChain
            .then(() => {
                done();
                return null;
            })
            .catch(() => done.fail());
    });

    it("call[G|S]etter calls through to provided implementation", async () => {
        const testParam = "myTestParameter";

        await Promise.all(
            allAttributes.map(async (attribute: any) => {
                // only check getter if the attribute is readable
                if (attribute.get instanceof Function) {
                    implementation.get.mockClear();
                    implementation.get.mockReturnValue(testParam);

                    const result = await attribute.get();
                    expect(implementation.get).toHaveBeenCalled();
                    expect(result).toEqual([testParam]);
                }
            })
        );

        for (let i = 0; i < allAttributes.length; ++i) {
            if (allAttributes[i].set instanceof Function) {
                const attribute = allAttributes[i];
                implementation.set.mockClear();
                await attribute.set(testParam);
                expect(implementation.set).toHaveBeenCalled();
                expect(implementation.set).toHaveBeenCalledWith(testParam);
            }
        }
    });

    it("implements the observer concept correctly", () => {
        const value = {
            key: "value",
            1: 2,
            object: {}
        };

        for (let i = 0; i < allNotifyAttributes.length; ++i) {
            const attribute = allNotifyAttributes[i];

            const spy1 = jest.fn();
            const spy2 = jest.fn();

            attribute.registerObserver(spy1);
            attribute.registerObserver(spy2);

            expect(spy1).not.toHaveBeenCalled();
            expect(spy2).not.toHaveBeenCalled();

            attribute.valueChanged(value);

            expect(spy1).toHaveBeenCalled();
            expect(spy1).toHaveBeenCalledWith([value]);
            expect(spy2).toHaveBeenCalled();
            expect(spy2).toHaveBeenCalledWith([value]);

            attribute.unregisterObserver(spy2);

            attribute.valueChanged(value);

            expect(spy1.mock.calls.length).toEqual(2);
            expect(spy2.mock.calls.length).toEqual(1);

            attribute.unregisterObserver(spy1);

            attribute.valueChanged(value);

            expect(spy1.mock.calls.length).toEqual(2);
            expect(spy2.mock.calls.length).toEqual(1);
        }
    });

    function setNewValueCallsValueChangedObserver(attribute: any, promiseChain: any) {
        const spy1 = jest.fn();
        const spy2 = jest.fn();

        attribute.registerObserver(spy1);
        attribute.registerObserver(spy2);

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
                attribute.unregisterObserver(spy2);

                value = new ComplexRadioStation({
                    name: "nameValue2",
                    station: "stationValue2",
                    source: Country.AUSTRIA
                });

                return attribute.set(value);
            })
            .then(() => {
                expect(spy1.mock.calls.length).toEqual(2);
                expect(spy2.mock.calls.length).toEqual(1);

                // expect no observers to be called, as none are registered
                attribute.unregisterObserver(spy1);

                value = new ComplexRadioStation({
                    name: "nameValue3",
                    station: "stationValue3",
                    source: Country.AUSTRALIA
                });

                return attribute.set(value);
            })
            .then(() => {
                expect(spy1.mock.calls.length).toEqual(2);
                expect(spy2.mock.calls.length).toEqual(1);
            });
    }

    it("notifies observer when calling set with new value", async () => {
        const promiseChain = Promise.resolve();

        for (let i = 0; i < allNotifyAttributes.length; ++i) {
            const attribute = allNotifyAttributes[i];
            if (attribute.set) {
                await setNewValueCallsValueChangedObserver(attribute, promiseChain);
            }
        }
    });

    async function setSameValueDoesNotCallValueChangedObserver(attribute: any) {
        const spy1 = jest.fn();
        const spy2 = jest.fn();

        attribute.registerObserver(spy1);
        attribute.registerObserver(spy2);

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

        expect(spy1.mock.calls.length).toEqual(1);
        expect(spy2.mock.calls.length).toEqual(1);
    }

    it("doesn't notify observer when calling set with same values", async () => {
        for (let i = 0; i < allNotifyAttributes.length; ++i) {
            const attribute = allNotifyAttributes[i];
            if (attribute.set) {
                await setSameValueDoesNotCallValueChangedObserver(attribute);
            }
        }
    });

    it("calls provided setter implementation with enum as operation argument", () => {
        const fixture = new ProviderReadWriteAttribute(
            {},
            implementation,
            "testWithEnumAsAttributeType",
            TestEnum.ZERO._typeName
        );

        return fixture.set("ZERO").then(() => {
            expect(implementation.set).toHaveBeenCalledWith(TestEnum.ZERO);
        });
    });
});
