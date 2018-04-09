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
const ProviderEvent = require("../../../../main/js/joynr/provider/ProviderEvent");
const ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");
const BroadcastFilterParameters = require("../../../../main/js/joynr/proxy/BroadcastFilterParameters");

const safetyTimeoutDelta = 100;

describe("libjoynr-js.joynr.provider.ProviderEvent", () => {
    let weakSignal;

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

    function buildObserver(spy) {
        return function(data) {
            spy(data);
        };
    }

    it("implements the observer concept correctly", done => {
        let i,
            spy1,
            spy2,
            attribute,
            func1,
            func2,
            value = {
                key: "value",
                1: 2,
                object: {}
            };

        spy1 = jasmine.createSpy("spy1");
        spy2 = jasmine.createSpy("spy2");

        func1 = buildObserver(spy1);
        func2 = buildObserver(spy2);

        weakSignal.registerObserver(func1);
        weakSignal.registerObserver(func2);

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

        weakSignal.unregisterObserver(func2);

        weakSignal.fire(value);

        expect(spy1.calls.count()).toEqual(2);
        expect(spy2.calls.count()).toEqual(1);

        weakSignal.unregisterObserver(func1);

        weakSignal.fire(value);

        expect(spy1.calls.count()).toEqual(2);
        expect(spy2.calls.count()).toEqual(1);
        done();
    });

    function isPartOfList(list, data) {
        return list.indexOf(data) !== -1;
    }

    function buildObserver2(spy) {
        return function(data) {
            const filters = data.filters;
            const broadcastOutputParameters = data.broadcastOutputParameters;
            const filterParameters = {};
            let i;

            for (i = 0; i < filters.length; i++) {
                filters[i](broadcastOutputParameters, filterParameters);
            }
            spy(data);
        };
    }

    function buildObserver3(spy) {
        return function(broadcastOutputParameters, filterParameters) {
            spy(broadcastOutputParameters, filterParameters);
        };
    }

    it("implements the broadcast filter list correctly", done => {
        let i, spy1, spy2, attribute, observerFunc, filterFunc;
        const value = {
            key: "value",
            1: 2,
            object: {}
        };
        let spy3;

        spy1 = jasmine.createSpy("spy1");
        spy2 = jasmine.createSpy("spy2");

        observerFunc = buildObserver2(spy1);
        filterFunc = buildObserver3(spy2);

        weakSignal.addBroadcastFilter(filterFunc);
        weakSignal.registerObserver(observerFunc);

        expect(spy1).not.toHaveBeenCalled();
        expect(spy2).not.toHaveBeenCalled();

        const data = {
            broadcastOutputParameters: value,
            filters: [filterFunc],
            partitions: []
        };
        const filterParameters = {};

        weakSignal.fire(value);

        expect(spy1).toHaveBeenCalledWith(data);
        expect(spy2).toHaveBeenCalledWith(value, filterParameters);
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

    it("checkFilterParameters works", done => {
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
        done();
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
