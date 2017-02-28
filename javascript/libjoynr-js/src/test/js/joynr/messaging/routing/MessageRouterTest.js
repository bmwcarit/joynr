/*jslint es5: true */
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

define([
            "joynr/messaging/routing/MessageRouter",
            "joynr/system/RoutingTypes/BrowserAddress",
            "joynr/system/RoutingTypes/ChannelAddress",
            "joynr/messaging/inprocess/InProcessAddress",
            "joynr/messaging/JoynrMessage",
            "joynr/start/TypeRegistry",
            "global/Promise",
            "Date",
            "global/WaitsFor",
            "uuid"
        ],
        function(
                MessageRouter,
                BrowserAddress,
                ChannelAddress,
                InProcessAddress,
                JoynrMessage,
                TypeRegistry,
                Promise,
                Date,
                waitsFor,
                uuid) {
            var fakeTime;

            function increaseFakeTime(time_ms) {
                fakeTime = fakeTime + time_ms;
                jasmine.clock().tick(time_ms);
            }
            describe(
                    "libjoynr-js.joynr.messaging.routing.MessageRouter",
                    function() {
                        var store, typeRegistry, receiverParticipantId, receiverParticipantId2, joynrMessage, joynrMessage2;
                        var myChannelId, persistencySpy, otherChannelId, resultObj, address;
                        var messagingStubSpy, messagingSkeletonSpy, messagingStubFactorySpy, messagingSkeletonFactorySpy;
                        var messageQueueSpy, messageRouter, routingProxySpy, parentMessageRouterAddress, incomingAddress;
                        var multicastAddressCalculatorSpy;
                        var testGlobalClusterControllerAddress;

                        var createMessageRouter =
                                function(
                                        persistency,
                                        messageQueue,
                                        incomingAddress,
                                        parentMessageRouterAddress) {
                                    return new MessageRouter({
                                        initialRoutingTable : [],
                                        persistency : persistency,
                                        joynrInstanceId : "joynrInstanceID",
                                        messagingStubFactory : messagingStubFactorySpy,
                                        messagingSkeletonFactory : messagingSkeletonFactorySpy,
                                        multicastAddressCalculator : multicastAddressCalculatorSpy,
                                        messageQueue : messageQueue,
                                        incomingAddress : incomingAddress,
                                        parentMessageRouterAddress : parentMessageRouterAddress,
                                        typeRegistry : typeRegistry
                                    });
                                };

                        var createRootMessageRouter =
                                function(persistency, messageQueue) {
                                    return createMessageRouter(
                                            persistency,
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
                            multicastAddressCalculatorSpy = jasmine.createSpyObj("multicastAddressCalculator", [ "calculate" ]);
                            multicastAddressCalculatorSpy.calculate.and.returnValue(new BrowserAddress({
                                windowId : "incomingAddress"
                            }));

                            messagingStubSpy = jasmine.createSpyObj("messagingStub", [ "transmit"
                            ]);
                            messagingSkeletonSpy = jasmine.createSpyObj("messagingSkeletonSpy", [
                                "registerMulticastSubscription",
                                "unregisterMulticastSubscription"
                            ]);

                            messagingStubSpy.transmit.and.returnValue(Promise.resolve({
                                myKey : "myValue"
                            }));
                            messagingStubFactorySpy =
                                    jasmine.createSpyObj(
                                            "messagingStubFactorySpy",
                                            [ "createMessagingStub"
                                            ]);

                            messagingSkeletonFactorySpy =
                                jasmine.createSpyObj(
                                        "messagingSkeletonFactorySpy",
                                        [ "getSkeleton"
                                        ]);

                            messagingStubFactorySpy.createMessagingStub.and.returnValue(messagingStubSpy);

                            messagingSkeletonFactorySpy.getSkeleton.and.returnValue(messagingSkeletonSpy);

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

                            testGlobalClusterControllerAddress = "testGlobalAddress";
                            routingProxySpy = jasmine.createSpyObj("routingProxySpy", [
                               "addNextHop",
                               "removeNextHop",
                               "resolveNextHop",
                               "addMulticastReceiver",
                               "removeMulticastReceiver"
                               ]);
                            routingProxySpy.globalAddress = {
                                get : null
                            };
                            spyOn(routingProxySpy.globalAddress, "get").and.returnValue(Promise.resolve(testGlobalClusterControllerAddress));

                            messageRouter =
                                createRootMessageRouter(
                                        persistencySpy,
                                        messageQueueSpy);

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
                                "queue Message with unknown destinationParticipant",
                                function(done) {
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


                        describe("route multicast messages", function() {
                            var parameters;
                            var multicastMessage;
                            var addressOfSubscriberParticipant;
                            beforeEach(function() {
                                parameters = {
                                    multicastId : "multicastId- " + uuid(),
                                    subscriberParticipantId : "subscriberParticipantId",
                                    providerParticipantId : "providerParticipantId"
                                };

                                multicastMessage = new JoynrMessage({
                                    type : JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST
                                });
                                multicastMessage.expiryDate = 9360686108031;
                                multicastMessage.to = parameters.multicastId;
                                multicastMessage.from = "senderParticipantId";
                                multicastMessage.payload = "hello";


                                /* add routing table entry for parameters.subscriberParticipantId,
                                 * otherwise messaging stub call can be executed by the message router
                                 */
                                addressOfSubscriberParticipant = new BrowserAddress({
                                    windowId : "windowIdOfSubscriberParticipant"
                                });
                                messageRouter.addNextHop(parameters.subscriberParticipantId, addressOfSubscriberParticipant);
                            });

                            it("never, if message is received from global and NO local receiver", function() {
                                multicastMessage.setReceivedFromGlobal(true);
                                messageRouter.route(multicastMessage);
                                expect(messagingStubSpy.transmit).not.toHaveBeenCalled();
                            });

                            it("once, if message is received from global and has local receiver", function() {
                                messageRouter.addMulticastReceiver(parameters);
                                multicastMessage.setReceivedFromGlobal(true);
                                messageRouter.route(multicastMessage);
                                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                                expect(messagingStubSpy.transmit.calls.count()).toBe(1);
                            });

                            it("once, if message is NOT received from global and NO local receiver", function() {
                                messageRouter.route(multicastMessage);
                                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                                expect(messagingStubSpy.transmit.calls.count()).toBe(1);
                            });

                            it("twice, if message is received from global and local receiver available", function() {
                                messageRouter.addMulticastReceiver(parameters);
                                messageRouter.route(multicastMessage);
                                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                                expect(messagingStubSpy.transmit.calls.count()).toBe(2);
                            });

                            it("twice, if message is received from global and two local receivers available with same receiver address", function() {
                                messageRouter.addMulticastReceiver(parameters);
                                var parametersForSndReceiver = {
                                    multicastId : parameters.multicastId,
                                    subscriberParticipantId : "subscriberParticipantId2",
                                    providerParticipantId : "providerParticipantId"
                                };

                                messageRouter.addMulticastReceiver(parametersForSndReceiver);
                                messageRouter.addNextHop(parametersForSndReceiver.subscriberParticipantId, addressOfSubscriberParticipant);
                                messageRouter.route(multicastMessage);
                                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                                expect(messagingStubSpy.transmit.calls.count()).toBe(2);
                            });

                            it("three times, if message is received from global and two local receivers available with different receiver address", function() {
                                messageRouter.addMulticastReceiver(parameters);
                                var parametersForSndReceiver = {
                                    multicastId : parameters.multicastId,
                                    subscriberParticipantId : "subscriberParticipantId2",
                                    providerParticipantId : "providerParticipantId"
                                };

                                messageRouter.addMulticastReceiver(parametersForSndReceiver);
                                messageRouter.addNextHop(parametersForSndReceiver.subscriberParticipantId, new BrowserAddress({
                                    windowId : "windowIdOfNewSubscribeParticipant"
                                }));
                                messageRouter.route(multicastMessage);
                                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                                expect(messagingStubSpy.transmit.calls.count()).toBe(3);
                            });
                        }); // describe route multicast messages


                        it(
                                "routes messages using the messagingStubFactory and messageStub",
                                function(done) {

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


                        describe("ChildMessageRouter", function() {
                            beforeEach(function() {
                                messageRouter =
                                    createMessageRouter(
                                            persistencySpy,
                                            messageQueueSpy,
                                            incomingAddress,
                                            parentMessageRouterAddress);
                            });

                            it("queries global address from routing provider", function(done) {
                                messageRouter.setRoutingProxy(routingProxySpy)
                                .then(function() {
                                    expect(routingProxySpy.globalAddress.get).toHaveBeenCalled();
                                    done();
                                }).catch(function(error) {
                                    done.fail(error);
                                });
                            });

                            it(
                                    "address can be resolved once known to message router",
                                    function(done) {
                                        var participantId = "participantId-setToKnown";

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

                            describe("addMulticastReceiver", function() {
                                var parameters;
                                beforeEach(function() {
                                    parameters = {
                                        multicastId : "multicastId- " + uuid(),
                                        subscriberParticipantId : "subscriberParticipantId",
                                        providerParticipantId : "providerParticipantId"
                                    };

                                    messageRouter.setToKnown(parameters.providerParticipantId);

                                    routingProxySpy.addMulticastReceiver.and.returnValue(Promise.resolve());

                                   expect(messageRouter.hasMulticastReceivers()).toBe(false);
                                });

                                it("calls matching skeleton", function() {
                                    messageRouter.addMulticastReceiver(parameters);

                                    expect(messagingSkeletonSpy.registerMulticastSubscription).toHaveBeenCalled();

                                    expect(messagingSkeletonSpy.registerMulticastSubscription).toHaveBeenCalledWith(parameters.multicastId);

                                    expect(messageRouter.hasMulticastReceivers()).toBe(true);
                                });

                                it("calls routing proxy if available", function() {
                                    messageRouter.setRoutingProxy(routingProxySpy);

                                    messageRouter.addMulticastReceiver(parameters);

                                    expect(routingProxySpy.addMulticastReceiver).toHaveBeenCalled();

                                    expect(routingProxySpy.addMulticastReceiver).toHaveBeenCalledWith(parameters);

                                    expect(messageRouter.hasMulticastReceivers()).toBe(true);
                                });

                                it("does not call routing proxy for in process provider", function() {
                                    messageRouter.setRoutingProxy(routingProxySpy);

                                    parameters.providerParticipantId = "inProcessParticipant";
                                    messageRouter.addNextHop(parameters.providerParticipantId, new InProcessAddress(undefined));
                                    messageRouter.addMulticastReceiver(parameters);

                                    expect(routingProxySpy.addMulticastReceiver).not.toHaveBeenCalled();

                                    expect(messageRouter.hasMulticastReceivers()).toBe(true);
                                });

                                it("queues calls and forwards them once proxy is available", function() {
                                    messageRouter.addMulticastReceiver(parameters);

                                    messageRouter.setRoutingProxy(routingProxySpy);

                                    expect(routingProxySpy.addMulticastReceiver).toHaveBeenCalled();

                                    expect(routingProxySpy.addMulticastReceiver).toHaveBeenCalledWith(parameters);

                                    expect(messageRouter.hasMulticastReceivers()).toBe(true);
                                });
                            }); // describe addMulticastReceiver

                            describe("removeMulticastReceiver", function() {
                                var parameters;
                                beforeEach(function() {
                                    parameters = {
                                        multicastId : "multicastId- " + uuid(),
                                        subscriberParticipantId : "subscriberParticipantId",
                                        providerParticipantId : "providerParticipantId"
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

                                it("calls matching skeleton registration and unregistration", function() {
                                    messageRouter.removeMulticastReceiver(parameters);

                                    expect(messagingSkeletonSpy.unregisterMulticastSubscription).toHaveBeenCalled();

                                    expect(messagingSkeletonSpy.unregisterMulticastSubscription).toHaveBeenCalledWith(parameters.multicastId);
                                    expect(messageRouter.hasMulticastReceivers()).toBe(false);
                                });

                                it("calls routing proxy if available", function() {
                                    messageRouter.setRoutingProxy(routingProxySpy);

                                    messageRouter.removeMulticastReceiver(parameters);

                                    expect(routingProxySpy.removeMulticastReceiver).toHaveBeenCalled();

                                    expect(routingProxySpy.removeMulticastReceiver).toHaveBeenCalledWith(parameters);

                                    expect(messageRouter.hasMulticastReceivers()).toBe(false);
                                });

                                it("queues calls and forwards them once proxy is available", function() {
                                    messageRouter.removeMulticastReceiver(parameters);

                                    messageRouter.setRoutingProxy(routingProxySpy);

                                    expect(routingProxySpy.removeMulticastReceiver).toHaveBeenCalled();

                                    expect(routingProxySpy.removeMulticastReceiver).toHaveBeenCalledWith(parameters);

                                    expect(messageRouter.hasMulticastReceivers()).toBe(false);
                                });
                            }); // describe removeMulticastReceiver

                            it(
                                    "check if routing proxy is called with queued hop additions",
                                    function(done) {
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
                        }); // describe ChildMessageRouter

                    }); // describe MessageRouter
        }); // define
