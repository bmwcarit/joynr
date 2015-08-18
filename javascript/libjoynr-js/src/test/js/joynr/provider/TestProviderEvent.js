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

//TODO: some of this relies on the dummy implementation, change accordingly when implementating
joynrTestRequire("joynr/provider/TestProviderEvent", [
    "joynr/provider/ProviderEvent",
    "joynr/types/ProviderQos"
], function(ProviderEvent, ProviderQos) {

    var safetyTimeoutDelta = 100;

    describe("libjoynr-js.joynr.provider.ProviderEvent", function() {

        var settings;
        var weakSignal;
        var weakSignalNotifyReadOnly;
        var weakSignalNotifyWriteOnly;
        var weakSignalNotify;
        var weakSignalReadWrite;
        var weakSignalReadOnly, weakSignalWriteOnly;
        var weakSignalProviderEventNotifyReadWrite;
        var weakSignalProviderEventNotifyRead;
        var weakSignalProviderEventNotifyWrite;
        var weakSignalProviderEventNotify;
        var weakSignalProviderEventReadWrite;
        var weakSignalProviderEventRead;
        var weakSignalProviderEventWrite;
        var implementation;
        var provider;

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

            settings = {
                providerQos : new ProviderQos({
                    version : 123,
                    priority : 1234
                })
            };
            weakSignal = new ProviderEvent(provider, implementation, "weakSignal", [ {
                name : "weakSignalStation",
                type : "String"
            }
            ], {});
        });

        it("is of correct type", function() {
            expect(weakSignal).toBeDefined();
            expect(weakSignal).not.toBeNull();
            expect(typeof weakSignal === "object").toBeTruthy();
            expect(weakSignal instanceof ProviderEvent).toBeTruthy();
        });

        it("has correct members", function() {
            expect(weakSignal.createBroadcastOutputParameters).toBeDefined();
            expect(weakSignal.fire).toBeDefined();
            expect(weakSignal.registerObserver).toBeDefined();
            expect(weakSignal.unregisterObserver).toBeDefined();
            expect(weakSignal.addBroadcastFilter).toBeDefined();
            expect(weakSignal.deleteBroadcastFilter).toBeDefined();
        });

        function buildObserver(spy) {
            return function(data) {
                spy(data);
            };
        }

        it("implements the observer concept correctly", function() {
            var i, spy1, spy2, attribute, func1, func2, value = {
                key : "value",
                1 : 2,
                object : {}
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

            var data = {
                broadcastOutputParameters : value,
                filters : []
            };

            expect(spy1).toHaveBeenCalled();
            expect(spy1).toHaveBeenCalledWith(data);
            expect(spy2).toHaveBeenCalled();
            expect(spy2).toHaveBeenCalledWith(data);

            weakSignal.unregisterObserver(func2);

            weakSignal.fire(value);

            expect(spy1.calls.length).toEqual(2);
            expect(spy2.calls.length).toEqual(1);

            weakSignal.unregisterObserver(func1);

            weakSignal.fire(value);

            expect(spy1.calls.length).toEqual(2);
            expect(spy2.calls.length).toEqual(1);
        });

        function isPartOfList(list, data) {
            return list.indexOf(data) !== -1;
        }

        function buildObserver2(spy) {
            return function(data) {
                var filters = data.filters;
                var broadcastOutputParameters = data.broadcastOutputParameters;
                var filterParameters = {};
                var i;

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

        it("implements the broadcast filter list correctly", function() {
            var i, spy1, spy2, attribute, observerFunc, filterFunc;
            var value = {
                key : "value",
                1 : 2,
                object : {}
            };
            var spy3;

            spy1 = jasmine.createSpy("spy1");
            spy2 = jasmine.createSpy("spy2");

            observerFunc = buildObserver2(spy1);
            filterFunc = buildObserver3(spy2);

            weakSignal.addBroadcastFilter(filterFunc);
            weakSignal.registerObserver(observerFunc);

            expect(spy1).not.toHaveBeenCalled();
            expect(spy2).not.toHaveBeenCalled();

            var data = {
                broadcastOutputParameters : value,
                filters : [ filterFunc
                ]
            };
            var filterParameters = {};

            weakSignal.fire(value);

            expect(spy1).toHaveBeenCalledWith(data);
            expect(spy2).toHaveBeenCalledWith(value, filterParameters);
        });

        it("fire works", function() {
            expect(weakSignal.fire).toBeDefined();
            expect(typeof weakSignal.fire === "function").toBeTruthy();
            expect(function() {
                var boc = weakSignal.createBroadcastOutputParameters();
                boc.setWeakSignalStation("Bayern 3");
                weakSignal.fire(boc);
            }).not.toThrow();
        });

    });

}); // require
