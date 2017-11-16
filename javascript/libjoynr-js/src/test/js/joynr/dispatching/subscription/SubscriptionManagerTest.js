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
var SubscriptionManager = require("../../../../classes/joynr/dispatching/subscription/SubscriptionManager");
var MessagingQos = require("../../../../classes/joynr/messaging/MessagingQos");
var defaultMessagingSettings = require("../../../../classes/joynr/start/settings/defaultMessagingSettings");
var MulticastSubscriptionRequest = require("../../../../classes/joynr/dispatching/types/MulticastSubscriptionRequest");
var SubscriptionReply = require("../../../../classes/joynr/dispatching/types/SubscriptionReply");
var SubscriptionRequest = require("../../../../classes/joynr/dispatching/types/SubscriptionRequest");
var SubscriptionStop = require("../../../../classes/joynr/dispatching/types/SubscriptionStop");
var OnChangeWithKeepAliveSubscriptionQos = require("../../../../classes/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos");
var OnChangeSubscriptionQos = require("../../../../classes/joynr/proxy/OnChangeSubscriptionQos");
var SubscriptionQos = require("../../../../classes/joynr/proxy/SubscriptionQos");
var SubscriptionPublication = require("../../../../classes/joynr/dispatching/types/SubscriptionPublication");
var Promise = require("../../../../classes/global/Promise");
var Reply = require("../../../../classes/joynr/dispatching/types/Reply");
var PublicationMissedException = require("../../../../classes/joynr/exceptions/PublicationMissedException");
var SubscriptionException = require("../../../../classes/joynr/exceptions/SubscriptionException");
var LoggerFactory = require("../../../../classes/joynr/system/LoggerFactory");
var Date = require("../../../../test-classes/global/Date");
var waitsFor = require("../../../../test-classes/global/WaitsFor");
var TestEnum = require("../../../../test-classes/joynr/tests/testTypes/TestEnum");
var TypeRegistrySingleton = require("../../../../classes/joynr/types/TypeRegistrySingleton");
var DiscoveryEntryWithMetaInfo = require("../../../../classes/joynr/types/DiscoveryEntryWithMetaInfo");
var Version = require("../../../../classes/joynr/types/Version");
var ProviderQos = require("../../../../classes/joynr/types/ProviderQos");
var Util = require("../../../../classes/joynr/util/UtilInternal");

describe("libjoynr-js.joynr.dispatching.subscription.SubscriptionManager", function() {
    var subscriptionManager;
    var subscriptionManagerOnError;
    var log = LoggerFactory.getLogger("joynr.dispatching.SubscriptionManagerTest");
    var fakeTime = 1371553100000;
    var dispatcherSpy;
    var dispatcherSpyOnError;
    var storedSubscriptionId;
    var providerDiscoveryEntry;

    /**
     * Called before each test.
     */
    beforeEach(function(done) {
        providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
            domain: "testProviderDomain",
            interfaceName: "interfaceName",
            participantId: "providerParticipantId",
            qos: new ProviderQos({}),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "publicKeyId",
            isLocal: true
        });
        dispatcherSpy = jasmine.createSpyObj("DispatcherSpy", [
            "sendSubscriptionRequest",
            "sendBroadcastSubscriptionRequest",
            "sendMulticastSubscriptionStop",
            "sendSubscriptionStop"
        ]);

        dispatcherSpy.sendBroadcastSubscriptionRequest.and.callFake(function(settings) {
            storedSubscriptionId = settings.subscriptionRequest.subscriptionId;
            subscriptionManager.handleSubscriptionReply({
                subscriptionId: settings.subscriptionRequest.subscriptionId
            });
            return Promise.resolve();
        });

        dispatcherSpy.sendSubscriptionRequest.and.callFake(function(settings) {
            storedSubscriptionId = settings.subscriptionRequest.subscriptionId;
            subscriptionManager.handleSubscriptionReply({
                subscriptionId: settings.subscriptionRequest.subscriptionId
            });
            return Promise.resolve();
        });

        subscriptionManager = new SubscriptionManager(dispatcherSpy);

        dispatcherSpyOnError = jasmine.createSpyObj("DispatcherSpyOnError", [
            "sendSubscriptionRequest",
            "sendBroadcastSubscriptionRequest",
            "sendMulticastSubscriptionStop",
            "sendSubscriptionStop"
        ]);

        dispatcherSpyOnError.sendBroadcastSubscriptionRequest.and.callFake(function(settings) {
            storedSubscriptionId = settings.subscriptionRequest.subscriptionId;
            subscriptionManagerOnError.handleSubscriptionReply({
                subscriptionId: settings.subscriptionRequest.subscriptionId,
                error: new SubscriptionException({
                    subscriptionId: settings.subscriptionRequest.subscriptionId
                })
            });
            return Promise.resolve();
        });

        dispatcherSpyOnError.sendSubscriptionRequest.and.callFake(function(settings) {
            storedSubscriptionId = settings.subscriptionRequest.subscriptionId;
            subscriptionManagerOnError.handleSubscriptionReply({
                subscriptionId: settings.subscriptionRequest.subscriptionId,
                error: new SubscriptionException({
                    subscriptionId: settings.subscriptionRequest.subscriptionId
                })
            });
            return Promise.resolve();
        });

        subscriptionManagerOnError = new SubscriptionManager(dispatcherSpyOnError);

        jasmine.clock().install();
        spyOn(Date, "now").and.callFake(function() {
            return fakeTime;
        });

        /*
         * Make sure 'TestEnum' is properly registered as a type.
         * Just requiring the module is insufficient since the
         * automatically generated code called async methods.
         * Execution might be still in progress.
         */
        TypeRegistrySingleton.getInstance()
            .getTypeRegisteredPromise("joynr.tests.testTypes.TestEnum", 1000)
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    afterEach(function() {
        jasmine.clock().uninstall();
    });

    function increaseFakeTime(time_ms) {
        fakeTime = fakeTime + time_ms;
        jasmine.clock().tick(time_ms);
    }

    it("is instantiable", function(done) {
        expect(subscriptionManager).toBeDefined();
        done();
    });

    it("sends broadcast subscription requests", function(done) {
        var ttl = 250;
        var parameters = {
            proxyId: "subscriber",
            providerDiscoveryEntry: providerDiscoveryEntry,
            broadcastName: "broadcastName",
            subscriptionQos: new OnChangeSubscriptionQos({
                expiryDateMs: Date.now() + ttl
            })
        };
        var expectedDiscoveryEntry = Util.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(expectedDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(expectedDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        dispatcherSpy.sendBroadcastSubscriptionRequest.calls.reset();
        var spySubscribePromise = jasmine.createSpyObj("spySubscribePromise", ["resolve", "reject"]);

        subscriptionManager
            .registerBroadcastSubscription(parameters)
            .then(spySubscribePromise.resolve)
            .catch(spySubscribePromise.reject);
        increaseFakeTime(1);

        waitsFor(
            function() {
                return dispatcherSpy.sendBroadcastSubscriptionRequest.calls.count() === 1;
            },
            "dispatcherSpy.sendBroadcastSubscriptionRequest called",
            100
        )
            .then(function() {
                expect(dispatcherSpy.sendBroadcastSubscriptionRequest).toHaveBeenCalled();
                expect(dispatcherSpy.sendBroadcastSubscriptionRequest.calls.argsFor(0)[0].messagingQos.ttl).toEqual(
                    ttl
                );
                expect(dispatcherSpy.sendBroadcastSubscriptionRequest.calls.argsFor(0)[0].toDiscoveryEntry).toEqual(
                    expectedDiscoveryEntry
                );
                expect(dispatcherSpy.sendBroadcastSubscriptionRequest.calls.argsFor(0)[0].from).toEqual(
                    parameters.proxyId
                );
                expect(
                    dispatcherSpy.sendBroadcastSubscriptionRequest.calls.argsFor(0)[0].subscriptionRequest
                        .subscribedToName
                ).toEqual(parameters.broadcastName);
                done();
                return null;
            })
            .catch(fail);
    });

    it("alerts on missed publication and stops", function(done) {
        var publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        var publicationMissedSpy = jasmine.createSpy("publicationMissedSpy");
        var subscriptionId;
        var alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS;

        //log.debug("registering subscription");
        subscriptionManager
            .registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry: providerDiscoveryEntry,
                attributeName: "testAttribute",
                attributeType: "String",
                qos: new OnChangeWithKeepAliveSubscriptionQos({
                    alertAfterIntervalMs: alertAfterIntervalMs,
                    expiryDateMs: Date.now() + 50 + 2 * alertAfterIntervalMs
                }),
                onReceive: publicationReceivedSpy,
                onError: publicationMissedSpy
            })
            .then(function(subscriptionId) {
                expect(publicationMissedSpy).not.toHaveBeenCalled();
                increaseFakeTime(alertAfterIntervalMs + 1);
                expect(publicationMissedSpy).toHaveBeenCalled();
                expect(publicationMissedSpy.calls.argsFor(0)[0] instanceof PublicationMissedException);
                expect(publicationMissedSpy.calls.argsFor(0)[0].subscriptionId).toEqual(subscriptionId);
                increaseFakeTime(alertAfterIntervalMs + 1);
                expect(publicationMissedSpy.calls.count()).toEqual(2);
                // expiryDate should be reached, expect no more interactions
                increaseFakeTime(alertAfterIntervalMs + 1);
                expect(publicationMissedSpy.calls.count()).toEqual(2);
                increaseFakeTime(alertAfterIntervalMs + 1);
                expect(publicationMissedSpy.calls.count()).toEqual(2);
                done();
                return null;
            })
            .catch(function(error) {
                log.error("Error in sendSubscriptionRequest :" + error);
                fail();
            });

        increaseFakeTime(1);
    });

    it("sets messagingQos.ttl correctly according to subscriptionQos.expiryDateMs", function(done) {
        var ttl = 250;
        var subscriptionSettings = {
            proxyId: "subscriber",
            providerDiscoveryEntry: providerDiscoveryEntry,
            attributeName: "testAttribute",
            attributeType: "String",
            qos: new OnChangeWithKeepAliveSubscriptionQos({
                alertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS,
                expiryDateMs: Date.now() + ttl
            }),
            onReceive: function() {},
            onError: function() {}
        };

        dispatcherSpy.sendSubscriptionRequest.calls.reset();
        subscriptionManager.registerSubscription(subscriptionSettings).catch(function(error) {
            expect("Error in sendSubscriptionRequest :" + error).toBeTruthy();
        });
        increaseFakeTime(1);

        waitsFor(
            function() {
                return dispatcherSpy.sendSubscriptionRequest.calls.count() === 1;
            },
            "dispatcherSpy.sendSubscriptionRequest called the first time",
            100
        )
            .then(function() {
                expect(dispatcherSpy.sendSubscriptionRequest.calls.argsFor(0)[0].messagingQos.ttl).toEqual(ttl);
                subscriptionSettings.qos.expiryDateMs = SubscriptionQos.NO_EXPIRY_DATE;
                subscriptionManager.registerSubscription(subscriptionSettings).catch(function(error) {
                    expect("Error in sendSubscriptionRequest :" + error).toBeTruthy();
                });
                increaseFakeTime(1);
                return waitsFor(
                    function() {
                        return dispatcherSpy.sendSubscriptionRequest.calls.count() === 2;
                    },
                    "dispatcherSpy.sendSubscriptionRequest called the first time",
                    100
                );
            })
            .then(function() {
                expect(dispatcherSpy.sendSubscriptionRequest.calls.count()).toEqual(2);
                expect(dispatcherSpy.sendSubscriptionRequest.calls.argsFor(1)[0].messagingQos.ttl).toEqual(
                    defaultMessagingSettings.MAX_MESSAGING_TTL_MS
                );
                done();
                return null;
            })
            .catch(fail);
    });

    it("forwards publication payload", function(done) {
        var publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        var publicationMissedSpy = jasmine.createSpy("publicationMissedSpy");
        var alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS;

        var resolveSpy = {
            // called when the subscription is registered successfully (see below)
            resolveMethod: function(subscriptionId) {
                // increase time by 50ms and see if alert was triggered
                increaseFakeTime(alertAfterIntervalMs / 2);
                expect(publicationMissedSpy).not.toHaveBeenCalled();
                var publication = new SubscriptionPublication({
                    response: ["test"],
                    subscriptionId: subscriptionId
                });
                // simulate incoming publication
                subscriptionManager.handlePublication(publication);
                // make sure publication payload is forwarded
                expect(publicationReceivedSpy).toHaveBeenCalledWith(publication.response[0]);
                increaseFakeTime(alertAfterIntervalMs / 2 + 1);
                // make sure no alert is triggered if publication is received
                expect(publicationMissedSpy).not.toHaveBeenCalled();
                increaseFakeTime(alertAfterIntervalMs + 1);
                // if no publications follow alert should be triggered again
                expect(publicationMissedSpy).toHaveBeenCalled();
            }
        };

        spyOn(resolveSpy, "resolveMethod").and.callThrough();

        //log.debug("registering subscription");
        // register the subscription and call the resolve method when ready
        subscriptionManager
            .registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry: providerDiscoveryEntry,
                messagingQos: new MessagingQos(),
                attributeName: "testAttribute",
                attributeType: "String",
                qos: new OnChangeWithKeepAliveSubscriptionQos({
                    alertAfterIntervalMs: alertAfterIntervalMs,
                    expiryDateMs: Date.now() + 50 + 2 * alertAfterIntervalMs
                }),
                onReceive: publicationReceivedSpy,
                onError: publicationMissedSpy
            })
            .then(resolveSpy.resolveMethod);
        increaseFakeTime(1);

        waitsFor(
            function() {
                // wait until the subscriptionReply was received
                return resolveSpy.resolveMethod.calls.count() > 0;
            },
            "resolveSpy.resolveMethod called",
            100
        )
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    it("augments incoming publications with information from the joynr type system", function(done) {
        var publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        var publicationMissedSpy = jasmine.createSpy("publicationMissedSpy");

        var resolveSpy = {
            // called when the subscription is registered successfully (see below)
            resolveMethod: function(subscriptionId) {
                // increase time by 50ms and see if alert was triggered
                increaseFakeTime(50);
                expect(publicationMissedSpy).not.toHaveBeenCalled();
                var publication = new SubscriptionPublication({
                    response: ["ZERO"],
                    subscriptionId: subscriptionId
                });
                // simulate incoming publication
                subscriptionManager.handlePublication(publication);
                // make sure publication payload is forwarded
                expect(publicationReceivedSpy).toHaveBeenCalledWith(TestEnum.ZERO);
            }
        };

        spyOn(resolveSpy, "resolveMethod").and.callThrough();

        //log.debug("registering subscription");
        // register the subscription and call the resolve method when ready
        /*jslint nomen: true */
        subscriptionManager
            .registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry: providerDiscoveryEntry,
                attributeName: "testAttribute",
                attributeType: TestEnum.ZERO._typeName,
                qos: new OnChangeSubscriptionQos({
                    expiryDateMs: Date.now() + 250
                }),
                onReceive: publicationReceivedSpy,
                onError: publicationMissedSpy
            })
            .then(resolveSpy.resolveMethod);
        increaseFakeTime(1);
        /*jslint nomen: false */

        waitsFor(
            function() {
                // wait until the subscriptionReply was received
                return resolveSpy.resolveMethod.calls.count() > 0;
            },
            "resolveSpy.resolveMethod called",
            100
        )
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    it("augments incoming broadcasts with information from the joynr type system", function(done) {
        var publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        var onErrorSpy = jasmine.createSpy("onErrorSpy");

        var resolveSpy = {
            // called when the subscription is registered successfully (see below)
            resolveMethod: function(subscriptionId) {
                var testString = "testString";
                var testInt = 2;
                var testEnum = TestEnum.ZERO;
                expect(onErrorSpy).not.toHaveBeenCalled();
                var publication = new SubscriptionPublication({
                    response: [testString, testInt, testEnum.name],
                    subscriptionId: subscriptionId
                });
                // simulate incoming publication
                subscriptionManager.handlePublication(publication);
                // make sure publication payload is forwarded
                expect(publicationReceivedSpy).toHaveBeenCalledWith([testString, testInt, testEnum]);
            }
        };

        spyOn(resolveSpy, "resolveMethod").and.callThrough();

        //log.debug("registering subscription");
        // register the subscription and call the resolve method when ready
        /*jslint nomen: true */
        subscriptionManager
            .registerBroadcastSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry: providerDiscoveryEntry,
                broadcastName: "broadcastName",
                broadcastParameter: [
                    {
                        name: "param1",
                        type: "String"
                    },
                    {
                        name: "param2",
                        type: "Integer"
                    },
                    {
                        name: "param3",
                        type: TestEnum.ZERO._typeName
                    }
                ],
                qos: new OnChangeSubscriptionQos({
                    expiryDateMs: Date.now() + 250
                }),
                onReceive: publicationReceivedSpy,
                onError: onErrorSpy
            })
            .then(resolveSpy.resolveMethod);
        increaseFakeTime(1);
        /*jslint nomen: false */

        waitsFor(
            function() {
                // wait until the subscriptionReply was received
                return resolveSpy.resolveMethod.calls.count() > 0;
            },
            "resolveSpy.resolveMethod called",
            100
        )
            .then(function() {
                done();
                return null;
            })
            .catch(fail);
    });

    function createDummyBroadcastSubscriptionRequest(parameters) {
        var onReceiveSpy = jasmine.createSpy("onReceiveSpy");
        var onErrorSpy = jasmine.createSpy("onErrorSpy");
        return {
            proxyId: "proxy",
            providerDiscoveryEntry: providerDiscoveryEntry,
            broadcastName: parameters.broadcastName,
            broadcastParameter: [
                {
                    name: "param1",
                    type: "String"
                }
            ],
            qos: new OnChangeSubscriptionQos({
                expiryDateMs: Date.now() + 250
            }),
            partitions: parameters.partitions,
            selective: parameters.selective,
            onReceive: onReceiveSpy,
            onError: onErrorSpy
        };
    }

    it("register multicast subscription request", function(done) {
        var request = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        expect(subscriptionManager.hasMulticastSubscriptions()).toBe(false);
        subscriptionManager
            .registerBroadcastSubscription(request)
            .then(function() {
                expect(dispatcherSpy.sendBroadcastSubscriptionRequest).toHaveBeenCalled();
                var forwardedRequest = dispatcherSpy.sendBroadcastSubscriptionRequest.calls.argsFor(0)[0]
                    .subscriptionRequest;
                expect(forwardedRequest instanceof MulticastSubscriptionRequest).toBe(true);
                expect(forwardedRequest.subscribedToName).toEqual(request.broadcastName);
                expect(subscriptionManager.hasMulticastSubscriptions()).toBe(true);
                done();
                return null;
            })
            .catch(fail);
    });

    it("register and unregisters multicast subscription request", function(done) {
        var request = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        expect(subscriptionManager.hasMulticastSubscriptions()).toBe(false);
        subscriptionManager
            .registerBroadcastSubscription(request)
            .then(function(subscriptionId) {
                expect(subscriptionManager.hasMulticastSubscriptions()).toBe(true);
                return subscriptionManager.unregisterSubscription({
                    subscriptionId: subscriptionId
                });
            })
            .then(function() {
                expect(subscriptionManager.hasMulticastSubscriptions()).toBe(false);
                expect(subscriptionManager.hasOpenSubscriptions()).toBe(false);
                done();
                return null;
            })
            .catch(fail);
    });

    it("is able to handle multicast publications", function(done) {
        var request = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        var response = ["response"];
        subscriptionManager
            .registerBroadcastSubscription(request)
            .then(function(subscriptionId) {
                request.subscriptionId = subscriptionId;
                request.multicastId = dispatcherSpy.sendBroadcastSubscriptionRequest.calls.argsFor(
                    0
                )[0].subscriptionRequest.multicastId;
                subscriptionManager.handleMulticastPublication({
                    multicastId: request.multicastId,
                    response: response
                });
                return null;
            })
            .then(function() {
                expect(request.onReceive).toHaveBeenCalled();
                expect(request.onReceive).toHaveBeenCalledWith(response);
                //stop subscription
                request.onReceive.calls.reset();
                return subscriptionManager.unregisterSubscription({
                    subscriptionId: request.subscriptionId
                });
            })
            .then(function() {
                //send another publication and do not expect calls
                expect(subscriptionManager.hasOpenSubscriptions()).toBe(false);
                expect(function() {
                    subscriptionManager.handleMulticastPublication({
                        multicastId: request.multicastId,
                        response: response
                    });
                }).toThrow();
                expect(request.onReceive).not.toHaveBeenCalled();
                done();
            })
            .catch(fail);
    });

    it("sends out subscriptionStop and stops alerts on unsubscribe", function(done) {
        var publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        var publicationMissedSpy = jasmine.createSpy("publicationMissedSpy");
        var alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS;
        var expectedDiscoveryEntry = Util.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(expectedDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(expectedDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        //log.debug("registering subscription");
        subscriptionManager
            .registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry: providerDiscoveryEntry,
                attributeName: "testAttribute",
                attributeType: "String",
                qos: new OnChangeWithKeepAliveSubscriptionQos({
                    alertAfterIntervalMs: alertAfterIntervalMs,
                    expiryDateMs: Date.now() + 5 * alertAfterIntervalMs
                }),
                onReceive: publicationReceivedSpy,
                onError: publicationMissedSpy
            })
            .then(function(subscriptionId) {
                increaseFakeTime(alertAfterIntervalMs / 2);
                expect(publicationMissedSpy).not.toHaveBeenCalled();
                increaseFakeTime(alertAfterIntervalMs / 2 + 1);
                expect(publicationMissedSpy).toHaveBeenCalled();
                increaseFakeTime(101);
                increaseFakeTime(alertAfterIntervalMs + 1);
                expect(publicationMissedSpy.calls.count()).toEqual(2);

                // unsubscribe and expect no more missed publication alerts
                var unsubscrMsgQos = new MessagingQos();
                subscriptionManager.unregisterSubscription({
                    subscriptionId: subscriptionId,
                    messagingQos: unsubscrMsgQos
                });

                var subscriptionStop = new SubscriptionStop({
                    subscriptionId: subscriptionId
                });

                expect(dispatcherSpy.sendSubscriptionStop).toHaveBeenCalledWith({
                    from: "subscriber",
                    toDiscoveryEntry: expectedDiscoveryEntry,
                    subscriptionStop: subscriptionStop,
                    messagingQos: unsubscrMsgQos
                });
                increaseFakeTime(alertAfterIntervalMs + 1);
                expect(publicationMissedSpy.calls.count()).toEqual(2);
                increaseFakeTime(alertAfterIntervalMs + 1);
                expect(publicationMissedSpy.calls.count()).toEqual(2);
                increaseFakeTime(alertAfterIntervalMs + 1);
                expect(publicationMissedSpy.calls.count()).toEqual(2);
                done();
                return null;
            })
            .catch(function(error) {
                log.error("Error in sendSubscriptionRequest :" + error);
                fail();
            });
        increaseFakeTime(1);
    });

    it("sends out MulticastSubscriptionStop", function(done) {
        var request = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        var expectedDiscoveryEntry = Util.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(expectedDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(expectedDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        subscriptionManager
            .registerBroadcastSubscription(request)
            .then(function(subscriptionId) {
                expect(dispatcherSpy.sendBroadcastSubscriptionRequest).toHaveBeenCalled();

                var multicastId = dispatcherSpy.sendBroadcastSubscriptionRequest.calls.argsFor(0)[0].subscriptionRequest
                    .multicastId;
                expect(multicastId).toBeDefined();
                expect(multicastId).not.toEqual(null);

                var unsubscrMsgQos = new MessagingQos();
                subscriptionManager.unregisterSubscription({
                    subscriptionId: subscriptionId,
                    messagingQos: unsubscrMsgQos
                });

                var subscriptionStop = new SubscriptionStop({
                    subscriptionId: subscriptionId
                });

                expect(dispatcherSpy.sendMulticastSubscriptionStop).toHaveBeenCalledWith({
                    from: request.proxyId,
                    toDiscoveryEntry: expectedDiscoveryEntry,
                    messagingQos: unsubscrMsgQos,
                    multicastId: multicastId,
                    subscriptionStop: subscriptionStop
                });
                done();
                return null;
            })
            .catch(function(error) {
                fail("caught error: " + error);
            });
    });

    it("returns a rejected promise when unsubscribing with a non-existant subscriptionId", function(done) {
        subscriptionManager
            .unregisterSubscription({
                subscriptionId: "non-existant"
            })
            .then()
            .catch(function(value) {
                expect(value).toBeDefined();
                var className = Object.prototype.toString.call(value).slice(8, -1);
                expect(className).toMatch("Error");
                done();
                return null;
            });
    });

    it("registers subscription, resolves with subscriptionId and calls onSubscribed callback", function(done) {
        var publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        var publicationErrorSpy = jasmine.createSpy("publicationErrorSpy");
        var publicationSubscribedSpy = jasmine.createSpy("publicationSubscribedSpy");
        dispatcherSpy.sendSubscriptionRequest.calls.reset();
        var expectedDiscoveryEntry = Util.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(providerDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(providerDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        subscriptionManager
            .registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry: providerDiscoveryEntry,
                attributeName: "testAttribute",
                attributeType: "String",
                qos: new OnChangeSubscriptionQos(),
                onReceive: publicationReceivedSpy,
                onError: publicationErrorSpy,
                onSubscribed: publicationSubscribedSpy
            })
            .then(function(receivedSubscriptionId) {
                expect(receivedSubscriptionId).toBeDefined();
                expect(receivedSubscriptionId).toEqual(storedSubscriptionId);
                return waitsFor(
                    function() {
                        return publicationSubscribedSpy.calls.count() === 1;
                    },
                    "publicationSubscribedSpy called",
                    1000
                );
            })
            .then(function() {
                expect(publicationReceivedSpy).not.toHaveBeenCalled();
                expect(publicationErrorSpy).not.toHaveBeenCalled();
                expect(publicationSubscribedSpy).toHaveBeenCalled();
                expect(publicationSubscribedSpy.calls.argsFor(0)[0]).toEqual(storedSubscriptionId);
                expect(dispatcherSpy.sendSubscriptionRequest).toHaveBeenCalled();
                expect(dispatcherSpy.sendSubscriptionRequest.calls.argsFor(0)[0].toDiscoveryEntry).toEqual(
                    expectedDiscoveryEntry
                );
                done();
                return null;
            })
            .catch(fail);

        increaseFakeTime(1);
    });

    it("registers broadcast subscription, resolves with subscriptionId and calls onSubscribed callback", function(
        done
    ) {
        var publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        var publicationErrorSpy = jasmine.createSpy("publicationErrorSpy");
        var publicationSubscribedSpy = jasmine.createSpy("publicationSubscribedSpy");

        subscriptionManager
            .registerBroadcastSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry: providerDiscoveryEntry,
                broadcastName: "broadcastName",
                qos: new OnChangeSubscriptionQos(),
                onReceive: publicationReceivedSpy,
                onError: publicationErrorSpy,
                onSubscribed: publicationSubscribedSpy
            })
            .then(function(receivedSubscriptionId) {
                expect(receivedSubscriptionId).toBeDefined();
                expect(receivedSubscriptionId).toEqual(storedSubscriptionId);
                return waitsFor(
                    function() {
                        return publicationSubscribedSpy.calls.count() === 1;
                    },
                    "publicationSubscribedSpy called",
                    1000
                );
            })
            .then(function() {
                expect(publicationReceivedSpy).not.toHaveBeenCalled();
                expect(publicationErrorSpy).not.toHaveBeenCalled();
                expect(publicationSubscribedSpy).toHaveBeenCalled();
                expect(publicationSubscribedSpy.calls.argsFor(0)[0]).toEqual(storedSubscriptionId);
                done();
                return null;
            })
            .catch(fail);

        increaseFakeTime(1);
    });

    it("rejects on subscription registration failures and calls onError callback", function(done) {
        var publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        var publicationErrorSpy = jasmine.createSpy("publicationErrorSpy");
        var publicationSubscribedSpy = jasmine.createSpy("publicationSubscribedSpy");

        subscriptionManagerOnError
            .registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry: providerDiscoveryEntry,
                attributeName: "testAttribute",
                attributeType: "String",
                qos: new OnChangeSubscriptionQos(),
                onReceive: publicationReceivedSpy,
                onError: publicationErrorSpy,
                onSubscribed: publicationSubscribedSpy
            })
            .then(function(subscriptionId) {
                fail("unexpected success");
            })
            .catch(function(error) {
                expect(error instanceof SubscriptionException);
                expect(error.subscriptionId).toBeDefined();
                expect(error.subscriptionId).toEqual(storedSubscriptionId);

                return waitsFor(
                    function() {
                        return publicationErrorSpy.calls.count() === 1;
                    },
                    "publicationErrorSpy called",
                    1000
                );
            })
            .then(function() {
                expect(publicationReceivedSpy).not.toHaveBeenCalled();
                expect(publicationSubscribedSpy).not.toHaveBeenCalled();
                expect(publicationErrorSpy).toHaveBeenCalled();
                expect(publicationErrorSpy.calls.argsFor(0)[0] instanceof SubscriptionException);
                expect(publicationErrorSpy.calls.argsFor(0)[0].subscriptionId).toEqual(storedSubscriptionId);
                done();
                return null;
            })
            .catch(fail);

        increaseFakeTime(1);
    });

    it("rejects on broadcast subscription registration failures and calls onError callback", function(done) {
        var publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        var publicationErrorSpy = jasmine.createSpy("publicationErrorSpy");
        var publicationSubscribedSpy = jasmine.createSpy("publicationSubscribedSpy");

        subscriptionManagerOnError
            .registerBroadcastSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry: providerDiscoveryEntry,
                broadcastName: "broadcastName",
                qos: new OnChangeSubscriptionQos(),
                onReceive: publicationReceivedSpy,
                onError: publicationErrorSpy,
                onSubscribed: publicationSubscribedSpy
            })
            .then(function(subscriptionId) {
                fail("unexpected success");
            })
            .catch(function(error) {
                expect(error instanceof SubscriptionException);
                expect(error.subscriptionId).toBeDefined();
                expect(error.subscriptionId).toEqual(storedSubscriptionId);

                return waitsFor(
                    function() {
                        return publicationErrorSpy.calls.count() === 1;
                    },
                    "publicationErrorSpy called",
                    1000
                );
            })
            .then(function() {
                expect(publicationReceivedSpy).not.toHaveBeenCalled();
                expect(publicationSubscribedSpy).not.toHaveBeenCalled();
                expect(publicationErrorSpy).toHaveBeenCalled();
                expect(publicationErrorSpy.calls.argsFor(0)[0] instanceof SubscriptionException);
                expect(publicationErrorSpy.calls.argsFor(0)[0].subscriptionId).toEqual(storedSubscriptionId);
                done();
                return null;
            })
            .catch(fail);

        increaseFakeTime(1);
    });

    it(" throws exception when called while shut down", function(done) {
        subscriptionManager.shutdown();

        subscriptionManager
            .registerSubscription({})
            .then(fail)
            .catch(function() {
                return subscriptionManager.registerBroadcastSubscription({}).then(fail);
            })
            .catch(function() {
                expect(function() {
                    subscriptionManager.unregisterSubscription({});
                }).toThrow();
                done();
            });
    });
});
