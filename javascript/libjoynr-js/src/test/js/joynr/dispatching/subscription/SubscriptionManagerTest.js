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
require("../../../node-unit-test-helper");
const SubscriptionManager = require("../../../../../main/js/joynr/dispatching/subscription/SubscriptionManager");
const MessagingQos = require("../../../../../main/js/joynr/messaging/MessagingQos");
const defaultMessagingSettings = require("../../../../../main/js/joynr/start/settings/defaultMessagingSettings");
const MulticastSubscriptionRequest = require("../../../../../main/js/joynr/dispatching/types/MulticastSubscriptionRequest");
const SubscriptionStop = require("../../../../../main/js/joynr/dispatching/types/SubscriptionStop");
const OnChangeWithKeepAliveSubscriptionQos = require("../../../../../main/js/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos");
const OnChangeSubscriptionQos = require("../../../../../main/js/joynr/proxy/OnChangeSubscriptionQos");
const SubscriptionQos = require("../../../../../main/js/joynr/proxy/SubscriptionQos");
const SubscriptionPublication = require("../../../../../main/js/joynr/dispatching/types/SubscriptionPublication");
const Promise = require("../../../../../main/js/global/Promise");
const PublicationMissedException = require("../../../../../main/js/joynr/exceptions/PublicationMissedException");
const SubscriptionException = require("../../../../../main/js/joynr/exceptions/SubscriptionException");
const LoggingManager = require("../../../../../main/js/joynr/system/LoggingManager");
const Date = require("../../../../../test/js/global/Date");
const waitsFor = require("../../../../../test/js/global/WaitsFor");
const TestEnum = require("../../../../generated/joynr/tests/testTypes/TestEnum");
const TypeRegistrySingleton = require("../../../../../main/js/joynr/types/TypeRegistrySingleton");
const DiscoveryEntryWithMetaInfo = require("../../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo");
const Version = require("../../../../../main/js/generated/joynr/types/Version");
const ProviderQos = require("../../../../../main/js/generated/joynr/types/ProviderQos");
const Util = require("../../../../../main/js/joynr/util/UtilInternal");

describe("libjoynr-js.joynr.dispatching.subscription.SubscriptionManager", () => {
    let subscriptionManager;
    let subscriptionManagerOnError;
    const log = LoggingManager.getLogger("joynr.dispatching.SubscriptionManagerTest");
    let fakeTime = 1371553100000;
    let dispatcherSpy;
    let dispatcherSpyOnError;
    let storedSubscriptionId;
    let providerDiscoveryEntry;

    /**
     * Called before each test.
     */
    beforeEach(done => {
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

        dispatcherSpy.sendBroadcastSubscriptionRequest.and.callFake(settings => {
            storedSubscriptionId = settings.subscriptionRequest.subscriptionId;
            subscriptionManager.handleSubscriptionReply({
                subscriptionId: settings.subscriptionRequest.subscriptionId
            });
            return Promise.resolve();
        });

        dispatcherSpy.sendSubscriptionRequest.and.callFake(settings => {
            storedSubscriptionId = settings.subscriptionRequest.subscriptionId;
            subscriptionManager.handleSubscriptionReply({
                subscriptionId: settings.subscriptionRequest.subscriptionId
            });
            return Promise.resolve();
        });

        dispatcherSpy.sendSubscriptionStop.and.callFake(() => {
            return Promise.resolve();
        });

        subscriptionManager = new SubscriptionManager(dispatcherSpy);

        dispatcherSpyOnError = jasmine.createSpyObj("DispatcherSpyOnError", [
            "sendSubscriptionRequest",
            "sendBroadcastSubscriptionRequest",
            "sendMulticastSubscriptionStop",
            "sendSubscriptionStop"
        ]);

        dispatcherSpyOnError.sendBroadcastSubscriptionRequest.and.callFake(settings => {
            storedSubscriptionId = settings.subscriptionRequest.subscriptionId;
            subscriptionManagerOnError.handleSubscriptionReply({
                subscriptionId: settings.subscriptionRequest.subscriptionId,
                error: new SubscriptionException({
                    subscriptionId: settings.subscriptionRequest.subscriptionId
                })
            });
            return Promise.resolve();
        });

        dispatcherSpyOnError.sendSubscriptionRequest.and.callFake(settings => {
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
        spyOn(Date, "now").and.callFake(() => {
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
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    afterEach(() => {
        jasmine.clock().uninstall();
    });

    function increaseFakeTime(time_ms) {
        fakeTime = fakeTime + time_ms;
        jasmine.clock().tick(time_ms);
    }

    it("is instantiable", done => {
        expect(subscriptionManager).toBeDefined();
        done();
    });

    it("sends broadcast subscription requests", done => {
        const ttl = 250;
        const parameters = {
            proxyId: "subscriber",
            providerDiscoveryEntry,
            broadcastName: "broadcastName",
            subscriptionQos: new OnChangeSubscriptionQos({
                expiryDateMs: Date.now() + ttl
            })
        };
        let expectedDiscoveryEntry = Util.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(expectedDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(expectedDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        dispatcherSpy.sendBroadcastSubscriptionRequest.calls.reset();
        const spySubscribePromise = jasmine.createSpyObj("spySubscribePromise", ["resolve", "reject"]);

        subscriptionManager
            .registerBroadcastSubscription(parameters)
            .then(spySubscribePromise.resolve)
            .catch(spySubscribePromise.reject);
        increaseFakeTime(1);

        waitsFor(
            () => {
                return dispatcherSpy.sendBroadcastSubscriptionRequest.calls.count() === 1;
            },
            "dispatcherSpy.sendBroadcastSubscriptionRequest called",
            100
        )
            .then(() => {
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

    it("alerts on missed publication and stops", done => {
        const publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        const publicationMissedSpy = jasmine.createSpy("publicationMissedSpy");
        const alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS;

        //log.debug("registering subscription");
        subscriptionManager
            .registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry,
                attributeName: "testAttribute",
                attributeType: "String",
                qos: new OnChangeWithKeepAliveSubscriptionQos({
                    alertAfterIntervalMs,
                    expiryDateMs: Date.now() + 50 + 2 * alertAfterIntervalMs
                }),
                onReceive: publicationReceivedSpy,
                onError: publicationMissedSpy
            })
            .then(subscriptionId => {
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
            .catch(error => {
                log.error("Error in sendSubscriptionRequest :" + error);
                fail();
            });

        increaseFakeTime(1);
    });

    it("sets messagingQos.ttl correctly according to subscriptionQos.expiryDateMs", done => {
        const ttl = 250;
        const subscriptionSettings = {
            proxyId: "subscriber",
            providerDiscoveryEntry,
            attributeName: "testAttribute",
            attributeType: "String",
            qos: new OnChangeWithKeepAliveSubscriptionQos({
                alertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS,
                expiryDateMs: Date.now() + ttl
            }),
            onReceive() {},
            onError() {}
        };

        dispatcherSpy.sendSubscriptionRequest.calls.reset();
        subscriptionManager.registerSubscription(subscriptionSettings).catch(error => {
            expect("Error in sendSubscriptionRequest :" + error).toBeTruthy();
        });
        increaseFakeTime(1);

        waitsFor(
            () => {
                return dispatcherSpy.sendSubscriptionRequest.calls.count() === 1;
            },
            "dispatcherSpy.sendSubscriptionRequest called the first time",
            100
        )
            .then(() => {
                expect(dispatcherSpy.sendSubscriptionRequest.calls.argsFor(0)[0].messagingQos.ttl).toEqual(ttl);
                subscriptionSettings.qos.expiryDateMs = SubscriptionQos.NO_EXPIRY_DATE;
                subscriptionManager.registerSubscription(subscriptionSettings).catch(error => {
                    expect("Error in sendSubscriptionRequest :" + error).toBeTruthy();
                });
                increaseFakeTime(1);
                return waitsFor(
                    () => {
                        return dispatcherSpy.sendSubscriptionRequest.calls.count() === 2;
                    },
                    "dispatcherSpy.sendSubscriptionRequest called the first time",
                    100
                );
            })
            .then(() => {
                expect(dispatcherSpy.sendSubscriptionRequest.calls.count()).toEqual(2);
                expect(dispatcherSpy.sendSubscriptionRequest.calls.argsFor(1)[0].messagingQos.ttl).toEqual(
                    defaultMessagingSettings.MAX_MESSAGING_TTL_MS
                );
                done();
                return null;
            })
            .catch(fail);
    });

    it("forwards publication payload", done => {
        const publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        const publicationMissedSpy = jasmine.createSpy("publicationMissedSpy");
        const alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS;

        const resolveSpy = {
            // called when the subscription is registered successfully (see below)
            resolveMethod(subscriptionId) {
                // increase time by 50ms and see if alert was triggered
                increaseFakeTime(alertAfterIntervalMs / 2);
                expect(publicationMissedSpy).not.toHaveBeenCalled();
                const publication = new SubscriptionPublication({
                    response: ["test"],
                    subscriptionId
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
                providerDiscoveryEntry,
                messagingQos: new MessagingQos(),
                attributeName: "testAttribute",
                attributeType: "String",
                qos: new OnChangeWithKeepAliveSubscriptionQos({
                    alertAfterIntervalMs,
                    expiryDateMs: Date.now() + 50 + 2 * alertAfterIntervalMs
                }),
                onReceive: publicationReceivedSpy,
                onError: publicationMissedSpy
            })
            .then(resolveSpy.resolveMethod);
        increaseFakeTime(1);

        waitsFor(
            () => {
                // wait until the subscriptionReply was received
                return resolveSpy.resolveMethod.calls.count() > 0;
            },
            "resolveSpy.resolveMethod called",
            100
        )
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("augments incoming publications with information from the joynr type system", done => {
        const publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        const publicationMissedSpy = jasmine.createSpy("publicationMissedSpy");

        const resolveSpy = {
            // called when the subscription is registered successfully (see below)
            resolveMethod(subscriptionId) {
                // increase time by 50ms and see if alert was triggered
                increaseFakeTime(50);
                expect(publicationMissedSpy).not.toHaveBeenCalled();
                const publication = new SubscriptionPublication({
                    response: ["ZERO"],
                    subscriptionId
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
        subscriptionManager
            .registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry,
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

        waitsFor(
            () => {
                // wait until the subscriptionReply was received
                return resolveSpy.resolveMethod.calls.count() > 0;
            },
            "resolveSpy.resolveMethod called",
            100
        )
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("augments incoming broadcasts with information from the joynr type system", done => {
        const publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        const onErrorSpy = jasmine.createSpy("onErrorSpy");

        const resolveSpy = {
            // called when the subscription is registered successfully (see below)
            resolveMethod(subscriptionId) {
                const testString = "testString";
                const testInt = 2;
                const testEnum = TestEnum.ZERO;
                expect(onErrorSpy).not.toHaveBeenCalled();
                const publication = new SubscriptionPublication({
                    response: [testString, testInt, testEnum.name],
                    subscriptionId
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
        subscriptionManager
            .registerBroadcastSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry,
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

        waitsFor(
            () => {
                // wait until the subscriptionReply was received
                return resolveSpy.resolveMethod.calls.count() > 0;
            },
            "resolveSpy.resolveMethod called",
            100
        )
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    function createDummyBroadcastSubscriptionRequest(parameters) {
        const onReceiveSpy = jasmine.createSpy("onReceiveSpy");
        const onErrorSpy = jasmine.createSpy("onErrorSpy");
        return {
            proxyId: "proxy",
            providerDiscoveryEntry,
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

    function createDummySubscriptionRequest() {
        const onReceiveSpy = jasmine.createSpy("onReceiveSpy");
        const onErrorSpy = jasmine.createSpy("onErrorSpy");

        return {
            proxyId: "subscriber",
            providerDiscoveryEntry,
            attributeName: "testAttribute",
            attributeType: "String",
            qos: new OnChangeSubscriptionQos({
                expiryDateMs: Date.now() + 250
            }),
            onReceive: onReceiveSpy,
            onError: onErrorSpy
        };
    }

    it("register multicast subscription request", done => {
        const request = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        expect(subscriptionManager.hasMulticastSubscriptions()).toBe(false);
        subscriptionManager
            .registerBroadcastSubscription(request)
            .then(() => {
                expect(dispatcherSpy.sendBroadcastSubscriptionRequest).toHaveBeenCalled();
                const forwardedRequest = dispatcherSpy.sendBroadcastSubscriptionRequest.calls.argsFor(0)[0]
                    .subscriptionRequest;
                expect(forwardedRequest instanceof MulticastSubscriptionRequest).toBe(true);
                expect(forwardedRequest.subscribedToName).toEqual(request.broadcastName);
                expect(subscriptionManager.hasMulticastSubscriptions()).toBe(true);
                done();
                return null;
            })
            .catch(fail);
    });

    it("register and unregisters multicast subscription request", done => {
        const request = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        expect(subscriptionManager.hasMulticastSubscriptions()).toBe(false);
        subscriptionManager
            .registerBroadcastSubscription(request)
            .then(subscriptionId => {
                expect(subscriptionManager.hasMulticastSubscriptions()).toBe(true);
                return subscriptionManager.unregisterSubscription({
                    subscriptionId
                });
            })
            .then(() => {
                expect(subscriptionManager.hasMulticastSubscriptions()).toBe(false);
                expect(subscriptionManager.hasOpenSubscriptions()).toBe(false);
                done();
                return null;
            })
            .catch(fail);
    });

    it("is able to handle multicast publications", done => {
        const request = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        const response = ["response"];
        subscriptionManager
            .registerBroadcastSubscription(request)
            .then(subscriptionId => {
                request.subscriptionId = subscriptionId;
                request.multicastId = dispatcherSpy.sendBroadcastSubscriptionRequest.calls.argsFor(
                    0
                )[0].subscriptionRequest.multicastId;
                subscriptionManager.handleMulticastPublication({
                    multicastId: request.multicastId,
                    response
                });
                return null;
            })
            .then(() => {
                expect(request.onReceive).toHaveBeenCalled();
                expect(request.onReceive).toHaveBeenCalledWith(response);
                //stop subscription
                request.onReceive.calls.reset();
                return subscriptionManager.unregisterSubscription({
                    subscriptionId: request.subscriptionId
                });
            })
            .then(() => {
                //send another publication and do not expect calls
                expect(subscriptionManager.hasOpenSubscriptions()).toBe(false);
                expect(() => {
                    subscriptionManager.handleMulticastPublication({
                        multicastId: request.multicastId,
                        response
                    });
                }).toThrow();
                expect(request.onReceive).not.toHaveBeenCalled();
                done();
            })
            .catch(fail);
    });

    it("sends out subscriptionStop and stops alerts on unsubscribe", done => {
        const publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        const publicationMissedSpy = jasmine.createSpy("publicationMissedSpy");
        const alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS;
        let expectedDiscoveryEntry = Util.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(expectedDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(expectedDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        //log.debug("registering subscription");
        subscriptionManager
            .registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry,
                attributeName: "testAttribute",
                attributeType: "String",
                qos: new OnChangeWithKeepAliveSubscriptionQos({
                    alertAfterIntervalMs,
                    expiryDateMs: Date.now() + 5 * alertAfterIntervalMs
                }),
                onReceive: publicationReceivedSpy,
                onError: publicationMissedSpy
            })
            .then(subscriptionId => {
                increaseFakeTime(alertAfterIntervalMs / 2);
                expect(publicationMissedSpy).not.toHaveBeenCalled();
                increaseFakeTime(alertAfterIntervalMs / 2 + 1);
                expect(publicationMissedSpy).toHaveBeenCalled();
                increaseFakeTime(101);
                increaseFakeTime(alertAfterIntervalMs + 1);
                expect(publicationMissedSpy.calls.count()).toEqual(2);

                // unsubscribe and expect no more missed publication alerts
                const unsubscrMsgQos = new MessagingQos();
                subscriptionManager.unregisterSubscription({
                    subscriptionId,
                    messagingQos: unsubscrMsgQos
                });

                const subscriptionStop = new SubscriptionStop({
                    subscriptionId
                });

                expect(dispatcherSpy.sendSubscriptionStop).toHaveBeenCalledWith({
                    from: "subscriber",
                    toDiscoveryEntry: expectedDiscoveryEntry,
                    subscriptionStop,
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
            .catch(error => {
                log.error("Error in sendSubscriptionRequest :" + error);
                fail();
            });
        increaseFakeTime(1);
    });

    it("sends out MulticastSubscriptionStop", done => {
        const request = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        let expectedDiscoveryEntry = Util.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(expectedDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(expectedDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        subscriptionManager
            .registerBroadcastSubscription(request)
            .then(subscriptionId => {
                expect(dispatcherSpy.sendBroadcastSubscriptionRequest).toHaveBeenCalled();

                const multicastId = dispatcherSpy.sendBroadcastSubscriptionRequest.calls.argsFor(0)[0]
                    .subscriptionRequest.multicastId;
                expect(multicastId).toBeDefined();
                expect(multicastId).not.toEqual(null);

                const unsubscrMsgQos = new MessagingQos();
                subscriptionManager.unregisterSubscription({
                    subscriptionId,
                    messagingQos: unsubscrMsgQos
                });

                const subscriptionStop = new SubscriptionStop({
                    subscriptionId
                });

                expect(dispatcherSpy.sendMulticastSubscriptionStop).toHaveBeenCalledWith({
                    from: request.proxyId,
                    toDiscoveryEntry: expectedDiscoveryEntry,
                    messagingQos: unsubscrMsgQos,
                    multicastId,
                    subscriptionStop
                });
                done();
                return null;
            })
            .catch(error => {
                fail("caught error: " + error);
            });
    });

    it("returns a rejected promise when unsubscribing with a non-existant subscriptionId", done => {
        subscriptionManager
            .unregisterSubscription({
                subscriptionId: "non-existant"
            })
            .then()
            .catch(value => {
                expect(value).toBeDefined();
                const className = Object.prototype.toString.call(value).slice(8, -1);
                expect(className).toMatch("Error");
                done();
                return null;
            });
    });

    it("registers subscription, resolves with subscriptionId and calls onSubscribed callback", done => {
        const publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        const publicationErrorSpy = jasmine.createSpy("publicationErrorSpy");
        const publicationSubscribedSpy = jasmine.createSpy("publicationSubscribedSpy");
        dispatcherSpy.sendSubscriptionRequest.calls.reset();
        let expectedDiscoveryEntry = Util.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(providerDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(providerDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        subscriptionManager
            .registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry,
                attributeName: "testAttribute",
                attributeType: "String",
                qos: new OnChangeSubscriptionQos(),
                onReceive: publicationReceivedSpy,
                onError: publicationErrorSpy,
                onSubscribed: publicationSubscribedSpy
            })
            .then(receivedSubscriptionId => {
                expect(receivedSubscriptionId).toBeDefined();
                expect(receivedSubscriptionId).toEqual(storedSubscriptionId);
                return waitsFor(
                    () => {
                        return publicationSubscribedSpy.calls.count() === 1;
                    },
                    "publicationSubscribedSpy called",
                    1000
                );
            })
            .then(() => {
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

    it("registers broadcast subscription, resolves with subscriptionId and calls onSubscribed callback", done => {
        const publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        const publicationErrorSpy = jasmine.createSpy("publicationErrorSpy");
        const publicationSubscribedSpy = jasmine.createSpy("publicationSubscribedSpy");

        subscriptionManager
            .registerBroadcastSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry,
                broadcastName: "broadcastName",
                qos: new OnChangeSubscriptionQos(),
                onReceive: publicationReceivedSpy,
                onError: publicationErrorSpy,
                onSubscribed: publicationSubscribedSpy
            })
            .then(receivedSubscriptionId => {
                expect(receivedSubscriptionId).toBeDefined();
                expect(receivedSubscriptionId).toEqual(storedSubscriptionId);
                return waitsFor(
                    () => {
                        return publicationSubscribedSpy.calls.count() === 1;
                    },
                    "publicationSubscribedSpy called",
                    1000
                );
            })
            .then(() => {
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

    it("rejects on subscription registration failures and calls onError callback", done => {
        const publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        const publicationErrorSpy = jasmine.createSpy("publicationErrorSpy");
        const publicationSubscribedSpy = jasmine.createSpy("publicationSubscribedSpy");

        subscriptionManagerOnError
            .registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry,
                attributeName: "testAttribute",
                attributeType: "String",
                qos: new OnChangeSubscriptionQos(),
                onReceive: publicationReceivedSpy,
                onError: publicationErrorSpy,
                onSubscribed: publicationSubscribedSpy
            })
            .then(subscriptionId => {
                fail("unexpected success: " + subscriptionId);
            })
            .catch(error => {
                expect(error instanceof SubscriptionException);
                expect(error.subscriptionId).toBeDefined();
                expect(error.subscriptionId).toEqual(storedSubscriptionId);

                return waitsFor(
                    () => {
                        return publicationErrorSpy.calls.count() === 1;
                    },
                    "publicationErrorSpy called",
                    1000
                );
            })
            .then(() => {
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

    it("rejects on broadcast subscription registration failures and calls onError callback", done => {
        const publicationReceivedSpy = jasmine.createSpy("publicationReceivedSpy");
        const publicationErrorSpy = jasmine.createSpy("publicationErrorSpy");
        const publicationSubscribedSpy = jasmine.createSpy("publicationSubscribedSpy");

        subscriptionManagerOnError
            .registerBroadcastSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry,
                broadcastName: "broadcastName",
                qos: new OnChangeSubscriptionQos(),
                onReceive: publicationReceivedSpy,
                onError: publicationErrorSpy,
                onSubscribed: publicationSubscribedSpy
            })
            .then(subscriptionId => {
                fail("unexpected success: " + subscriptionId);
            })
            .catch(error => {
                expect(error instanceof SubscriptionException);
                expect(error.subscriptionId).toBeDefined();
                expect(error.subscriptionId).toEqual(storedSubscriptionId);

                return waitsFor(
                    () => {
                        return publicationErrorSpy.calls.count() === 1;
                    },
                    "publicationErrorSpy called",
                    1000
                );
            })
            .then(() => {
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

    it(" throws exception when called while shut down", done => {
        subscriptionManager.shutdown();

        subscriptionManager
            .registerSubscription({})
            .then(fail)
            .catch(() => {
                return subscriptionManager.registerBroadcastSubscription({}).then(fail);
            })
            .catch(() => {
                expect(() => {
                    subscriptionManager.unregisterSubscription({});
                }).toThrow();
                done();
            });
    });

    it(" it unsubscribes all Subscriptions when terminateSubscriptions is being called", function(done) {
        const subscriptionSettings = createDummySubscriptionRequest();
        const broadcastSettings = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        const clearSubscriptionsTimeoutMs = 1000;

        subscriptionManager
            .registerSubscription(subscriptionSettings)
            .then(subscriptionManager.registerBroadcastSubscription.bind(this, broadcastSettings))
            .then(subscriptionManager.terminateSubscriptions.bind(subscriptionManager, clearSubscriptionsTimeoutMs))
            .then(subscriptionManager.shutdown)
            .then(() => {
                expect(dispatcherSpy.sendSubscriptionStop).toHaveBeenCalled();
                expect(dispatcherSpy.sendMulticastSubscriptionStop).toHaveBeenCalled();
                done();
            })
            .catch(fail);
    });
});
