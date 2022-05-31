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

import SubscriptionManager from "../../../../../main/js/joynr/dispatching/subscription/SubscriptionManager";
import MessagingQos from "../../../../../main/js/joynr/messaging/MessagingQos";
import defaultMessagingSettings from "../../../../../main/js/joynr/start/settings/defaultMessagingSettings";
import MulticastSubscriptionRequest from "../../../../../main/js/joynr/dispatching/types/MulticastSubscriptionRequest";
import SubscriptionStop from "../../../../../main/js/joynr/dispatching/types/SubscriptionStop";
import OnChangeWithKeepAliveSubscriptionQos from "../../../../../main/js/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos";
import OnChangeSubscriptionQos from "../../../../../main/js/joynr/proxy/OnChangeSubscriptionQos";
import SubscriptionQos from "../../../../../main/js/joynr/proxy/SubscriptionQos";
import * as SubscriptionPublication from "../../../../../main/js/joynr/dispatching/types/SubscriptionPublication";
import PublicationMissedException from "../../../../../main/js/joynr/exceptions/PublicationMissedException";
import SubscriptionException from "../../../../../main/js/joynr/exceptions/SubscriptionException";
import TestEnum from "../../../../generated/joynr/tests/testTypes/TestEnum";
import TypeRegistrySingleton from "../../../../../main/js/joynr/types/TypeRegistrySingleton";
import DiscoveryEntryWithMetaInfo from "../../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo";
import Version from "../../../../../main/js/generated/joynr/types/Version";
import ProviderQos from "../../../../../main/js/generated/joynr/types/ProviderQos";
import * as UtilInternal from "../../../../../main/js/joynr/util/UtilInternal";
import testUtil = require("../../../testUtil");

describe("libjoynr-js.joynr.dispatching.subscription.SubscriptionManager", () => {
    let subscriptionManager: SubscriptionManager;
    let subscriptionManagerOnError: any;
    let fakeTime = 1371553100000;
    let dispatcherSpy: any;
    let dispatcherSpyOnError: any;
    let storedSubscriptionId: any;
    let providerDiscoveryEntry: any;

    /**
     * Called before each test.
     */
    beforeEach(() => {
        providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
            domain: "testProviderDomain",
            interfaceName: "interfaceName",
            participantId: "providerParticipantId",
            qos: new ProviderQos({} as any),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "publicKeyId",
            isLocal: true
        });
        dispatcherSpy = {
            sendSubscriptionRequest: jest.fn(),
            sendBroadcastSubscriptionRequest: jest.fn(),
            sendMulticastSubscriptionStop: jest.fn(),
            sendSubscriptionStop: jest.fn()
        };

        dispatcherSpy.sendBroadcastSubscriptionRequest.mockImplementation((settings: any) => {
            storedSubscriptionId = settings.subscriptionRequest.subscriptionId;
            subscriptionManager.handleSubscriptionReply({
                subscriptionId: settings.subscriptionRequest.subscriptionId
            } as any);
            return Promise.resolve();
        });

        dispatcherSpy.sendSubscriptionRequest.mockImplementation((settings: any) => {
            storedSubscriptionId = settings.subscriptionRequest.subscriptionId;
            subscriptionManager.handleSubscriptionReply({
                subscriptionId: settings.subscriptionRequest.subscriptionId
            } as any);
            return Promise.resolve();
        });

        dispatcherSpy.sendSubscriptionStop.mockImplementation(() => {
            return Promise.resolve();
        });

        subscriptionManager = new SubscriptionManager(dispatcherSpy);

        dispatcherSpyOnError = {
            sendSubscriptionRequest: jest.fn(),
            sendBroadcastSubscriptionRequest: jest.fn(),
            sendMulticastSubscriptionStop: jest.fn(),
            sendSubscriptionStop: jest.fn()
        };

        dispatcherSpyOnError.sendBroadcastSubscriptionRequest.mockImplementation((settings: any) => {
            storedSubscriptionId = settings.subscriptionRequest.subscriptionId;
            subscriptionManagerOnError.handleSubscriptionReply({
                _typeName: "",
                subscriptionId: settings.subscriptionRequest.subscriptionId,
                error: new SubscriptionException({
                    subscriptionId: settings.subscriptionRequest.subscriptionId,
                    detailMessage: ""
                })
            });
            return Promise.resolve();
        });

        dispatcherSpyOnError.sendSubscriptionRequest.mockImplementation((settings: any) => {
            storedSubscriptionId = settings.subscriptionRequest.subscriptionId;
            subscriptionManagerOnError.handleSubscriptionReply({
                _typeName: "",
                subscriptionId: settings.subscriptionRequest.subscriptionId,
                error: new SubscriptionException({
                    subscriptionId: settings.subscriptionRequest.subscriptionId,
                    detailMessage: ""
                })
            });
            return Promise.resolve();
        });

        subscriptionManagerOnError = new SubscriptionManager(dispatcherSpyOnError);

        jest.useFakeTimers();
        jest.spyOn(Date, "now").mockImplementation(() => {
            return fakeTime;
        });

        /*
         * Make sure 'TestEnum' is properly registered as a type.
         * Just requiring the module is insufficient since the
         * automatically generated code called async methods.
         * Execution might be still in progress.
         */
        TypeRegistrySingleton.getInstance().addType(TestEnum);
    });

    afterEach(() => {
        jest.useRealTimers();
    });

    async function increaseFakeTime(timeMs: number): Promise<void> {
        fakeTime = fakeTime + timeMs;
        jest.advanceTimersByTime(timeMs);
        await testUtil.multipleSetImmediate();
    }

    it("is instantiable", () => {
        expect(subscriptionManager).toBeDefined();
    });

    it("sends broadcast subscription requests", async () => {
        const ttl = 250;
        const parameters = {
            proxyId: "subscriber",
            providerDiscoveryEntry,
            broadcastName: "broadcastName",
            subscriptionQos: new OnChangeSubscriptionQos({
                expiryDateMs: Date.now() + ttl
            })
        };
        let expectedDiscoveryEntry: any = UtilInternal.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(expectedDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(expectedDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        dispatcherSpy.sendBroadcastSubscriptionRequest.mockClear();

        await subscriptionManager.registerBroadcastSubscription(parameters as any);

        expect(dispatcherSpy.sendBroadcastSubscriptionRequest).toHaveBeenCalled();
        expect(dispatcherSpy.sendBroadcastSubscriptionRequest.mock.calls[0][0].messagingQos.ttl).toEqual(ttl);
        expect(dispatcherSpy.sendBroadcastSubscriptionRequest.mock.calls[0][0].toDiscoveryEntry).toEqual(
            expectedDiscoveryEntry
        );
        expect(dispatcherSpy.sendBroadcastSubscriptionRequest.mock.calls[0][0].from).toEqual(parameters.proxyId);
        expect(
            dispatcherSpy.sendBroadcastSubscriptionRequest.mock.calls[0][0].subscriptionRequest.subscribedToName
        ).toEqual(parameters.broadcastName);
    });

    it("rejects broadcast subscription requests with ttl expired when no reply arrives", async () => {
        const ttl = 250;
        const parameters = {
            proxyId: "subscriber",
            providerDiscoveryEntry,
            broadcastName: "broadcastName",
            subscriptionQos: new OnChangeSubscriptionQos({
                expiryDateMs: Date.now() + ttl
            })
        };

        dispatcherSpy.sendBroadcastSubscriptionRequest.mockReturnValue(Promise.resolve());
        const promise = testUtil.reversePromise(subscriptionManager.registerBroadcastSubscription(parameters as any));
        await increaseFakeTime(ttl + 1);
        await promise;
    });

    it("alerts on missed publication and stops", async () => {
        const publicationReceivedSpy = jest.fn();
        const publicationMissedSpy = jest.fn();
        const alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS;

        const subscriptionId = await subscriptionManager.registerSubscription({
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
        });

        expect(publicationMissedSpy).not.toHaveBeenCalled();
        await increaseFakeTime(alertAfterIntervalMs + 1);
        expect(publicationMissedSpy).toHaveBeenCalled();
        expect(publicationMissedSpy.mock.calls[0][0]).toBeInstanceOf(PublicationMissedException);
        expect(publicationMissedSpy.mock.calls[0][0].subscriptionId).toEqual(subscriptionId);
        await increaseFakeTime(alertAfterIntervalMs + 1);
        expect(publicationMissedSpy.mock.calls.length).toEqual(2);
        // expiryDate should be reached, expect no more interactions
        await increaseFakeTime(alertAfterIntervalMs + 1);
        expect(publicationMissedSpy.mock.calls.length).toEqual(2);
        await increaseFakeTime(alertAfterIntervalMs + 1);
        expect(publicationMissedSpy.mock.calls.length).toEqual(2);
    });

    it("sets messagingQos.ttl correctly according to subscriptionQos.expiryDateMs", async () => {
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
            onReceive() {
                // do nothing
            },
            onError() {
                // do nothing
            }
        };

        dispatcherSpy.sendSubscriptionRequest.mockClear();

        await subscriptionManager.registerSubscription(subscriptionSettings);

        await increaseFakeTime(1);

        expect(dispatcherSpy.sendSubscriptionRequest.mock.calls[0][0].messagingQos.ttl).toEqual(ttl);
        subscriptionSettings.qos.expiryDateMs = SubscriptionQos.NO_EXPIRY_DATE;
        await subscriptionManager.registerSubscription(subscriptionSettings);
        await increaseFakeTime(1);

        expect(dispatcherSpy.sendSubscriptionRequest.mock.calls.length).toEqual(2);
        expect(dispatcherSpy.sendSubscriptionRequest.mock.calls[1][0].messagingQos.ttl).toEqual(
            defaultMessagingSettings.MAX_MESSAGING_TTL_MS
        );
    });

    it("forwards publication payload", async () => {
        const publicationReceivedSpy = jest.fn();
        const publicationMissedSpy = jest.fn();
        const alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS;

        const resolveSpy = {
            // called when the subscription is registered successfully (see below)
            resolveMethod(subscriptionId: string) {
                // increase time by 50ms and see if alert was triggered
                increaseFakeTime(alertAfterIntervalMs / 2);
                expect(publicationMissedSpy).not.toHaveBeenCalled();
                const publication = SubscriptionPublication.create({
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

        jest.spyOn(resolveSpy, "resolveMethod");

        await subscriptionManager.registerSubscription({
            proxyId: "subscriber",
            providerDiscoveryEntry,
            messagingQos: new MessagingQos(undefined as any),
            attributeName: "testAttribute",
            attributeType: "String",
            qos: new OnChangeWithKeepAliveSubscriptionQos({
                alertAfterIntervalMs,
                expiryDateMs: Date.now() + 50 + 2 * alertAfterIntervalMs
            }),
            onReceive: publicationReceivedSpy,
            onError: publicationMissedSpy
        } as any);
    });

    it("augments incoming publications with information from the joynr type system", async () => {
        const publicationReceivedSpy = jest.fn();
        const publicationMissedSpy = jest.fn();

        const resolveSpy = {
            // called when the subscription is registered successfully (see below)
            resolveMethod(subscriptionId: string) {
                // increase time by 50ms and see if alert was triggered
                increaseFakeTime(50);
                expect(publicationMissedSpy).not.toHaveBeenCalled();
                const publication = SubscriptionPublication.create({
                    response: ["ZERO"],
                    subscriptionId
                });
                // simulate incoming publication
                subscriptionManager.handlePublication(publication);
                // make sure publication payload is forwarded
                expect(publicationReceivedSpy).toHaveBeenCalledWith(TestEnum.ZERO);
            }
        };

        jest.spyOn(resolveSpy, "resolveMethod");

        await subscriptionManager.registerSubscription({
            proxyId: "subscriber",
            providerDiscoveryEntry,
            attributeName: "testAttribute",
            attributeType: TestEnum.ZERO._typeName,
            qos: new OnChangeSubscriptionQos({
                expiryDateMs: Date.now() + 250
            }),
            onReceive: publicationReceivedSpy,
            onError: publicationMissedSpy
        });
    });

    it("augments incoming broadcasts with information from the joynr type system", async () => {
        const publicationReceivedSpy = jest.fn();
        const onErrorSpy = jest.fn();

        const resolveSpy = {
            // called when the subscription is registered successfully (see below)
            resolveMethod(subscriptionId: string) {
                const testString = "testString";
                const testInt = 2;
                const testEnum = TestEnum.ZERO;
                expect(onErrorSpy).not.toHaveBeenCalled();
                const publication = SubscriptionPublication.create({
                    response: [testString, testInt, testEnum.name],
                    subscriptionId
                });
                // simulate incoming publication
                subscriptionManager.handlePublication(publication);
                // make sure publication payload is forwarded
                expect(publicationReceivedSpy).toHaveBeenCalledWith([testString, testInt, testEnum]);
            }
        };

        jest.spyOn(resolveSpy, "resolveMethod");

        await subscriptionManager.registerBroadcastSubscription({
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
            subscriptionQos: new OnChangeSubscriptionQos({
                expiryDateMs: Date.now() + 250
            }),
            onReceive: publicationReceivedSpy,
            onError: onErrorSpy
        } as any);
    });

    function createDummyBroadcastSubscriptionRequest(parameters: any) {
        const onReceiveSpy = jest.fn();
        const onErrorSpy = jest.fn();
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
        const onReceiveSpy = jest.fn();
        const onErrorSpy = jest.fn();

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

    it("register multicast subscription request", async () => {
        const request = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        expect(subscriptionManager.hasMulticastSubscriptions()).toBe(false);

        await subscriptionManager.registerBroadcastSubscription(request as any);

        expect(dispatcherSpy.sendBroadcastSubscriptionRequest).toHaveBeenCalled();
        const forwardedRequest = dispatcherSpy.sendBroadcastSubscriptionRequest.mock.calls[0][0].subscriptionRequest;
        expect(forwardedRequest).toBeInstanceOf(MulticastSubscriptionRequest);
        expect(forwardedRequest.subscribedToName).toEqual(request.broadcastName);
        expect(subscriptionManager.hasMulticastSubscriptions()).toBe(true);
    });

    it("register and unregisters multicast subscription request", async () => {
        const request = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        expect(subscriptionManager.hasMulticastSubscriptions()).toBe(false);

        const subscriptionId = await subscriptionManager.registerBroadcastSubscription(request as any);

        expect(subscriptionManager.hasMulticastSubscriptions()).toBe(true);

        await subscriptionManager.unregisterSubscription({
            subscriptionId
        } as any);

        expect(subscriptionManager.hasMulticastSubscriptions()).toBe(false);
        expect(subscriptionManager.hasOpenSubscriptions()).toBe(false);
    });

    it("is able to handle multicast publications", async () => {
        const request: any = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        const response = ["response"];

        request.subscriptionId = await subscriptionManager.registerBroadcastSubscription(request);
        request.multicastId =
            dispatcherSpy.sendBroadcastSubscriptionRequest.mock.calls[0][0].subscriptionRequest.multicastId;
        subscriptionManager.handleMulticastPublication({
            multicastId: request.multicastId,
            response
        } as any);
        expect(request.onReceive).toHaveBeenCalled();
        expect(request.onReceive).toHaveBeenCalledWith(response);
        //stop subscription
        request.onReceive.mockClear();

        await subscriptionManager.unregisterSubscription({
            subscriptionId: request.subscriptionId
        } as any);

        //send another publication and do not expect calls
        expect(subscriptionManager.hasOpenSubscriptions()).toBe(false);
        expect(() => {
            subscriptionManager.handleMulticastPublication({
                multicastId: request.multicastId,
                response
            } as any);
        }).toThrow();
        expect(request.onReceive).not.toHaveBeenCalled();
    });

    it("sends out subscriptionStop and stops alerts on unsubscribe", async () => {
        const publicationReceivedSpy = jest.fn();
        const publicationMissedSpy = jest.fn();
        const alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS;
        let expectedDiscoveryEntry: any = UtilInternal.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(expectedDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(expectedDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        const subscriptionId = await subscriptionManager.registerSubscription({
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
        });

        await increaseFakeTime(alertAfterIntervalMs / 2);
        expect(publicationMissedSpy).not.toHaveBeenCalled();
        await increaseFakeTime(alertAfterIntervalMs / 2 + 1);
        expect(publicationMissedSpy).toHaveBeenCalled();
        await increaseFakeTime(101);
        await increaseFakeTime(alertAfterIntervalMs + 1);
        expect(publicationMissedSpy.mock.calls.length).toEqual(2);

        // unsubscribe and expect no more missed publication alerts
        const unsubscrMsgQos = new MessagingQos();
        await subscriptionManager.unregisterSubscription({
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
        await increaseFakeTime(alertAfterIntervalMs + 1);
        expect(publicationMissedSpy.mock.calls.length).toEqual(2);
        await increaseFakeTime(alertAfterIntervalMs + 1);
        expect(publicationMissedSpy.mock.calls.length).toEqual(2);
        await increaseFakeTime(alertAfterIntervalMs + 1);
        expect(publicationMissedSpy.mock.calls.length).toEqual(2);
    });

    it("sends out MulticastSubscriptionStop", async () => {
        const request = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        let expectedDiscoveryEntry: any = UtilInternal.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(expectedDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(expectedDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        const subscriptionId = await subscriptionManager.registerBroadcastSubscription(request as any);

        expect(dispatcherSpy.sendBroadcastSubscriptionRequest).toHaveBeenCalled();

        const multicastId =
            dispatcherSpy.sendBroadcastSubscriptionRequest.mock.calls[0][0].subscriptionRequest.multicastId;
        expect(multicastId).toBeDefined();
        expect(multicastId).not.toEqual(null);

        const unsubscrMsgQos = new MessagingQos();
        await subscriptionManager.unregisterSubscription({
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
    });

    it("returns a rejected promise when unsubscribing with a non-existant subscriptionId", async () => {
        const error = await testUtil.reversePromise(
            subscriptionManager.unregisterSubscription({
                subscriptionId: "non-existant"
            } as any)
        );

        expect(error).toBeDefined();
        const className = Object.prototype.toString.call(error).slice(8, -1);
        expect(className).toMatch("Error");
    });

    it("registers subscription, resolves with subscriptionId and calls onSubscribed callback", async () => {
        const publicationReceivedSpy = jest.fn();
        const publicationErrorSpy = jest.fn();
        const publicationSubscribedSpy = jest.fn();
        dispatcherSpy.sendSubscriptionRequest.mockClear();
        let expectedDiscoveryEntry: any = UtilInternal.extendDeep({}, providerDiscoveryEntry);
        expectedDiscoveryEntry.providerVersion = new Version(providerDiscoveryEntry.providerVersion);
        expectedDiscoveryEntry.qos = new ProviderQos(providerDiscoveryEntry.qos);
        expectedDiscoveryEntry = new DiscoveryEntryWithMetaInfo(expectedDiscoveryEntry);

        const receivedSubscriptionId = await subscriptionManager.registerSubscription({
            proxyId: "subscriber",
            providerDiscoveryEntry,
            attributeName: "testAttribute",
            attributeType: "String",
            qos: new OnChangeSubscriptionQos(),
            onReceive: publicationReceivedSpy,
            onError: publicationErrorSpy,
            onSubscribed: publicationSubscribedSpy
        });

        expect(receivedSubscriptionId).toBeDefined();
        expect(receivedSubscriptionId).toEqual(storedSubscriptionId);

        await testUtil.multipleSetImmediate();

        expect(publicationReceivedSpy).not.toHaveBeenCalled();
        expect(publicationErrorSpy).not.toHaveBeenCalled();
        expect(publicationSubscribedSpy).toHaveBeenCalled();
        expect(publicationSubscribedSpy.mock.calls[0][0]).toEqual(storedSubscriptionId);
        expect(dispatcherSpy.sendSubscriptionRequest).toHaveBeenCalled();
        expect(dispatcherSpy.sendSubscriptionRequest.mock.calls[0][0].toDiscoveryEntry).toEqual(expectedDiscoveryEntry);
    });

    it("rejects subscription requests with ttl expired when no reply arrives", async () => {
        const ttl = 250;
        const parameters = {
            proxyId: "subscriber",
            providerDiscoveryEntry,
            attributeName: "testAttribute",
            attributeType: "String",
            qos: new OnChangeSubscriptionQos({
                expiryDateMs: Date.now() + ttl
            })
        };

        dispatcherSpy.sendSubscriptionRequest.mockResolvedValue(undefined);

        const promise = testUtil.reversePromise(subscriptionManager.registerSubscription(parameters as any));
        await increaseFakeTime(ttl + 1);
        await promise;
    });

    it("registers broadcast subscription, resolves with subscriptionId and calls onSubscribed callback", async () => {
        const publicationReceivedSpy = jest.fn();
        const publicationErrorSpy = jest.fn();
        const publicationSubscribedSpy = jest.fn();

        const receivedSubscriptionId = await subscriptionManager.registerBroadcastSubscription({
            proxyId: "subscriber",
            providerDiscoveryEntry,
            broadcastName: "broadcastName",
            subscriptionQos: new OnChangeSubscriptionQos(),
            onReceive: publicationReceivedSpy,
            onError: publicationErrorSpy,
            onSubscribed: publicationSubscribedSpy
        } as any);

        expect(receivedSubscriptionId).toBeDefined();
        expect(receivedSubscriptionId).toEqual(storedSubscriptionId);

        await testUtil.multipleSetImmediate();

        expect(publicationReceivedSpy).not.toHaveBeenCalled();
        expect(publicationErrorSpy).not.toHaveBeenCalled();
        expect(publicationSubscribedSpy).toHaveBeenCalled();
        expect(publicationSubscribedSpy.mock.calls[0][0]).toEqual(storedSubscriptionId);
    });

    it("rejects on subscription registration failures and calls onError callback", async () => {
        const publicationReceivedSpy = jest.fn();
        const publicationErrorSpy = jest.fn();
        const publicationSubscribedSpy = jest.fn();

        const error = await testUtil.reversePromise(
            subscriptionManagerOnError.registerSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry,
                attributeName: "testAttribute",
                attributeType: "String",
                qos: new OnChangeSubscriptionQos(),
                onReceive: publicationReceivedSpy,
                onError: publicationErrorSpy,
                onSubscribed: publicationSubscribedSpy
            })
        );
        expect(error).toBeInstanceOf(SubscriptionException);
        expect(error.subscriptionId).toBeDefined();
        expect(error.subscriptionId).toEqual(storedSubscriptionId);

        await testUtil.multipleSetImmediate();

        expect(publicationReceivedSpy).not.toHaveBeenCalled();
        expect(publicationSubscribedSpy).not.toHaveBeenCalled();
        expect(publicationErrorSpy).toHaveBeenCalled();
        expect(publicationErrorSpy.mock.calls[0][0]).toBeInstanceOf(SubscriptionException);
        expect(publicationErrorSpy.mock.calls[0][0].subscriptionId).toEqual(storedSubscriptionId);
    });

    it("rejects on broadcast subscription registration failures and calls onError callback", async () => {
        const publicationReceivedSpy = jest.fn();
        const publicationErrorSpy = jest.fn();
        const publicationSubscribedSpy = jest.fn();

        const error = await testUtil.reversePromise(
            subscriptionManagerOnError.registerBroadcastSubscription({
                proxyId: "subscriber",
                providerDiscoveryEntry,
                broadcastName: "broadcastName",
                qos: new OnChangeSubscriptionQos(),
                onReceive: publicationReceivedSpy,
                onError: publicationErrorSpy,
                onSubscribed: publicationSubscribedSpy
            } as any)
        );

        expect(error).toBeInstanceOf(SubscriptionException);
        expect(error.subscriptionId).toBeDefined();
        expect(error.subscriptionId).toEqual(storedSubscriptionId);
        await testUtil.multipleSetImmediate();

        expect(publicationReceivedSpy).not.toHaveBeenCalled();
        expect(publicationSubscribedSpy).not.toHaveBeenCalled();
        expect(publicationErrorSpy).toHaveBeenCalled();
        expect(publicationErrorSpy.mock.calls[0][0]).toBeInstanceOf(SubscriptionException);
        expect(publicationErrorSpy.mock.calls[0][0].subscriptionId).toEqual(storedSubscriptionId);
    });

    it(" throws exception when called while shut down", async () => {
        subscriptionManager.shutdown();
        await testUtil.reversePromise(subscriptionManager.registerSubscription({} as any));
        await testUtil.reversePromise(subscriptionManager.registerBroadcastSubscription({} as any));
        expect(() => subscriptionManager.unregisterSubscription({} as any)).toThrow();
    });

    it(" it unsubscribes all Subscriptions when terminateSubscriptions is being called", async () => {
        const subscriptionSettings = createDummySubscriptionRequest();
        const broadcastSettings = createDummyBroadcastSubscriptionRequest({
            broadcastName: "broadcastName",
            selective: false
        });
        const clearSubscriptionsTimeoutMs = 1000;
        const DEFAULT_TTL = 60000;

        await subscriptionManager.registerSubscription(subscriptionSettings);
        await subscriptionManager.registerBroadcastSubscription(broadcastSettings as any);
        await subscriptionManager.terminateSubscriptions(clearSubscriptionsTimeoutMs);
        subscriptionManager.shutdown();

        expect(dispatcherSpy.sendSubscriptionStop).toHaveBeenCalled();
        expect(dispatcherSpy.sendSubscriptionStop.mock.calls.length).toEqual(1);
        expect(dispatcherSpy.sendSubscriptionStop.mock.calls[0][0].messagingQos).toEqual(
            new MessagingQos({ ttl: DEFAULT_TTL })
        );
        expect(dispatcherSpy.sendMulticastSubscriptionStop.mock.calls[0][0].messagingQos).toEqual(
            new MessagingQos({ ttl: DEFAULT_TTL })
        );
        expect(dispatcherSpy.sendMulticastSubscriptionStop.mock.calls.length).toEqual(1);
    });
});
