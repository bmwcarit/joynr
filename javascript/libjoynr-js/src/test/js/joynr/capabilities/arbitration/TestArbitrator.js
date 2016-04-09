/*global joynrTestRequire: true */
/*jslint es5: true */

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
        "joynr/capabilities/arbitration/TestArbitrator",
        [
            "joynr/capabilities/arbitration/Arbitrator",
            "joynr/types/DiscoveryEntry",
            "joynr/types/ProviderQos",
            "joynr/types/CustomParameter",
            "joynr/proxy/DiscoveryQos",
            "joynr/types/ArbitrationStrategyCollection",
            "joynr/types/DiscoveryQos",
            "joynr/types/DiscoveryScope",
            "global/Promise",
            "Date"
        ],
        function(
                Arbitrator,
                DiscoveryEntry,
                ProviderQos,
                CustomParameter,
                DiscoveryQos,
                ArbitrationStrategyCollection,
                DiscoveryQosGen,
                DiscoveryScope,
                Promise,
                Date) {
            var capabilities, fakeTime, staticArbitrationSettings, staticArbitrationSpy, domain;
            var interfaceName, discoveryQos, capDiscoverySpy, arbitrator, discoveryEntries, nrTimes;
            var safetyTimeoutDelta = 100;

            function increaseFakeTime(time_ms) {
                fakeTime = fakeTime + time_ms;
                jasmine.Clock.tick(time_ms);
            }

            function getDiscoveryEntry(domain, interfaceName, discoveryStrategy) {
                return new DiscoveryEntry({
                    domain : domain,
                    interfaceName : interfaceName,
                    qos : new ProviderQos([ new CustomParameter("theName", "theValue")
                    ], 123, 1234, discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY
                            ? true
                            : false, true),
                    participandId : "700"
                });
            }

            describe(
                    "libjoynr-js.joynr.capabilities.arbitration.Arbitrator",
                    function() {
                        capabilities = [
                            {
                                domain : "myDomain",
                                interfaceName : "myInterface",
                                participantId : 1
                            },
                            {
                                domain : "myDomain",
                                interfaceName : "myInterface",
                                participantId : 2
                            },
                            {
                                domain : "otherDomain",
                                interfaceName : "otherInterface",
                                participantId : 3
                            },
                            {
                                domain : "thirdDomain",
                                interfaceName : "otherInterface",
                                participantId : 4
                            }
                        ];

                        beforeEach(function() {
                            var i;
                            domain = "myDomain";
                            interfaceName = "myInterface";
                            discoveryQos = new DiscoveryQos({
                                discoveryTimeoutMs : 5000,
                                discoveryRetryDelayMs : 900,
                                arbitrationStrategy : ArbitrationStrategyCollection.Nothing,
                                cacheMaxAge : 0,
                                discoveryScope : DiscoveryScope.LOCAL_THEN_GLOBAL,
                                additionalParameters : {}
                            });

                            staticArbitrationSettings = {
                                domain : domain,
                                interfaceName : interfaceName,
                                discoveryQos : discoveryQos,
                                staticArbitration : true
                            };

                            staticArbitrationSpy = jasmine.createSpyObj("staticArbitrationSpy", [
                                "resolve",
                                "reject"
                            ]);

                            capDiscoverySpy =
                                    jasmine.createSpyObj("capabilityDiscovery", [ "lookup"
                                    ]);
                            capDiscoverySpy.lookup.andReturn(Promise.resolve([]));

                            arbitrator = new Arbitrator(capDiscoverySpy, capabilities);

                            discoveryEntries = [];
                            for (i = 0; i < 12; ++i) {
                                discoveryEntries.push(getDiscoveryEntry(
                                        domain + i.toString(),
                                        interfaceName + i.toString(),
                                        discoveryQos.discoveryStrategy));
                            }
                            //discoveryQos.arbitrationStrategy.andReturn([]);

                            nrTimes = 5;
                            fakeTime = 0;

                            jasmine.Clock.useMock();
                            jasmine.Clock.reset();
                            spyOn(Date, "now").andCallFake(function() {
                                return fakeTime;
                            });
                        });

                        it("is instantiable", function() {
                            expect(new Arbitrator({})).toBeDefined();
                        });

                        it(
                                "is of correct type and has all members",
                                function() {
                                    expect(arbitrator).toBeDefined();
                                    expect(arbitrator).not.toBeNull();
                                    expect(typeof arbitrator === "object").toBeTruthy();
                                    expect(arbitrator instanceof Arbitrator).toEqual(true);
                                    expect(arbitrator.startArbitration).toBeDefined();
                                    expect(typeof arbitrator.startArbitration === "function")
                                            .toEqual(true);
                                });

                        it("calls capabilityDiscovery upon arbitration", function() {

                            // return some discoveryEntries so that arbitration is faster
                            // (instantly instead of discoveryTimeoutMs)
                            capDiscoverySpy.lookup.andReturn(Promise.resolve(discoveryEntries));
                            spyOn(discoveryQos, "arbitrationStrategy").andReturn(discoveryEntries);
                            arbitrator = new Arbitrator(capDiscoverySpy);

                            var resolved = false;

                            runs(function() {
                                // start arbitration
                                arbitrator.startArbitration({
                                    domain : domain,
                                    interfaceName : interfaceName,
                                    discoveryQos : discoveryQos
                                }).then(function() {
                                    resolved = true;
                                });
                                increaseFakeTime(1);
                            });

                            waitsFor(function() {
                                return resolved;
                            }, "successful arbitration", 100);

                            runs(function() {
                                expect(resolved).toBeTruthy();
                                expect(capDiscoverySpy.lookup).toHaveBeenCalled();
                                /* The arbitrator.startArbitration does a deep copy of its arguments.
                                 * Thus, two discoveryScope objects cannot be compared, as during deep copy
                                 * complex types are created as pure objects
                                 */
                                expect(capDiscoverySpy.lookup.mostRecentCall.args[0]).toBe(domain);
                                expect(capDiscoverySpy.lookup.mostRecentCall.args[1]).toBe(interfaceName);
                                expect(capDiscoverySpy.lookup.mostRecentCall.args[2].cacheMaxAge).toBe(discoveryQos.cacheMaxAge);
                                expect(capDiscoverySpy.lookup.mostRecentCall.args[2].discoveryScope.name).toBe(discoveryQos.discoveryScope.name);
                            });
                        });

                        it("returns capabilities from discovery", function() {
                            var onFulfilledSpy, onRejectedSpy;

                            // return discoveryEntries to check whether these are eventually
                            // returned by the arbitrator
                            capDiscoverySpy.lookup.andReturn(Promise.resolve(discoveryEntries));
                            arbitrator = new Arbitrator(capDiscoverySpy);

                            // spy on and instrument arbitrationStrategy
                            spyOn(discoveryQos, "arbitrationStrategy").andCallThrough();

                            // call arbitrator
                            onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            onRejectedSpy = jasmine.createSpy("onRejectedSpy");

                            runs(function() {
                                arbitrator.startArbitration({
                                    domain : domain,
                                    interfaceName : interfaceName,
                                    discoveryQos : discoveryQos
                                }).then(onFulfilledSpy).catch(onRejectedSpy);
                                increaseFakeTime(1);
                            });

                            waitsFor(function() {
                                return onFulfilledSpy.callCount > 0;
                            }, "until onresolve has been invoked", 300);

                            runs(function() {

                                // arbitrator finally returned the discoveryEntries (unfiltered
                                // because of ArbitrationStrategyCollection.Nothing)
                                expect(onRejectedSpy).not.toHaveBeenCalled();
                                expect(onFulfilledSpy).toHaveBeenCalled();
                                expect(onFulfilledSpy).toHaveBeenCalledWith(discoveryEntries);
                            });
                        });

                        it(
                                "timeouts after the given discoveryTimeoutMs on empty results",
                                function() {
                                    var onFulfilledSpy, onRejectedSpy;

                                    onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                                    onRejectedSpy = jasmine.createSpy("onRejectedSpy");

                                    runs(function() {
                                        arbitrator.startArbitration({
                                            domain : domain,
                                            interfaceName : interfaceName,
                                            discoveryQos : discoveryQos
                                        }).then(onFulfilledSpy).catch(onRejectedSpy);
                                        // let discoveryTimeoutMs - 1 pass
                                        increaseFakeTime(discoveryQos.discoveryTimeoutMs - 1);

                                        expect(onFulfilledSpy).not.toHaveBeenCalled();
                                        expect(onRejectedSpy).not.toHaveBeenCalled();

                                        // let discoveryTimeoutMs pass
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return onRejectedSpy.callCount > 0;
                                    }, "until onReject has been invoked", 300);

                                    runs(function() {
                                        expect(onFulfilledSpy).not.toHaveBeenCalled();
                                        expect(onRejectedSpy).toHaveBeenCalled();
                                        expect(
                                                Object.prototype.toString
                                                        .call(onRejectedSpy.mostRecentCall.args[0]) === "[object Error]")
                                                .toBeTruthy();
                                    });
                                });

                        it(
                                "reruns discovery for empty discovery results according to discoveryTimeoutMs and discoveryRetryDelayMs",
                                function() {
                                    expect(capDiscoverySpy.lookup).not.toHaveBeenCalled();
                                    spyOn(discoveryQos, "arbitrationStrategy").andReturn([]);

                                    runs(function() {
                                        arbitrator.startArbitration({
                                            domain : domain,
                                            interfaceName : interfaceName,
                                            discoveryQos : discoveryQos
                                        });
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return discoveryQos.arbitrationStrategy.callCount === 1;
                                    }, "capDiscoverySpy.lookup call", 10);

                                    runs(function() {
                                        expect(capDiscoverySpy.lookup.callCount).toBe(1);
                                    });

                                    var internalCheck =
                                            function(i) {
                                                runs(function() {
                                                    increaseFakeTime(discoveryQos.discoveryRetryDelayMs - 2);
                                                });

                                                waitsFor(
                                                        function() {
                                                            return discoveryQos.arbitrationStrategy.callCount === i;
                                                        },
                                                        "discoveryQos.arbitrationStrategy.callCount call i="
                                                            + i,
                                                        1000);

                                                runs(function() {
                                                    expect(capDiscoverySpy.lookup.callCount)
                                                            .toBe(i);
                                                    increaseFakeTime(2);
                                                });

                                                waitsFor(
                                                        function() {
                                                            return discoveryQos.arbitrationStrategy.callCount === (i + 1);
                                                        },
                                                        "discoveryQos.arbitrationStrategy.callCount call i+1="
                                                            + (i + 1),
                                                        1000);

                                                runs(function() {
                                                    expect(capDiscoverySpy.lookup.callCount).toBe(
                                                            i + 1);
                                                });
                                            };

                                    var i;
                                    for (i = 1; i < nrTimes + 1; ++i) {
                                        internalCheck(i);
                                    }

                                });

                        it("uses arbitration strategy and returns its results", function() {
                            var onFulfilledSpy, onRejectedSpy, fakeDiscoveredCaps = [
                                {},
                                {},
                                {},
                                {},
                                {}
                            ];

                            // just return some object so that arbitration is successful and
                            // arbitration strategy is called
                            capDiscoverySpy.lookup.andReturn(Promise.resolve(fakeDiscoveredCaps));
                            arbitrator = new Arbitrator(capDiscoverySpy);

                            // spy on and instrument arbitrationStrategy to return discoveryEntries
                            spyOn(discoveryQos, "arbitrationStrategy").andReturn(discoveryEntries);
                            onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                            runs(function() {
                                // call arbitrator
                                arbitrator.startArbitration({
                                    domain : domain,
                                    interfaceName : interfaceName,
                                    discoveryQos : discoveryQos
                                }).then(onFulfilledSpy);
                                // increaseFakeTime: is required for test purpose to ensure the
                                // resolve/reject callbacks are called
                                increaseFakeTime(1);
                            });

                            waitsFor(function() {
                                return onFulfilledSpy.callCount > 0;
                            }, "until onresolve has been invoked", 300);

                            runs(function() {
                                // the arbitrationStrategy was called with the fakeDiscoveredCaps
                                // returned by the discovery spy
                                expect(discoveryQos.arbitrationStrategy).toHaveBeenCalled();
                                expect(discoveryQos.arbitrationStrategy).toHaveBeenCalledWith(
                                        fakeDiscoveredCaps);

                                // arbitrator returns discoveryEntries that were returned by the
                                // arbitrationStrategy spy
                                expect(onFulfilledSpy).toHaveBeenCalled();
                                expect(onFulfilledSpy).toHaveBeenCalledWith(discoveryEntries);
                            });
                        });

                        it("is instantiable, of correct type and has all members", function() {
                            expect(Arbitrator).toBeDefined();
                            expect(typeof Arbitrator === "function").toBeTruthy();
                            expect(arbitrator).toBeDefined();
                            expect(arbitrator instanceof Arbitrator).toBeTruthy();
                            expect(arbitrator.startArbitration).toBeDefined();
                            expect(typeof arbitrator.startArbitration === "function").toBeTruthy();
                        });

                        function arbitratesCorrectly(domain, interfaceName, expected) {
                            runs(function() {
                                staticArbitrationSpy.resolve.reset();
                                staticArbitrationSpy.reject.reset();
                                staticArbitrationSettings.domain = domain;
                                staticArbitrationSettings.interfaceName = interfaceName;
                                arbitrator.startArbitration(staticArbitrationSettings).then(
                                        staticArbitrationSpy.resolve).catch(staticArbitrationSpy.reject);
                                // resolve/reject callbacks are called
                                increaseFakeTime(1);
                            });

                            waitsFor(function() {
                                return staticArbitrationSpy.resolve.callCount > 0;
                            }, "successfull call of startArbitration", 300);

                            runs(function() {
                                expect(staticArbitrationSpy.resolve).toHaveBeenCalledWith(expected);
                                expect(staticArbitrationSpy.reject).not.toHaveBeenCalled();
                            });
                        }

                        it("arbitrates correctly", function() {
                            spyOn(discoveryQos, "arbitrationStrategy").andCallFake(
                                    function(discoveredCaps) {
                                        return discoveredCaps;
                                    });

                            arbitratesCorrectly("myDomain", "noneExistingInterface", []);
                            arbitratesCorrectly("noneExistingDomain", "myInterface", []);
                            arbitratesCorrectly("myDomain", "myInterface", [
                                capabilities[0],
                                capabilities[1]
                            ]);
                            arbitratesCorrectly("otherDomain", "otherInterface", [ capabilities[2]
                            ]);
                            arbitratesCorrectly("thirdDomain", "otherInterface", [ capabilities[3]
                            ]);
                        });

                        it("uses the provided arbitrationStrategy", function() {
                            spyOn(discoveryQos, "arbitrationStrategy").andReturn(discoveryEntries);

                            runs(function() {
                                arbitrator.startArbitration(staticArbitrationSettings).then(
                                        staticArbitrationSpy.resolve).catch(staticArbitrationSpy.reject);
                                // resolve/reject callbacks are called
                                increaseFakeTime(1);
                            });

                            waitsFor(function() {
                                return staticArbitrationSpy.resolve.callCount > 0;
                            }, "until arbitration has been performed successfully", 100);

                            runs(function() {
                                expect(discoveryQos.arbitrationStrategy).toHaveBeenCalledWith([
                                    capabilities[0],
                                    capabilities[1]
                                ]);
                                expect(staticArbitrationSpy.resolve).toHaveBeenCalledWith(
                                        discoveryEntries);
                                expect(staticArbitrationSpy.reject).not.toHaveBeenCalled();
                            });
                        });

                        it(
                                "fails if arbitrationStrategy throws an exception",
                                function() {
                                    var rejectCalled = false;
                                    spyOn(discoveryQos, "arbitrationStrategy").andCallFake(
                                            function(discoveredCaps) {
                                                throw new Error("myError");
                                            });

                                    expect(function() {
                                        arbitrator.startArbitration(staticArbitrationSettings);
                                    }).not.toThrow();

                                    runs(function() {
                                        arbitrator.startArbitration(staticArbitrationSettings)
                                                .then(staticArbitrationSpy.resolve).catch(
                                                        function(error) {
                                                            rejectCalled = true;
                                                            staticArbitrationSpy.reject(error);
                                                        });
                                        // increaseFakeTime: is required for test purpose to ensure the
                                        // resolve/reject callbacks are called
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return rejectCalled;
                                    }, "until onReject has been invoked", 300);

                                    runs(function() {
                                        expect(staticArbitrationSpy.resolve).not.toHaveBeenCalled();
                                        expect(
                                                Object.prototype.toString
                                                        .call(staticArbitrationSpy.reject.mostRecentCall.args[0]) === "[object Error]")
                                                .toBeTruthy();
                                    });
                                });
                    });

        }); // require
