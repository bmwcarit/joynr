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
var MessageRouter = require('../../../../classes/joynr/messaging/routing/MessageRouter');
var BrowserAddress = require('../../../../classes/joynr/system/RoutingTypes/BrowserAddress');
var ChannelAddress = require('../../../../classes/joynr/system/RoutingTypes/ChannelAddress');
var InProcessAddress = require('../../../../classes/joynr/messaging/inprocess/InProcessAddress');
var JoynrMessage = require('../../../../classes/joynr/messaging/JoynrMessage');
var TypeRegistry = require('../../../../classes/joynr/start/TypeRegistry');
var Promise = require('../../../../classes/global/Promise');
var Date = require('../../../../test-classes/global/Date');
var waitsFor = require('../../../../test-classes/global/WaitsFor');
var Util = require('../../../../classes/joynr/util/UtilInternal');
var uuid = require('../../../../classes/lib/uuid-annotated');
            var fakeTime;

            function increaseFakeTime(time_ms) {
                fakeTime = fakeTime + time_ms;
                jasmine.clock().tick(time_ms);
            }
            describe(
                    "libjoynr-js.joynr.messaging.routing.MessageRouter",
                    function() {
                        var store, typeRegistry;
                        var senderParticipantId, receiverParticipantId, receiverParticipantId2;
                        var joynrMessage, joynrMessage2;
                        var myChannelId, persistencySpy, otherChannelId, resultObj, address;
                        var messagingStubSpy, messagingSkeletonSpy, messagingStubFactorySpy, messagingSkeletonFactorySpy;
                        var messageQueueSpy, messageRouter, routingProxySpy, parentMessageRouterAddress, incomingAddress;
                        var multicastAddressCalculatorSpy;
                        var serializedTestGlobalClusterControllerAddress;
                        var multicastAddress;

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
                            senderParticipantId = "testSenderParticipantId_" + Date.now();
                            receiverParticipantId = "TestMessageRouter_participantId_" + Date.now();
                            receiverParticipantId2 =
                                    "TestMessageRouter_delayedParticipantId_" + Date.now();
                            joynrMessage = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
                            });
                            joynrMessage.expiryDate = 9360686108031;
                            joynrMessage.to = receiverParticipantId;
                            joynrMessage.from = senderParticipantId;
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
                            multicastAddress = new BrowserAddress({ windowId : "incomingAddress" });
                            multicastAddressCalculatorSpy.calculate.and.returnValue(multicastAddress);

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

                            serializedTestGlobalClusterControllerAddress = "testGlobalAddress";
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
                            spyOn(routingProxySpy.globalAddress, "get").and.returnValue(Promise.resolve(serializedTestGlobalClusterControllerAddress));

                            routingProxySpy.replyToAddress = {
                                get : null
                            };
                            spyOn(routingProxySpy.replyToAddress, "get").and.returnValue(Promise.resolve(serializedTestGlobalClusterControllerAddress));

                            messageRouter =
                                createRootMessageRouter(
                                        persistencySpy,
                                        messageQueueSpy);
                            messageRouter.setReplyToAddress(serializedTestGlobalClusterControllerAddress);

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

                                        var isGloballyVisible = true;
                                        messageRouter.addNextHop(joynrMessage2.to, address, isGloballyVisible).catch(function() {});
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
                                    var messageQueue = [];
                                    messageQueue[0] = joynrMessage2;

                                    var returnValue = messageRouter.route(joynrMessage2);
                                    returnValue.catch(function() {});
                                    increaseFakeTime(1);

                                    waitsFor(function() {
                                        return messageQueueSpy.putMessage.calls.count() > 0;
                                    }, "messageQueueSpy.putMessage invoked", 1000).then(function() {
                                        expect(messageQueueSpy.putMessage).toHaveBeenCalledTimes(1);
                                        expect(messageQueueSpy.putMessage).toHaveBeenCalledWith(
                                                joynrMessage2);
                                        expect(messageQueueSpy.getAndRemoveMessages).not.toHaveBeenCalled();

                                        messageQueueSpy.getAndRemoveMessages
                                                .and.returnValue(messageQueue);
                                        increaseFakeTime(2000 + 1);
                                        var isGloballyVisible = true;
                                        messageRouter.addNextHop(joynrMessage2.to, address, isGloballyVisible);
                                        increaseFakeTime(1);

                                        return(waitsFor(function() {
                                                return messageQueueSpy.getAndRemoveMessages.calls.count() > 0;
                                        }, "messageQueueSpy.getAndRemoveMessages to be invoked", 1000));
                                    }).then(function() {
                                        expect(messageQueueSpy.putMessage).toHaveBeenCalledTimes(1);
                                        expect(messageQueueSpy.getAndRemoveMessages).toHaveBeenCalledTimes(1);
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

                        it("route drops expired message", function(done) {
                            joynrMessage.expiryDate = Date.now() - 1;
                            try {
                                messageRouter.route(joynrMessage);
                                done.fail("did not throw");
                            } catch (e) {
                                expect(e.detailMessage.indexOf("expired message") >= 0).toBe(true);
                            }
                            expect(messagingStubFactorySpy.createMessagingStub).not
                                    .toHaveBeenCalled();
                            done();
                        });

                        it("sets replyTo address for non local messages", function(done) {
                            var isGloballyVisible = true;
                            messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

                            joynrMessage.setIsLocalMessage(false);
                            expect(joynrMessage.replyChannelId).toEqual(undefined);

                            messageRouter.route(joynrMessage);

                            expect(messagingStubSpy.transmit).toHaveBeenCalled();
                            var transmittedJoynrMessage = messagingStubSpy.transmit.calls.argsFor(0)[0];
                            expect(transmittedJoynrMessage.replyChannelId).toEqual(serializedTestGlobalClusterControllerAddress);
                            done();
                        });

                        it("does not set replyTo address for local messages", function(done) {
                            var isGloballyVisible = false;
                            messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

                            joynrMessage.setIsLocalMessage(true);
                            expect(joynrMessage.replyChannelId).toEqual(undefined);

                            messageRouter.route(joynrMessage);

                            expect(messagingStubSpy.transmit).toHaveBeenCalled();
                            var transmittedJoynrMessage = messagingStubSpy.transmit.calls.argsFor(0)[0];
                            expect(transmittedJoynrMessage.replyChannelId).toEqual(undefined);
                            done();
                        });

                        function routeMessageWithValidReplyToAddressCallsAddNextHop() {
                            messageRouter.addNextHop.calls.reset();
                            expect(messageRouter.addNextHop).not.toHaveBeenCalled();
                            var channelId = "testChannelId_" + Date.now();
                            var channelAddress = new ChannelAddress({
                                messagingEndpointUrl : "http://testurl.com",
                                channelId : channelId
                            });
                            joynrMessage.replyChannelId = JSON.stringify(channelAddress);

                            messageRouter.route(joynrMessage);

                            expect(messageRouter.addNextHop).toHaveBeenCalledTimes(1);
                            expect(messageRouter.addNextHop.calls.argsFor(0)[0])
                                    .toBe(senderParticipantId);
                            expect(messageRouter.addNextHop.calls.argsFor(0)[1].channelId)
                                    .toBe(channelId);
                            expect(messageRouter.addNextHop.calls.argsFor(0)[2]).toBe(true);
                        }

                        it("route calls addNextHop for request messages received from global", function(done) {
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
                            var channelId = "testChannelId_" + Date.now();
                            var channelAddress = new ChannelAddress({
                                messagingEndpointUrl : "http://testurl.com",
                                channelId : channelId
                            });
                            joynrMessage.replyChannelId = JSON.stringify(channelAddress);

                            messageRouter.route(joynrMessage);

                            expect(messageRouter.addNextHop).not.toHaveBeenCalled();
                        }

                        it("route does NOT call addNextHop for request messages NOT received from global", function(done) {
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

                        it("route does NOT call addNextHop for non request messages received from global", function(done) {
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

                        it("route does NOT call addNextHop for request messages received from global without replyTo address", function(done) {
                            spyOn(messageRouter, "addNextHop");
                            joynrMessage.isReceivedFromGlobal = true;

                            joynrMessage.type = JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST;

                            messageRouter.route(joynrMessage);

                            expect(messageRouter.addNextHop).not.toHaveBeenCalled();

                            done();
                        });

                        describe("route multicast messages", function() {
                            var parameters;
                            var multicastMessage;
                            var addressOfSubscriberParticipant;
                            var isGloballyVisible;
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
                                isGloballyVisible = true;
                                messageRouter.addNextHop(parameters.subscriberParticipantId, addressOfSubscriberParticipant, isGloballyVisible);

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
                                expect(messagingStubFactorySpy.createMessagingStub).toHaveBeenCalled();
                                expect(messagingStubFactorySpy.createMessagingStub.calls.count()).toEqual(1);
                                var address = messagingStubFactorySpy.createMessagingStub.calls.argsFor(0)[0];
                                expect(address).toEqual(multicastAddress);
                                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                                expect(messagingStubSpy.transmit.calls.count()).toBe(1);
                            });

                            it("twice, if message is NOT received from global and local receiver available", function() {
                                messageRouter.addMulticastReceiver(parameters);
                                messageRouter.route(multicastMessage);
                                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                                expect(messagingStubSpy.transmit.calls.count()).toBe(2);
                            });

                            it("twice, if message is NOT received from global and two local receivers available with same receiver address", function() {
                                messageRouter.addMulticastReceiver(parameters);
                                var parametersForSndReceiver = {
                                    multicastId : parameters.multicastId,
                                    subscriberParticipantId : "subscriberParticipantId2",
                                    providerParticipantId : "providerParticipantId"
                                };

                                messageRouter.addMulticastReceiver(parametersForSndReceiver);
                                messageRouter.addNextHop(parametersForSndReceiver.subscriberParticipantId, addressOfSubscriberParticipant, isGloballyVisible);
                                messageRouter.route(multicastMessage);
                                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                                expect(messagingStubSpy.transmit.calls.count()).toBe(2);
                            });

                            it("three times, if message is NOT received from global and two local receivers available with different receiver address", function() {
                                messageRouter.addMulticastReceiver(parameters);
                                var parametersForSndReceiver = {
                                    multicastId : parameters.multicastId,
                                    subscriberParticipantId : "subscriberParticipantId2",
                                    providerParticipantId : "providerParticipantId"
                                };

                                messageRouter.addMulticastReceiver(parametersForSndReceiver);
                                messageRouter.addNextHop(parametersForSndReceiver.subscriberParticipantId, new BrowserAddress({
                                    windowId : "windowIdOfNewSubscribeParticipant"
                                }), isGloballyVisible);
                                messageRouter.route(multicastMessage);
                                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                                expect(messagingStubSpy.transmit.calls.count()).toBe(3);
                            });
                        }); // describe route multicast messages


                        it(
                                "routes messages using the messagingStubFactory and messageStub",
                                function(done) {
                                    var isGloballyVisible = true;
                                    messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);
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
                                messageRouter.setReplyToAddress(serializedTestGlobalClusterControllerAddress);
                            });

                            it("queries global address from routing provider", function(done) {
                                messageRouter =
                                    createMessageRouter(
                                            persistencySpy,
                                            messageQueueSpy,
                                            incomingAddress,
                                            parentMessageRouterAddress);
                                messageRouter.setRoutingProxy(routingProxySpy)
                                .then(function() {
                                    expect(routingProxySpy.replyToAddress.get).toHaveBeenCalled();
                                    done();
                                }).catch(function(error) {
                                    done.fail(error);
                                });
                            });

                            it("sets replyTo address for non local messages", function(done) {
                                var isGloballyVisible = true;
                                messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

                                joynrMessage.setIsLocalMessage(false);
                                expect(joynrMessage.replyChannelId).toEqual(undefined);

                                messageRouter.route(joynrMessage);

                                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                                var transmittedJoynrMessage = messagingStubSpy.transmit.calls.argsFor(0)[0];
                                expect(transmittedJoynrMessage.replyChannelId).toEqual(serializedTestGlobalClusterControllerAddress);
                                done();
                            });

                            it("does not set replyTo address for local messages", function(done) {
                                var isGloballyVisible = true;
                                messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

                                joynrMessage.setIsLocalMessage(true);
                                expect(joynrMessage.replyChannelId).toEqual(undefined);

                                messageRouter.route(joynrMessage);

                                expect(messagingStubSpy.transmit).toHaveBeenCalled();
                                var transmittedJoynrMessage = messagingStubSpy.transmit.calls.argsFor(0)[0];
                                expect(transmittedJoynrMessage.replyChannelId).toEqual(undefined);
                                done();
                            });

                            it("queues non local messages until global address is available", function(done) {
                                var isGloballyVisible = true;
                                messageRouter =
                                    createMessageRouter(
                                            persistencySpy,
                                            messageQueueSpy,
                                            incomingAddress,
                                            parentMessageRouterAddress);
                                routingProxySpy.addNextHop.and.returnValue(Promise.resolve());
                                messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);

                                joynrMessage.setIsLocalMessage(false);
                                var expectedJoynrMessage = new JoynrMessage(Util.extendDeep({}, joynrMessage));
                                expectedJoynrMessage.replyChannelId = serializedTestGlobalClusterControllerAddress;

                                messageRouter.route(joynrMessage)
                                .then(function() {
                                    expect(messagingStubSpy.transmit).not.toHaveBeenCalled();

                                    messageRouter.setRoutingProxy(routingProxySpy);

                                    waitsFor(function() {
                                        return (messagingStubSpy.transmit.calls.count() >= 1);
                                    }, "wait for tranmsit to be done", 1000).finally(function() {
                                        expect(messagingStubSpy.transmit).toHaveBeenCalledWith(expectedJoynrMessage);
                                        done();
                                        return null;
                                    });
                                })
                                .catch(function(error) {
                                    done.fail("unexpected error from messageRouter.route: " + error);
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
                                    var isGloballyVisible = true;
                                    messageRouter.setRoutingProxy(routingProxySpy);

                                    parameters.providerParticipantId = "inProcessParticipant";
                                    messageRouter.addNextHop(parameters.providerParticipantId, new InProcessAddress(undefined), isGloballyVisible);
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

                            function checkRoutingProxyAddNextHop(done, participantId, address, isGloballyVisible) {
                                routingProxySpy.addNextHop.calls.reset();

                                var expectedParticipantId = participantId;
                                var expectedAddress = incomingAddress;
                                var expectedIsGloballyVisible = isGloballyVisible;

                                messageRouter.addNextHop(participantId, address, isGloballyVisible).catch(
                                        done.fail);

                                expect(routingProxySpy.addNextHop).toHaveBeenCalledTimes(1);
                                expect(
                                        routingProxySpy.addNextHop.calls.argsFor(0)[0].participantId)
                                        .toEqual(expectedParticipantId);
                                expect(
                                        routingProxySpy.addNextHop.calls.argsFor(0)[0].browserAddress)
                                        .toEqual(expectedAddress);
                                expect(
                                        routingProxySpy.addNextHop.calls.argsFor(0)[0].isGloballyVisible)
                                        .toEqual(expectedIsGloballyVisible);
                            }

                            it(
                                    "check if routing proxy is called correctly for hop additions",
                                    function(done) {
                                        routingProxySpy.addNextHop.and.returnValue(Promise.resolve());
                                        messageRouter.setRoutingProxy(routingProxySpy);

                                        var isGloballyVisible = true;
                                        checkRoutingProxyAddNextHop(done, joynrMessage.to, address, isGloballyVisible);

                                        isGloballyVisible = false;
                                        checkRoutingProxyAddNextHop(done, joynrMessage.to, address, isGloballyVisible);
                                        done();
                                    });

                            it(
                                    "check if routing proxy is called with queued hop additions",
                                    function(done) {
                                        routingProxySpy.addNextHop.and.returnValue(Promise.resolve());
                                        var onFulfilledSpy = jasmine.createSpy("onFulfilledSpy");

                                        var isGloballyVisible = true;
                                        messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible).then(
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

                                        var isGloballyVisible = true;
                                        messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible);
                                        messageRouter.addNextHop(joynrMessage2.to, address, isGloballyVisible).then(
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
                                        var isGloballyVisible = true;
                                        var addNextHopPromise = messageRouter.addNextHop(joynrMessage.to, address, isGloballyVisible).then(fail);
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
