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
const MessageRouter = require("../../../../../main/js/joynr/messaging/routing/MessageRouter");
const BrowserAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/BrowserAddress");
const ChannelAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/ChannelAddress");
const InProcessAddress = require("../../../../../main/js/joynr/messaging/inprocess/InProcessAddress");
const JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");
const TypeRegistry = require("../../../../../main/js/joynr/start/TypeRegistry");
const Date = require("../../../../../test/js/global/Date");
const waitsFor = require("../../../../../test/js/global/WaitsFor");
const UtilInternal = require("../../../../../main/js/joynr/util/UtilInternal");
const uuid = require("uuid/v4");
let fakeTime;

function increaseFakeTime(time_ms) {
    fakeTime = fakeTime + time_ms;
    jasmine.clock().tick(time_ms);
}
describe("libjoynr-js.joynr.messaging.routing.MessageRouter", () => {
    let store, typeRegistry;
    let senderParticipantId, receiverParticipantId, receiverParticipantId2;
    let joynrMessage, joynrMessage2;
    let persistencySpy, address;
    let messagingStubSpy, messagingSkeletonSpy, messagingStubFactorySpy, messagingSkeletonFactorySpy;
    let messageQueueSpy, messageRouter, routingProxySpy, parentMessageRouterAddress, incomingAddress;
    let multicastAddressCalculatorSpy;
    let serializedTestGlobalClusterControllerAddress;
    let multicastAddress;

    const createMessageRouter = function(persistency, messageQueue, incomingAddress, parentMessageRouterAddress) {
        return new MessageRouter({
            initialRoutingTable: [],
            persistency,
            joynrInstanceId: "joynrInstanceID",
            messagingStubFactory: messagingStubFactorySpy,
            messagingSkeletonFactory: messagingSkeletonFactorySpy,
            multicastAddressCalculator: multicastAddressCalculatorSpy,
            messageQueue,
            incomingAddress,
            parentMessageRouterAddress,
            typeRegistry
        });
    };

    const createRootMessageRouter = function(persistency, messageQueue) {
        return createMessageRouter(persistency, messageQueue, undefined, undefined);
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
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        joynrMessage2.to = receiverParticipantId2;
        joynrMessage2.from = "senderParticipantId";
        joynrMessage2.payload = "hello2";

        address = {
            addressInformation: "some info"
        };
        multicastAddressCalculatorSpy = jasmine.createSpyObj("multicastAddressCalculator", ["calculate"]);
        multicastAddress = new BrowserAddress({ windowId: "incomingAddress" });
        multicastAddressCalculatorSpy.calculate.and.returnValue(multicastAddress);

        messagingStubSpy = jasmine.createSpyObj("messagingStub", ["transmit"]);
        messagingSkeletonSpy = jasmine.createSpyObj("messagingSkeletonSpy", [
            "registerMulticastSubscription",
            "unregisterMulticastSubscription"
        ]);

        messagingStubSpy.transmit.and.returnValue(
            Promise.resolve({
                myKey: "myValue"
            })
        );
        messagingStubFactorySpy = jasmine.createSpyObj("messagingStubFactorySpy", ["createMessagingStub"]);

        messagingSkeletonFactorySpy = jasmine.createSpyObj("messagingSkeletonFactorySpy", ["getSkeleton"]);

        messagingStubFactorySpy.createMessagingStub.and.returnValue(messagingStubSpy);

        messagingSkeletonFactorySpy.getSkeleton.and.returnValue(messagingSkeletonSpy);

        messageQueueSpy = jasmine.createSpyObj("messageQueueSpy", ["putMessage", "getAndRemoveMessages", "shutdown"]);

        store = {};
        persistencySpy = jasmine.createSpyObj("persistencySpy", ["setItem", "removeItem", "getItem"]);
        persistencySpy.setItem.and.callFake((key, value) => {
            store[key] = value;
        });
        persistencySpy.getItem.and.callFake(key => {
            return store[key];
        });

        fakeTime = Date.now();
        jasmine.clock().install();
        spyOn(Date, "now").and.callFake(() => {
            return fakeTime;
        });

        typeRegistry = new TypeRegistry();
        typeRegistry.addType("joynr.system.RoutingTypes.ChannelAddress", ChannelAddress);
        typeRegistry.addType("joynr.system.RoutingTypes.BrowserAddress", BrowserAddress);

        serializedTestGlobalClusterControllerAddress = "testGlobalAddress";
        routingProxySpy = jasmine.createSpyObj("routingProxySpy", [
            "addNextHop",
            "removeNextHop",
            "resolveNextHop",
            "addMulticastReceiver",
            "removeMulticastReceiver"
        ]);
        routingProxySpy.globalAddress = {
            get: null
        };
        spyOn(routingProxySpy.globalAddress, "get").and.returnValue(
            Promise.resolve(serializedTestGlobalClusterControllerAddress)
        );

        routingProxySpy.replyToAddress = {
            get: null
        };
        routingProxySpy.proxyParticipantId = "proxyParticipantId";
        routingProxySpy.addNextHop.and.returnValue(Promise.resolve());
        spyOn(routingProxySpy.replyToAddress, "get").and.returnValue(
            Promise.resolve(serializedTestGlobalClusterControllerAddress)
        );

        messageRouter = createRootMessageRouter(persistencySpy, messageQueueSpy);
        messageRouter.setReplyToAddress(serializedTestGlobalClusterControllerAddress);

        done();
    });

    afterEach(done => {
        jasmine.clock().uninstall();
        done();
    });

    it("resolves a previously persisted channel address", done => {
        const participantId = "participantId";
        const channelAddress = new ChannelAddress({
            messagingEndpointUrl: "http://testurl.com",
            channelId: "channelId"
        });
        persistencySpy.setItem(messageRouter.getStorageKey(participantId), JSON.stringify(channelAddress));

        messageRouter
            .resolveNextHop(participantId)
            .then(returnedAddress => {
                expect(returnedAddress).toEqual(channelAddress);
                done();
                return null;
            })
            .catch(error => {
                fail(`got reject from resolveNextHop: ${error}`);
                return null;
            });
        increaseFakeTime(1);
    });

    it("resolves a previously persisted browser address", done => {
        const participantId = "participantId";
        const browserAddress = new BrowserAddress({
            windowId: "windowId"
        });
        persistencySpy.setItem(messageRouter.getStorageKey(participantId), JSON.stringify(browserAddress));

        messageRouter
            .resolveNextHop(participantId)
            .then(returnedAddress => {
                expect(returnedAddress).toEqual(browserAddress);
                done();
                return null;
            })
            .catch(error => {
                fail(`got reject from resolveNextHop: ${error}`);
                return null;
            });
        increaseFakeTime(1);
    });

    it("does not queue Reply and Publication Messages with unknown destinationParticipant", async () => {
        const msgTypes = [
            JoynrMessage.JOYNRMESSAGE_TYPE_REPLY,
            JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY,
            JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION
        ];
        for (let i = 0; i < msgTypes.length; i++) {
            joynrMessage2 = new JoynrMessage({
                type: msgTypes[i]
            });
            joynrMessage2.expiryDate = Date.now() + 2000;
            joynrMessage2.to = receiverParticipantId2;
            joynrMessage2.from = "senderParticipantId";
            joynrMessage2.payload = "hello2";

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
                type: msgTypes[i]
            });
            joynrMessage2.expiryDate = Date.now() + 2000;
            joynrMessage2.to = receiverParticipantId2;
            joynrMessage2.from = "senderParticipantId";
            joynrMessage2.payload = "hello2";

            await messageRouter.route(joynrMessage2);
            expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(joynrMessage2);
            expect(messageQueueSpy.getAndRemoveMessages).not.toHaveBeenCalled();
            expect(messagingStubFactorySpy.createMessagingStub).not.toHaveBeenCalled();
            expect(messagingStubSpy.transmit).not.toHaveBeenCalled();

            messageQueueSpy.putMessage.calls.reset();
        }
    });

    it("routes previously queued message once respective participant gets registered", done => {
        const messageQueue = [];
        messageQueue[0] = joynrMessage2;
        joynrMessage2.expiryDate = Date.now() + 2000;

        const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

        messageRouter
            .route(joynrMessage2)
            .then(onFulfilledSpy)
            .catch(() => {});
        increaseFakeTime(1);

        waitsFor(
            () => {
                return messageQueueSpy.putMessage.calls.count() > 0;
            },
            "messageQueueSpy to be invoked the first time",
            1000
        )
            .then(() => {
                expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(joynrMessage2);
                expect(messagingStubFactorySpy.createMessagingStub).not.toHaveBeenCalled();
                expect(messagingStubSpy.transmit).not.toHaveBeenCalled();

                const isGloballyVisible = true;
                messageRouter.addNextHop(joynrMessage2.to, address, isGloballyVisible).catch(() => {});
                messageQueueSpy.getAndRemoveMessages.and.returnValue(messageQueue);
                messageRouter.participantRegistered(joynrMessage2.to);
                increaseFakeTime(1);

                return waitsFor(
                    () => {
                        return (
                            messageQueueSpy.getAndRemoveMessages.calls.count() > 0 &&
                            messagingStubFactorySpy.createMessagingStub.calls.count() > 0 &&
                            messagingStubSpy.transmit.calls.count() > 0
                        );
                    },
                    "messageQueueSpy.getAndRemoveMessages spy to be invoked",
                    1000
                );
            })
            .then(() => {
                expect(messageQueueSpy.getAndRemoveMessages).toHaveBeenCalledWith(joynrMessage2.to);
                expect(messagingStubFactorySpy.createMessagingStub).toHaveBeenCalledWith(address);
                expect(messagingStubSpy.transmit).toHaveBeenCalledWith(joynrMessage2);
                messageRouter.removeNextHop(joynrMessage2.to);
                increaseFakeTime(1);
                done();
                return null;
            })
            .catch(fail);
    });

    it("drop previously queued message if respective participant gets registered after expiry date", done => {
        joynrMessage2.expiryDate = Date.now() + 2000;
        const messageQueue = [];
        messageQueue[0] = joynrMessage2;

        const returnValue = messageRouter.route(joynrMessage2);
        returnValue.catch(() => {});
        increaseFakeTime(1);

        waitsFor(
            () => {
                return messageQueueSpy.putMessage.calls.count() > 0;
            },
            "messageQueueSpy.putMessage invoked",
            1000
        )
            .then(() => {
                expect(messageQueueSpy.putMessage).toHaveBeenCalledTimes(1);
                expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(joynrMessage2);
                expect(messageQueueSpy.getAndRemoveMessages).not.toHaveBeenCalled();

                messageQueueSpy.getAndRemoveMessages.and.returnValue(messageQueue);
                increaseFakeTime(2000 + 1);
                const isGloballyVisible = true;
                messageRouter.addNextHop(joynrMessage2.to, address, isGloballyVisible);
                increaseFakeTime(1);

                return waitsFor(
                    () => {
                        return messageQueueSpy.getAndRemoveMessages.calls.count() > 0;
                    },
                    "messageQueueSpy.getAndRemoveMessages to be invoked",
                    1000
                );
            })
            .then(() => {
                expect(messageQueueSpy.putMessage).toHaveBeenCalledTimes(1);
                expect(messageQueueSpy.getAndRemoveMessages).toHaveBeenCalledTimes(1);
                expect(messageQueueSpy.getAndRemoveMessages).toHaveBeenCalledWith(joynrMessage2.to);
                expect(messagingStubFactorySpy.createMessagingStub).not.toHaveBeenCalled();
                expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
                messageRouter.removeNextHop(joynrMessage2.to);
                increaseFakeTime(1);
                done();
                return null;
            })
            .catch(fail);
    });

    it("route drops expired messages, but will resolve the Promise", done => {
        const isGloballyVisible = true;
        messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);
        joynrMessage.expiryDate = Date.now() - 1;
        joynrMessage.isLocalMessage = true;
        messageRouter
            .route(joynrMessage)
            .then(() => {
                expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
                done();
            })
            .catch(fail);
    });

    it("sets replyTo address for non local messages", done => {
        const isGloballyVisible = true;
        messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

        joynrMessage.isLocalMessage = false;
        expect(joynrMessage.replyChannelId).toEqual(undefined);

        messageRouter.route(joynrMessage);

        expect(messagingStubSpy.transmit).toHaveBeenCalled();
        const transmittedJoynrMessage = messagingStubSpy.transmit.calls.argsFor(0)[0];
        expect(transmittedJoynrMessage.replyChannelId).toEqual(serializedTestGlobalClusterControllerAddress);
        done();
    });

    it("does not set replyTo address for local messages", done => {
        const isGloballyVisible = false;
        messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

        joynrMessage.isLocalMessage = true;
        expect(joynrMessage.replyChannelId).toEqual(undefined);

        messageRouter.route(joynrMessage);

        expect(messagingStubSpy.transmit).toHaveBeenCalled();
        const transmittedJoynrMessage = messagingStubSpy.transmit.calls.argsFor(0)[0];
        expect(transmittedJoynrMessage.replyChannelId).toEqual(undefined);
        done();
    });

    function routeMessageWithValidReplyToAddressCallsAddNextHop() {
        messageRouter.addNextHop.calls.reset();
        expect(messageRouter.addNextHop).not.toHaveBeenCalled();
        const channelId = `testChannelId_${Date.now()}`;
        const channelAddress = new ChannelAddress({
            messagingEndpointUrl: "http://testurl.com",
            channelId
        });
        joynrMessage.replyChannelId = JSON.stringify(channelAddress);

        messageRouter.route(joynrMessage);

        expect(messageRouter.addNextHop).toHaveBeenCalledTimes(1);
        expect(messageRouter.addNextHop.calls.argsFor(0)[0]).toBe(senderParticipantId);
        expect(messageRouter.addNextHop.calls.argsFor(0)[1].channelId).toBe(channelId);
        expect(messageRouter.addNextHop.calls.argsFor(0)[2]).toBe(true);
    }

    it("route calls addNextHop for request messages received from global", done => {
        spyOn(messageRouter, "addNextHop");
        joynrMessage.isReceivedFromGlobal = true;

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST;
        routeMessageWithValidReplyToAddressCallsAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressCallsAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressCallsAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressCallsAddNextHop();

        done();
    });

    function routeMessageWithValidReplyToAddressDoesNotCallAddNextHop() {
        messageRouter.addNextHop.calls.reset();
        const channelId = `testChannelId_${Date.now()}`;
        const channelAddress = new ChannelAddress({
            messagingEndpointUrl: "http://testurl.com",
            channelId
        });
        joynrMessage.replyChannelId = JSON.stringify(channelAddress);

        messageRouter.route(joynrMessage);

        expect(messageRouter.addNextHop).not.toHaveBeenCalled();
    }

    it("route does NOT call addNextHop for request messages NOT received from global", done => {
        spyOn(messageRouter, "addNextHop");
        joynrMessage.isReceivedFromGlobal = false;

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
        routeMessageWithValidReplyToAddressDoesNotCallAddNextHop();

        done();
    });

    it("route does NOT call addNextHop for non request messages received from global", done => {
        spyOn(messageRouter, "addNextHop");
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

        done();
    });

    it("route does NOT call addNextHop for request messages received from global without replyTo address", done => {
        spyOn(messageRouter, "addNextHop");
        joynrMessage.isReceivedFromGlobal = true;

        joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST;

        messageRouter.route(joynrMessage);

        expect(messageRouter.addNextHop).not.toHaveBeenCalled();

        done();
    });

    it("addNextHop won't write InProcessAdresses", () => {
        address = new InProcessAddress();
        messageRouter.addNextHop(joynrMessage.to, address);
        expect(persistencySpy.setItem).not.toHaveBeenCalled();

        expect(messageRouter.removeNextHop.bind(null, joynrMessage.to)).not.toThrow();
    });

    it("addNextHop will work without Persistency", done => {
        messageRouter = createMessageRouter(null, messageQueueSpy);
        messageRouter.setReplyToAddress(serializedTestGlobalClusterControllerAddress);
        messageRouter.addNextHop(joynrMessage.to, address);
        messageRouter
            .route(joynrMessage)
            .then(() => {
                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                done();
            })
            .catch(fail);
    });

    describe("route multicast messages", () => {
        let parameters;
        let multicastMessage;
        let addressOfSubscriberParticipant;
        let isGloballyVisible;
        beforeEach(() => {
            parameters = {
                multicastId: `multicastId- ${uuid()}`,
                subscriberParticipantId: "subscriberParticipantId",
                providerParticipantId: "providerParticipantId"
            };

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
            expect(messagingStubSpy.transmit.calls.count()).toBe(1);
        });

        it("once, if message is NOT received from global and NO local receiver", () => {
            messageRouter.route(multicastMessage);
            expect(messagingStubFactorySpy.createMessagingStub).toHaveBeenCalled();
            expect(messagingStubFactorySpy.createMessagingStub.calls.count()).toEqual(1);
            const address = messagingStubFactorySpy.createMessagingStub.calls.argsFor(0)[0];
            expect(address).toEqual(multicastAddress);
            expect(messagingStubSpy.transmit).toHaveBeenCalled();
            expect(messagingStubSpy.transmit.calls.count()).toBe(1);
        });

        it("twice, if message is NOT received from global and local receiver available", () => {
            messageRouter.addMulticastReceiver(parameters);
            messageRouter.route(multicastMessage);
            expect(messagingStubSpy.transmit).toHaveBeenCalled();
            expect(messagingStubSpy.transmit.calls.count()).toBe(2);
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
            expect(messagingStubSpy.transmit.calls.count()).toBe(2);
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
            expect(messagingStubSpy.transmit.calls.count()).toBe(3);
        });
    }); // describe route multicast messages

    it("routes messages using the messagingStubFactory and messageStub", done => {
        const isGloballyVisible = true;
        messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);
        messageRouter.route(joynrMessage);
        increaseFakeTime(1);

        waitsFor(
            () => {
                return (
                    messagingStubFactorySpy.createMessagingStub.calls.count() > 0 &&
                    messagingStubSpy.transmit.calls.count() > 0
                );
            },
            "messagingStubFactorySpy.createMessagingStub to be invoked",
            1000
        )
            .then(() => {
                expect(messagingStubFactorySpy.createMessagingStub).toHaveBeenCalledWith(address);
                expect(messagingStubSpy.transmit).toHaveBeenCalledWith(joynrMessage);
                messageRouter.removeNextHop(joynrMessage.to);
                increaseFakeTime(1);
                done();
                return null;
            })
            .catch(fail);
    });

    it("discards messages without resolvable address", done => {
        const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
        const onRejectedSpy = jasmine.createSpy("onRejectedSpy");

        messageRouter
            .route(joynrMessage)
            .then(onFulfilledSpy)
            .catch(onRejectedSpy);
        increaseFakeTime(1);

        waitsFor(
            () => {
                return onFulfilledSpy.calls.count() > 0 || onRejectedSpy.calls.count() > 0;
            },
            "onFulfilled or onRejected spy to be invoked",
            1000
        )
            .then(() => {
                expect(messagingStubFactorySpy.createMessagingStub).not.toHaveBeenCalled();
                expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
                done();
                return null;
            })
            .catch(fail);
    });

    describe("ChildMessageRouter", () => {
        beforeEach(() => {
            messageRouter = createMessageRouter(
                persistencySpy,
                messageQueueSpy,
                incomingAddress,
                parentMessageRouterAddress
            );
            messageRouter.setReplyToAddress(serializedTestGlobalClusterControllerAddress);
        });

        it("queries global address from routing provider", done => {
            messageRouter = createMessageRouter(
                persistencySpy,
                messageQueueSpy,
                incomingAddress,
                parentMessageRouterAddress
            );
            routingProxySpy.addNextHop.and.returnValue(Promise.resolve());
            messageRouter
                .setRoutingProxy(routingProxySpy)
                .then(() => {
                    expect(routingProxySpy.replyToAddress.get).toHaveBeenCalled();
                    done();
                })
                .catch(error => {
                    done.fail(error);
                });
        });

        it("sets replyTo address for non local messages", done => {
            const isGloballyVisible = true;
            messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

            joynrMessage.isLocalMessage = false;
            expect(joynrMessage.replyChannelId).toEqual(undefined);

            messageRouter.route(joynrMessage);

            expect(messagingStubSpy.transmit).toHaveBeenCalled();
            const transmittedJoynrMessage = messagingStubSpy.transmit.calls.argsFor(0)[0];
            expect(transmittedJoynrMessage.replyChannelId).toEqual(serializedTestGlobalClusterControllerAddress);
            done();
        });

        it("does not set replyTo address for local messages", done => {
            const isGloballyVisible = true;
            messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

            joynrMessage.isLocalMessage = true;
            expect(joynrMessage.replyChannelId).toEqual(undefined);

            messageRouter.route(joynrMessage);

            expect(messagingStubSpy.transmit).toHaveBeenCalled();
            const transmittedJoynrMessage = messagingStubSpy.transmit.calls.argsFor(0)[0];
            expect(transmittedJoynrMessage.replyChannelId).toEqual(undefined);
            done();
        });

        it("queues non local messages until global address is available", done => {
            const isGloballyVisible = true;
            messageRouter = createMessageRouter(
                persistencySpy,
                messageQueueSpy,
                incomingAddress,
                parentMessageRouterAddress
            );
            routingProxySpy.addNextHop.and.returnValue(Promise.resolve());
            messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

            joynrMessage.isLocalMessage = false;
            const expectedJoynrMessage = JoynrMessage.parseMessage(UtilInternal.extendDeep({}, joynrMessage));
            expectedJoynrMessage.replyChannelId = serializedTestGlobalClusterControllerAddress;

            messageRouter
                .route(joynrMessage)
                .then(() => {
                    expect(messagingStubSpy.transmit).not.toHaveBeenCalled();

                    messageRouter.setRoutingProxy(routingProxySpy);

                    return waitsFor(
                        () => {
                            return messagingStubSpy.transmit.calls.count() >= 1;
                        },
                        "wait for tranmsit to be done",
                        1000
                    );
                })
                .then(() => {
                    expect(messagingStubSpy.transmit).toHaveBeenCalledWith(expectedJoynrMessage);
                    done();
                    return null;
                })
                .catch(done.fail);
        });

        it("address can be resolved once known to message router", done => {
            const participantId = "participantId-setToKnown";

            messageRouter.resolveNextHop(participantId).then(address => {
                expect(address).toBe(undefined);
                // it is expected that the given participantId cannot be resolved
                messageRouter.setToKnown(participantId);
                return messageRouter
                    .resolveNextHop(participantId)
                    .then(address => {
                        expect(address).toBe(parentMessageRouterAddress);
                        return done();
                    })
                    .catch(done.fail);
            });
        });

        describe("addMulticastReceiver", () => {
            let parameters;
            beforeEach(() => {
                parameters = {
                    multicastId: `multicastId- ${uuid()}`,
                    subscriberParticipantId: "subscriberParticipantId",
                    providerParticipantId: "providerParticipantId"
                };

                messageRouter.setToKnown(parameters.providerParticipantId);

                routingProxySpy.addMulticastReceiver.and.returnValue(Promise.resolve());

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
                    new InProcessAddress(undefined),
                    isGloballyVisible
                );
                messageRouter.addMulticastReceiver(parameters);

                expect(routingProxySpy.addMulticastReceiver).not.toHaveBeenCalled();

                expect(messageRouter.hasMulticastReceivers()).toBe(true);
            });

            it("queues calls and forwards them once proxy is available", done => {
                messageRouter.addMulticastReceiver(parameters);
                expect(routingProxySpy.addMulticastReceiver).not.toHaveBeenCalled();

                messageRouter
                    .setRoutingProxy(routingProxySpy)
                    .then(() => {
                        expect(routingProxySpy.addMulticastReceiver).toHaveBeenCalled();

                        expect(routingProxySpy.addMulticastReceiver).toHaveBeenCalledWith(parameters);

                        expect(messageRouter.hasMulticastReceivers()).toBe(true);

                        done();
                    })
                    .catch(done.fail);
            });
        }); // describe addMulticastReceiver

        describe("removeMulticastReceiver", () => {
            let parameters;
            beforeEach(() => {
                parameters = {
                    multicastId: `multicastId- ${uuid()}`,
                    subscriberParticipantId: "subscriberParticipantId",
                    providerParticipantId: "providerParticipantId"
                };

                messageRouter.setToKnown(parameters.providerParticipantId);

                routingProxySpy.addMulticastReceiver.and.returnValue(Promise.resolve());
                routingProxySpy.removeMulticastReceiver.and.returnValue(Promise.resolve());

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

            it("queues calls and forwards them once proxy is available", done => {
                messageRouter.removeMulticastReceiver(parameters);
                expect(routingProxySpy.removeMulticastReceiver).not.toHaveBeenCalled();

                messageRouter
                    .setRoutingProxy(routingProxySpy)
                    .then(() => {
                        expect(routingProxySpy.removeMulticastReceiver).toHaveBeenCalled();

                        expect(routingProxySpy.removeMulticastReceiver).toHaveBeenCalledWith(parameters);

                        expect(messageRouter.hasMulticastReceivers()).toBe(false);

                        done();
                    })
                    .catch(done.fail);
            });
        }); // describe removeMulticastReceiver

        function checkRoutingProxyAddNextHop(done, participantId, address, isGloballyVisible) {
            routingProxySpy.addNextHop.calls.reset();

            const expectedParticipantId = participantId;
            const expectedAddress = incomingAddress;
            const expectedIsGloballyVisible = isGloballyVisible;

            messageRouter.addNextHop(participantId, address, isGloballyVisible).catch(done.fail);

            expect(routingProxySpy.addNextHop).toHaveBeenCalledTimes(1);
            expect(routingProxySpy.addNextHop.calls.argsFor(0)[0].participantId).toEqual(expectedParticipantId);
            expect(routingProxySpy.addNextHop.calls.argsFor(0)[0].browserAddress).toEqual(expectedAddress);
            expect(routingProxySpy.addNextHop.calls.argsFor(0)[0].isGloballyVisible).toEqual(expectedIsGloballyVisible);
        }

        it("check if routing proxy is called correctly for hop additions", done => {
            routingProxySpy.addNextHop.and.returnValue(Promise.resolve());
            messageRouter.setRoutingProxy(routingProxySpy);

            let isGloballyVisible = true;
            checkRoutingProxyAddNextHop(done, joynrMessage.to, address, isGloballyVisible);

            isGloballyVisible = false;
            checkRoutingProxyAddNextHop(done, joynrMessage.to, address, isGloballyVisible);
            done();
        });

        it("check if routing proxy is called with queued hop additions", done => {
            routingProxySpy.addNextHop.and.returnValue(Promise.resolve());
            const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

            const isGloballyVisible = true;
            messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible).then(onFulfilledSpy);
            increaseFakeTime(1);

            expect(routingProxySpy.addNextHop).not.toHaveBeenCalled();

            messageRouter
                .setRoutingProxy(routingProxySpy)
                .then(() => {
                    expect(routingProxySpy.addNextHop).toHaveBeenCalledTimes(2);
                    expect(routingProxySpy.addNextHop.calls.argsFor(1)[0].participantId).toEqual(joynrMessage.to);
                    expect(routingProxySpy.addNextHop.calls.argsFor(1)[0].browserAddress).toEqual(incomingAddress);
                    done();
                })
                .catch(done.fail);
        });

        it("check if resolved hop from routing proxy is cached", done => {
            routingProxySpy.resolveNextHop.and.returnValue(Promise.resolve({ resolved: true }));
            messageRouter.setRoutingProxy(routingProxySpy);

            messageRouter
                .resolveNextHop(joynrMessage.to)
                .then(address => {
                    expect(address).toBe(parentMessageRouterAddress);
                    expect(routingProxySpy.resolveNextHop.calls.count()).toBe(1);
                    routingProxySpy.resolveNextHop.calls.reset();
                    return messageRouter.resolveNextHop(joynrMessage.to);
                })
                .then(address => {
                    expect(address).toBe(parentMessageRouterAddress);
                    expect(routingProxySpy.resolveNextHop).not.toHaveBeenCalled();
                    done();
                    return null;
                })
                .catch(done.fail);
        });

        it("check if routing proxy is called with multiple queued hop additions", done => {
            const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

            const isGloballyVisible = true;
            messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);
            messageRouter.addNextHop(joynrMessage2.to, address, isGloballyVisible).then(onFulfilledSpy);
            routingProxySpy.addNextHop.and.returnValue(Promise.resolve());
            increaseFakeTime(1);
            expect(routingProxySpy.addNextHop).not.toHaveBeenCalled();

            messageRouter
                .setRoutingProxy(routingProxySpy)
                .then(() => {
                    expect(routingProxySpy.addNextHop).toHaveBeenCalledTimes(3);
                    expect(routingProxySpy.addNextHop.calls.argsFor(1)[0].participantId).toEqual(joynrMessage.to);
                    expect(routingProxySpy.addNextHop.calls.argsFor(1)[0].browserAddress).toEqual(incomingAddress);
                    expect(routingProxySpy.addNextHop.calls.argsFor(2)[0].participantId).toEqual(joynrMessage2.to);
                    expect(routingProxySpy.addNextHop.calls.argsFor(2)[0].browserAddress).toEqual(incomingAddress);
                    done();
                })
                .catch(done.fail);
        });

        it("check if routing proxy is called with queued hop removals", done => {
            const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

            routingProxySpy.removeNextHop.and.returnValue(Promise.resolve());
            messageRouter.removeNextHop(joynrMessage.to).then(onFulfilledSpy);
            expect(onFulfilledSpy).not.toHaveBeenCalled();
            onFulfilledSpy.calls.reset();
            expect(routingProxySpy.removeNextHop).not.toHaveBeenCalled();
            messageRouter.setRoutingProxy(routingProxySpy);
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return routingProxySpy.removeNextHop.calls.count() > 0;
                },
                "routingProxySpy.removeNextHop to be invoked",
                1000
            )
                .then(() => {
                    expect(routingProxySpy.removeNextHop).toHaveBeenCalled();
                    expect(routingProxySpy.removeNextHop.calls.argsFor(0)[0].participantId).toEqual(joynrMessage.to);
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("check if routing proxy is called with multiple queued hop removals", done => {
            const onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

            routingProxySpy.removeNextHop.and.returnValue(Promise.resolve());
            messageRouter.removeNextHop(joynrMessage.to);
            messageRouter.removeNextHop(joynrMessage2.to).then(onFulfilledSpy);
            expect(onFulfilledSpy).not.toHaveBeenCalled();
            expect(routingProxySpy.removeNextHop).not.toHaveBeenCalled();
            messageRouter.setRoutingProxy(routingProxySpy);
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return routingProxySpy.removeNextHop.calls.count() === 2;
                },
                "routingProxySpy.removeNextHop to be invoked",
                1000
            )
                .then(() => {
                    expect(routingProxySpy.removeNextHop).toHaveBeenCalled();
                    expect(routingProxySpy.removeNextHop.calls.argsFor(0)[0].participantId).toEqual(joynrMessage.to);
                    expect(routingProxySpy.removeNextHop.calls.argsFor(1)[0].participantId).toEqual(joynrMessage2.to);
                    done();
                    return null;
                })
                .catch(fail);
        });

        it("check if routing proxy is called with queued hop removals", done => {
            const resolveNextHopSpy = jasmine.createSpy("resolveNextHopSpy");
            routingProxySpy.resolveNextHop.and.returnValue(
                Promise.resolve({
                    resolved: true
                })
            );

            messageRouter.resolveNextHop(joynrMessage.to).then(resolveNextHopSpy);
            increaseFakeTime(1);

            waitsFor(
                () => {
                    return resolveNextHopSpy.calls.count() > 0;
                },
                "resolveNextHop returned first time",
                1000
            )
                .then(() => {
                    expect(resolveNextHopSpy).toHaveBeenCalledWith(undefined);
                    resolveNextHopSpy.calls.reset();
                    messageRouter.setRoutingProxy(routingProxySpy);
                    expect(routingProxySpy.resolveNextHop).not.toHaveBeenCalled();
                    messageRouter.resolveNextHop(joynrMessage.to).then(resolveNextHopSpy);
                    increaseFakeTime(1);

                    return waitsFor(
                        () => {
                            return resolveNextHopSpy.calls.count() > 0;
                        },
                        "resolveNextHop returned second time",
                        1000
                    );
                })
                .then(() => {
                    expect(routingProxySpy.resolveNextHop).toHaveBeenCalled();
                    expect(routingProxySpy.resolveNextHop.calls.argsFor(0)[0].participantId).toEqual(joynrMessage.to);
                    expect(resolveNextHopSpy).toHaveBeenCalledWith(parentMessageRouterAddress);
                    done();
                    return null;
                })
                .catch(fail);
        });
        it(" throws exception when called while shut down", done => {
            messageRouter.shutdown();

            expect(messageQueueSpy.shutdown).toHaveBeenCalled();
            messageRouter
                .removeNextHop("hopId")
                .then(fail)
                .catch(() => {
                    return messageRouter.resolveNextHop("hopId").then(fail);
                })
                .catch(() => {
                    return messageRouter.addNextHop("hopId", {}).then(fail);
                })
                .catch(done);
        });

        it(" reject pending promises when shut down", done => {
            const isGloballyVisible = true;
            const addNextHopPromise = messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible).then(fail);
            const removeNextHopPromise = messageRouter.removeNextHop(joynrMessage.to).then(fail);
            increaseFakeTime(1);

            messageRouter.shutdown();
            addNextHopPromise.catch(() => {
                return removeNextHopPromise.catch(() => {
                    done();
                });
            });
        });
    }); // describe ChildMessageRouter
}); // describe MessageRouter
