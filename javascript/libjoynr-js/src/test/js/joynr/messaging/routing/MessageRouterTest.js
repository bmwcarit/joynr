/*jslint es5: true */
/*global fail: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
            "Date",
            "global/WaitsFor"
        ],
        function(
                MessageRouter,
                BrowserAddress,
                ChannelAddress,
                JoynrMessage,
                TypeRegistry,
                Promise,
                Date,
                waitsFor) {
            var fakeTime;

            function increaseFakeTime(time_ms) {
                fakeTime = fakeTime + time_ms;
                jasmine.clock().tick(time_ms);
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

                        beforeEach(function(done) {
                            incomingAddress = new BrowserAddress({
                                windowId : "incomingAddress"
                            });
                            parentMessageRouterAddress = new BrowserAddress({
                                windowId : "parentMessageRouterAddress"
                            });
                            receiverParticipantId = "TestMessageRouter_participantId_" + Date.now();
                            receiverParticipantId2 =
                                    "TestMessageRouter_delayedParticipantId_" + Date.now();
                            joynrMessage = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
                            });
                            joynrMessage.expiryDate = 9360686108031;
                            joynrMessage.to = receiverParticipantId;
                            joynrMessage.from = "senderParticipantId";
                            joynrMessage.payload = "hello";

                            joynrMessage2 = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
                            });
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
                            messagingStubSpy.transmit.and.returnValue(Promise.resolve({
                                myKey : "myValue"
                            }));
                            messagingStubFactorySpy =
                                    jasmine.createSpyObj(
                                            "messagingStubFactorySpy",
                                            [ "createMessagingStub"
                                            ]);
                            messagingStubFactorySpy.createMessagingStub.and.returnValue(messagingStubSpy);

                            messageQueueSpy = jasmine.createSpyObj("messageQueueSpy", [
                                "putMessage",
                                "getAndRemoveMessages",
                                "shutdown"
                            ]);

                            store = {};
                            persistencySpy = jasmine.createSpyObj("persistencySpy", [
                                "setItem",
                                "removeItem",
                                "getItem"
                            ]);
                            persistencySpy.setItem.and.callFake(function(key, value) {
                                store[key] = value;
                            });
                            persistencySpy.getItem.and.callFake(function(key) {
                                return store[key];
                            });

                            fakeTime = Date.now();
                            jasmine.clock().install();
                            spyOn(Date, "now").and.callFake(function() {
                                return fakeTime;
                            });

                            typeRegistry = new TypeRegistry();
                            typeRegistry.addType("joynr.system.RoutingTypes.ChannelAddress", ChannelAddress);
                            typeRegistry.addType("joynr.system.RoutingTypes.BrowserAddress", BrowserAddress);
                            done();
                        });

                        afterEach(function(done) {
                            jasmine.clock().uninstall();
                            done();
                          });

                        it(
                                "resolves a previously persisted channel address",
                                function(done) {
                                    var participantId = "participantId", channelAddress;
                                    messageRouter =
                                            createRootMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy);

                                    channelAddress = new ChannelAddress({
                                        messagingEndpointUrl : "http://testurl.com",
                                        channelId : "channelId"
                                    });
                                    persistencySpy.setItem(messageRouter
                                            .getStorageKey(participantId), JSON
                                            .stringify(channelAddress));

                                    messageRouter.resolveNextHop(participantId).then(function(returnedAddress) {
                                        expect(returnedAddress).toEqual(channelAddress);
                                        done();
                                        return null;
                                    }).catch(function(error) {
                                        fail("got reject from resolveNextHop: " + error);
                                        return null;
                                    });
                                    increaseFakeTime(1);
                                });

                        it(
                                "resolves a previously persisted browser address",
                                function(done) {
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

                                    messageRouter.resolveNextHop(participantId).then(function(returnedAddress) {
                                        expect(returnedAddress).toEqual(browserAddress);
                                        done();
                                        return null;
                                    }).catch(function(error) {
                                        fail("got reject from resolveNextHop: " + error);
                                        return null;
                                    });
                                    increaseFakeTime(1);
                                });

                        it(
                                "address can be resolved once known to message router",
                                function(done) {
                                    var participantId = "participantId-setToKnown";
                                    messageRouter =
                                            createMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy,
                                                    incomingAddress,
                                                    parentMessageRouterAddress);

                                    messageRouter.resolveNextHop(participantId)
                                        .then(function(address) {
                                            expect(address).toBe(undefined);
                                            // it is expected that the given participantId cannot be resolved
                                            messageRouter.setToKnown(participantId);
                                            return messageRouter.resolveNextHop(participantId).then(function(address) {
                                                expect(address).toBe(parentMessageRouterAddress);
                                                return done();
                                            }).catch(done.fail);
                                        });
                                });

                        it(
                                "queue Message with unknown destinationParticipant",
                                function(done) {
                                    messageRouter =
                                            createRootMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy);
                                    joynrMessage2.expiryDate = Date.now() + 2000;

                                    var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                                    // avoid unhandled rejection warning by providing catch block
                                    messageRouter.route(joynrMessage2).then(onFulfilledSpy).catch(function() {
                                        return null;
                                    });
                                    increaseFakeTime(1);

                                    waitsFor(function() {
                                        return messageQueueSpy.putMessage.calls.count() > 0;
                                    }, "messageQueueSpy to be invoked", 1000).then(function() {
                                        expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(
                                                joynrMessage2);
                                        expect(messageQueueSpy.getAndRemoveMessages).not
                                                .toHaveBeenCalled();
                                        expect(messagingStubFactorySpy.createMessagingStub).not
                                                .toHaveBeenCalled();
                                        expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "routes previously queued message once respective participant gets registered",
                                function(done) {
                                    messageRouter =
                                            createRootMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy);
                                    var messageQueue = [];
                                    messageQueue[0] = joynrMessage2;
                                    joynrMessage2.expiryDate = Date.now() + 2000;

                                    var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                                    messageRouter.route(joynrMessage2).then(onFulfilledSpy).catch(function() {});
                                    increaseFakeTime(1);

                                    waitsFor(function() {
                                        return messageQueueSpy.putMessage.calls.count() > 0;
                                    }, "messageQueueSpy to be invoked the first time", 1000).then(function() {
                                        expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(
                                                joynrMessage2);
                                        expect(messagingStubFactorySpy.createMessagingStub).not
                                                .toHaveBeenCalled();
                                        expect(messagingStubSpy.transmit).not.toHaveBeenCalled();

                                        messageRouter.addNextHop(joynrMessage2.to, address).catch(function() {});
                                        messageQueueSpy.getAndRemoveMessages
                                                .and.returnValue(messageQueue);
                                        messageRouter.participantRegistered(joynrMessage2.to);
                                        increaseFakeTime(1);

                                        return(waitsFor(
                                            function() {
                                                return messageQueueSpy.getAndRemoveMessages.calls.count() > 0
                                                    && messagingStubFactorySpy.createMessagingStub.calls.count() > 0
                                                    && messagingStubSpy.transmit.calls.count() > 0;
                                            },
                                            "messageQueueSpy.getAndRemoveMessages spy to be invoked",
                                            1000));
                                    }).then(function() {
                                        expect(messageQueueSpy.getAndRemoveMessages)
                                            .toHaveBeenCalledWith(joynrMessage2.to);
                                        expect(messagingStubFactorySpy.createMessagingStub)
                                            .toHaveBeenCalledWith(address);
                                        expect(messagingStubSpy.transmit).toHaveBeenCalledWith(
                                            joynrMessage2);
                                        messageRouter.removeNextHop(joynrMessage2.to);
                                        increaseFakeTime(1);
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "drop previously queued message if respective participant gets registered after expiry date",
                                function(done) {
                                    messageRouter =
                                            createRootMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy);
                                    joynrMessage2.expiryDate = Date.now() + 2000;

                                    var returnValue = messageRouter.route(joynrMessage2);
                                    returnValue.catch(function() {});
                                    increaseFakeTime(1);

                                    waitsFor(function() {
                                        return messageQueueSpy.putMessage.calls.count() > 0;
                                    }, "messageQueueSpy.putMessage invoked", 1000).then(function() {
                                        expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(
                                                joynrMessage2);

                                        increaseFakeTime(2000 + 1);
                                        messageRouter.addNextHop(joynrMessage2.to, address);
                                        messageRouter.participantRegistered(joynrMessage2.to);
                                        increaseFakeTime(1);

                                        return(waitsFor(function() {
                                                return messageQueueSpy.getAndRemoveMessages.calls.count() > 0;
                                        }, "messageQueueSpy.getAndRemoveMessages to be invoked", 1000));
                                    }).then(function() {
                                        expect(messageQueueSpy.getAndRemoveMessages)
                                                .toHaveBeenCalledWith(joynrMessage2.to);
                                        expect(messagingStubFactorySpy.createMessagingStub).not
                                                .toHaveBeenCalled();
                                        expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
                                        messageRouter.removeNextHop(joynrMessage2.to);
                                        increaseFakeTime(1);
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "routes messages using the messagingStubFactory and messageStub",
                                function(done) {
                                    messageRouter =
                                            createRootMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy);

                                    messageRouter.addNextHop(joynrMessage.to, address);
                                    messageRouter.route(joynrMessage);
                                    increaseFakeTime(1);

                                    waitsFor(
                                            function() {
                                                return messagingStubFactorySpy.createMessagingStub.calls.count() > 0
                                                    && messagingStubSpy.transmit.calls.count() > 0;
                                            },
                                            "messagingStubFactorySpy.createMessagingStub to be invoked",
                                    1000).then(function() {
                                        expect(messagingStubFactorySpy.createMessagingStub)
                                                .toHaveBeenCalledWith(address);
                                        expect(messagingStubSpy.transmit).toHaveBeenCalledWith(
                                                joynrMessage);
                                        messageRouter.removeNextHop(joynrMessage.to);
                                        increaseFakeTime(1);
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it("discards messages without resolvable address", function(done) {
                            messageRouter =
                                    createRootMessageRouter(
                                            persistencySpy,
                                            messagingStubFactorySpy,
                                            messageQueueSpy);

                            var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");
                            var onRejectedSpy = jasmine.createSpy("onRejectedSpy");

                            messageRouter.route(joynrMessage).then(
                                onFulfilledSpy).catch(onRejectedSpy);
                            increaseFakeTime(1);

                            waitsFor(function() {
                                return onFulfilledSpy.calls.count() > 0 || onRejectedSpy.calls.count() > 0;
                            }, "onFulfilled or onRejected spy to be invoked", 1000).then(function() {
                                expect(messagingStubFactorySpy.createMessagingStub).not
                                        .toHaveBeenCalled();
                                expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "check if routing proxy is called with queued hop additions",
                                function(done) {
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
                                    routingProxySpy.addNextHop.and.returnValue(Promise.resolve());
                                    var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                                    messageRouter.addNextHop(joynrMessage.to, address).then(
                                            onFulfilledSpy);
                                    increaseFakeTime(1);

                                    expect(routingProxySpy.addNextHop).not.toHaveBeenCalled();
                                    messageRouter.setRoutingProxy(routingProxySpy);
                                    expect(routingProxySpy.addNextHop).toHaveBeenCalled();
                                    expect(
                                            routingProxySpy.addNextHop.calls.argsFor(0)[0].participantId)
                                            .toEqual(joynrMessage.to);
                                    expect(
                                            routingProxySpy.addNextHop.calls.argsFor(0)[0].browserAddress)
                                            .toEqual(incomingAddress);
                                    increaseFakeTime(1);
                                    done();
                                });

                        it(
                                "check if resolved hop from routing proxy is cached",
                                function(done) {
                                    messageRouter =
                                            createMessageRouter(
                                                    persistencySpy,
                                                    messagingStubFactorySpy,
                                                    messageQueueSpy,
                                                    incomingAddress,
                                                    parentMessageRouterAddress);
                                    routingProxySpy = jasmine.createSpyObj("routingProxySpy", [
                                        "resolveNextHop"
                                    ]);
                                    routingProxySpy.resolveNextHop.and.returnValue(Promise.resolve({ resolved: true }));
                                    messageRouter.setRoutingProxy(routingProxySpy);

                                    messageRouter.resolveNextHop(joynrMessage.to).then(function(address) {
                                        expect(address).toBe(parentMessageRouterAddress);
                                        expect(routingProxySpy.resolveNextHop.calls.count()).toBe(1);
                                        routingProxySpy.resolveNextHop.calls.reset();
                                        return messageRouter.resolveNextHop(joynrMessage.to);
                                    }).then(function(address){
                                        expect(address).toBe(parentMessageRouterAddress);
                                        expect(routingProxySpy.resolveNextHop).not.toHaveBeenCalled();
                                        done();
                                        return null;
                                    }).catch(done.fail);
                                });

                        it(
                                "check if routing proxy is called with multiple queued hop additions",
                                function(done) {
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

                                    messageRouter.addNextHop(joynrMessage.to, address);
                                    messageRouter.addNextHop(joynrMessage2.to, address).then(
                                            onFulfilledSpy);
                                    routingProxySpy.addNextHop.and.returnValue(Promise.resolve());
                                    increaseFakeTime(1);
                                    expect(routingProxySpy.addNextHop).not.toHaveBeenCalled();
                                    messageRouter.setRoutingProxy(routingProxySpy);
                                    expect(routingProxySpy.addNextHop).toHaveBeenCalled();
                                    expect(
                                            routingProxySpy.addNextHop.calls.argsFor(0)[0].participantId)
                                            .toEqual(joynrMessage.to);
                                    expect(
                                            routingProxySpy.addNextHop.calls.argsFor(0)[0].browserAddress)
                                            .toEqual(incomingAddress);
                                    expect(
                                            routingProxySpy.addNextHop.calls.argsFor(1)[0].participantId)
                                            .toEqual(joynrMessage2.to);
                                    expect(
                                            routingProxySpy.addNextHop.calls.argsFor(1)[0].browserAddress)
                                            .toEqual(incomingAddress);
                                    increaseFakeTime(1);
                                    done();
                                });

                        it(
                                "check if routing proxy is called with queued hop removals",
                                function(done) {
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

                                    routingProxySpy.removeNextHop.and.returnValue(Promise.resolve());
                                    messageRouter.removeNextHop(joynrMessage.to).then(
                                            onFulfilledSpy);
                                    expect(onFulfilledSpy).not.toHaveBeenCalled();
                                    onFulfilledSpy.calls.reset();
                                    expect(routingProxySpy.removeNextHop).not
                                            .toHaveBeenCalled();
                                    messageRouter.setRoutingProxy(routingProxySpy);
                                    increaseFakeTime(1);

                                    waitsFor(function() {
                                        return routingProxySpy.removeNextHop.calls.count() > 0;
                                    }, "routingProxySpy.removeNextHop to be invoked", 1000).then(function() {
                                        expect(routingProxySpy.removeNextHop).toHaveBeenCalled();
                                        expect(
                                                routingProxySpy.removeNextHop.calls.argsFor(0)[0].participantId)
                                                .toEqual(joynrMessage.to);
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "check if routing proxy is called with multiple queued hop removals",
                                function(done) {
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

                                    routingProxySpy.removeNextHop.and.returnValue(Promise.resolve());
                                    messageRouter.removeNextHop(joynrMessage.to);
                                    messageRouter.removeNextHop(joynrMessage2.to).then(
                                            onFulfilledSpy);
                                    expect(onFulfilledSpy).not.toHaveBeenCalled();
                                    expect(routingProxySpy.removeNextHop).not
                                            .toHaveBeenCalled();
                                    messageRouter.setRoutingProxy(routingProxySpy);
                                    increaseFakeTime(1);

                                    waitsFor(function() {
                                        return routingProxySpy.removeNextHop.calls.count() === 2;
                                    }, "routingProxySpy.removeNextHop to be invoked", 1000).then(function() {
                                        expect(routingProxySpy.removeNextHop).toHaveBeenCalled();
                                        expect(
                                                routingProxySpy.removeNextHop.calls.argsFor(0)[0].participantId)
                                                .toEqual(joynrMessage.to);
                                        expect(
                                                routingProxySpy.removeNextHop.calls.argsFor(1)[0].participantId)
                                                .toEqual(joynrMessage2.to);
                                        done();
                                        return null;
                                    }).catch(fail);
                                });

                        it(
                                "check if routing proxy is called with queued hop removals",
                                function(done) {
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
                                    routingProxySpy.resolveNextHop.and.returnValue(Promise.resolve({
                                        resolved:true
                                    }));

                                    messageRouter.resolveNextHop(joynrMessage.to).then(
                                        resolveNextHopSpy);
                                    increaseFakeTime(1);

                                    waitsFor(function() {
                                        return resolveNextHopSpy.calls.count() > 0;
                                    }, "resolveNextHop returned first time", 1000).then(function() {
                                        expect(resolveNextHopSpy).toHaveBeenCalledWith(undefined);
                                        resolveNextHopSpy.calls.reset();
                                        messageRouter.setRoutingProxy(routingProxySpy);
                                        expect(routingProxySpy.resolveNextHop).not
                                                .toHaveBeenCalled();
                                        messageRouter.resolveNextHop(joynrMessage.to).then(
                                                resolveNextHopSpy);
                                        increaseFakeTime(1);

                                        return waitsFor(function() {
                                            return resolveNextHopSpy.calls.count() > 0;
                                        }, "resolveNextHop returned second time", 1000);
                                    }).then(function() {
                                        expect(routingProxySpy.resolveNextHop).toHaveBeenCalled();
                                        expect(
                                                routingProxySpy.resolveNextHop.calls.argsFor(0)[0].participantId)
                                                .toEqual(joynrMessage.to);
                                        expect(resolveNextHopSpy).toHaveBeenCalledWith(
                                                parentMessageRouterAddress);
                                        done();
                                        return null;
                                    }).catch(fail);
                                });
                        it(
                                " throws exception when called while shut down",
                                function(done) {
                                    messageRouter =
                                        createMessageRouter(
                                                persistencySpy,
                                                messagingStubFactorySpy,
                                                messageQueueSpy,
                                                incomingAddress,
                                                parentMessageRouterAddress);

                                    messageRouter.shutdown();

                                    expect(messageQueueSpy.shutdown).toHaveBeenCalled();
                                    messageRouter.removeNextHop("hopId").then(fail).catch(function() {
                                        return messageRouter.resolveNextHop("hopId").then(fail);
                                    }).catch(function() {
                                        return messageRouter.addNextHop("hopId", {}).then(fail);
                                    }).catch(done);
                                });

                        it(
                                " reject pending promises when shut down",
                                function(done) {
                                    messageRouter =
                                        createMessageRouter(
                                                persistencySpy,
                                                messagingStubFactorySpy,
                                                messageQueueSpy,
                                                incomingAddress,
                                                parentMessageRouterAddress);

                                    var addNextHopPromise = messageRouter.addNextHop(joynrMessage.to, address).then(fail);
                                    var removeNextHopPromise = messageRouter.removeNextHop(joynrMessage.to).then(fail);
                                    increaseFakeTime(1);

                                    messageRouter.shutdown();
                                    addNextHopPromise.catch(function() {
                                       return removeNextHopPromise.catch(function() {
                                           done();
                                       });
                                    });
                                });
                    });
        });
