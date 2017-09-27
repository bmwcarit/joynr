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
var ProxyBuilder = require('../../../classes/joynr/proxy/ProxyBuilder');
var ProxyOperation = require('../../../classes/joynr/proxy/ProxyOperation');
var ProxyEvent = require('../../../classes/joynr/proxy/ProxyEvent');
var DiscoveryQos = require('../../../classes/joynr/proxy/DiscoveryQos');
var MessagingQos = require('../../../classes/joynr/messaging/MessagingQos');
var ProviderQos = require('../../../classes/joynr/types/ProviderQos');
var ProviderScope = require('../../../classes/joynr/types/ProviderScope');
var DiscoveryEntryWithMetaInfo = require('../../../classes/joynr/types/DiscoveryEntryWithMetaInfo');
var ArbitrationStrategyCollection = require('../../../classes/joynr/types/ArbitrationStrategyCollection');
var DiscoveryScope = require('../../../classes/joynr/types/DiscoveryScope');
var Version = require('../../../classes/joynr/types/Version');
var InProcessAddress = require('../../../classes/joynr/messaging/inprocess/InProcessAddress');
var RadioProxy = require('../../../test-classes/joynr/vehicle/RadioProxy');
var RadioStation = require('../../../test-classes/joynr/vehicle/radiotypes/RadioStation');
var Promise = require('../../../classes/global/Promise');
var WaitsFor = require('../../../test-classes/global/WaitsFor');
module.exports = (function (ProxyBuilder, ProxyOperation, ProxyEvent, DiscoveryQos, MessagingQos, ProviderQos, ProviderScope, DiscoveryEntryWithMetaInfo, ArbitrationStrategyCollection, DiscoveryScope, Version, InProcessAddress, RadioProxy, RadioStation, Promise, waitsFor) {

            var safetyTimeoutDelta = 100;

            var interfaceName = "io/joynr/apps/radio";

            describe(
                    "libjoynr-js.joynr.proxy.ProxyBuilder",
                    function() {
                        var proxyBuilder;
                        var domain;
                        var arbitratorSpy;
                        var discoveryQos;
                        var settings;
                        var capInfo;
                        var arbitratedCaps;
                        var messagingQos;
                        var messageRouterSpy;
                        var loggingManagerSpy;
                        var libjoynrMessagingAddress;

                        beforeEach(function(done) {
                            domain = "myDomain";
                            interfaceName = "vehicle/Radio";
                            discoveryQos = new DiscoveryQos({
                                discoveryTimeoutMs : 5000,
                                discoveryRetryDelayMs : 900,
                                arbitrationStrategy : ArbitrationStrategyCollection.Nothing,
                                cacheMaxAgeMs : 0,
                                discoveryScope : DiscoveryScope.LOCAL_THEN_GLOBAL,
                                additionalParameters : {}
                            });
                            messagingQos = new MessagingQos();
                            settings = {
                                domain : domain,
                                discoveryQos : discoveryQos,
                                messagingQos : messagingQos,
                                staticArbitration : false
                            };

                            capInfo = new DiscoveryEntryWithMetaInfo({
                                providerVersion : new Version({ majorVersion: 47, minorVersion: 11}),
                                domain : domain,
                                interfaceName : interfaceName,
                                participantId : "myParticipantId",
                                qos : new ProviderQos({
                                    customParameter : [],
                                    priority : 1,
                                    scope : ProviderScope.GLOBAL,
                                    supportsOnChangeSubscriptions : true
                                }),
                                lastSeenDateMs : Date.now(),
                                expiryDateMs : Date.now() + 60000,
                                publicKeyId : "",
                                isLocal : false
                            });

                            arbitratedCaps = [ capInfo
                            ];

                            arbitratorSpy =
                                    jasmine.createSpyObj("arbitratorSpy", [ "startArbitration"
                                    ]);
                            messageRouterSpy =
                                    jasmine.createSpyObj("messageRouterSpy", [ "addNextHop", "setToKnown"
                                    ]);
                            loggingManagerSpy =
                                    jasmine.createSpyObj("loggingManagerSpy", [ "setLoggingContext"
                                    ]);
                            libjoynrMessagingAddress = {
                                key : "libjoynrMessagingAddress"
                            };

                            var resolvedPromise = Promise.resolve(arbitratedCaps);
                            arbitratorSpy.startArbitration.and.returnValue(resolvedPromise);
                            messageRouterSpy.addNextHop.and.returnValue(resolvedPromise);
                            proxyBuilder = new ProxyBuilder({
                                arbitrator : arbitratorSpy
                            }, {
                                messageRouter : messageRouterSpy,
                                libjoynrMessagingAddress : libjoynrMessagingAddress,
                                loggingManager : loggingManagerSpy
                            });
                            done();
                        });

                        it("is defined and of correct type", function(done) {
                            expect(proxyBuilder).toBeDefined();
                            expect(typeof proxyBuilder.build === "function").toBe(true);
                            done();
                        });

                        it("throws exceptions upon missing or wrongly typed arguments", function(done) {
                            // settings is undefined
                            expect(function() {
                                proxyBuilder.build(RadioProxy);
                            }).toThrow();
                            // settings is not of type object
                            expect(function() {
                                proxyBuilder.build(RadioProxy, "notObject");
                            }).toThrow();
                            // domain is undefined
                            expect(function() {
                                proxyBuilder.build(RadioProxy, {
                                    discoveryQos : new DiscoveryQos(),
                                    messagingQos : new MessagingQos()
                                });
                            }).toThrow();
                            // domain is not of type string
                            expect(function() {
                                proxyBuilder.build(RadioProxy, {
                                    domain : 1234,
                                    discoveryQos : new DiscoveryQos(),
                                    messagingQos : new MessagingQos()
                                });
                            }).toThrow();
                            // discoveryQos is not of type DiscoveryQos
                            expect(function() {
                                proxyBuilder.build(RadioProxy, {
                                    domain : "",
                                    discoveryQos : {},
                                    messagingQos : new MessagingQos()
                                });
                            }).not.toThrow();
                            // messagingQos is is not of type MessagingQos
                            expect(function() {
                                proxyBuilder.build(RadioProxy, {
                                    domain : "",
                                    discoveryQos : new DiscoveryQos(),
                                    messagingQos : {}
                                });
                            }).not.toThrow();
                            // discoveryQos is missing
                            expect(function() {
                                proxyBuilder.build(RadioProxy, {
                                    domain : "",
                                    messagingQos : new MessagingQos()
                                });
                            }).not.toThrow();
                            // messagingQos is missing
                            expect(function() {
                                proxyBuilder.build(RadioProxy, {
                                    domain : "",
                                    discoveryQos : new DiscoveryQos()
                                });
                            }).not.toThrow();
                            // messagingQos and discoveryQos are missing
                            expect(function() {
                                proxyBuilder.build(RadioProxy, {
                                    domain : ""
                                });
                            }).not.toThrow();
                            // ok
                            expect(function() {
                                proxyBuilder.build(RadioProxy, {
                                    domain : "",
                                    discoveryQos : new DiscoveryQos(),
                                    messagingQos : new MessagingQos()
                                });
                            }).not.toThrow();
                            done();
                        });

                        it("does not throw", function(done) {
                            expect(function() {
                                proxyBuilder.build(RadioProxy, settings);
                            }).not.toThrow();
                            done();
                        });

                        it("returns an A+ Promise object", function(done) {
                            var promise = proxyBuilder.build(RadioProxy, settings);
                            expect(promise).toBeDefined();
                            expect(promise).not.toBeNull();
                            expect(typeof promise === "object").toBeTruthy();
                            expect(promise.then).toBeDefined();
                            done();
                        });

                        it("calls arbitrator with correct arguments", function(done) {
                            proxyBuilder.build(RadioProxy, settings);

                            waitsFor(function() {
                                return arbitratorSpy.startArbitration.calls.count() > 0;
                            }, "startArbitration invoked", 100).then(function() {
                                expect(arbitratorSpy.startArbitration).toHaveBeenCalled();
                                expect(arbitratorSpy.startArbitration).toHaveBeenCalledWith({
                                    domains : [settings.domain],
                                    interfaceName : interfaceName,
                                    discoveryQos : settings.discoveryQos,
                                    staticArbitration : settings.staticArbitration,
                                    proxyVersion : new Version({ majorVersion: 47, minorVersion: 11})
                                });
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("returned promise is resolved with a frozen proxy object by default", function(done) {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            proxyBuilder.build(RadioProxy, settings).then(function(argument) {
                                spy.onFulfilled(argument);
                            }).catch(spy.onRejected);

                            waitsFor(
                                    function() {
                                        return spy.onFulfilled.calls.count() > 0;
                                    },
                                    "until the ProxyBuilder promise is not pending any more",
                            safetyTimeoutDelta).then(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                expect(spy.onFulfilled).toHaveBeenCalledWith(
                                        jasmine.any(RadioProxy));
                                expect(spy.onRejected).not.toHaveBeenCalled();
                                var proxy = spy.onFulfilled.calls.mostRecent().args[0];
                                proxy.adaptfrozenObjectShouldNotWork = "adaptfrozenObjectShouldNotWork";
                                expect(proxy.adaptfrozenObjectShouldNotWork).not.toBeDefined();
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("returned promise is resolved with an unfrozen proxy object if set accordingly", function(done) {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            settings.freeze = false;
                            proxyBuilder.build(RadioProxy, settings).then(function(argument) {
                                spy.onFulfilled(argument);
                            }).catch(spy.onRejected);

                            waitsFor(
                                    function() {
                                        return spy.onFulfilled.calls.count() > 0;
                                    },
                                    "until the ProxyBuilder promise is not pending any more",
                            safetyTimeoutDelta).then(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                expect(spy.onFulfilled).toHaveBeenCalledWith(
                                        jasmine.any(RadioProxy));
                                expect(spy.onRejected).not.toHaveBeenCalled();
                                var proxy = spy.onFulfilled.calls.mostRecent().args[0];
                                proxy.adaptUnfrozenObjectShouldWork = "adaptUnfrozenObjectShouldWork";
                                expect(proxy.adaptUnfrozenObjectShouldWork).toEqual("adaptUnfrozenObjectShouldWork");
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it("returned promise is rejected with error", function(done) {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);
                            var error = new Error("MyError");

                            var onRejectedCalled = false;

                            arbitratorSpy.startArbitration.and.returnValue(Promise.reject(error));
                            proxyBuilder = new ProxyBuilder({
                                arbitrator : arbitratorSpy
                            });
                            proxyBuilder.build(RadioProxy, settings).then(spy.onFulfilled).catch(
                                    function(error) {
                                        onRejectedCalled = true;
                                        spy.onRejected(error);
                                    });

                            waitsFor(
                                    function() {
                                        return onRejectedCalled;
                                    },
                                    "until the ProxyBuilder promise is not pending any more",
                            safetyTimeoutDelta).then(function() {
                                expect(spy.onFulfilled).not.toHaveBeenCalled();
                                expect(spy.onRejected).toHaveBeenCalled();
                                expect(spy.onRejected).toHaveBeenCalledWith(error);
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "returned promise is resolved with proxy object with injected providerParticipantId",
                                function(done) {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    var onFulfilledCalled = false;

                                    proxyBuilder.build(RadioProxy, settings).then(
                                            function(argument) {
                                                onFulfilledCalled = true;
                                                spy.onFulfilled(argument);
                                    }).catch(spy.onRejected);

                                    waitsFor(
                                            function() {
                                                return onFulfilledCalled;
                                            },
                                            "until the ProxyBuilder promise is not pending any more",
                                            safetyTimeoutDelta).then(function() {
                                        expect(
                                                spy.onFulfilled.calls.argsFor(0)[0].providerDiscoveryEntry)
                                                .toEqual(arbitratedCaps[0]);
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "adds a routing table entry for proxy and knows provider",
                                function(done) {
                                    var promise, spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    var onFulfilledCalled = false;

                                    proxyBuilder.build(RadioProxy, settings).then(
                                        function(argument) {
                                            onFulfilledCalled = true;
                                            spy.onFulfilled(argument);
                                            return null;
                                    }).catch(function() {
                                        spy.onRejected();
                                        return null;
                                    });

                                    waitsFor(
                                            function() {
                                                return onFulfilledCalled;
                                            },
                                            "until the ProxyBuilder promise is not pending any more",
                                            safetyTimeoutDelta).then(function() {
                                        expect(
                                                spy.onFulfilled.calls.argsFor(0)[0].providerDiscoveryEntry)
                                                .toEqual(arbitratedCaps[0]);
                                        expect(
                                                typeof messageRouterSpy.addNextHop.calls.mostRecent().args[0] === "string")
                                                .toBeTruthy();
                                        expect(messageRouterSpy.setToKnown.calls.mostRecent().args[0])
                                        .toEqual(arbitratedCaps[0].participantId);
                                        expect(messageRouterSpy.addNextHop.calls.mostRecent().args[1])
                                                .toEqual(libjoynrMessagingAddress);
                                        expect(messageRouterSpy.addNextHop.calls.mostRecent().args[2])
                                        .toEqual(true);
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it("sets the logging context", function(done) {
                            var proxy, spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);
                            var onFulfilledCalled = false;
                            settings.loggingContext = {
                                myContext : "myContext",
                                myContextNumber : 1
                            };
                            proxyBuilder.build(RadioProxy, settings).then(function(argument) {
                                onFulfilledCalled = true;
                                spy.onFulfilled(argument);
                                return null;
                            }).catch(function() {
                                spy.onRejected();
                                return null;
                            });

                            waitsFor(
                                    function() {
                                        return onFulfilledCalled;
                                    },
                                    "until the ProxyBuilder promise is not pending any more",
                                    safetyTimeoutDelta).then(function() {
                                proxy = spy.onFulfilled.calls.argsFor(0)[0];
                                expect(loggingManagerSpy.setLoggingContext).toHaveBeenCalledWith(
                                        proxy.proxyParticipantId,
                                        settings.loggingContext);
                                done();
                                return null;
                            }).catch(fail);
                        });
                    });

}(ProxyBuilder, ProxyOperation, ProxyEvent, DiscoveryQos, MessagingQos, ProviderQos, ProviderScope, DiscoveryEntryWithMetaInfo, ArbitrationStrategyCollection, DiscoveryScope, Version, InProcessAddress, RadioProxy, RadioStation, Promise, WaitsFor));    // require
