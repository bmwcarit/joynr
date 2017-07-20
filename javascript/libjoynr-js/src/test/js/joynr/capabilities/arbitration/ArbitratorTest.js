/*jslint es5: true */
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

define([
            "joynr/capabilities/arbitration/Arbitrator",
            "joynr/types/DiscoveryEntryWithMetaInfo",
            "joynr/types/ProviderQos",
            "joynr/types/CustomParameter",
            "joynr/proxy/DiscoveryQos",
            "joynr/types/ArbitrationStrategyCollection",
            "joynr/types/DiscoveryQos",
            "joynr/types/DiscoveryScope",
            "joynr/exceptions/DiscoveryException",
            "joynr/exceptions/NoCompatibleProviderFoundException",
            "joynr/types/Version",
            "joynr/util/UtilInternal",
            "global/Promise",
            "Date",
            "global/WaitsFor"
        ],
        function(
                Arbitrator,
                DiscoveryEntryWithMetaInfo,
                ProviderQos,
                CustomParameter,
                DiscoveryQos,
                ArbitrationStrategyCollection,
                DiscoveryQosGen,
                DiscoveryScope,
                DiscoveryException,
                NoCompatibleProviderFoundException,
                Version,
                Util,
                Promise,
                Date,
                waitsFor) {
            var capabilities, fakeTime, staticArbitrationSettings, staticArbitrationSpy, domain;
            var interfaceName, discoveryQos, capDiscoverySpy, arbitrator, discoveryEntries, nrTimes;
            var discoveryEntriesWithDifferentProviderVersions;
            var safetyTimeoutDelta = 100;
            var discoveryEntryWithMajor47AndMinor0, discoveryEntryWithMajor47AndMinor1;
            var discoveryEntryWithMajor47AndMinor2, discoveryEntryWithMajor47AndMinor3;
            var discoveryEntryWithMajor48AndMinor2;
            // save values once before installing jasmine clock mocks
            var originalSetInterval = setInterval;
            var originalClearInterval = clearInterval;

            function increaseFakeTime(time_ms) {
                fakeTime = fakeTime + time_ms;
                jasmine.clock().tick(time_ms);
            }

            function getDiscoveryEntry(domain, interfaceName, discoveryStrategy, providerVersion, supportsOnChangeSubscriptions) {
                return new DiscoveryEntryWithMetaInfo({
                    providerVersion : providerVersion,
                    domain : domain,
                    interfaceName : interfaceName,
                    qos : new ProviderQos({
                        customParameter : [new CustomParameter( { name : "theName", value : "theValue"})],
                        priority : 123,
                        scope : discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY ? true : false,
                        supportsOnChangeSubscriptions : supportsOnChangeSubscriptions}),
                    participandId : "700",
                    lastSeenDateMs : Date.now(),
                    publicKeyId : "",
                    isLocal : true
                });
            }

            describe(
                    "libjoynr-js.joynr.capabilities.arbitration.Arbitrator",
                    function() {
                        capabilities = [
                            {
                                domain : "myDomain",
                                interfaceName : "myInterface",
                                participantId : 1,
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 11}),
                                qos : {
                                    supportsOnChangeSubscriptions : true
                                }
                            },
                            {
                                domain : "myDomain",
                                interfaceName : "myInterface",
                                participantId : 2,
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 11}),
                                qos : {
                                    supportsOnChangeSubscriptions : false
                                }
                            },
                            {
                                domain : "otherDomain",
                                interfaceName : "otherInterface",
                                participantId : 3,
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 11}),
                                qos : {
                                    supportsOnChangeSubscriptions : true
                                }
                            },
                            {
                                domain : "thirdDomain",
                                interfaceName : "otherInterface",
                                participantId : 4,
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 11}),
                                qos : {
                                    supportsOnChangeSubscriptions : false
                                }

                            }
                        ];

                        beforeEach(function() {
                            var i;
                            var providerQos;
                            domain = "myDomain";
                            interfaceName = "myInterface";
                            discoveryQos = new DiscoveryQos({
                                discoveryTimeoutMs : 5000,
                                discoveryRetryDelayMs : 900,
                                arbitrationStrategy : ArbitrationStrategyCollection.Nothing,
                                cacheMaxAgeMs : 0,
                                discoveryScope : DiscoveryScope.LOCAL_THEN_GLOBAL,
                                additionalParameters : {}
                            });

                            staticArbitrationSettings = {
                                domains : [domain],
                                interfaceName : interfaceName,
                                discoveryQos : discoveryQos,
                                staticArbitration : true,
                                proxyVersion : new Version({ majorVersion: 47, minorVersion: 11})
                            };

                            staticArbitrationSpy = jasmine.createSpyObj("staticArbitrationSpy", [
                                "resolve",
                                "reject"
                            ]);

                            capDiscoverySpy =
                                    jasmine.createSpyObj("capabilityDiscovery", [ "lookup"
                                    ]);
                            capDiscoverySpy.lookup.and.returnValue(Promise.resolve([]));

                            arbitrator = new Arbitrator(capDiscoverySpy, capabilities);

                            discoveryEntries = [];
                            for (i = 0; i < 12; ++i) {
                                discoveryEntries.push(getDiscoveryEntry(
                                        domain + i.toString(),
                                        interfaceName + i.toString(),
                                        discoveryQos.discoveryStrategy,
                                        new Version({ majorVersion: 47, minorVersion: 11}),
                                        false
                                ));
                            }

                            // prepare a number of similar discovery entries with different
                            // provider versions

                            providerQos = new ProviderQos({
                                    customParameters : [],
                                    priority: 123,
                                    scope: discoveryQos.discoveryScope === DiscoveryScope.LOCAL_ONLY
                                            ? true : false,
                                    supportsOnChangeSubscriptions : true
                            });

                            discoveryEntryWithMajor47AndMinor0 = new DiscoveryEntryWithMetaInfo({
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 0 }),
                                domain : domain,
                                interfaceName : interfaceName,
                                qos : providerQos,
                                participantId : "700",
                                lastSeenDateMs : Date.now(),
                                publicKeyId : "",
                                isLocal : true
                            });

                            discoveryEntryWithMajor47AndMinor1 = new DiscoveryEntryWithMetaInfo({
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 1 }),
                                domain : domain,
                                interfaceName : interfaceName,
                                qos : providerQos,
                                participantId : "700",
                                lastSeenDateMs : Date.now(),
                                publicKeyId : "",
                                isLocal : false
                            });

                            discoveryEntryWithMajor47AndMinor2 = new DiscoveryEntryWithMetaInfo({
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 2 }),
                                domain : domain,
                                interfaceName : interfaceName,
                                qos : providerQos,
                                participantId : "700",
                                lastSeenDateMs : Date.now(),
                                publicKeyId : "",
                                isLocal : true
                            });

                            discoveryEntryWithMajor47AndMinor3 = new DiscoveryEntryWithMetaInfo({
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 3 }),
                                domain : domain,
                                interfaceName : interfaceName,
                                qos : providerQos,
                                participantId : "700",
                                lastSeenDateMs : Date.now(),
                                publicKeyId : "",
                                isLocal : false
                            });

                            discoveryEntryWithMajor48AndMinor2 = new DiscoveryEntryWithMetaInfo({
                                providerVersion : new Version({ majorVersion: 48, minorVersion: 2 }),
                                domain : domain,
                                interfaceName : interfaceName,
                                qos : providerQos,
                                participantId : "700",
                                lastSeenDateMs : Date.now(),
                                publicKeyId : "",
                                isLocal : true
                            });

                            //discoveryQos.arbitrationStrategy.and.returnValue([]);

                            nrTimes = 5;
                            fakeTime = 0;

                            jasmine.clock().install();
                            spyOn(Date, "now").and.callFake(function() {
                                return fakeTime;
                            });
                        });

                        afterEach(function() {
                            jasmine.clock().uninstall();
                          });

                        it("is instantiable", function(done) {
                            expect(new Arbitrator({})).toBeDefined();
                            done();
                        });

                        it(
                                "is of correct type and has all members",
                                function(done) {
                                    expect(arbitrator).toBeDefined();
                                    expect(arbitrator).not.toBeNull();
                                    expect(typeof arbitrator === "object").toBeTruthy();
                                    expect(arbitrator instanceof Arbitrator).toEqual(true);
                                    expect(arbitrator.startArbitration).toBeDefined();
                                    expect(typeof arbitrator.startArbitration === "function")
                                            .toEqual(true);
                                    done();
                                });

                        it("calls capabilityDiscovery upon arbitration", function(done) {

                            // return some discoveryEntries so that arbitration is faster
                            // (instantly instead of discoveryTimeout)
                            capDiscoverySpy.lookup.and.returnValue(Promise.resolve(discoveryEntries));
                            spyOn(discoveryQos, "arbitrationStrategy").and.returnValue(discoveryEntries);
                            arbitrator = new Arbitrator(capDiscoverySpy);

                            // start arbitration
                            arbitrator.startArbitration({
                                domains : [domain],
                                interfaceName : interfaceName,
                                discoveryQos : discoveryQos,
                                proxyVersion : new Version({ majorVersion: 47, minorVersion: 11})
                            }).then(function() {
                                expect(capDiscoverySpy.lookup).toHaveBeenCalled();
                                /* The arbitrator.startArbitration does a deep copy of its arguments.
                                 * Thus, two discoveryScope objects cannot be compared, as during deep copy
                                 * complex types are created as pure objects
                                 */
                                //expect(capDiscoverySpy.lookup.calls.mostRecent().args[0]).toBe([domain]);
                                expect(capDiscoverySpy.lookup.calls.mostRecent().args[0]).toEqual([domain]);
                                //expect(capDiscoverySpy.lookup.calls.mostRecent().args[1]).toBe(interfaceName);
                                expect(capDiscoverySpy.lookup.calls.mostRecent().args[1]).toEqual(interfaceName);
                                //expect(capDiscoverySpy.lookup.calls.mostRecent().args[2].cacheMaxAge).toBe(discoveryQos.cacheMaxAgeMs);
                                expect(capDiscoverySpy.lookup.calls.mostRecent().args[2].cacheMaxAge).toEqual(discoveryQos.cacheMaxAgeMs);
                                //expect(capDiscoverySpy.lookup.calls.mostRecent().args[2].discoveryScope.name).toBe(discoveryQos.discoveryScope.name);
                                expect(capDiscoverySpy.lookup.calls.mostRecent().args[2].discoveryScope.name).toEqual(discoveryQos.discoveryScope.name);
                                done();
                                return null;
                            }).catch(function() {
                                fail("startArbitration got rejected");
                                return null;
                            });
                            increaseFakeTime(1);
                        });

                        function returnCapabilitiesFromDiscovery(providerMustSupportOnChange, discoveryEntries, expected, done) {
                            var onFulfilledSpy, onRejectedSpy;

                            // return discoveryEntries to check whether these are eventually
                            // returned by the arbitrator
                            capDiscoverySpy.lookup.and.returnValue(Promise.resolve(discoveryEntries));
                            arbitrator = new Arbitrator(capDiscoverySpy);

                            // spy on and instrument arbitrationStrategy
                            spyOn(discoveryQos, "arbitrationStrategy").and.callThrough();

                            // call arbitrator
                            onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            onRejectedSpy = jasmine.createSpy("onRejectedSpy");

                            arbitrator.startArbitration({
                                domains : [domain],
                                interfaceName : interfaceName,
                                discoveryQos : new DiscoveryQos(Util.extend(discoveryQos, { providerMustSupportOnChange : providerMustSupportOnChange})),
                                proxyVersion : new Version({ majorVersion: 47, minorVersion: 11})
                            }).then(onFulfilledSpy).catch(onRejectedSpy).then(function() {
                                // arbitrator finally returned the discoveryEntries (unfiltered
                                // because of ArbitrationStrategyCollection.Nothing)
                                expect(onRejectedSpy).not.toHaveBeenCalled();
                                expect(onFulfilledSpy).toHaveBeenCalled();
                                expect(onFulfilledSpy).toHaveBeenCalledWith(expected);
                                done();
                                return null;
                            }).catch(function() {
                                fail("startArbitration got rejected");
                                return null;
                            });
                            increaseFakeTime(1);
                        }

                        function setSupportsOnChangeSubscriptionsToTrue(discoveryEntry) {
                            discoveryEntry.qos = new ProviderQos(Util.extend(discoveryEntry.qos, { supportsOnChangeSubscriptions : true}));
                        }

                        it("returns capabilities from discovery", function(done) {
                            returnCapabilitiesFromDiscovery(false, discoveryEntries, discoveryEntries, done);
                        });

                        it("returns filtered capabilities from discovery if discoveryQos.providerMustSupportOnChange is true", function(done) {
                            setSupportsOnChangeSubscriptionsToTrue(discoveryEntries[1]);
                            setSupportsOnChangeSubscriptionsToTrue(discoveryEntries[5]);
                            setSupportsOnChangeSubscriptionsToTrue(discoveryEntries[11]);
                            var filteredDiscoveryEntries = [discoveryEntries[1], discoveryEntries[5], discoveryEntries[11]];
                            returnCapabilitiesFromDiscovery(true, discoveryEntries, filteredDiscoveryEntries, done);
                        });

                        it("returns capabilities with matching provider version", function(done) {
                            var discoveryEntriesWithDifferentProviderVersions = [
                                discoveryEntryWithMajor47AndMinor0,
                                discoveryEntryWithMajor47AndMinor1,
                                discoveryEntryWithMajor47AndMinor2,
                                discoveryEntryWithMajor47AndMinor3,
                                discoveryEntryWithMajor48AndMinor2
                            ];

                            // return discoveryEntries to check whether these are eventually
                            // returned by the arbitrator
                            capDiscoverySpy.lookup.and.returnValue(Promise.resolve(discoveryEntriesWithDifferentProviderVersions));
                            arbitrator = new Arbitrator(capDiscoverySpy);

                            // spy on and instrument arbitrationStrategy
                            spyOn(discoveryQos, "arbitrationStrategy").and.callThrough();

                            arbitrator.startArbitration({
                                domains : [domain],
                                interfaceName : interfaceName,
                                discoveryQos : discoveryQos,
                                proxyVersion : new Version({ majorVersion: 47, minorVersion: 2})
                            }).then(function(returnedDiscoveryEntries) {
                                // remove lastSeenDateMs from object since it has been modified
                                // by the lookup attempt
                                // by providerVersion because of ArbitrationStrategyCollection.Nothing
                                expect(returnedDiscoveryEntries).not.toContain(discoveryEntryWithMajor47AndMinor0);
                                expect(returnedDiscoveryEntries).not.toContain(discoveryEntryWithMajor47AndMinor1);
                                expect(returnedDiscoveryEntries).toContain(discoveryEntryWithMajor47AndMinor2);
                                expect(returnedDiscoveryEntries).toContain(discoveryEntryWithMajor47AndMinor3);
                                expect(returnedDiscoveryEntries).not.toContain(discoveryEntryWithMajor48AndMinor2);
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("rejects with NoCompatibleProviderFoundException including a list of incompatible provider version of latest lookup", function(done) {
                            var onFulfilledSpy, onRejectedSpy;
                            var expectedMinimumMinorVersion = 2;
                            var firstLookupResult = [
                                discoveryEntryWithMajor47AndMinor0,
                                discoveryEntryWithMajor47AndMinor1,
                                discoveryEntryWithMajor47AndMinor2,
                                discoveryEntryWithMajor47AndMinor3,
                                discoveryEntryWithMajor48AndMinor2
                            ];
                            var secondLookupResult = [
                                discoveryEntryWithMajor47AndMinor0,
                                discoveryEntryWithMajor48AndMinor2
                            ];

                            // return discoveryEntries to check whether these are eventually
                            // returned by the arbitrator
                            capDiscoverySpy.lookup.and.returnValue(Promise.resolve(firstLookupResult));
                            arbitrator = new Arbitrator(capDiscoverySpy);

                            var discoveryQosWithShortTimers = new DiscoveryQos({
                                discoveryTimeoutMs : 1000,
                                discoveryRetryDelayMs : 600,
                                arbitrationStrategy : ArbitrationStrategyCollection.Nothing,
                                cacheMaxAgeMs : 0,
                                discoveryScope : DiscoveryScope.LOCAL_THEN_GLOBAL,
                                additionalParameters : {}
                            });

                            // spy on and instrument arbitrationStrategy
                            spyOn(discoveryQosWithShortTimers, "arbitrationStrategy").and.callThrough();

                            // call arbitrator
                            onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            onRejectedSpy = jasmine.createSpy("onRejectedSpy");

                            arbitrator.startArbitration({
                                domains : [domain],
                                interfaceName : interfaceName,
                                discoveryQos : discoveryQosWithShortTimers,
                                proxyVersion : new Version({ majorVersion: 49, minorVersion: expectedMinimumMinorVersion})
                            }).then(onFulfilledSpy).catch(onRejectedSpy);

                            increaseFakeTime(1);

                            // wait until immediate lookup is finished
                            waitsFor(function() {
                                return discoveryQosWithShortTimers.arbitrationStrategy.calls.count() === 1;
                            }, "capDiscoverySpy.lookup call", 100).then(function() {
                                expect(capDiscoverySpy.lookup.calls.count()).toBe(1);
                                increaseFakeTime(discoveryQosWithShortTimers.discoveryRetryDelayMs - 2);
                                return waitsFor(function() {
                                    return discoveryQosWithShortTimers.arbitrationStrategy.calls.count() === 1;
                                }, "discoveryQosWithShortTimers.arbitrationStrategy.calls.count() called", 1000);
                            }).then(function() {
                                capDiscoverySpy.lookup.and.returnValue(Promise.resolve(secondLookupResult));
                                expect(capDiscoverySpy.lookup.calls.count()).toBe(1);
                                increaseFakeTime(2);

                                // wait until 1st retry is finished, which returns different
                                // discovery entries
                                return waitsFor(function() {
                                    return discoveryQosWithShortTimers.arbitrationStrategy.calls.count() === 2;
                                }, "capDiscoverySpy.lookup call", 1000);
                            }).then(function() {
                                expect(capDiscoverySpy.lookup.calls.count()).toBe(2);
                                increaseFakeTime(1000);

                                // wait until arbitration expires, the incompatible versions found
                                // during the 2nd lookup (= 1st retry) should be returned inside
                                // a NoCompatibleProviderFoundException
                                return waitsFor(function() {
                                    return onRejectedSpy.calls.count() > 0;
                                }, "until onRejected has been invoked", 5000);
                            }).then(function() {
                                expect(onRejectedSpy).toHaveBeenCalled();
                                expect(onFulfilledSpy).not.toHaveBeenCalled();
                                expect(onRejectedSpy.calls.argsFor(0)[0] instanceof NoCompatibleProviderFoundException).toBeTruthy();
                                // discoverVersion should contain all not matching entries of only the last(!) lookup
                                var discoveredVersions = onRejectedSpy.calls.argsFor(0)[0].discoveredVersions;
                                expect(discoveredVersions).toContain(discoveryEntryWithMajor47AndMinor0.providerVersion);
                                expect(discoveredVersions).not.toContain(discoveryEntryWithMajor47AndMinor1.providerVersion);
                                expect(discoveredVersions).not.toContain(discoveryEntryWithMajor47AndMinor2.providerVersion);
                                expect(discoveredVersions).not.toContain(discoveryEntryWithMajor47AndMinor3.providerVersion);
                                expect(discoveredVersions).toContain(discoveryEntryWithMajor48AndMinor2.providerVersion);
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "timeouts after the given discoveryTimeoutMs on empty results",
                                function(done) {
                                    var onRejectedSpy = jasmine.createSpy("onRejectedSpy");
                                    arbitrator.startArbitration({
                                        domains : [domain],
                                        interfaceName : interfaceName,
                                        discoveryQos : discoveryQos,
                                        proxyVersion : new Version({ majorVersion: 47, minorVersion: 11})
                                    }).then(function() {
                                        fail("startArbitration got resolved unexpectedly");
                                        return null;
                                    }).catch(function(error) {
                                        onRejectedSpy(error);
                                        return null;
                                    });

                                    // let discoveryTimeout - 1 pass
                                    increaseFakeTime(discoveryQos.discoveryTimeoutMs - 1);
                                    expect(onRejectedSpy).not.toHaveBeenCalled();

                                    // let discoveryTimeoutMs pass
                                    increaseFakeTime(1);

                                    waitsFor(function() {
                                        return onRejectedSpy.calls.count() > 0;
                                    }, "startArbitration to fail", 1000).then(function() {
                                        expect(onRejectedSpy).toHaveBeenCalled();
                                        expect(onRejectedSpy.calls.mostRecent().args[0] instanceof DiscoveryException).toBeTruthy();
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "timeouts after the given discoveryTimeoutMs when capabilityDiscoveryStub throws exceptions",
                                function(done) {
                                    var onRejectedSpy = jasmine.createSpy("onRejectedSpy");
                                    var fakeError = new Error("simulate discovery exception");
                                    capDiscoverySpy.lookup.and.returnValue(Promise.reject(fakeError));
                                    discoveryQos.discoveryTimeoutMs = 500;
                                    discoveryQos.discoveryRetryDelayMs = 30;

                                    arbitrator.startArbitration({
                                        domains : [domain],
                                        interfaceName : interfaceName,
                                        discoveryQos : discoveryQos,
                                        proxyVersion : new Version({ majorVersion: 47, minorVersion: 11})
                                    }).then(function() {
                                        fail("startArbitration got resolved unexpectedly");
                                        return null;
                                    }).catch(function(error) {
                                        onRejectedSpy(error);
                                        return null;
                                    });

                                    var checkpointMs = discoveryQos.discoveryRetryDelayMs * 5;
                                    // let checkpoint ms pass
                                    increaseFakeTime(checkpointMs);
                                    var waitsForPromise = waitsFor(function() {
                                        return onRejectedSpy.calls.count() > 0;
                                    }, "startArbitration to fail", checkpointMs);
                                    waitsForPromise.then(fail).catch(function() {
                                        expect(onRejectedSpy).not.toHaveBeenCalled();
                                        // let discoveryTimeoutMs pass
                                        increaseFakeTime(discoveryQos.discoveryTimeoutMs - checkpointMs);
                                        return waitsFor(function() {
                                            return onRejectedSpy.calls.count() > 0;
                                        }, "startArbitration to fail", discoveryQos.discoveryRetryDelayMs).then(function() {
                                            expect(onRejectedSpy).toHaveBeenCalled();
                                            expect(onRejectedSpy.calls.mostRecent().args[0] instanceof DiscoveryException).toBeTruthy();
                                            expect(onRejectedSpy.calls.mostRecent().args[0].detailMessage).toMatch(fakeError.message);
                                            done();
                                        }).catch(fail);
                                    });
                                });

                        it(
                                "reruns discovery for empty discovery results according to discoveryTimeoutMs and discoveryRetryDelayMs",
                                function(done) {
                                    //capDiscoverySpy.lookup.and.returnValue(Promise.resolve([]));
                                    capDiscoverySpy.lookup.and.callFake(function() {
                                        return Promise.resolve([]);
                                    });
                                    arbitrator = new Arbitrator(capDiscoverySpy);

                                    expect(capDiscoverySpy.lookup).not.toHaveBeenCalled();
                                    spyOn(discoveryQos, "arbitrationStrategy").and.returnValue([]);

                                    arbitrator.startArbitration({
                                        domains : [domain],
                                        interfaceName : interfaceName,
                                        discoveryQos : discoveryQos,
                                        proxyVersion : new Version({ majorVersion: 47, minorVersion: 11})
                                    }).catch(function() {
                                        // startAbitration caught error [expected!]
                                        done();
                                        return null;
                                    });

                                    increaseFakeTime(1);
                                    var promiseChain = waitsFor(function() {
                                        return capDiscoverySpy.lookup.calls.count() === 1;
                                    }, "first lookup", 1000);

                                    var i;

                                    function createFunc(i, promiseChain) {
                                        return promiseChain.then(function() {
                                            increaseFakeTime(discoveryQos.discoveryRetryDelayMs - 2);
                                            return waitsFor(function() {
                                                return capDiscoverySpy.lookup.calls.count() === i;
                                            }, "lookup " + i, 1000).then(function() {
                                                expect(capDiscoverySpy.lookup.calls.count()).toBe(i);
                                                increaseFakeTime(2);
                                                return waitsFor(function() {
                                                        return (capDiscoverySpy.lookup.calls.count() === i + 1);
                                                }).then(function() {
                                                    expect(capDiscoverySpy.lookup.calls.count()).toBe(i + 1);
                                                });
                                            });
                                        });
                                    }

                                    for (i = 1; i < nrTimes + 1; ++i) {
                                        promiseChain = createFunc(i, promiseChain);
                                    }
                                    promiseChain.then(function() {
                                        // this should trigger the done in the catch of startArbitration
                                        // above
                                        increaseFakeTime(discoveryQos.discoveryTimeoutMs);
                                        return null;
                                    }).catch(function(error) {
                                        fail("caught reject from promise chain");
                                        return null;
                                    });
                                }, 7000);

                        it("uses arbitration strategy and returns its results", function(done) {
                            var fakeDiscoveredCaps = [
                                {},
                                {},
                                {},
                                {},
                                {}
                            ];

                            // just return some object so that arbitration is successful and
                            // arbitration strategy is called
                            capDiscoverySpy.lookup.and.returnValue(Promise.resolve(fakeDiscoveredCaps));
                            arbitrator = new Arbitrator(capDiscoverySpy);

                            // spy on and instrument arbitrationStrategy to return discoveryEntries
                            spyOn(discoveryQos, "arbitrationStrategy").and.returnValue(discoveryEntries);

                            // call arbitrator
                            arbitrator.startArbitration({
                                domains : [domain],
                                interfaceName : interfaceName,
                                discoveryQos : discoveryQos,
                                proxyVersion : new Version({ majorVersion: 47, minorVersion: 11})
                            }).then(function(discoveredEntries) {
                                // the arbitrationStrategy was called with the fakeDiscoveredCaps
                                // returned by the discovery spy
                                expect(discoveryQos.arbitrationStrategy).toHaveBeenCalled();
                                expect(discoveryQos.arbitrationStrategy).toHaveBeenCalledWith(
                                        fakeDiscoveredCaps);
                                expect(discoveredEntries).toEqual(discoveryEntries);
                                done();
                                return null;
                            });
                            // increaseFakeTime: is required for test purpose to ensure the
                            // resolve/reject callbacks are called
                            increaseFakeTime(1);
                        });

                        it("is instantiable, of correct type and has all members", function(done) {
                            expect(Arbitrator).toBeDefined();
                            expect(typeof Arbitrator === "function").toBeTruthy();
                            expect(arbitrator).toBeDefined();
                            expect(arbitrator instanceof Arbitrator).toBeTruthy();
                            expect(arbitrator.startArbitration).toBeDefined();
                            expect(typeof arbitrator.startArbitration === "function").toBeTruthy();
                            done();
                        });

                        function arbitratesCorrectly(settings) {
                            staticArbitrationSettings.domains = settings.domains;
                            staticArbitrationSettings.interfaceName = settings.interfaceName;
                            staticArbitrationSettings.discoveryQos = new DiscoveryQos(Util.extend(staticArbitrationSettings.discoveryQos, {
                                providerMustSupportOnChange : settings.providerMustSupportOnChange || false
                            }));
                            return arbitrator.startArbitration(staticArbitrationSettings).then(function(discoveredEntries) {
                                expect(discoveredEntries).toEqual(settings.expected);
                            });
                        }

                        it("arbitrates correctly static capabilities", function(done) {
                            spyOn(discoveryQos, "arbitrationStrategy").and.callFake(
                                    function(discoveredCaps) {
                                        return discoveredCaps;
                                    });

                            arbitratesCorrectly({
                                domains: ["myDomain"],
                                interfaceName: "noneExistingInterface",
                                expected: []
                            }).then(function(){
                                return arbitratesCorrectly({
                                    domains: ["noneExistingDomain"],
                                    interfaceName: "myInterface",
                                    expected: []
                                });
                            }).then(function() {
                                return arbitratesCorrectly({
                                    domains: ["myDomain"],
                                    interfaceName: "myInterface",
                                    expected: [capabilities[0],
                                               capabilities[1]]
                                });
                            }).then(function() {
                                return arbitratesCorrectly({
                                    domains: ["otherDomain"],
                                    interfaceName: "otherInterface",
                                    expected: [capabilities[2]]
                                });
                            }).then(function() {
                                return arbitratesCorrectly({
                                    domains: ["thirdDomain"],
                                    interfaceName: "otherInterface",
                                    expected: [capabilities[3]]
                                });
                            }).then(function(){
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("Arbitrator supports discoveryQos.providerSupportsOnChange for static arbitration", function(done) {
                            spyOn(discoveryQos, "arbitrationStrategy").and.callFake(
                                    function(discoveredCaps) {
                                        return discoveredCaps;
                                    });

                            arbitratesCorrectly({
                                domains: ["myDomain"],
                                interfaceName: "noneExistingInterface",
                                providerMustSupportOnChange : true,
                                expected: []
                            }).then(function() {
                                return arbitratesCorrectly({
                                    domains: ["noneExistingDomain"],
                                    interfaceName: "myInterface",
                                    providerMustSupportOnChange : true,
                                    expected: []
                                });
                            }).then(function(){
                                return arbitratesCorrectly({
                                    domains: ["myDomain"],
                                    interfaceName: "myInterface",
                                    providerMustSupportOnChange : false,
                                    expected: [capabilities[0],
                                               capabilities[1]]
                                });
                            }).then(function() {
                                return arbitratesCorrectly({
                                    domains: ["myDomain"],
                                    interfaceName: "myInterface",
                                    providerMustSupportOnChange : true,
                                    expected: [capabilities[0]]
                                });
                            }).then(function() {
                                return arbitratesCorrectly({
                                    domains: ["otherDomain"],
                                    interfaceName: "otherInterface",
                                    providerMustSupportOnChange : true,
                                    expected: [capabilities[2]]
                                });
                            }).then(function() {
                                return arbitratesCorrectly({
                                    domains: ["thirdDomain"],
                                    interfaceName: "otherInterface",
                                    providerMustSupportOnChange : true,
                                    expected: []
                                });
                            }).then(function() {
                                return arbitratesCorrectly({
                                    domains: ["thirdDomain"],
                                    interfaceName: "otherInterface",
                                    providerMustSupportOnChange : false,
                                    expected: [capabilities[3]]
                                });
                            }).then(function(){
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("uses the provided arbitrationStrategy", function(done) {
                            spyOn(discoveryQos, "arbitrationStrategy").and.returnValue(discoveryEntries);

                                arbitrator.startArbitration(staticArbitrationSettings).then(function(discoveredEntries){
                                    expect(discoveredEntries).toEqual(discoveryEntries);
                                    expect(discoveryQos.arbitrationStrategy).toHaveBeenCalledWith([
                                                                                                   capabilities[0],
                                                                                                   capabilities[1]
                                                                                                   ]);
                                    done();
                                    return null;
                                }).catch(function(error){
                                    fail("an unexpected ArbitrationException was caught: " + error);
                                    return null;
                                });
                                // resolve/reject callbacks are called
                                increaseFakeTime(1);
                        });

                        it(
                                "fails if arbitrationStrategy throws an exception",
                                function(done) {
                                    var rejectCalled = false;
                                    spyOn(discoveryQos, "arbitrationStrategy").and.callFake(
                                            function(discoveredCaps) {
                                                throw new Error("myError");
                                            });

                                    expect(function() {
                                            arbitrator.startArbitration(staticArbitrationSettings).then(function() { return null; }).catch(function() { return null; });
                                    }).not.toThrow();

                                    arbitrator.startArbitration(staticArbitrationSettings)
                                            .then(function(result){
                                                fail("arbitration should have failed");
                                                return null;
                                            }).catch(
                                                    function(error) {
                                                        expect(
                                                                Object.prototype.toString
                                                                .call(error) === "[object Error]")
                                                                .toBeTruthy();
                                                        rejectCalled = true;
                                                        staticArbitrationSpy.reject(error);
                                                        done();
                                                        return null;
                                                    });
                                    // increaseFakeTime: is required for test purpose to ensure the
                                    // resolve/reject callbacks are called
                                    increaseFakeTime(1);
                                });

                        it(
                                "rejects pending arbitrations when shutting down",
                                function(done) {
                                    var rejectCalled = false;
                                    capDiscoverySpy.lookup.and.returnValue(Promise.resolve([]));
                                    spyOn(discoveryQos, "arbitrationStrategy").and.returnValue([]);
                                    arbitrator = new Arbitrator(capDiscoverySpy);

                                    // start arbitration
                                    var arbitrationPromise = arbitrator.startArbitration({
                                        domains : [domain],
                                        interfaceName : interfaceName,
                                        discoveryQos : discoveryQos,
                                        proxyVersion : new Version({ majorVersion: 47, minorVersion: 11})
                                    });
                                    increaseFakeTime(100);
                                    arbitrator.shutdown();
                                    arbitrationPromise.then(fail).catch(done);
                                });
            });
        }); // require
