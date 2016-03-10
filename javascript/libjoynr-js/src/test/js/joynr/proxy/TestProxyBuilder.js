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
        "joynr/proxy/TestProxyBuilder",
        [
            "joynr/proxy/ProxyBuilder",
            "joynr/proxy/ProxyAttributeNotifyReadWrite",
            "joynr/proxy/ProxyOperation",
            "joynr/proxy/ProxyEvent",
            "joynr/proxy/DiscoveryQos",
            "joynr/messaging/MessagingQos",
            "joynr/types/ProviderQos",
            "joynr/types/ProviderScope",
            "joynr/types/CapabilityInformation",
            "joynr/types/ArbitrationStrategyCollection",
            "joynr/types/DiscoveryScope",
            "joynr/messaging/inprocess/InProcessAddress",
            "joynr/vehicle/RadioProxy",
            "joynr/vehicle/radiotypes/RadioStation",
            "global/Promise"
        ],
        function(
                ProxyBuilder,
                ProxyAttributeNotifyReadWrite,
                ProxyOperation,
                ProxyEvent,
                DiscoveryQos,
                MessagingQos,
                ProviderQos,
                ProviderScope,
                CapabilityInformation,
                ArbitrationStrategyCollection,
                DiscoveryScope,
                InProcessAddress,
                RadioProxy,
                RadioStation,
                Promise) {

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

                        beforeEach(function() {
                            domain = "myDomain";
                            interfaceName = "vehicle/Radio";
                            discoveryQos = new DiscoveryQos({
                                discoveryTimeout : 5000,
                                discoveryRetryDelay : 900,
                                arbitrationStrategy : ArbitrationStrategyCollection.Nothing,
                                maxAgeOfCachedProviders : 0,
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

                            capInfo = new CapabilityInformation({
                                domain : domain,
                                interfaceName : interfaceName,
                                providerQos : new ProviderQos({
                                    customParameter : [],
                                    priority : 1,
                                    scope : ProviderScope.GLOBAL,
                                    supportsOnChangeSubscriptions : true
                                }),
                                channelId : "channelId",
                                participantId : "myParticipantId"
                            });

                            arbitratedCaps = [ capInfo
                            ];

                            arbitratorSpy =
                                    jasmine.createSpyObj("arbitratorSpy", [ "startArbitration"
                                    ]);
                            messageRouterSpy =
                                    jasmine.createSpyObj("messageRouterSpy", [ "addNextHop"
                                    ]);
                            loggingManagerSpy =
                                    jasmine.createSpyObj("loggingManagerSpy", [ "setLoggingContext"
                                    ]);
                            libjoynrMessagingAddress = {
                                key : "libjoynrMessagingAddress"
                            };

                            var resolvedPromise = Promise.resolve(arbitratedCaps);
                            arbitratorSpy.startArbitration.andReturn(resolvedPromise);
                            proxyBuilder = new ProxyBuilder({
                                arbitrator : arbitratorSpy
                            }, {
                                messageRouter : messageRouterSpy,
                                libjoynrMessagingAddress : libjoynrMessagingAddress,
                                loggingManager : loggingManagerSpy
                            });
                        });

                        it("is defined and of correct type", function() {
                            expect(proxyBuilder).toBeDefined();
                            expect(typeof proxyBuilder.build === "function").toBe(true);
                        });

                        it("throws exceptions upon missing or wrongly typed arguments", function() {
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
                        });

                        it("does not throw", function() {
                            expect(function() {
                                proxyBuilder.build(RadioProxy, settings);
                            }).not.toThrow();
                        });

                        it("returns an A+ Promise object", function() {
                            var promise = proxyBuilder.build(RadioProxy, settings);
                            expect(promise).toBeDefined();
                            expect(promise).not.toBeNull();
                            expect(typeof promise === "object").toBeTruthy();
                            expect(promise.then).toBeDefined();
                        });

                        it("calls arbitrator with correct arguments", function() {
                            runs(function() {
                                proxyBuilder.build(RadioProxy, settings);
                            });

                            waitsFor(function() {
                                return arbitratorSpy.startArbitration.callCount > 0;
                            }, "startArbitration invoked", 100);

                            runs(function(){
                                expect(arbitratorSpy.startArbitration).toHaveBeenCalled();
                                expect(arbitratorSpy.startArbitration).toHaveBeenCalledWith({
                                    domain : settings.domain,
                                    interfaceName : interfaceName,
                                    discoveryQos : settings.discoveryQos,
                                    staticArbitration : settings.staticArbitration
                                });
                            });
                        });

                        it("returned promise is resolved with a frozen proxy object by default", function() {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                proxyBuilder.build(RadioProxy, settings).then(function(argument) {
                                    spy.onFulfilled(argument);
                                }).catch(spy.onRejected);
                            });

                            waitsFor(
                                    function() {
                                        return spy.onFulfilled.callCount > 0;
                                    },
                                    "until the ProxyBuilder promise is not pending any more",
                                    safetyTimeoutDelta);

                            runs(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                expect(spy.onFulfilled).toHaveBeenCalledWith(
                                        jasmine.any(RadioProxy));
                                expect(spy.onRejected).not.toHaveBeenCalled();
                                var proxy = spy.onFulfilled.mostRecentCall.args[0];
                                proxy.adaptfrozenObjectShouldNotWork = "adaptfrozenObjectShouldNotWork";
                                expect(proxy.adaptfrozenObjectShouldNotWork).not.toBeDefined();
                            });
                        });

                        it("returned promise is resolved with an unfrozen proxy object if set accordingly", function() {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                settings.freeze = false;
                                proxyBuilder.build(RadioProxy, settings).then(function(argument) {
                                    spy.onFulfilled(argument);
                                }).catch(spy.onRejected);
                            });

                            waitsFor(
                                    function() {
                                        return spy.onFulfilled.callCount > 0;
                                    },
                                    "until the ProxyBuilder promise is not pending any more",
                                    safetyTimeoutDelta);

                            runs(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                expect(spy.onFulfilled).toHaveBeenCalledWith(
                                        jasmine.any(RadioProxy));
                                expect(spy.onRejected).not.toHaveBeenCalled();
                                var proxy = spy.onFulfilled.mostRecentCall.args[0];
                                proxy.adaptUnfrozenObjectShouldWork = "adaptUnfrozenObjectShouldWork";
                                expect(proxy.adaptUnfrozenObjectShouldWork).toEqual("adaptUnfrozenObjectShouldWork");
                            });
                        });

                        it("returned promise is rejected with error", function() {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);
                            var error = new Error("MyError");

                            var onRejectedCalled = false;

                            runs(function() {
                                arbitratorSpy.startArbitration.andReturn(Promise.reject(error));
                                proxyBuilder = new ProxyBuilder({
                                    arbitrator : arbitratorSpy
                                });
                                proxyBuilder.build(RadioProxy, settings).then(spy.onFulfilled).catch(
                                        function(error) {
                                            onRejectedCalled = true;
                                            spy.onRejected(error);
                                        });
                            });

                            waitsFor(
                                    function() {
                                        return onRejectedCalled;
                                    },
                                    "until the ProxyBuilder promise is not pending any more",
                                    safetyTimeoutDelta);

                            runs(function() {
                                expect(spy.onFulfilled).not.toHaveBeenCalled();
                                expect(spy.onRejected).toHaveBeenCalled();
                                expect(spy.onRejected).toHaveBeenCalledWith(error);
                            });
                        });

                        it(
                                "returned promise is resolved with proxy object with injected providerParticipantId",
                                function() {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    var onFulfilledCalled = false;

                                    runs(function() {
                                        proxyBuilder.build(RadioProxy, settings).then(
                                                function(argument) {
                                                    onFulfilledCalled = true;
                                                    spy.onFulfilled(argument);
                                                }).catch(spy.onRejected);
                                    });

                                    waitsFor(
                                            function() {
                                                return onFulfilledCalled;
                                            },
                                            "until the ProxyBuilder promise is not pending any more",
                                            safetyTimeoutDelta);

                                    runs(function() {
                                        expect(
                                                spy.onFulfilled.calls[0].args[0].providerParticipantId)
                                                .toEqual(arbitratedCaps[0].participantId);
                                    });
                                });

                        it(
                                "adds a routing table entry",
                                function() {
                                    var promise, spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);

                                    var onFulfilledCalled = false;

                                    runs(function() {
                                        proxyBuilder.build(RadioProxy, settings).then(
                                                function(argument) {
                                                    onFulfilledCalled = true;
                                                    spy.onFulfilled(argument);
                                                }).catch(spy.onRejected);
                                    });

                                    waitsFor(
                                            function() {
                                                return onFulfilledCalled;
                                            },
                                            "until the ProxyBuilder promise is not pending any more",
                                            safetyTimeoutDelta);

                                    runs(function() {
                                        expect(
                                                spy.onFulfilled.calls[0].args[0].providerParticipantId)
                                                .toEqual(arbitratedCaps[0].participantId);
                                        expect(
                                                typeof messageRouterSpy.addNextHop.mostRecentCall.args[0] === "string")
                                                .toBeTruthy();
                                        expect(messageRouterSpy.addNextHop.mostRecentCall.args[1])
                                                .toEqual(libjoynrMessagingAddress);
                                    });
                                });

                        it("sets the logging context", function() {
                            var proxy, spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);
                            var onFulfilledCalled = false;
                            settings.loggingContext = {
                                myContext : "myContext",
                                myContextNumber : 1
                            };
                            runs(function() {
                                proxyBuilder.build(RadioProxy, settings).then(function(argument) {
                                    onFulfilledCalled = true;
                                    spy.onFulfilled(argument);
                                }).catch(spy.onRejected);
                            });

                            waitsFor(
                                    function() {
                                        return onFulfilledCalled;
                                    },
                                    "until the ProxyBuilder promise is not pending any more",
                                    safetyTimeoutDelta);

                            runs(function() {
                                proxy = spy.onFulfilled.calls[0].args[0];
                                expect(loggingManagerSpy.setLoggingContext).toHaveBeenCalledWith(
                                        proxy.proxyParticipantId,
                                        settings.loggingContext);
                            });
                        });
                    });

        }); // require
