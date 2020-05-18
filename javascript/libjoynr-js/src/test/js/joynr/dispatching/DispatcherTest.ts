/* eslint prefer-promise-reject-errors: "off" */
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
import JoynrMessage from "../../../../main/js/joynr/messaging/JoynrMessage";
import MessagingQos from "../../../../main/js/joynr/messaging/MessagingQos";
import * as MessagingQosEffort from "../../../../main/js/joynr/messaging/MessagingQosEffort";
import * as OneWayRequest from "../../../../main/js/joynr/dispatching/types/OneWayRequest";
import * as Request from "../../../../main/js/joynr/dispatching/types/Request";
import * as Reply from "../../../../main/js/joynr/dispatching/types/Reply";
import BroadcastSubscriptionRequest from "../../../../main/js/joynr/dispatching/types/BroadcastSubscriptionRequest";
import MulticastSubscriptionRequest from "../../../../main/js/joynr/dispatching/types/MulticastSubscriptionRequest";
import SubscriptionRequest from "../../../../main/js/joynr/dispatching/types/SubscriptionRequest";
import SubscriptionReply from "../../../../main/js/joynr/dispatching/types/SubscriptionReply";
import SubscriptionStop from "../../../../main/js/joynr/dispatching/types/SubscriptionStop";
import * as MulticastPublication from "../../../../main/js/joynr/dispatching/types/MulticastPublication";
import * as SubscriptionPublication from "../../../../main/js/joynr/dispatching/types/SubscriptionPublication";
import TypeRegistrySingleton from "../../../../main/js/joynr/types/TypeRegistrySingleton";
import DiscoveryEntryWithMetaInfo from "../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo";
import Version from "../../../../main/js/generated/joynr/types/Version";
import ProviderQos from "../../../../main/js/generated/joynr/types/ProviderQos";
import nanoid from "nanoid";
import TestEnum from "../../../generated/joynr/tests/testTypes/TestEnum";

import Dispatcher from "../../../../main/js/joynr/dispatching/Dispatcher";
import testUtil = require("../../testUtil");

const providerId = "providerId";
const proxyId = "proxyId";
let toDiscoveryEntry: any, globalToDiscoveryEntry: any;

describe("libjoynr-js.joynr.dispatching.Dispatcher", () => {
    let dispatcher: Dispatcher,
        requestReplyManager: any,
        subscriptionManager: any,
        publicationManager: any,
        messageRouter: any,
        clusterControllerMessagingStub: any,
        securityManager: any;
    const subscriptionId = `mySubscriptionId-${nanoid()}`;
    const multicastId = `multicastId-${nanoid()}`;
    const requestReplyId = "requestReplyId";

    /**
     * Called before each test.
     */
    beforeEach(() => {
        toDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
            domain: "testProviderDomain",
            interfaceName: "interfaceName",
            participantId: providerId,
            qos: new ProviderQos(undefined as any),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "publicKeyId",
            isLocal: true
        });
        globalToDiscoveryEntry = new DiscoveryEntryWithMetaInfo(toDiscoveryEntry);
        globalToDiscoveryEntry.isLocal = false;
        requestReplyManager = {
            handleOneWayRequest: jest.fn(),
            handleRequest: jest.fn(),
            handleReply: jest.fn()
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
            callbackDispatcher: Function,
            callbackDispatcherSettings: any
        ) {
            callbackDispatcher(
                callbackDispatcherSettings,
                new SubscriptionReply({
                    subscriptionId: subscriptionRequest.subscriptionId
                })
            );
        }

        publicationManager = {
            handleSubscriptionRequest: sendSubscriptionReply,
            handleMulticastSubscriptionRequest: sendSubscriptionReply,
            handleSubscriptionStop() {}
        };
        jest.spyOn(publicationManager, "handleSubscriptionRequest");
        jest.spyOn(publicationManager, "handleMulticastSubscriptionRequest");
        jest.spyOn(publicationManager, "handleSubscriptionStop");

        messageRouter = {
            addMulticastReceiver: jest.fn(),
            removeMulticastReceiver: jest.fn()
        };
        messageRouter.addMulticastReceiver.mockReturnValue(Promise.resolve());
        clusterControllerMessagingStub = {
            transmit: jest.fn()
        };
        clusterControllerMessagingStub.transmit.mockReturnValue(Promise.resolve());

        securityManager = {
            getCurrentProcessUserId: jest.fn()
        };

        dispatcher = new Dispatcher(clusterControllerMessagingStub, securityManager);
        dispatcher.registerRequestReplyManager(requestReplyManager);
        dispatcher.registerSubscriptionManager(subscriptionManager);
        dispatcher.registerPublicationManager(publicationManager);
        dispatcher.registerMessageRouter(messageRouter);

        TypeRegistrySingleton.getInstance().addType(TestEnum);
    });

    it("is instantiable and of correct type", () => {
        expect(Dispatcher).toBeDefined();
        expect(typeof Dispatcher === "function").toBeTruthy();
        expect(dispatcher).toBeDefined();
        expect(dispatcher).toBeInstanceOf(Dispatcher);
        expect(dispatcher.registerRequestReplyManager).toBeDefined();
        expect(typeof dispatcher.registerRequestReplyManager === "function").toBeTruthy();
        expect(dispatcher.registerSubscriptionManager).toBeDefined();
        expect(typeof dispatcher.registerSubscriptionManager === "function").toBeTruthy();
        expect(dispatcher.registerPublicationManager).toBeDefined();
        expect(typeof dispatcher.registerPublicationManager === "function").toBeTruthy();
        expect(dispatcher.sendRequest).toBeDefined();
        expect(typeof dispatcher.sendRequest === "function").toBeTruthy();
        expect(dispatcher.sendOneWayRequest).toBeDefined();
        expect(typeof dispatcher.sendOneWayRequest === "function").toBeTruthy();
        expect(dispatcher.sendBroadcastSubscriptionRequest).toBeDefined();
        expect(typeof dispatcher.sendBroadcastSubscriptionRequest === "function").toBeTruthy();
        expect(dispatcher.sendSubscriptionRequest).toBeDefined();
        expect(typeof dispatcher.sendSubscriptionRequest === "function").toBeTruthy();
        expect(dispatcher.sendSubscriptionStop).toBeDefined();
        expect(typeof dispatcher.sendSubscriptionStop === "function").toBeTruthy();
        expect(dispatcher.sendPublication).toBeDefined();
        expect(typeof dispatcher.sendPublication === "function").toBeTruthy();
        expect(dispatcher.receive).toBeDefined();
        expect(typeof dispatcher.receive === "function").toBeTruthy();
    });

    function receiveJoynrMessage(parameters: any) {
        const joynrMessage = new JoynrMessage({
            type: parameters.type,
            payload: JSON.stringify(parameters.payload)
        });
        joynrMessage.from = proxyId;
        joynrMessage.to = providerId;
        dispatcher.receive(joynrMessage);
    }

    it("forwards subscription request to Publication Manager", () => {
        const payload = {
            subscribedToName: "attributeName",
            subscriptionId
        };
        receiveJoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
            payload
        });
        expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalled();
        expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalledWith(
            proxyId,
            providerId,
            new SubscriptionRequest(payload as any),
            expect.any(Function),
            expect.any(Object)
        );
    });

    it("forwards multicast subscription request to Publication Manager", () => {
        const payload = {
            subscribedToName: "multicastEvent",
            subscriptionId,
            multicastId
        };
        receiveJoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
            payload
        });
        expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalled();
        expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalledWith(
            proxyId,
            providerId,
            new MulticastSubscriptionRequest(payload),
            expect.any(Function),
            expect.any(Object)
        );
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(sentMessage.payload).toMatch(
            `{"_typeName":"joynr.SubscriptionReply","subscriptionId":"${subscriptionId}"}`
        );
    });

    it("forwards subscription reply to Subscription Manager", () => {
        const payload = {
            subscriptionId
        };
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY,
            payload: JSON.stringify(payload)
        });
        dispatcher.receive(joynrMessage);
        expect(subscriptionManager.handleSubscriptionReply).toHaveBeenCalled();
        expect(subscriptionManager.handleSubscriptionReply).toHaveBeenCalledWith(new SubscriptionReply(payload));
    });

    it("forwards subscription stop to SubscriptionPublication Manager", () => {
        const payload = {
            subscriptionId
        };
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP,
            payload: JSON.stringify(payload)
        });
        dispatcher.receive(joynrMessage);
        expect(publicationManager.handleSubscriptionStop).toHaveBeenCalled();
        expect(publicationManager.handleSubscriptionStop).toHaveBeenCalledWith(new SubscriptionStop(payload));
    });

    function receivePublication(parameters: any) {
        const joynrMessage = new JoynrMessage({
            type: parameters.type,
            payload: JSON.stringify(parameters.payload)
        });
        dispatcher.receive(joynrMessage);
    }

    it("forwards publication to Subscription Manager", () => {
        const payload = {
            subscriptionId,
            response: "myResponse"
        };
        receivePublication({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION,
            payload
        });
        expect(subscriptionManager.handlePublication).toHaveBeenCalled();
        expect(subscriptionManager.handlePublication).toHaveBeenCalledWith(SubscriptionPublication.create(payload));
    });

    it("forwards multicast publication to Subscription Manager", () => {
        const payload = {
            multicastId,
            response: "myResponse"
        };
        receivePublication({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST,
            payload
        });
        expect(subscriptionManager.handleMulticastPublication).toHaveBeenCalled();
        expect(subscriptionManager.handleMulticastPublication).toHaveBeenCalledWith(
            MulticastPublication.create(payload)
        );
    });

    it("forwards request to RequestReply Manager", () => {
        const request = Request.create({
            methodName: "methodName"
        });
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: JSON.stringify(request)
        });
        joynrMessage.to = providerId;
        joynrMessage.from = proxyId;
        dispatcher.receive(joynrMessage);
        expect(requestReplyManager.handleRequest).toHaveBeenCalled();
        expect(requestReplyManager.handleRequest.mock.calls.slice(-1)[0][0]).toEqual(providerId);
        expect(requestReplyManager.handleRequest.mock.calls.slice(-1)[0][1]).toEqual(request);
    });

    it("forwards one-way request to RequestReply Manager", () => {
        const oneWayRequest = OneWayRequest.create({
            methodName: "methodName"
        });
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY,
            payload: JSON.stringify(oneWayRequest)
        });
        joynrMessage.to = providerId;
        joynrMessage.from = proxyId;
        dispatcher.receive(joynrMessage);
        expect(requestReplyManager.handleOneWayRequest).toHaveBeenCalled();
        expect(requestReplyManager.handleOneWayRequest.mock.calls.slice(-1)[0][0]).toEqual(providerId);
        expect(requestReplyManager.handleOneWayRequest.mock.calls.slice(-1)[0][1]).toEqual(oneWayRequest);
    });

    it("forwards reply to RequestReply Manager", () => {
        const reply = Reply.create({
            requestReplyId,
            response: []
        });
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REPLY,
            payload: JSON.stringify(reply)
        });
        dispatcher.receive(joynrMessage);
        expect(requestReplyManager.handleReply).toHaveBeenCalled();
        expect(requestReplyManager.handleReply).toHaveBeenCalledWith(reply);
    });

    function sendBroadcastSubscriptionRequest(request: Request) {
        const messagingQos = new MessagingQos();
        return dispatcher.sendBroadcastSubscriptionRequest({
            from: proxyId,
            toDiscoveryEntry,
            messagingQos,
            subscriptionRequest: request as any
        });
    }

    it("does not send multicast subscription request if addMulticastReceiver fails", async () => {
        const multicastId = "multicastId";
        const multicastSubscriptionRequest = new MulticastSubscriptionRequest({
            subscribedToName: "multicastEvent",
            subscriptionId: "subscriptionId",
            multicastId
        });

        messageRouter.addMulticastReceiver.mockReturnValue(Promise.reject());
        expect(messageRouter.addMulticastReceiver).not.toHaveBeenCalled();
        expect(clusterControllerMessagingStub.transmit).not.toHaveBeenCalled();
        await testUtil.reversePromise(sendBroadcastSubscriptionRequest(multicastSubscriptionRequest as any));
        expect(messageRouter.addMulticastReceiver).toHaveBeenCalledTimes(1);
        expect(clusterControllerMessagingStub.transmit).not.toHaveBeenCalled();
    });

    it("is able to send selective broadcast subscription request", () => {
        expect(clusterControllerMessagingStub.transmit).not.toHaveBeenCalled();
        const broadcastSubscriptionRequest = new BroadcastSubscriptionRequest({
            subscribedToName: "broadcastEvent",
            subscriptionId: "subscriptionId"
        } as any);
        const serializedPayload = JSON.stringify(broadcastSubscriptionRequest);

        sendBroadcastSubscriptionRequest(broadcastSubscriptionRequest as any);

        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalledTimes(1);
        const sentMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(sentMessage.type).toEqual(JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST);
        expect(sentMessage.from).toEqual(proxyId);
        expect(sentMessage.to).toEqual(providerId);
        expect(sentMessage.payload).toEqual(serializedPayload);
    });

    it("sets isLocalMessage in request messages", () => {
        let sentMessage: any;
        const messagingQos = new MessagingQos();

        const request = Request.create({
            methodName: "methodName"
        });
        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        sentMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(sentMessage.isLocalMessage).toEqual(true);

        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry: globalToDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        sentMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(sentMessage.isLocalMessage).toEqual(false);
    });

    it("sets compress in request messages", () => {
        let sentMessage: any;
        const messagingQos = new MessagingQos();
        messagingQos.compress = true;

        const request = Request.create({
            methodName: "methodName"
        });
        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        sentMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(sentMessage.isLocalMessage).toEqual(true);

        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry: globalToDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        sentMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(sentMessage.compress).toEqual(true);
    });

    function verifyEffortForReply(request: any, expectedEffort: any) {
        expect(requestReplyManager.handleRequest).toHaveBeenCalled();
        expect(requestReplyManager.handleRequest.mock.calls.slice(-1)[0][0]).toEqual(providerId);
        expect(requestReplyManager.handleRequest.mock.calls.slice(-1)[0][1]).toEqual(request);
        expect(clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0].effort).toEqual(expectedEffort);

        requestReplyManager.handleRequest.mockClear();
        clusterControllerMessagingStub.transmit.mockClear();
    }

    it("sets the correct effort for replies if the effort was set in the corresponding request", async () => {
        const request = Request.create({
            methodName: "methodName"
        });
        const joynrMessage: any = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: JSON.stringify(request)
        });
        joynrMessage.to = providerId;
        joynrMessage.from = proxyId;
        joynrMessage.compress = true;

        requestReplyManager.handleRequest.mockImplementation(
            (_to: any, request: Request, cb: any, replySettings: any) => {
                cb(replySettings, request);
                return Promise.resolve();
            }
        );

        joynrMessage.effort = undefined; // default
        await dispatcher.receive(joynrMessage);
        verifyEffortForReply(request, undefined);

        joynrMessage.effort = MessagingQosEffort.NORMAL.value;
        await dispatcher.receive(joynrMessage);
        verifyEffortForReply(request, undefined);

        joynrMessage.effort = MessagingQosEffort.BEST_EFFORT.value;
        await dispatcher.receive(joynrMessage);
        verifyEffortForReply(request, MessagingQosEffort.BEST_EFFORT.value);

        joynrMessage.effort = "INVALID_EFFORT";
        await dispatcher.receive(joynrMessage);
        verifyEffortForReply(request, undefined);
    });

    it("sets the compressed flag for replies if the request was compressed", async () => {
        const request = Request.create({
            methodName: "methodName"
        });
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: JSON.stringify(request)
        });
        joynrMessage.to = providerId;
        joynrMessage.from = proxyId;
        joynrMessage.compress = true;

        requestReplyManager.handleRequest.mockImplementation(
            (_to: any, request: Request, cb: any, replySettings: any) => {
                cb(replySettings, request);
                return Promise.resolve();
            }
        );

        await dispatcher.receive(joynrMessage);
        expect(requestReplyManager.handleRequest).toHaveBeenCalled();
        expect(requestReplyManager.handleRequest.mock.calls.slice(-1)[0][0]).toEqual(providerId);
        expect(requestReplyManager.handleRequest.mock.calls.slice(-1)[0][1]).toEqual(request);
        expect(clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0].compress).toBe(true);
    });

    it("enriches requests with custom headers", () => {
        const request = Request.create({
            methodName: "methodName"
        });
        const messagingQos = new MessagingQos();
        const headerKey = "key";
        const headerValue = "value";
        messagingQos.putCustomMessageHeader(headerKey, headerValue);
        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(sentMessage.getCustomHeaders()[headerKey]).toEqual(headerValue);
    });

    it("enriches requests with effort header", () => {
        const request = Request.create({
            methodName: "methodName"
        });
        const messagingQos = new MessagingQos();
        messagingQos.effort = MessagingQosEffort.BEST_EFFORT;
        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(sentMessage.effort).toEqual(MessagingQosEffort.BEST_EFFORT.value);
    });

    it("enriches one way requests with custom headers", () => {
        const request = OneWayRequest.create({
            methodName: "methodName"
        });
        const messagingQos = new MessagingQos();
        const headerKey = "key";
        const headerValue = "value";
        messagingQos.putCustomMessageHeader(headerKey, headerValue);
        dispatcher.sendOneWayRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(sentMessage.getCustomHeaders()[headerKey]).toEqual(headerValue);
    });

    it("enriches replies with custom headers from request", async () => {
        const request = Request.create({
            methodName: "methodName"
        });
        const messagingQos = new MessagingQos();
        const headerKey = "key";
        const headerValue = "value";
        messagingQos.putCustomMessageHeader(headerKey, headerValue);
        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentRequestMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        const clusterControllerMessagingStubTransmitCallsCount =
            clusterControllerMessagingStub.transmit.mock.calls.length;
        // get ready for an incoming request: when handleRequest is called, pass an empty reply back.
        requestReplyManager.handleRequest.mockImplementation(
            (_to: any, request: Request, cb: any, replySettings: any) => {
                cb(replySettings, request);
                return Promise.resolve();
            }
        );
        await dispatcher.receive(sentRequestMessage);
        const sentReplyMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(clusterControllerMessagingStub.transmit.mock.calls.length).toBe(
            clusterControllerMessagingStubTransmitCallsCount + 1
        );
        expect(sentReplyMessage.getCustomHeaders()[headerKey]).toEqual(headerValue);
    });

    it("sends subscription reply on subscription request", async () => {
        const payload = {
            subscribedToName: "attributeName",
            subscriptionId
        };
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
            payload: JSON.stringify(payload)
        });
        joynrMessage.from = proxyId;
        joynrMessage.to = providerId;

        const subscriptionReply = new SubscriptionReply(payload);

        /*
         * The dispatcher.receive() based on the message type calls
         * publicationManager.handleSubscriptionRequest()
         * and hands over a callback that invokes sendSubscriptionReply().
         * The resulting message is finally sent out using
         * clusterControllerMessagingStub.transmit().
         */

        publicationManager.handleSubscriptionRequest.mockImplementation(
            (
                _proxyId: any,
                _providerId: any,
                _subscriptionRequest: SubscriptionRequest,
                callback: any,
                settings: any
            ) => {
                callback(settings, subscriptionReply);
            }
        );

        await dispatcher.receive(joynrMessage);

        /*
         * Note: We can directly expect stuff here only just because
         * the jasmine spies do not emulate async action which would
         * otherwise occur in real code.
         */
        expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalled();
        expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalledWith(
            proxyId,
            providerId,
            new SubscriptionRequest(payload as any),
            expect.any(Function),
            expect.any(Object)
        );

        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentMessage = clusterControllerMessagingStub.transmit.mock.calls.slice(-1)[0][0];
        expect(sentMessage.type).toEqual(JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY);
        expect(sentMessage.payload).toEqual(JSON.stringify(subscriptionReply));
    });

    it("accepts messages with Parse Errors", async () => {
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REPLY,
            payload: "invalidJSONPayload[/}"
        });

        await dispatcher.receive(joynrMessage);
    });
});
