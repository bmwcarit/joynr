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

import * as UtilInternal from "../../../../main/js/joynr/util/UtilInternal";
import Dispatcher from "../../../../main/js/joynr/dispatching/Dispatcher";
import JoynrMessage from "../../../../main/js/joynr/messaging/JoynrMessage";
import MessagingQos from "../../../../main/js/joynr/messaging/MessagingQos";
import DiscoveryEntryWithMetaInfo from "../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo";
import * as Reply from "../../../../main/js/joynr/dispatching/types/Reply";
import BroadcastSubscriptionRequest from "../../../../main/js/joynr/dispatching/types/BroadcastSubscriptionRequest";
import MulticastSubscriptionRequest from "../../../../main/js/joynr/dispatching/types/MulticastSubscriptionRequest";
import SubscriptionRequest from "../../../../main/js/joynr/dispatching/types/SubscriptionRequest";
import SubscriptionReply from "../../../../main/js/joynr/dispatching/types/SubscriptionReply";
import SubscriptionStop from "../../../../main/js/joynr/dispatching/types/SubscriptionStop";
import * as MulticastPublication from "../../../../main/js/joynr/dispatching/types/MulticastPublication";
import { nanoid } from "nanoid";

const providerId = "providerId";
const providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
    domain: "testProviderDomain",
    interfaceName: "interfaceName",
    participantId: providerId,
    lastSeenDateMs: Date.now(),
    expiryDateMs: Date.now() + 60000,
    publicKeyId: "publicKeyId",
    isLocal: false,
    qos: undefined as any,
    providerVersion: undefined as any
});
const proxyId = "proxyId";
const noTtlUplift = 0;
const ttlUpliftMs = 10000;
const toleranceMs = 50;

describe("libjoynr-js.joynr.ttlUpliftTest", () => {
    let dispatcher: Dispatcher, dispatcherWithTtlUplift: any;
    let clusterControllerMessagingStub: any, securityManager: any;
    let requestReplyManager: any, subscriptionManager: any, publicationManager: any, messageRouter: any;
    const subscriptionId = `mySubscriptionId-${nanoid()}`;
    const multicastId = `multicastId-${nanoid()}`;

    let ttl: number, messagingQos: MessagingQos, expiryDateMs: number, expiryDateWithTtlUplift: any;
    let publicationTtlMs: number, subscriptionQos: any;

    function receiveJoynrMessage(parameters: any) {
        const joynrMessage = new JoynrMessage({
            type: parameters.type,
            payload: JSON.stringify(parameters.payload)
        });
        joynrMessage.from = proxyId;
        joynrMessage.to = providerId;
        joynrMessage.expiryDate = parameters.expiryDate;
        return dispatcher.receive(joynrMessage);
    }

    function receiveJoynrMessageTtlUplift(parameters: any) {
        const joynrMessage = new JoynrMessage({
            type: parameters.type,
            payload: JSON.stringify(parameters.payload)
        });
        joynrMessage.from = proxyId;
        joynrMessage.to = providerId;
        joynrMessage.expiryDate = parameters.expiryDate;
        return dispatcherWithTtlUplift.receive(joynrMessage);
    }

    function checkMessageFromProxyWithTolerance(messageType: any, expectedExpiryDate: any) {
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const msg = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(msg.type).toEqual(messageType);
        expect(msg.from).toEqual(proxyId);
        expect(msg.to).toEqual(providerId);
        expect(msg.expiryDate).not.toBeNull();
        expect(msg.expiryDate).toBeDefined();
        expect(expectedExpiryDate).not.toBeNull();
        expect(expectedExpiryDate).toBeDefined();
        expect(msg.expiryDate).toBeGreaterThanOrEqual(expectedExpiryDate);
        expect(msg.expiryDate).toBeLessThan(expectedExpiryDate + toleranceMs);
    }

    function checkMessageFromProvider(messageType: any, expectedExpiryDate: any) {
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const msg = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(msg.type).toEqual(messageType);
        expect(msg.from).toEqual(providerId);
        expect(msg.to).toEqual(proxyId);
        expect(msg.expiryDate).toEqual(expectedExpiryDate);
    }

    function checkRequestReplyMessage(expectedExpiryDate: any) {
        checkMessageFromProvider(JoynrMessage.JOYNRMESSAGE_TYPE_REPLY, expectedExpiryDate);
    }

    function checkSubscriptionReplyMessage(expectedExpiryDate: any) {
        checkMessageFromProvider(JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY, expectedExpiryDate);
    }

    beforeEach(() => {
        requestReplyManager = {
            handleRequest: jest.fn().mockImplementation((_providerParticipantId, request, cb, replySettings) => {
                const reply = Reply.create({
                    response: "response" as any,
                    requestReplyId: request.requestReplyId
                });
                return Promise.resolve(cb(replySettings, reply));
            })
        };
        subscriptionManager = {
            handleSubscriptionReply: jest.fn(),
            handleMulticastPublication: jest.fn(),
            handlePublication: jest.fn()
        };
        function sendSubscriptionReply(
            _proxyParticipantId: string,
            _providerParticipantId: string,
            subscriptionRequest: any,
            callbackDispatcher: any,
            settings: any
        ) {
            callbackDispatcher(
                settings,
                new SubscriptionReply({
                    subscriptionId: subscriptionRequest.subscriptionId
                })
            );
        }

        publicationManager = {
            handleSubscriptionRequest: jest.fn().mockImplementation(sendSubscriptionReply),
            handleBroadcastSubscriptionRequest: jest.fn().mockImplementation(sendSubscriptionReply),
            handleMulticastSubscriptionRequest: jest.fn().mockImplementation(sendSubscriptionReply),
            handleSubscriptionStop: jest.fn()
        };

        messageRouter = {
            addMulticastReceiver: jest.fn().mockReturnValue(Promise.resolve()),
            removeMulticastReceiver: jest.fn()
        };

        clusterControllerMessagingStub = {
            transmit: jest.fn().mockReturnValue(Promise.resolve())
        };

        securityManager = {
            getCurrentProcessUserId: jest.fn()
        };

        dispatcher = new Dispatcher(clusterControllerMessagingStub, securityManager, noTtlUplift);
        dispatcher.registerRequestReplyManager(requestReplyManager);
        dispatcher.registerSubscriptionManager(subscriptionManager);
        dispatcher.registerPublicationManager(publicationManager);
        dispatcher.registerMessageRouter(messageRouter);

        dispatcherWithTtlUplift = new Dispatcher(clusterControllerMessagingStub, securityManager, ttlUpliftMs);
        dispatcherWithTtlUplift.registerRequestReplyManager(requestReplyManager);
        dispatcherWithTtlUplift.registerSubscriptionManager(subscriptionManager);
        dispatcherWithTtlUplift.registerPublicationManager(publicationManager);
        dispatcherWithTtlUplift.registerMessageRouter(messageRouter);

        ttl = 300;
        messagingQos = new MessagingQos({
            ttl
        });
        publicationTtlMs = 1000;
        expiryDateMs = Date.now() + ttl;
        expiryDateWithTtlUplift = expiryDateMs + ttlUpliftMs;

        subscriptionQos = {
            expiryDateMs,
            publicationTtlMs,
            minIntervalMs: 0
        };
    });

    describe("no ttl uplift (default)", () => {
        it("send request", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                request: "request"
            };

            dispatcher.sendRequest(settings as any);

            checkMessageFromProxyWithTolerance(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST, expiryDateMs);
        });

        it("send one way request", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                request: "oneWayRequest"
            };

            dispatcher.sendOneWayRequest(settings as any);

            checkMessageFromProxyWithTolerance(JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY, expiryDateMs);
        });

        it("send subscription request", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                subscriptionRequest: new SubscriptionRequest({
                    subscriptionId: "subscriptionId",
                    subscribedToName: "attributeName",
                    qos: subscriptionQos
                })
            };
            dispatcher.sendSubscriptionRequest(settings);

            checkMessageFromProxyWithTolerance(JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST, expiryDateMs);
        });

        it("send broadcast subscription request", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                subscriptionRequest: new BroadcastSubscriptionRequest({
                    subscriptionId: "subscriptionId",
                    subscribedToName: "broadcastEvent",
                    qos: subscriptionQos
                })
            };

            dispatcher.sendBroadcastSubscriptionRequest(settings);

            checkMessageFromProxyWithTolerance(
                JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                expiryDateMs
            );
        });

        it("send multicast subscription stop", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                multicastId: "multicastId",
                subscriptionStop: new SubscriptionStop({
                    subscriptionId: "subscriptionId"
                })
            };

            dispatcher.sendMulticastSubscriptionStop(settings);

            checkMessageFromProxyWithTolerance(JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP, expiryDateMs);
        });

        it("send subscription stop", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                subscriptionStop: new SubscriptionStop({
                    subscriptionId: "subscriptionId"
                })
            };

            dispatcher.sendSubscriptionStop(settings);

            checkMessageFromProxyWithTolerance(JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP, expiryDateMs);
        });

        it("send publication", () => {
            const settings = {
                from: proxyId,
                to: providerId,
                expiryDate: expiryDateMs
            };

            dispatcher.sendPublication(settings, "publication" as any);

            checkMessageFromProxyWithTolerance(JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION, expiryDateMs);
        });

        it("send multicast publication", () => {
            const settings = {
                from: providerId,
                expiryDate: expiryDateMs
            };
            const multicastId = "multicastId";
            const publication = MulticastPublication.create({
                multicastId
            });

            dispatcher.sendMulticastPublication(settings, publication);

            expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
            const msg = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
            expect(msg.type).toEqual(JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST);
            expect(msg.from).toEqual(providerId);
            expect(msg.to).toEqual(multicastId);
            expect(msg.expiryDate).not.toBeNull();
            expect(msg.expiryDate).toBeDefined();
            expect(expiryDateMs).not.toBeNull();
            expect(expiryDateMs).toBeDefined();
            expect(msg.expiryDate).toBeGreaterThanOrEqual(expiryDateMs);
            expect(msg.expiryDate).toBeLessThan(expiryDateMs + toleranceMs);
        });

        it("request and reply", async () => {
            const payload = {
                methodName: "methodName"
            };

            await receiveJoynrMessage({
                type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                payload,
                expiryDate: expiryDateMs
            });

            expect(requestReplyManager.handleRequest).toHaveBeenCalled();
            expect(requestReplyManager.handleRequest).toHaveBeenCalledWith(
                providerId,
                expect.objectContaining({
                    _typeName: "joynr.Request"
                }),
                expect.any(Function),
                expect.any(Object)
            );

            checkRequestReplyMessage(expiryDateMs);
        });

        it("subscription expiry date and subscription reply", () => {
            //                var expiryDateMs = Date.now() + 100000;
            //                var publicationTtlMs = 10000;
            //                var qos = {
            //                    expiryDateMs : expiryDateMs,
            //                    publicationTtlMs : publicationTtlMs,
            //                    minIntervalMs : 0
            //                };
            const payload = {
                subscribedToName: "attributeName",
                subscriptionId,
                qos: subscriptionQos
            };

            const payloadCopy = UtilInternal.extendDeep({}, payload);
            const expectedSubscriptionRequest = new SubscriptionRequest(payloadCopy as any);

            receiveJoynrMessage({
                type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                payload,
                expiryDate: expiryDateMs
            });

            expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalled();
            expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalledWith(
                proxyId,
                providerId,
                expectedSubscriptionRequest,
                expect.any(Function),
                expect.any(Object)
            );

            checkSubscriptionReplyMessage(expiryDateMs);
        });

        it("broadcast subscription expiry date and subscription reply", () => {
            const payload = {
                subscribedToName: "broadcastEvent",
                subscriptionId,
                qos: subscriptionQos
            };

            const payloadCopy = UtilInternal.extendDeep({}, payload);
            const expectedSubscriptionRequest = new BroadcastSubscriptionRequest(payloadCopy as any);

            receiveJoynrMessage({
                type: JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                payload,
                expiryDate: expiryDateMs
            });

            expect(publicationManager.handleBroadcastSubscriptionRequest).toHaveBeenCalled();
            expect(publicationManager.handleBroadcastSubscriptionRequest).toHaveBeenCalledWith(
                proxyId,
                providerId,
                expectedSubscriptionRequest,
                expect.any(Function),
                expect.any(Object)
            );

            checkSubscriptionReplyMessage(expiryDateMs);
        });

        it("multicast subscription expiryDate and subscription reply", () => {
            const payload = {
                subscribedToName: "multicastEvent",
                subscriptionId,
                multicastId,
                qos: subscriptionQos
            };

            const payloadCopy = UtilInternal.extendDeep({}, payload);
            const expectedSubscriptionRequest = new MulticastSubscriptionRequest(payloadCopy as any);

            receiveJoynrMessage({
                type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
                payload,
                expiryDate: expiryDateMs
            });

            expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalled();
            expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalledWith(
                proxyId,
                providerId,
                expectedSubscriptionRequest,
                expect.any(Function),
                expect.any(Object)
            );

            checkSubscriptionReplyMessage(expiryDateMs);
        });
    }); // describe "no ttl uplift (default)"

    describe("with ttlUplift", () => {
        it("send request", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                request: "request"
            };

            dispatcherWithTtlUplift.sendRequest(settings);

            checkMessageFromProxyWithTolerance(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST, expiryDateWithTtlUplift);
        });

        it("send one way request", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                request: "oneWayRequest"
            };

            dispatcherWithTtlUplift.sendOneWayRequest(settings);

            checkMessageFromProxyWithTolerance(JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY, expiryDateWithTtlUplift);
        });

        it("send subscription request", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                subscriptionRequest: new SubscriptionRequest({
                    subscriptionId: "subscriptionId",
                    subscribedToName: "attributeName",
                    qos: subscriptionQos
                })
            };

            dispatcherWithTtlUplift.sendSubscriptionRequest(settings);

            checkMessageFromProxyWithTolerance(
                JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                expiryDateWithTtlUplift
            );
        });

        it("send broadcast subscription request", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                subscriptionRequest: new BroadcastSubscriptionRequest({
                    subscriptionId: "subscriptionId",
                    subscribedToName: "broadcastEvent",
                    qos: subscriptionQos
                })
            };

            dispatcherWithTtlUplift.sendBroadcastSubscriptionRequest(settings);

            checkMessageFromProxyWithTolerance(
                JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                expiryDateWithTtlUplift
            );
        });

        it("send multicast subscription stop", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                multicastId: "multicastId",
                subscriptionStop: new SubscriptionStop({
                    subscriptionId: "subscriptionId"
                })
            };

            dispatcherWithTtlUplift.sendMulticastSubscriptionStop(settings);

            checkMessageFromProxyWithTolerance(
                JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP,
                expiryDateWithTtlUplift
            );
        });

        it("send subscription stop", () => {
            const settings = {
                from: proxyId,
                toDiscoveryEntry: providerDiscoveryEntry,
                messagingQos,
                subscriptionStop: new SubscriptionStop({
                    subscriptionId: "subscriptionId"
                })
            };

            dispatcherWithTtlUplift.sendSubscriptionStop(settings);

            checkMessageFromProxyWithTolerance(
                JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP,
                expiryDateWithTtlUplift
            );
        });

        it("send publication", () => {
            const settings = {
                from: proxyId,
                to: providerId,
                expiryDate: expiryDateMs
            };

            dispatcherWithTtlUplift.sendPublication(settings, "publication");

            checkMessageFromProxyWithTolerance(JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION, expiryDateWithTtlUplift);
        });

        it("send multicast publication", () => {
            const settings = {
                from: providerId,
                expiryDate: expiryDateMs
            };
            const multicastId = "multicastId";
            const publication = MulticastPublication.create({
                multicastId
            });

            dispatcherWithTtlUplift.sendMulticastPublication(settings, publication);

            expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
            const msg = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
            expect(msg.type).toEqual(JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST);
            expect(msg.from).toEqual(providerId);
            expect(msg.to).toEqual(multicastId);
            expect(msg.expiryDate).not.toBeNull();
            expect(msg.expiryDate).toBeDefined();
            expect(expiryDateWithTtlUplift).not.toBeNull();
            expect(expiryDateWithTtlUplift).toBeDefined();
            expect(msg.expiryDate).toBeGreaterThanOrEqual(expiryDateWithTtlUplift);
            expect(msg.expiryDate).toBeLessThan(expiryDateWithTtlUplift + toleranceMs);
        });

        it("request and reply", async () => {
            const payload = {
                methodName: "methodName"
            };

            await receiveJoynrMessageTtlUplift({
                type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                payload,
                expiryDate: expiryDateMs + ttlUpliftMs
            });

            expect(requestReplyManager.handleRequest).toHaveBeenCalled();
            expect(requestReplyManager.handleRequest).toHaveBeenCalledWith(
                providerId,
                expect.objectContaining({
                    _typeName: "joynr.Request"
                }),
                expect.any(Function),
                expect.any(Object)
            );

            checkRequestReplyMessage(expiryDateWithTtlUplift);
        });

        it("subscription expiry date and subscription reply", () => {
            const payload = {
                subscribedToName: "attributeName",
                subscriptionId,
                qos: subscriptionQos
            };

            const payloadCopy = UtilInternal.extendDeep({}, payload);
            const expectedSubscriptionRequest = new SubscriptionRequest(payloadCopy as any);
            expectedSubscriptionRequest.qos.expiryDateMs = expiryDateWithTtlUplift;

            receiveJoynrMessageTtlUplift({
                type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                payload,
                expiryDate: expiryDateMs + ttlUpliftMs
            });

            expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalled();
            expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalledWith(
                proxyId,
                providerId,
                expectedSubscriptionRequest,
                expect.any(Function),
                expect.any(Object)
            );

            checkSubscriptionReplyMessage(expiryDateWithTtlUplift);
        });

        it("broadcast subscription expiry date and subscription reply", () => {
            const payload = {
                subscribedToName: "broadcastEvent",
                subscriptionId,
                qos: subscriptionQos
            };

            const payloadCopy = UtilInternal.extendDeep({}, payload);
            const expectedSubscriptionRequest = new BroadcastSubscriptionRequest(payloadCopy as any);
            expectedSubscriptionRequest.qos.expiryDateMs = expiryDateWithTtlUplift;

            receiveJoynrMessageTtlUplift({
                type: JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                payload,
                expiryDate: expiryDateMs + ttlUpliftMs
            });

            expect(publicationManager.handleBroadcastSubscriptionRequest).toHaveBeenCalled();
            expect(publicationManager.handleBroadcastSubscriptionRequest).toHaveBeenCalledWith(
                proxyId,
                providerId,
                expectedSubscriptionRequest,
                expect.any(Function),
                expect.any(Object)
            );

            checkSubscriptionReplyMessage(expiryDateWithTtlUplift);
        });

        it("multicast subscription expiry date and subscription reply", () => {
            const payload = {
                subscribedToName: "multicastEvent",
                subscriptionId,
                multicastId,
                qos: subscriptionQos
            };

            const payloadCopy = UtilInternal.extendDeep({}, payload);
            const expectedSubscriptionRequest = new MulticastSubscriptionRequest(payloadCopy as any);
            expectedSubscriptionRequest.qos.expiryDateMs = expiryDateWithTtlUplift;

            receiveJoynrMessageTtlUplift({
                type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
                payload,
                expiryDate: expiryDateMs + ttlUpliftMs
            });

            expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalled();
            expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalledWith(
                proxyId,
                providerId,
                expectedSubscriptionRequest,
                expect.any(Function),
                expect.any(Object)
            );

            checkSubscriptionReplyMessage(expiryDateWithTtlUplift);
        });
    }); // describe "with ttlUplift"
}); // describe ttlUpliftTest
