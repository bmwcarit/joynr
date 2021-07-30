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

import MessageRouter from "../../../../../main/js/joynr/messaging/routing/MessageRouter";
import BrowserAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/BrowserAddress";
import ChannelAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/ChannelAddress";
import InProcessAddress from "../../../../../main/js/joynr/messaging/inprocess/InProcessAddress";
import JoynrMessage from "../../../../../main/js/joynr/messaging/JoynrMessage";
import TypeRegistry from "../../../../../main/js/joynr/start/TypeRegistry";
import * as UtilInternal from "../../../../../main/js/joynr/util/UtilInternal";
import nanoid from "nanoid";
import testUtil = require("../../../testUtil");
import UdsAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/UdsAddress";
import UdsClientAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/UdsClientAddress";
const typeRegistry = require("../../../../../main/js/joynr/types/TypeRegistrySingleton").getInstance();
typeRegistry.addType(BrowserAddress).addType(ChannelAddress);
let fakeTime: number;

async function increaseFakeTime(timeMs: number) {
    fakeTime = fakeTime + timeMs;
    jest.advanceTimersByTime(timeMs);
    await testUtil.multipleSetImmediate();
}
describe("libjoynr-js.joynr.messaging.routing.MessageRouter", () => {
    let typeRegistry: any;
    let senderParticipantId: any, receiverParticipantId: any, receiverParticipantId2: any;
    let joynrMessage: any, joynrMessage2: any;
    let address: any;
    let messagingStubSpy: any, messagingSkeletonSpy: any, messagingStubFactorySpy: any;
    let messageQueueSpy: any,
        messageRouter: any,
        messageRouterWithUdsAddress: any,
        routingProxySpy: any,
        parentMessageRouterAddress: any,
        incomingAddress: any;
    let multicastAddressCalculatorSpy: any;
    let serializedTestGlobalClusterControllerAddress: any;
    let multicastAddress: any;
    const routingTable: any = {};
    const multicastSkeletons: any = {};

    const createMessageRouter = function(
        messageQueue: any,
        incomingAddress?: any,
        parentMessageRouterAddress?: any
    ): MessageRouter {
        return new MessageRouter({
            initialRoutingTable: routingTable,
            messagingStubFactory: messagingStubFactorySpy,
            multicastSkeletons,
            multicastAddressCalculator: multicastAddressCalculatorSpy,
            messageQueue,
            incomingAddress,
            parentMessageRouterAddress
        });
    };

    const createRootMessageRouter = function(messageQueue: any) {
        return createMessageRouter(messageQueue);
    };

    beforeEach(done => {
        incomingAddress = new BrowserAddress({
            windowId: "incomingAddress"
        });
        parentMessageRouterAddress = new BrowserAddress({
            windowId: "parentMessageRouterAddress"
        });
        senderParticipantId = `testSenderParticipantId_${Date.now()}`;
        receiverParticipantId = `TestMessageRouter_participantId_${Date.now()}`;
        receiverParticipantId2 = `TestMessageRouter_delayedParticipantId_${Date.now()}`;
        joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: "hello"
        });
        joynrMessage.expiryDate = 9360686108031;
        joynrMessage.to = receiverParticipantId;
        joynrMessage.from = senderParticipantId;

        joynrMessage2 = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: "hello2"
        });
        joynrMessage2.to = receiverParticipantId2;
        joynrMessage2.from = "senderParticipantId";

        address = {
            addressInformation: "some info"
        };
        multicastAddressCalculatorSpy = {
            calculate: jest.fn()
        };
        multicastAddress = new BrowserAddress({ windowId: "incomingAddress" });
        multicastAddressCalculatorSpy.calculate.mockReturnValue(multicastAddress);

        messagingStubSpy = {
            transmit: jest.fn()
        };
        messagingSkeletonSpy = {
            registerMulticastSubscription: jest.fn(),
            unregisterMulticastSubscription: jest.fn()
        };

        messagingStubSpy.transmit.mockReturnValue(
            Promise.resolve({
                myKey: "myValue"
            })
        );
        messagingStubFactorySpy = {
            createMessagingStub: jest.fn()
        };

        messagingStubFactorySpy.createMessagingStub.mockReturnValue(messagingStubSpy);

        messageQueueSpy = {
            putMessage: jest.fn(),
            getAndRemoveMessages: jest.fn(),
            shutdown: jest.fn()
        };

        fakeTime = Date.now();
        jest.useFakeTimers();
        jest.spyOn(Date, "now").mockImplementation(() => {
            return fakeTime;
        });

        typeRegistry = new TypeRegistry();
        typeRegistry.addType(ChannelAddress);
        typeRegistry.addType(BrowserAddress);

        serializedTestGlobalClusterControllerAddress = "testGlobalAddress";
        routingProxySpy = {
            addNextHop: jest.fn().mockResolvedValue(undefined),
            removeNextHop: jest.fn(),
            resolveNextHop: jest.fn(),
            addMulticastReceiver: jest.fn(),
            removeMulticastReceiver: jest.fn(),
            globalAddress: {
                get: jest.fn().mockResolvedValue(serializedTestGlobalClusterControllerAddress)
            },
            replyToAddress: {
                get: jest.fn().mockResolvedValue(serializedTestGlobalClusterControllerAddress)
            },
            proxyParticipantId: "proxyParticipantId"
        };

        messageRouter = createRootMessageRouter(messageQueueSpy);
        messageRouter.setReplyToAddress(serializedTestGlobalClusterControllerAddress);

        done();
    });

    afterEach(done => {
        jest.useRealTimers();
        done();
    });

    it("does not queue Reply and Publication Messages with unknown destinationParticipant", async () => {
        const msgTypes = [
            JoynrMessage.JOYNRMESSAGE_TYPE_REPLY,
            JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY,
            JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION
        ];
        for (let i = 0; i < msgTypes.length; i++) {
            joynrMessage2 = new JoynrMessage({
                type: msgTypes[i],
                payload: "hello2"
            });
            joynrMessage2.expiryDate = Date.now() + 2000;
            joynrMessage2.to = receiverParticipantId2;
            joynrMessage2.from = "senderParticipantId";

            await messageRouter.route(joynrMessage2);
            expect(messageQueueSpy.putMessage).not.toHaveBeenCalledWith(joynrMessage2);
            expect(messageQueueSpy.getAndRemoveMessages).not.toHaveBeenCalled();
            expect(messagingStubFactorySpy.createMessagingStub).not.toHaveBeenCalled();
            expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
        }
    });

    it("queues Messages with unknown destinationParticipant which are not of type Reply or Publication", async () => {
        const msgTypes = [
            JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY,
            JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
            JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
            JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
            JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP
        ];

        for (let i = 0; i < msgTypes.length; i++) {
            joynrMessage2 = new JoynrMessage({
                type: msgTypes[i],
                payload: "hello2"
            });
            joynrMessage2.expiryDate = Date.now() + 2000;
            joynrMessage2.to = receiverParticipantId2;
            joynrMessage2.from = "senderParticipantId";

            await messageRouter.route(joynrMessage2);
            expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(joynrMessage2);
            expect(messageQueueSpy.getAndRemoveMessages).not.toHaveBeenCalled();
            expect(messagingStubFactorySpy.createMessagingStub).not.toHaveBeenCalled();
            expect(messagingStubSpy.transmit).not.toHaveBeenCalled();

            messageQueueSpy.putMessage.mockClear();
        }
    });

    it("routes previously queued message once respective participant gets registered", async () => {
        const messageQueue = [joynrMessage2];
        joynrMessage2.expiryDate = Date.now() + 2000;

        const onFulfilledSpy = jest.fn();

        messageRouter
            .route(joynrMessage2)
            .then(onFulfilledSpy)
            .catch(() => {});
        await increaseFakeTime(1);

        expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(joynrMessage2);
        expect(messagingStubFactorySpy.createMessagingStub).not.toHaveBeenCalled();
        expect(messagingStubSpy.transmit).not.toHaveBeenCalled();

        const isGloballyVisible = true;
        messageRouter.addNextHop(joynrMessage2.to, address, isGloballyVisible).catch(() => {});
        messageQueueSpy.getAndRemoveMessages.mockReturnValue(messageQueue);
        messageRouter.participantRegistered(joynrMessage2.to);
        await increaseFakeTime(1);
        expect(messageQueueSpy.getAndRemoveMessages).toHaveBeenCalledWith(joynrMessage2.to);
        expect(messagingStubFactorySpy.createMessagingStub).toHaveBeenCalledWith(address);
        expect(messagingStubSpy.transmit).toHaveBeenCalledWith(joynrMessage2);
        await messageRouter.removeNextHop(joynrMessage2.to);
        await increaseFakeTime(1);
    });

    it("drop previously queued message if respective participant gets registered after expiry date", async () => {
        joynrMessage2.expiryDate = Date.now() + 2000;
        const messageQueue = [];
        messageQueue[0] = joynrMessage2;

        await messageRouter.route(joynrMessage2);
        await increaseFakeTime(1);

        expect(messageQueueSpy.putMessage).toHaveBeenCalledTimes(1);
        expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(joynrMessage2);
        expect(messageQueueSpy.getAndRemoveMessages).not.toHaveBeenCalled();

        messageQueueSpy.getAndRemoveMessages.mockReturnValue(messageQueue);
        await increaseFakeTime(2000 + 1);
        const isGloballyVisible = true;
        await messageRouter.addNextHop(joynrMessage2.to, address, isGloballyVisible);
        await increaseFakeTime(1);

        expect(messageQueueSpy.putMessage).toHaveBeenCalledTimes(1);
        expect(messageQueueSpy.getAndRemoveMessages).toHaveBeenCalledTimes(1);
        expect(messageQueueSpy.getAndRemoveMessages).toHaveBeenCalledWith(joynrMessage2.to);
        expect(messagingStubFactorySpy.createMessagingStub).not.toHaveBeenCalled();
        expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
        await messageRouter.removeNextHop(joynrMessage2.to);
        await increaseFakeTime(1);
    });

    it("route drops expired messages, but will resolve the Promise", async () => {
        const isGloballyVisible = true;
        await messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);
        joynrMessage.expiryDate = Date.now() - 1;
        joynrMessage.isLocalMessage = true;

        await messageRouter.route(joynrMessage);

        expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
    });

    it("sets replyTo address for non local messages", () => {
        const isGloballyVisible = true;
        messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

        joynrMessage.isLocalMessage = false;
        expect(joynrMessage.replyChannelId).toEqual(undefined);

        messageRouter.route(joynrMessage);

        expect(messagingStubSpy.transmit).toHaveBeenCalled();
        const transmittedJoynrMessage = messagingStubSpy.transmit.mock.calls[0][0];
        expect(transmittedJoynrMessage.replyChannelId).toEqual(serializedTestGlobalClusterControllerAddress);
    });

    it("does not set replyTo address for local messages", () => {
        const isGloballyVisible = false;
        messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

        joynrMessage.isLocalMessage = true;
        expect(joynrMessage.replyChannelId).toEqual(undefined);

        messageRouter.route(joynrMessage);

        expect(messagingStubSpy.transmit).toHaveBeenCalled();
        const transmittedJoynrMessage = messagingStubSpy.transmit.mock.calls[0][0];
        expect(transmittedJoynrMessage.replyChannelId).toEqual(undefined);
    });

    function routeMessageWithValidReplyToAddressCallsAddNextHop() {
        messageRouter.addNextHop.mockClear();
        expect(messageRouter.addNextHop).not.toHaveBeenCalled();
        const channelId = `testChannelId_${Date.now()}`;
        const channelAddress = new ChannelAddress({
            messagingEndpointUrl: "http://testurl.com",
            channelId
        });
        joynrMessage.replyChannelId = JSON.stringify(channelAddress);

        messageRouter.route(joynrMessage);

        expect(messageRouter.addNextHop).toHaveBeenCalledTimes(1);
        expect(messageRouter.addNextHop.mock.calls[0][0]).toBe(senderParticipantId);
        expect(messageRouter.addNextHop.mock.calls[0][1].channelId).toBe(channelId);
        expect(messageRouter.addNextHop.mock.calls[0][2]).toBe(true);
    }

    it("route calls addNextHop for request messages received from global", () => {
        jest.spyOn(messageRouter, "addNextHop");
        joynrMessage.isReceivedFromGlobal = true;

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST;
        routeMessageWithValidReplyToAddressCallsAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressCallsAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressCallsAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressCallsAddNextHop();
    });

    function routeMessageWithValidReplyToAddressDoesNotCallAddNextHop() {
        messageRouter.addNextHop.mockClear();
        const channelId = `testChannelId_${Date.now()}`;
        const channelAddress = new ChannelAddress({
            messagingEndpointUrl: "http://testurl.com",
            channelId
        });
        joynrMessage.replyChannelId = JSON.stringify(channelAddress);

        messageRouter.route(joynrMessage);

        expect(messageRouter.addNextHop).not.toHaveBeenCalled();
    }

    it("route does NOT call addNextHop for request messages NOT received from global", () => {
        jest.spyOn(messageRouter, "addNextHop");
        joynrMessage.isReceivedFromGlobal = false;

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();
    });

    it("route does NOT call addNextHop for non request messages received from global", () => {
        jest.spyOn(messageRouter, "addNextHop");
        joynrMessage.isReceivedFromGlobal = true;

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_REPLY;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();
    });

    it("route does NOT call addNextHop for request messages received from global without replyTo address", () => {
        jest.spyOn(messageRouter, "addNextHop");
        joynrMessage.isReceivedFromGlobal = true;

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST;

        messageRouter.route(joynrMessage);

        expect(messageRouter.addNextHop).not.toHaveBeenCalled();
    });

    it("addNextHop will work", async () => {
        messageRouter = createMessageRouter(messageQueueSpy);
        messageRouter.setReplyToAddress(serializedTestGlobalClusterControllerAddress);
        messageRouter.addNextHop(joynrMessage.to, address);

        await messageRouter.route(joynrMessage);

        expect(messagingStubSpy.transmit).toHaveBeenCalled();
    });

    describe("route multicast messages", () => {
        let parameters: any;
        let multicastMessage: any;
        let addressOfSubscriberParticipant: any;
        let isGloballyVisible: any;
        beforeEach(() => {
            parameters = {
                multicastId: `multicastId- ${nanoid()}`,
                subscriberParticipantId: "subscriberParticipantId",
                providerParticipantId: "providerParticipantId"
            };
            routingTable["providerParticipantId"] = { _typeName: "typeName" };
            multicastSkeletons["typeName"] = messagingSkeletonSpy;

            multicastMessage = new JoynrMessage({
                type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST,
                payload: "hello"
            });
            multicastMessage.expiryDate = 9360686108031;
            multicastMessage.to = parameters.multicastId;
            multicastMessage.from = "senderParticipantId";

            /* add routing table entry for parameters.subscriberParticipantId,
             * otherwise messaging stub call can be executed by the message router
             */
            addressOfSubscriberParticipant = new BrowserAddress({
                windowId: "windowIdOfSubscriberParticipant"
            });
            isGloballyVisible = true;
            messageRouter.addNextHop(
                parameters.subscriberParticipantId,
                addressOfSubscriberParticipant,
                isGloballyVisible
            );
        });

        it("never, if message is received from global and NO local receiver", () => {
            multicastMessage.isReceivedFromGlobal = true;
            messageRouter.route(multicastMessage);
            expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
        });

        it("once, if message is received from global and has local receiver", () => {
            messageRouter.addMulticastReceiver(parameters);
            multicastMessage.isReceivedFromGlobal = true;
            messageRouter.route(multicastMessage);
            expect(messagingStubSpy.transmit).toHaveBeenCalled();
            expect(messagingStubSpy.transmit.mock.calls.length).toBe(1);
        });

        it("once, if message is NOT received from global and NO local receiver", () => {
            messageRouter.route(multicastMessage);
            expect(messagingStubFactorySpy.createMessagingStub).toHaveBeenCalled();
            expect(messagingStubFactorySpy.createMessagingStub.mock.calls.length).toEqual(1);
            const address = messagingStubFactorySpy.createMessagingStub.mock.calls[0][0];
            expect(address).toEqual(multicastAddress);
            expect(messagingStubSpy.transmit).toHaveBeenCalled();
            expect(messagingStubSpy.transmit.mock.calls.length).toBe(1);
        });

        it("twice, if message is NOT received from global and local receiver available", () => {
            messageRouter.addMulticastReceiver(parameters);
            messageRouter.route(multicastMessage);
            expect(messagingStubSpy.transmit).toHaveBeenCalled();
            expect(messagingStubSpy.transmit.mock.calls.length).toBe(2);
        });

        it("twice, if message is NOT received from global and two local receivers available with same receiver address", () => {
            messageRouter.addMulticastReceiver(parameters);
            const parametersForSndReceiver = {
                multicastId: parameters.multicastId,
                subscriberParticipantId: "subscriberParticipantId2",
                providerParticipantId: "providerParticipantId"
            };

            messageRouter.addMulticastReceiver(parametersForSndReceiver);
            messageRouter.addNextHop(
                parametersForSndReceiver.subscriberParticipantId,
                addressOfSubscriberParticipant,
                isGloballyVisible
            );
            messageRouter.route(multicastMessage);
            expect(messagingStubSpy.transmit).toHaveBeenCalled();
            expect(messagingStubSpy.transmit.mock.calls.length).toBe(2);
        });

        it("three times, if message is NOT received from global and two local receivers available with different receiver address", () => {
            messageRouter.addMulticastReceiver(parameters);
            const parametersForSndReceiver = {
                multicastId: parameters.multicastId,
                subscriberParticipantId: "subscriberParticipantId2",
                providerParticipantId: "providerParticipantId"
            };

            messageRouter.addMulticastReceiver(parametersForSndReceiver);
            messageRouter.addNextHop(
                parametersForSndReceiver.subscriberParticipantId,
                new BrowserAddress({
                    windowId: "windowIdOfNewSubscribeParticipant"
                }),
                isGloballyVisible
            );
            messageRouter.route(multicastMessage);
            expect(messagingStubSpy.transmit).toHaveBeenCalled();
            expect(messagingStubSpy.transmit.mock.calls.length).toBe(3);
        });
    }); // describe route multicast messages

    it("routes messages using the messagingStubFactory and messageStub", async () => {
        const isGloballyVisible = true;
        await messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);
        await messageRouter.route(joynrMessage);
        await increaseFakeTime(1);

        expect(messagingStubFactorySpy.createMessagingStub).toHaveBeenCalledWith(address);
        expect(messagingStubSpy.transmit).toHaveBeenCalledWith(joynrMessage);
        messageRouter.removeNextHop(joynrMessage.to);
        await increaseFakeTime(1);
    });

    it("discards messages without resolvable address", async () => {
        await messageRouter.route(joynrMessage);
        await increaseFakeTime(1);

        expect(messagingStubFactorySpy.createMessagingStub).not.toHaveBeenCalled();
        expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
    });

    describe("ChildMessageRouterWithUdsAddress", () => {
        beforeEach(done => {
            incomingAddress = new UdsClientAddress({
                id: "udsId"
            });
            parentMessageRouterAddress = new UdsAddress({
                path: "path"
            });

            messageRouterWithUdsAddress = createMessageRouter(
                messageQueueSpy,
                incomingAddress,
                parentMessageRouterAddress
            );
            messageRouterWithUdsAddress.setReplyToAddress(serializedTestGlobalClusterControllerAddress);

            done();
        });

        it("addNextHop adds UdsClientAddress to local and parent routing table", async () => {
            const expectedParameters = {
                participantId: joynrMessage.to,
                udsClientAddress: incomingAddress,
                isGloballyVisible: false
            };
            await messageRouterWithUdsAddress.setRoutingProxy(routingProxySpy);
            expect(routingTable[joynrMessage.to]).toBeUndefined();
            address = new UdsClientAddress({ id: "udsId" });
            await messageRouterWithUdsAddress.addNextHop(joynrMessage.to, address, false);
            expect(routingTable[joynrMessage.to].id).toEqual("udsId");
            expect(routingTable[joynrMessage.to]._typeName).toEqual(address._typeName);
            expect(() => messageRouterWithUdsAddress.removeNextHop(joynrMessage.to)).not.toThrow();
            expect(routingProxySpy.addNextHop).toHaveBeenCalledWith(expectedParameters);
        });
    }); // describe ChildMessageRouterWithUdsAddress

    describe("ChildMessageRouter", () => {
        beforeEach(() => {
            messageRouter = createMessageRouter(messageQueueSpy, incomingAddress, parentMessageRouterAddress);
            messageRouter.setReplyToAddress(serializedTestGlobalClusterControllerAddress);
        });

        it("queries global address from routing provider", () => {
            messageRouter = createMessageRouter(messageQueueSpy, incomingAddress, parentMessageRouterAddress);
            routingProxySpy.addNextHop.mockReturnValue(Promise.resolve());
        });

        it("sets replyTo address for non local messages", () => {
            const isGloballyVisible = true;
            messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

            joynrMessage.isLocalMessage = false;
            expect(joynrMessage.replyChannelId).toEqual(undefined);

            messageRouter.route(joynrMessage);

            expect(messagingStubSpy.transmit).toHaveBeenCalled();
            const transmittedJoynrMessage = messagingStubSpy.transmit.mock.calls[0][0];
            expect(transmittedJoynrMessage.replyChannelId).toEqual(serializedTestGlobalClusterControllerAddress);
        });

        it("does not set replyTo address for local messages", () => {
            const isGloballyVisible = true;
            messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

            joynrMessage.isLocalMessage = true;
            expect(joynrMessage.replyChannelId).toEqual(undefined);

            messageRouter.route(joynrMessage);

            expect(messagingStubSpy.transmit).toHaveBeenCalled();
            const transmittedJoynrMessage = messagingStubSpy.transmit.mock.calls[0][0];
            expect(transmittedJoynrMessage.replyChannelId).toEqual(undefined);
        });

        it("queues non local messages until global address is available", async () => {
            const isGloballyVisible = true;
            messageRouter = createMessageRouter(messageQueueSpy, incomingAddress, parentMessageRouterAddress);
            routingProxySpy.addNextHop.mockReturnValue(Promise.resolve());
            await messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

            joynrMessage.isLocalMessage = false;
            const expectedJoynrMessage = JoynrMessage.parseMessage(UtilInternal.extendDeep({}, joynrMessage));
            expectedJoynrMessage.replyChannelId = serializedTestGlobalClusterControllerAddress;

            await messageRouter.route(joynrMessage);
            expect(messagingStubSpy.transmit).not.toHaveBeenCalled();

            await messageRouter.setRoutingProxy(routingProxySpy);
            await messageRouter.configureReplyToAddressFromRoutingProxy();
            await testUtil.multipleSetImmediate();

            expect(messagingStubSpy.transmit).toHaveBeenCalledWith(expectedJoynrMessage);
        });

        it("address can be resolved once known to message router", async () => {
            const participantId = "participantId-setToKnown";

            const address = await messageRouter.resolveNextHop(participantId);
            expect(address).toBe(undefined);
            // it is expected that the given participantId cannot be resolved
            messageRouter.setToKnown(participantId);

            const address2 = await messageRouter.resolveNextHop(participantId);
            expect(address2).toBe(parentMessageRouterAddress);
        });

        describe("addMulticastReceiver", () => {
            let parameters: any;
            beforeEach(() => {
                parameters = {
                    multicastId: `multicastId- ${nanoid()}`,
                    subscriberParticipantId: "subscriberParticipantId",
                    providerParticipantId: "providerParticipantId"
                };
                routingTable["providerParticipantId"] = { _typeName: "typeName" };
                multicastSkeletons["typeName"] = messagingSkeletonSpy;

                messageRouter.setToKnown(parameters.providerParticipantId);

                routingProxySpy.addMulticastReceiver.mockReturnValue(Promise.resolve());

                expect(messageRouter.hasMulticastReceivers()).toBe(false);
            });

            it("calls matching skeleton", () => {
                messageRouter.addMulticastReceiver(parameters);

                expect(messagingSkeletonSpy.registerMulticastSubscription).toHaveBeenCalled();

                expect(messagingSkeletonSpy.registerMulticastSubscription).toHaveBeenCalledWith(parameters.multicastId);

                expect(messageRouter.hasMulticastReceivers()).toBe(true);
            });

            it("calls routing proxy if available", () => {
                messageRouter.setRoutingProxy(routingProxySpy);

                messageRouter.addMulticastReceiver(parameters);

                expect(routingProxySpy.addMulticastReceiver).toHaveBeenCalled();

                expect(routingProxySpy.addMulticastReceiver).toHaveBeenCalledWith(parameters);

                expect(messageRouter.hasMulticastReceivers()).toBe(true);
            });

            it("does not call routing proxy for in process provider", () => {
                const isGloballyVisible = true;
                messageRouter.setRoutingProxy(routingProxySpy);

                parameters.providerParticipantId = "inProcessParticipant";
                messageRouter.addNextHop(
                    parameters.providerParticipantId,
                    new InProcessAddress(undefined as any),
                    isGloballyVisible
                );
                messageRouter.addMulticastReceiver(parameters);

                expect(routingProxySpy.addMulticastReceiver).not.toHaveBeenCalled();

                expect(messageRouter.hasMulticastReceivers()).toBe(true);
            });

            it("queues calls and forwards them once proxy is available", () => {
                messageRouter.addMulticastReceiver(parameters);
                expect(routingProxySpy.addMulticastReceiver).not.toHaveBeenCalled();
            });
        }); // describe addMulticastReceiver

        describe("removeMulticastReceiver", () => {
            let parameters: any;
            beforeEach(() => {
                parameters = {
                    multicastId: `multicastId- ${nanoid()}`,
                    subscriberParticipantId: "subscriberParticipantId",
                    providerParticipantId: "providerParticipantId"
                };
                routingTable["providerParticipantId"] = { _typeName: "typeName" };
                multicastSkeletons["typeName"] = messagingSkeletonSpy;

                messageRouter.setToKnown(parameters.providerParticipantId);

                routingProxySpy.addMulticastReceiver.mockReturnValue(Promise.resolve());
                routingProxySpy.removeMulticastReceiver.mockReturnValue(Promise.resolve());

                expect(messageRouter.hasMulticastReceivers()).toBe(false);
                /* addMulticastReceiver is already tested, but added here for
                 * checking proper removeMulticastReceiver functionality */
                messageRouter.addMulticastReceiver(parameters);
                expect(messageRouter.hasMulticastReceivers()).toBe(true);
            });

            it("calls matching skeleton registration and unregistration", () => {
                messageRouter.removeMulticastReceiver(parameters);

                expect(messagingSkeletonSpy.unregisterMulticastSubscription).toHaveBeenCalled();

                expect(messagingSkeletonSpy.unregisterMulticastSubscription).toHaveBeenCalledWith(
                    parameters.multicastId
                );
                expect(messageRouter.hasMulticastReceivers()).toBe(false);
            });

            it("calls routing proxy if available", () => {
                messageRouter.setRoutingProxy(routingProxySpy);

                messageRouter.removeMulticastReceiver(parameters);

                expect(routingProxySpy.removeMulticastReceiver).toHaveBeenCalled();

                expect(routingProxySpy.removeMulticastReceiver).toHaveBeenCalledWith(parameters);

                expect(messageRouter.hasMulticastReceivers()).toBe(false);
            });

            it("queues calls and forwards them once proxy is available", () => {
                messageRouter.removeMulticastReceiver(parameters);
                expect(routingProxySpy.removeMulticastReceiver).not.toHaveBeenCalled();
            });
        }); // describe removeMulticastReceiver

        async function checkRoutingProxyAddNextHop(
            participantId: any,
            address: any,
            isGloballyVisible: any
        ): Promise<void> {
            routingProxySpy.addNextHop.mockClear();

            const expectedParticipantId = participantId;
            const expectedAddress = incomingAddress;
            const expectedIsGloballyVisible = isGloballyVisible;

            await messageRouter.addNextHop(participantId, address, isGloballyVisible);

            expect(routingProxySpy.addNextHop).toHaveBeenCalledTimes(1);
            expect(routingProxySpy.addNextHop.mock.calls[0][0].participantId).toEqual(expectedParticipantId);
            expect(routingProxySpy.addNextHop.mock.calls[0][0].browserAddress).toEqual(expectedAddress);
            expect(routingProxySpy.addNextHop.mock.calls[0][0].isGloballyVisible).toEqual(expectedIsGloballyVisible);
        }

        it("check if routing proxy is called correctly for hop additions", async () => {
            routingProxySpy.addNextHop.mockReturnValue(Promise.resolve());
            await messageRouter.setRoutingProxy(routingProxySpy);

            let isGloballyVisible = true;
            await checkRoutingProxyAddNextHop(joynrMessage.to, address, isGloballyVisible);

            isGloballyVisible = false;
            await checkRoutingProxyAddNextHop(joynrMessage.to, address, isGloballyVisible);
        });

        it("check if setRoutingProxy calls addNextHop", () => {
            routingProxySpy.addNextHop.mockReturnValue(Promise.resolve());
            expect(routingProxySpy.addNextHop).not.toHaveBeenCalled();
        });

        it("check if resolved hop from routing proxy is cached", () => {
            routingProxySpy.resolveNextHop.mockReturnValue(Promise.resolve({ resolved: true }));
            messageRouter.setRoutingProxy(routingProxySpy);
        });

        it("check if routing proxy is called with queued hop removals", async () => {
            const onFulfilledSpy = jest.fn();

            routingProxySpy.removeNextHop.mockReturnValue(Promise.resolve());
            messageRouter.removeNextHop(joynrMessage.to).then(onFulfilledSpy);
            expect(onFulfilledSpy).not.toHaveBeenCalled();
            onFulfilledSpy.mockClear();
            expect(routingProxySpy.removeNextHop).not.toHaveBeenCalled();
            messageRouter.setRoutingProxy(routingProxySpy);
            await increaseFakeTime(1);

            expect(routingProxySpy.removeNextHop).toHaveBeenCalled();
            expect(routingProxySpy.removeNextHop.mock.calls[0][0].participantId).toEqual(joynrMessage.to);
        });

        it("check if routing proxy is called with multiple queued hop removals", async () => {
            const onFulfilledSpy = jest.fn();

            routingProxySpy.removeNextHop.mockReturnValue(Promise.resolve());
            messageRouter.removeNextHop(joynrMessage.to).then(onFulfilledSpy);
            messageRouter.removeNextHop(joynrMessage2.to).then(onFulfilledSpy);
            await testUtil.multipleSetImmediate();
            expect(onFulfilledSpy).not.toHaveBeenCalled();
            expect(routingProxySpy.removeNextHop).not.toHaveBeenCalled();
            await messageRouter.setRoutingProxy(routingProxySpy);
            await increaseFakeTime(1);

            expect(routingProxySpy.removeNextHop).toHaveBeenCalled();
            expect(routingProxySpy.removeNextHop.mock.calls[0][0].participantId).toEqual(joynrMessage.to);
            expect(routingProxySpy.removeNextHop.mock.calls[1][0].participantId).toEqual(joynrMessage2.to);
        });

        it("check if routing proxy is called with queued hop removals 2", async () => {
            routingProxySpy.resolveNextHop.mockResolvedValue({
                resolved: true
            });

            await messageRouter.resolveNextHop(joynrMessage.to);

            await messageRouter.setRoutingProxy(routingProxySpy);
            expect(routingProxySpy.resolveNextHop).not.toHaveBeenCalled();
            const resolvedNextHop = await messageRouter.resolveNextHop(joynrMessage.to);

            expect(routingProxySpy.resolveNextHop).toHaveBeenCalled();
            expect(routingProxySpy.resolveNextHop.mock.calls[0][0].participantId).toEqual(joynrMessage.to);
            expect(resolvedNextHop).toEqual(parentMessageRouterAddress);
        });

        it(" throws exception when called while shut down", done => {
            messageRouter.shutdown();

            expect(messageQueueSpy.shutdown).toHaveBeenCalled();
            messageRouter
                .removeNextHop("hopId")
                .then(done.fail)
                .catch(() => {
                    return messageRouter.resolveNextHop("hopId").then(done.fail);
                })
                .catch(() => {
                    return messageRouter.addNextHop("hopId", {}).then(done.fail);
                })
                .catch(() => done());
        });
    }); // describe ChildMessageRouter
}); // describe MessageRouter
