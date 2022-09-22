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

import ProviderEvent from "../../../../main/js/joynr/provider/ProviderEvent";
import BroadcastFilterParameters from "../../../../main/js/joynr/proxy/BroadcastFilterParameters";

describe("libjoynr-js.joynr.provider.ProviderEvent", () => {
    let weakSignal: any;

    beforeEach(() => {
        weakSignal = new ProviderEvent({
            eventName: "weakSignal",
            outputParameterProperties: [
                {
                    name: "weakSignalStation",
                    type: "String"
                }
            ],
            filterSettings: {
                a: "reservedForTypeInfo",
                b: "reservedForTypeInfo",
                c: "reservedForTypeInfo"
            }
        });
    });

    it("is of correct type", done => {
        expect(weakSignal).toBeDefined();
        expect(weakSignal).not.toBeNull();
        expect(typeof weakSignal === "object").toBeTruthy();
        expect(weakSignal instanceof ProviderEvent).toBeTruthy();
        done();
    });

    it("has correct members", done => {
        expect(weakSignal.createBroadcastOutputParameters).toBeDefined();
        expect(weakSignal.checkFilterParameters).toBeDefined();
        expect(weakSignal.fire).toBeDefined();
        expect(weakSignal.registerObserver).toBeDefined();
        expect(weakSignal.unregisterObserver).toBeDefined();
        expect(weakSignal.addBroadcastFilter).toBeDefined();
        expect(weakSignal.deleteBroadcastFilter).toBeDefined();
        done();
    });

    it("implements the observer concept correctly", done => {
        const value = {
            key: "value",
            1: 2,
            object: {}
        };

        const spy1 = jest.fn();
        const spy2 = jest.fn();

        weakSignal.registerObserver(spy1);
        weakSignal.registerObserver(spy2);

        expect(spy1).not.toHaveBeenCalled();
        expect(spy2).not.toHaveBeenCalled();

        weakSignal.fire(value);

        const data = {
            broadcastOutputParameters: value,
            filters: [],
            partitions: []
        };

        expect(spy1).toHaveBeenCalled();
        expect(spy1).toHaveBeenCalledWith(data);
        expect(spy2).toHaveBeenCalled();
        expect(spy2).toHaveBeenCalledWith(data);

        weakSignal.unregisterObserver(spy2);

        weakSignal.fire(value);

        expect(spy1.mock.calls.length).toEqual(2);
        expect(spy2.mock.calls.length).toEqual(1);

        weakSignal.unregisterObserver(spy1);

        weakSignal.fire(value);

        expect(spy1.mock.calls.length).toEqual(2);
        expect(spy2.mock.calls.length).toEqual(1);
        done();
    });

    it("implements the broadcast filter list correctly", done => {
        const value = {
            key: "value",
            1: 2,
            object: {}
        };

        const observerFunction = jest.fn();
        const filterObject = { filter: jest.fn() };

        observerFunction.mockImplementation((data: any) => {
            const filters = data.filters;
            const broadcastOutputParameters = data.broadcastOutputParameters;
            const filterParameters = {};
            let i: any;

            for (i = 0; i < filters.length; i++) {
                filters[i].filter(broadcastOutputParameters, filterParameters);
            }
        });

        weakSignal.addBroadcastFilter(filterObject);
        weakSignal.registerObserver(observerFunction);

        expect(observerFunction).not.toHaveBeenCalled();
        expect(filterObject.filter).not.toHaveBeenCalled();

        const data = {
            broadcastOutputParameters: value,
            filters: [filterObject],
            partitions: []
        };
        const filterParameters = {};

        weakSignal.fire(value);

        expect(observerFunction).toHaveBeenCalledWith(data);
        expect(filterObject.filter).toHaveBeenCalledWith(value, filterParameters);
        done();
    });

    it("fire works", done => {
        expect(weakSignal.fire).toBeDefined();
        expect(typeof weakSignal.fire === "function").toBeTruthy();
        expect(() => {
            const boc = weakSignal.createBroadcastOutputParameters();
            boc.setWeakSignalStation("Bayern 3");
            weakSignal.fire(boc);
        }).not.toThrow();
        done();
    });

    it("checkFilterParameters works", () => {
        const correctFilterParameters = new BroadcastFilterParameters({
            a: "String",
            b: "String",
            c: "String"
        });
        correctFilterParameters.setA("a");
        correctFilterParameters.setB("b");
        correctFilterParameters.setC("c");
        const missingOnefilterParameters = new BroadcastFilterParameters({
            a: "String",
            b: "String",
            c: "String"
        });
        missingOnefilterParameters.setA("a");
        missingOnefilterParameters.setB("b");
        const missingTwofilterParameters = new BroadcastFilterParameters({
            a: "String",
            b: "String",
            c: "String"
        });
        missingTwofilterParameters.setA("a");

        expect(weakSignal.checkFilterParameters).toBeDefined();
        expect(typeof weakSignal.checkFilterParameters === "function").toBeTruthy();

        expect(weakSignal.checkFilterParameters({}).caughtErrors.length).toBe(0);
        expect(weakSignal.checkFilterParameters().caughtErrors.length).toBe(0);
        expect(weakSignal.checkFilterParameters(null).caughtErrors.length).toBe(0);

        expect(weakSignal.checkFilterParameters(correctFilterParameters).caughtErrors.length).toBe(0);
        expect(weakSignal.checkFilterParameters(missingOnefilterParameters).caughtErrors.length).toBe(1);
        expect(weakSignal.checkFilterParameters(missingTwofilterParameters).caughtErrors.length).toBe(2);
    });
    it("throws error if fire is invoked with invalid partitions", () => {
        expect(() => {
            weakSignal.fire(weakSignal.createBroadcastOutputParameters(), ["_"]);
        }).toThrow();
        expect(() => {
            weakSignal.fire(weakSignal.createBroadcastOutputParameters(), ["./$"]);
        }).toThrow();
    });
});
