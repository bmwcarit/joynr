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

define([
            "joynr/messaging/routing/MessageRouter",
            "joynr/system/RoutingTypes/BrowserAddress",
            "joynr/system/RoutingTypes/ChannelAddress",
            "joynr/messaging/JoynrMessage",
            "joynr/start/TypeRegistry",
            "global/Promise",
            "Date"
        ],
        function(
                MessageRouter,
                BrowserAddress,
                ChannelAddress,
                JoynrMessage,
                TypeRegistry,
                Promise,
                Date) {
            var fakeTime;

            function increaseFakeTime(time_ms) {
                fakeTime = fakeTime + time_ms;
                jasmine.Clock.tick(time_ms);
            }
            describe(
                    "libjoynr-js.joynr.messaging.routing.MessageRouter",
                    function() {
                        var store, typeRegistry, receiverParticipantId, receiverParticipantId2, joynrMessage, joynrMessage2, myChannelId, persistencySpy, otherChannelId, resultObj, address, messagingStubSpy, messagingStubFactorySpy, messageQueueSpy, messageRouter, routingProxySpy, parentMessageRouterAddress, incomingAddress;

                        var createMessageRouter =
                                function(
                                        persistency,
                                        messagingStubFactory,
                                        messageQueue,
                                        incomingAddress,
                                        parentMessageRouterAddress) {
                                    return new MessageRouter({
                                        initialRoutingTable : [],
                                        persistency : persistency,
                                        joynrInstanceId : "joynrInstanceID",
                                        messagingStubFactory : messagingStubFactory,
                                        messageQueue : messageQueue,
                                        incomingAddress : incomingAddress,
                                        parentMessageRouterAddress : parentMessageRouterAddress,
                                        typeRegistry : typeRegistry
                                    });
                                };

                        var createRootMessageRouter =
                                function(persistency, messagingStubFactory, messageQueue) {
                                    return createMessageRouter(
                                            persistency,
                                            messagingStubFactory,
                                            messageQueue,
                                            undefined,
                                            undefined);
                                };

                        beforeEach(function() {
                            incomingAddress = new BrowserAddress({
                                windowId : "incomingAddress"
                            });
                            parentMessageRouterAddress = new BrowserAddress({
                                windowId : "parentMessageRouterAddress"
                            });
                            receiverParticipantId = "TestMessageRouter_participantId_" + Date.now();
                            receiverParticipantId2 =
                                    "TestMessageRouter_delayedParticipantId_" + Date.now();
                            joynrMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
                            joynrMessage.expiryDate = 9360686108031;
                            joynrMessage.to = receiverParticipantId;
                            joynrMessage.from = "senderParticipantId";
                            joynrMessage.payload = "hello";

                            joynrMessage2 =
                                    new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
                            joynrMessage2.to = receiverParticipantId2;
                            joynrMessage2.from = "senderParticipantId";
                            joynrMessage2.payload = "hello2";

                            myChannelId = "myChannelId";
                            otherChannelId = "otherChannelId";
                            address = {
                                addressInformation : "some info"
                            };
                            messagingStubSpy = jasmine.createSpyObj("messagingStub", [ "transmit"
                            ]);
                            messagingStubSpy.transmit.andReturn(Promise.resolve({
                                myKey : "myValue"
                            }));
                            messagingStubFactorySpy =
                                    jasmine.createSpyObj(
                                            "messagingStubFactorySpy",
                                            [ "createMessagingStub"
                                            ]);
                            messagingStubFactorySpy.createMessagingStub.andReturn(messagingStubSpy);

                            messageQueueSpy = jasmine.createSpyObj("messageQueueSpy", [
                                "putMessage",
                                "getAndRemoveMessages"
                            ]);

                            store = {};
                            persistencySpy = jasmine.createSpyObj("persistencySpy", [
                                "setItem",
                                "removeItem",
                                "getItem"
                            ]);
                            persistencySpy.setItem.andCallFake(function(key, value) {
                                store[key] = value;
                            });
                            persistencySpy.getItem.andCallFake(function(key) {
                                return store[key];
                            });

                            fakeTime = Date.now();
                            jasmine.Clock.useMock();
                            jasmine.Clock.reset();
                            spyOn(Date, "now").andCallFake(function() {
                                return fakeTime;
                            });

                            typeRegistry = new TypeRegistry();
                            typeRegistry.addType("joynr.system.RoutingTypes.ChannelAddress", ChannelAddress);
                            typeRegistry.addType("joynr.system.RoutingTypes.BrowserAddress", BrowserAddress);

                        });

                        it(
                                "resolves a previously persisted channel address",
                                function() {
                                    var participantId = "participantId", channelAddress;
                                    var resolveNextHopSpy = jasmine.createSpy("resolveNextHopSpy");
                                    messageRouter =
                                            createRootMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy);

                                    channelAddress = new ChannelAddress({
                                        channelId : "channelId"
                                    });
                                    persistencySpy.setItem(messageRouter
                                            .getStorageKey(participantId), JSON
                                            .stringify(channelAddress));

                                    runs(function() {
                                        messageRouter.resolveNextHop(participantId).then(
                                                resolveNextHopSpy);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return resolveNextHopSpy.callCount > 0;
                                    }, "resolveNextHopSpy to be invoked", 100);

                                    runs(function() {
                                        expect(resolveNextHopSpy).toHaveBeenCalledWith(
                                                channelAddress);
                                    });
                                });

                        it(
                                "resolves a previously persisted browser address",
                                function() {
                                    var participantId = "participantId", browserAddress;
                                    var resolveNextHopSpy = jasmine.createSpy("resolveNextHopSpy");
                                    messageRouter =
                                            createRootMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy);

                                    browserAddress = new BrowserAddress({
                                        windowId : "windowId"
                                    });
                                    persistencySpy.setItem(messageRouter
                                            .getStorageKey(participantId), JSON
                                            .stringify(browserAddress));

                                    runs(function() {
                                        messageRouter.resolveNextHop(participantId).then(
                                                resolveNextHopSpy);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return resolveNextHopSpy.callCount > 0;
                                    }, "resolveNextHopSpy to be invoked", 100);

                                    runs(function() {
                                        expect(resolveNextHopSpy).toHaveBeenCalledWith(
                                                browserAddress);
                                    });
                                });

                        it(
                                "queue Message with unknown destinationParticipant",
                                function() {
                                    messageRouter =
                                            createRootMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy);
                                    joynrMessage2.expiryDate = Date.now() + 2000;

                                    var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                                    runs(function() {
                                        messageRouter.route(joynrMessage2).then(onFulfilledSpy);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return messageQueueSpy.putMessage.callCount > 0;
                                    }, "messageQueueSpy to be invoked", 100);

                                    runs(function() {
                                        expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(
                                                joynrMessage2);
                                        expect(messageQueueSpy.getAndRemoveMessages).not
                                                .toHaveBeenCalled();
                                        expect(messagingStubFactorySpy.createMessagingStub).not
                                                .toHaveBeenCalled();
                                        expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
                                    });
                                });

                        it(
                                "route previously queued message once respective participant gets registered",
                                function() {
                                    messageRouter =
                                            createRootMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy);
                                    var messageQueue = [];
                                    messageQueue[0] = joynrMessage2;
                                    joynrMessage2.expiryDate = Date.now() + 2000;

                                    var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                                    runs(function() {
                                        messageRouter.route(joynrMessage2).then(onFulfilledSpy);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return messageQueueSpy.putMessage.callCount > 0;
                                    }, "messageQueueSpy to be invoked the first time", 100);

                                    runs(function() {
                                        expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(
                                                joynrMessage2);
                                        expect(messagingStubFactorySpy.createMessagingStub).not
                                                .toHaveBeenCalled();
                                        expect(messagingStubSpy.transmit).not.toHaveBeenCalled();

                                        messageRouter.addNextHop(joynrMessage2.to, address);
                                        messageQueueSpy.getAndRemoveMessages
                                                .andReturn(messageQueue);
                                        messageRouter.participantRegistered(joynrMessage2.to);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return messageQueueSpy.getAndRemoveMessages.callCount > 0
                                                    && messagingStubFactorySpy.createMessagingStub.callCount > 0
                                                    && messagingStubSpy.transmit.callCount > 0;
                                            },
                                            "messageQueueSpy.getAndRemoveMessages spy to be invoked",
                                            100);

                                    runs(function() {
                                        expect(messageQueueSpy.getAndRemoveMessages)
                                                .toHaveBeenCalledWith(joynrMessage2.to);
                                        expect(messagingStubFactorySpy.createMessagingStub)
                                                .toHaveBeenCalledWith(address);
                                        expect(messagingStubSpy.transmit).toHaveBeenCalledWith(
                                                joynrMessage2);
                                        messageRouter.removeNextHop(joynrMessage2.to);
                                        increaseFakeTime(1);
                                    });
                                });

                        it(
                                "drop previously queued message if respective participant gets registered after expiry date",
                                function() {
                                    messageRouter =
                                            createRootMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy);
                                    joynrMessage2.expiryDate = Date.now() + 2000;

                                    runs(function() {
                                        var returnValue = messageRouter.route(joynrMessage2);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return messageQueueSpy.putMessage.callCount > 0;
                                    }, "messageQueueSpy.putMessage invoked", 100);

                                    runs(function() {
                                        expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(
                                                joynrMessage2);

                                        increaseFakeTime(2000 + 1);
                                        messageRouter.addNextHop(joynrMessage2.to, address);
                                        messageRouter.participantRegistered(joynrMessage2.to);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return messageQueueSpy.getAndRemoveMessages.callCount > 0;
                                    }, "messageQueueSpy.getAndRemoveMessages to be invoked", 100);

                                    runs(function() {
                                        expect(messageQueueSpy.getAndRemoveMessages)
                                                .toHaveBeenCalledWith(joynrMessage2.to);
                                        expect(messagingStubFactorySpy.createMessagingStub).not
                                                .toHaveBeenCalled();
                                        expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
                                        messageRouter.removeNextHop(joynrMessage2.to);
                                        increaseFakeTime(1);
                                    });
                                });

                        it(
                                "routes messages using the messagingStubFactory and messageStub",
                                function() {
                                    messageRouter =
                                            createRootMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy);

                                    runs(function() {
                                        messageRouter.addNextHop(joynrMessage.to, address);
                                        messageRouter.route(joynrMessage);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(
                                            function() {
                                                return messagingStubFactorySpy.createMessagingStub.callCount > 0
                                                    && messagingStubSpy.transmit.callCount > 0;
                                            },
                                            "messagingStubFactorySpy.createMessagingStub to be invoked",
                                            100);

                                    runs(function() {
                                        expect(messagingStubFactorySpy.createMessagingStub)
                                                .toHaveBeenCalledWith(address);
                                        expect(messagingStubSpy.transmit).toHaveBeenCalledWith(
                                                joynrMessage);
                                        messageRouter.removeNextHop(joynrMessage.to);
                                        increaseFakeTime(1);
                                    });
                                });

                        it("discards messages without resolvable address", function() {
                            messageRouter =
                                    createRootMessageRouter(
                                            persistencySpy,
                                            messagingStubFactorySpy,
                                            messageQueueSpy);

                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var onRejectedSpy = jasmine.createSpy("onRejectedSpy");

                            runs(function() {
                                messageRouter.route(joynrMessage).then(
                                        onFulfilledSpy).catch(onRejectedSpy);
                                increaseFakeTime(1);
                            });

                            waitsFor(function() {
                                return onFulfilledSpy.callCount > 0 || onRejectedSpy.callCount > 0;
                            }, "onFulfilled or onRejected spy to be invoked", 100);

                            runs(function() {
                                expect(messagingStubFactorySpy.createMessagingStub).not
                                        .toHaveBeenCalled();
                                expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
                            });
                        });

                        it(
                                "check if routing proxy is called with queued hop additions",
                                function() {
                                    messageRouter =
                                            createMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy,
                                                    incomingAddress,
                                                    parentMessageRouterAddress);
                                    routingProxySpy = jasmine.createSpyObj("routingProxySpy", [
                                        "removeNextHop",
                                        "addNextHop",
                                        "resolveNextHop"
                                    ]);
                                    routingProxySpy.addNextHop.andReturn(Promise.resolve());
                                    var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                                    runs(function() {
                                        messageRouter.addNextHop(joynrMessage.to, address).then(
                                                onFulfilledSpy);
                                        increaseFakeTime(1);

                                        expect(routingProxySpy.addNextHop).not.toHaveBeenCalled();
                                        messageRouter.setRoutingProxy(routingProxySpy);
                                        expect(routingProxySpy.addNextHop).toHaveBeenCalled();
                                        expect(
                                                routingProxySpy.addNextHop.calls[0].args[0].participantId)
                                                .toEqual(joynrMessage.to);
                                        expect(
                                                routingProxySpy.addNextHop.calls[0].args[0].browserAddress)
                                                .toEqual(incomingAddress);
                                        increaseFakeTime(1);
                                    });
                                });

                        it(
                                "check if routing proxy is called with multiple queued hop additions",
                                function() {
                                    messageRouter =
                                            createMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy,
                                                    incomingAddress,
                                                    parentMessageRouterAddress);
                                    routingProxySpy = jasmine.createSpyObj("routingProxySpy", [
                                        "removeNextHop",
                                        "addNextHop",
                                        "resolveNextHop"
                                    ]);

                                    var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                                    runs(function() {
                                        messageRouter.addNextHop(joynrMessage.to, address);
                                        messageRouter.addNextHop(joynrMessage2.to, address).then(
                                                onFulfilledSpy);
                                        routingProxySpy.addNextHop.andReturn(Promise.resolve());
                                        increaseFakeTime(1);
                                        expect(routingProxySpy.addNextHop).not.toHaveBeenCalled();
                                        messageRouter.setRoutingProxy(routingProxySpy);
                                        expect(routingProxySpy.addNextHop).toHaveBeenCalled();
                                        expect(
                                                routingProxySpy.addNextHop.calls[0].args[0].participantId)
                                                .toEqual(joynrMessage.to);
                                        expect(
                                                routingProxySpy.addNextHop.calls[0].args[0].browserAddress)
                                                .toEqual(incomingAddress);
                                        expect(
                                                routingProxySpy.addNextHop.calls[1].args[0].participantId)
                                                .toEqual(joynrMessage2.to);
                                        expect(
                                                routingProxySpy.addNextHop.calls[1].args[0].browserAddress)
                                                .toEqual(incomingAddress);
                                        increaseFakeTime(1);
                                    });
                                });

                        it(
                                "check if routing proxy is called with queued hop removals",
                                function() {
                                    messageRouter =
                                            createMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy,
                                                    incomingAddress,
                                                    parentMessageRouterAddress);
                                    routingProxySpy = jasmine.createSpyObj("routingProxySpy", [
                                        "removeNextHop",
                                        "addNextHop",
                                        "resolveNextHop"
                                    ]);

                                    var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                                    runs(function() {
                                        routingProxySpy.removeNextHop.andReturn(Promise.resolve());
                                        messageRouter.removeNextHop(joynrMessage.to).then(
                                                onFulfilledSpy);
                                        expect(onFulfilledSpy).not.toHaveBeenCalled();
                                        onFulfilledSpy.reset();
                                        expect(routingProxySpy.removeNextHop).not
                                                .toHaveBeenCalled();
                                        messageRouter.setRoutingProxy(routingProxySpy);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return routingProxySpy.removeNextHop.callCount > 0;
                                    }, "routingProxySpy.removeNextHop to be invoked", 100);

                                    runs(function() {
                                        expect(routingProxySpy.removeNextHop).toHaveBeenCalled();
                                        expect(
                                                routingProxySpy.removeNextHop.calls[0].args[0].participantId)
                                                .toEqual(joynrMessage.to);
                                    });
                                });

                        it(
                                "check if routing proxy is called with multiple queued hop removals",
                                function() {
                                    messageRouter =
                                            createMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy,
                                                    incomingAddress,
                                                    parentMessageRouterAddress);
                                    routingProxySpy = jasmine.createSpyObj("routingProxySpy", [
                                        "removeNextHop",
                                        "addNextHop",
                                        "resolveNextHop"
                                    ]);

                                    var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                                    runs(function() {
                                        routingProxySpy.removeNextHop.andReturn(Promise.resolve());
                                        messageRouter.removeNextHop(joynrMessage.to);
                                        messageRouter.removeNextHop(joynrMessage2.to).then(
                                                onFulfilledSpy);
                                        expect(onFulfilledSpy).not.toHaveBeenCalled();
                                        expect(routingProxySpy.removeNextHop).not
                                                .toHaveBeenCalled();
                                        messageRouter.setRoutingProxy(routingProxySpy);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return routingProxySpy.removeNextHop.callCount === 2;
                                    }, "routingProxySpy.removeNextHop to be invoked", 100);

                                    runs(function() {
                                        expect(routingProxySpy.removeNextHop).toHaveBeenCalled();
                                        expect(
                                                routingProxySpy.removeNextHop.calls[0].args[0].participantId)
                                                .toEqual(joynrMessage.to);
                                        expect(
                                                routingProxySpy.removeNextHop.calls[1].args[0].participantId)
                                                .toEqual(joynrMessage2.to);
                                    });
                                });

                        it(
                                "check if routing proxy is called with queued hop removals",
                                function() {
                                    var resolveNextHopSpy = jasmine.createSpy("resolveNextHopSpy");
                                    messageRouter =
                                            createMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy,
                                                    incomingAddress,
                                                    parentMessageRouterAddress);
                                    routingProxySpy = jasmine.createSpyObj("routingProxySpy", [
                                        "removeNextHop",
                                        "addNextHop",
                                        "resolveNextHop"
                                    ]);
                                    routingProxySpy.resolveNextHop.andReturn(Promise.resolve({
                                        resolved:true
                                    }));

                                    runs(function() {
                                        messageRouter.resolveNextHop(joynrMessage.to).then(
                                                resolveNextHopSpy);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return resolveNextHopSpy.callCount > 0;
                                    }, "resolveNextHop returned first time", 100);

                                    runs(function() {
                                        expect(resolveNextHopSpy).toHaveBeenCalledWith(undefined);
                                        resolveNextHopSpy.reset();
                                        messageRouter.setRoutingProxy(routingProxySpy);
                                        expect(routingProxySpy.resolveNextHop).not
                                                .toHaveBeenCalled();
                                        messageRouter.resolveNextHop(joynrMessage.to).then(
                                                resolveNextHopSpy);
                                        increaseFakeTime(1);
                                    });

                                    waitsFor(function() {
                                        return resolveNextHopSpy.callCount > 0;
                                    }, "resolveNextHop returned second time", 100);

                                    runs(function() {
                                        expect(routingProxySpy.resolveNextHop).toHaveBeenCalled();
                                        expect(
                                                routingProxySpy.resolveNextHop.calls[0].args[0].participantId)
                                                .toEqual(joynrMessage.to);
                                        expect(resolveNextHopSpy).toHaveBeenCalledWith(
                                                parentMessageRouterAddress);
                                    });
                                });
                    });
        });
