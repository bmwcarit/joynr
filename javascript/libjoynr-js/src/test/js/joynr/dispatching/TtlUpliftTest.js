/*global fail: true */
/*jslint es5: true, nomen: true */

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
    "joynr/util/UtilInternal",
    "joynr/dispatching/Dispatcher",
    "joynr/messaging/JoynrMessage",
    "joynr/messaging/MessagingQos",
    "joynr/types/DiscoveryEntryWithMetaInfo",
    "joynr/dispatching/types/Request",
    "joynr/dispatching/types/Reply",
    "joynr/dispatching/types/BroadcastSubscriptionRequest",
    "joynr/dispatching/types/MulticastSubscriptionRequest",
    "joynr/dispatching/types/SubscriptionRequest",
    "joynr/dispatching/types/SubscriptionReply",
    "joynr/dispatching/types/SubscriptionStop",
    "joynr/dispatching/types/MulticastPublication",
    "joynr/dispatching/types/SubscriptionPublication",
    "uuid"
], function(
        Util,
        Dispatcher,
        JoynrMessage,
        MessagingQos,
        DiscoveryEntryWithMetaInfo,
        Request,
        Reply,
        BroadcastSubscriptionRequest,
        MulticastSubscriptionRequest,
        SubscriptionRequest,
        SubscriptionReply,
        SubscriptionStop,
        MulticastPublication,
        SubscriptionPublication,
        uuid) {

    var providerId = "providerId";
    var providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
        domain : "testProviderDomain",
        interfaceName : "interfaceName",
        participantId : providerId,
        lastSeenDateMs : Date.now(),
        expiryDateMs : Date.now() + 60000,
        publicKeyId : "publicKeyId",
        isLocal : false
    });
    var proxyId = "proxyId";
    var noTtlUplift = 0;
    var ttlUpliftMs = 10000;
    var toleranceMs = 50;

    var customMatchers =
            {
                toEqualWithPositiveTolerance : function(util, customEqualityTesters) {
                    return {
                        compare : function(actual, expected) {
                            var result = {};
                            if (expected === undefined || expected === null) {
                                result.pass = false;
                                result.message = "Expected expectation not to be " + expected;
                                return result;
                            }
                            if (actual === undefined || actual === null) {
                                result.pass = false;
                                result.message = "Expected value not to be " + actual;
                                return result;
                            }

                            var diff = actual - expected;
                            result.pass = diff >= 0;
                            if (!result.pass) {
                                result.message =
                                        "Expected "
                                            + actual
                                            + " to be greater or equal than "
                                            + expected;
                                return result;
                            }
                            result.pass = diff < toleranceMs;
                            if (result.pass) {
                                result.message =
                                        actual
                                            + " differs less than "
                                            + toleranceMs
                                            + " from "
                                            + expected;
                            } else {
                                result.message =
                                        "Expected "
                                            + actual
                                            + " to differ less than "
                                            + toleranceMs
                                            + " from "
                                            + expected;
                            }
                            return result;
                        }
                    };
                }
            };

    describe("libjoynr-js.joynr.ttlUpliftTest", function() {

        var dispatcher, dispatcherWithTtlUplift;
        var clusterControllerMessagingStub, securityManager;
        var requestReplyManager, subscriptionManager, publicationManager, messageRouter;
        var subscriptionId = "mySubscriptionId-" + uuid();
        var multicastId = "multicastId-" + uuid();
        var requestReplyId = "requestReplyId";

        var ttl, messagingQos, expiryDateMs, expiryDateWithTtlUplift;
        var publicationTtlMs, subscriptionQos;

        function receiveJoynrMessage(parameters) {
            var joynrMessage = new JoynrMessage({
                type : parameters.type,
                payload : JSON.stringify(parameters.payload)
            });
            joynrMessage.setHeader(JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID, proxyId);
            joynrMessage.setHeader(JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID, providerId);
            joynrMessage.setHeader(
                    JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE,
                    parameters.expiryDate);
            dispatcher.receive(joynrMessage);
        }

        function receiveJoynrMessageTtlUplift(parameters) {
            var joynrMessage = new JoynrMessage({
                type : parameters.type,
                payload : JSON.stringify(parameters.payload)
            });
            joynrMessage.setHeader(JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID, proxyId);
            joynrMessage.setHeader(JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID, providerId);
            joynrMessage.setHeader(
                    JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE,
                    parameters.expiryDate);
            dispatcherWithTtlUplift.receive(joynrMessage);
        }

        function checkMessageFromProxyWithTolerance(messageType, expectedExpiryDate) {
            expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
            var msg = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
            expect(msg.type).toEqual(messageType);
            expect(msg.header.from).toEqual(proxyId);
            expect(msg.header.to).toEqual(providerId);
            expect(msg.header.expiryDate).toEqualWithPositiveTolerance(expectedExpiryDate);
        }

        function checkMessageFromProvider(messageType, expectedExpiryDate) {
            expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
            var msg = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
            expect(msg.type).toEqual(messageType);
            expect(msg.header.from).toEqual(providerId);
            expect(msg.header.to).toEqual(proxyId);
            expect(msg.header.expiryDate).toEqual(expectedExpiryDate);
        }

        function checkRequestReplyMessage(expectedExpiryDate) {
            checkMessageFromProvider(JoynrMessage.JOYNRMESSAGE_TYPE_REPLY, expectedExpiryDate);
        }

        function checkSubscriptionReplyMessage(expectedExpiryDate) {
            checkMessageFromProvider(
                    JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY,
                    expectedExpiryDate);
        }

        beforeEach(function() {
            jasmine.addMatchers(customMatchers);

            var sendRequestReply = function(providerParticipantId, request, callbackDispatcher) {
                callbackDispatcher(new Reply({
                    response : "response",
                    requestReplyId : request.requestReplyId
                }));
            };
            requestReplyManager = {
                handleRequest : sendRequestReply
            };
            spyOn(requestReplyManager, "handleRequest").and.callThrough();
            subscriptionManager = jasmine.createSpyObj("SubscriptionManager", [
                "handleSubscriptionReply",
                "handleMulticastPublication",
                "handlePublication"
            ]);
            var sendSubscriptionReply =
                    function(
                            proxyParticipantId,
                            providerParticipantId,
                            subscriptionRequest,
                            callbackDispatcher) {
                        callbackDispatcher(new SubscriptionReply({
                            subscriptionId : subscriptionRequest.subscriptionId
                        }));
                    };

            publicationManager = {
                handleSubscriptionRequest : sendSubscriptionReply,
                handleBroadcastSubscriptionRequest : sendSubscriptionReply,
                handleMulticastSubscriptionRequest : sendSubscriptionReply,
                handleSubscriptionStop : function() {}
            };
            spyOn(publicationManager, "handleSubscriptionRequest").and.callThrough();
            spyOn(publicationManager, "handleBroadcastSubscriptionRequest").and.callThrough();
            spyOn(publicationManager, "handleMulticastSubscriptionRequest").and.callThrough();
            spyOn(publicationManager, "handleSubscriptionStop");

            messageRouter = jasmine.createSpyObj("MessageRouter", [
                "addMulticastReceiver",
                "removeMulticastReceiver"
            ]);
            clusterControllerMessagingStub =
                    jasmine.createSpyObj("ClusterControllerMessagingStub", [ "transmit"
                    ]);

            securityManager = jasmine.createSpyObj("SecurityManager", [ "getCurrentProcessUserId"
            ]);

            dispatcher =
                    new Dispatcher(clusterControllerMessagingStub, securityManager, noTtlUplift);
            dispatcher.registerRequestReplyManager(requestReplyManager);
            dispatcher.registerSubscriptionManager(subscriptionManager);
            dispatcher.registerPublicationManager(publicationManager);
            dispatcher.registerMessageRouter(messageRouter);

            dispatcherWithTtlUplift =
                    new Dispatcher(clusterControllerMessagingStub, securityManager, ttlUpliftMs);
            dispatcherWithTtlUplift.registerRequestReplyManager(requestReplyManager);
            dispatcherWithTtlUplift.registerSubscriptionManager(subscriptionManager);
            dispatcherWithTtlUplift.registerPublicationManager(publicationManager);
            dispatcherWithTtlUplift.registerMessageRouter(messageRouter);

            ttl = 300;
            messagingQos = new MessagingQos({
                ttl : ttl
            });
            publicationTtlMs = 1000;
            expiryDateMs = Date.now() + ttl;
            expiryDateWithTtlUplift = expiryDateMs + ttlUpliftMs;

            subscriptionQos = {
                expiryDateMs : expiryDateMs,
                publicationTtlMs : publicationTtlMs,
                minIntervalMs : 0
            };
        });

        describe("no ttl uplift (default)", function() {

            it("send request", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    request : "request"
                };

                dispatcher.sendRequest(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                        expiryDateMs);
            });

            it("send one way request", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    request : "oneWayRequest"
                };

                dispatcher.sendOneWayRequest(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY,
                        expiryDateMs);
            });

            it("send subscription request", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    subscriptionRequest : new SubscriptionRequest({
                        subscriptionId : "subscriptionId",
                        subscribedToName : "attributeName",
                        qos : subscriptionQos
                    })
                };
                dispatcher.sendSubscriptionRequest(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                        expiryDateMs);
            });

            it("send broadcast subscription request", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    subscriptionRequest : new BroadcastSubscriptionRequest({
                        subscriptionId : "subscriptionId",
                        subscribedToName : "broadcastEvent",
                        qos : subscriptionQos
                    })
                };

                dispatcher.sendBroadcastSubscriptionRequest(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                        expiryDateMs);
            });

            it("send multicast subscription request", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    subscriptionRequest : new MulticastSubscriptionRequest({
                        multicastId : "multicastId",
                        subscriptionId : "subscriptionId",
                        subscribedToName : "multicastEvent",
                        qos : subscriptionQos
                    })
                };

                dispatcher.sendBroadcastSubscriptionRequest(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
                        expiryDateMs);
            });

            it("send multicast subscription stop", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    multicastId : "multicastId",
                    subscriptionStop : new SubscriptionStop({
                        subscriptionId : "subscriptionId"
                    })
                };

                dispatcher.sendMulticastSubscriptionStop(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP,
                        expiryDateMs);
            });

            it("send subscription stop", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    subscriptionStop : new SubscriptionStop({
                        subscriptionId : "subscriptionId"
                    })
                };

                dispatcher.sendSubscriptionStop(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP,
                        expiryDateMs);
            });

            it("send publication", function() {
                var settings = {
                    from : proxyId,
                    to : providerId,
                    expiryDate : expiryDateMs
                };
                var publication = new SubscriptionPublication({
                    subscriptionId : "subscriptionId"
                });

                dispatcher.sendPublication(settings, "publication");

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION,
                        expiryDateMs);
            });

            it("send multicast publication", function() {
                var settings = {
                    from : providerId,
                    expiryDate : expiryDateMs
                };
                var multicastId = "multicastId";
                var publication = new MulticastPublication({
                    multicastId : multicastId
                });

                dispatcher.sendMulticastPublication(settings, publication);

                expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
                var msg = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
                expect(msg.type).toEqual(JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST);
                expect(msg.header.from).toEqual(providerId);
                expect(msg.header.to).toEqual(multicastId);
                expect(msg.header.expiryDate).toEqualWithPositiveTolerance(expiryDateMs);
            });

            it("request and reply", function() {
                var payload = {
                    methodName : "methodName"
                };

                receiveJoynrMessage({
                    type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                    payload : payload,
                    expiryDate : expiryDateMs
                });

                expect(requestReplyManager.handleRequest).toHaveBeenCalled();
                expect(requestReplyManager.handleRequest).toHaveBeenCalledWith(
                        providerId,
                        jasmine.any(Request),
                        jasmine.any(Function));

                checkRequestReplyMessage(expiryDateMs);
            });

            it("subscription expiry date and subscription reply", function() {
                //                var expiryDateMs = Date.now() + 100000;
                //                var publicationTtlMs = 10000;
                //                var qos = {
                //                    expiryDateMs : expiryDateMs,
                //                    publicationTtlMs : publicationTtlMs,
                //                    minIntervalMs : 0
                //                };
                var payload = {
                    subscribedToName : "attributeName",
                    subscriptionId : subscriptionId,
                    qos : subscriptionQos
                };

                var payloadCopy = Util.extendDeep({}, payload);
                var expectedSubscriptionRequest = new SubscriptionRequest(payloadCopy);

                receiveJoynrMessage({
                    type : JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                    payload : payload,
                    expiryDate : expiryDateMs
                });

                expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalled();
                expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalledWith(
                        proxyId,
                        providerId,
                        expectedSubscriptionRequest,
                        jasmine.any(Function));

                checkSubscriptionReplyMessage(expiryDateMs);
            });

            it("broadcast subscription expiry date and subscription reply", function() {
                var payload = {
                    subscribedToName : "broadcastEvent",
                    subscriptionId : subscriptionId,
                    qos : subscriptionQos
                };

                var payloadCopy = Util.extendDeep({}, payload);
                var expectedSubscriptionRequest = new BroadcastSubscriptionRequest(payloadCopy);

                receiveJoynrMessage({
                    type : JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                    payload : payload,
                    expiryDate : expiryDateMs
                });

                expect(publicationManager.handleBroadcastSubscriptionRequest).toHaveBeenCalled();
                expect(publicationManager.handleBroadcastSubscriptionRequest).toHaveBeenCalledWith(
                        proxyId,
                        providerId,
                        expectedSubscriptionRequest,
                        jasmine.any(Function));

                checkSubscriptionReplyMessage(expiryDateMs);
            });

            it("multicast subscription expiryDate and subscription reply", function() {
                var payload = {
                    subscribedToName : "multicastEvent",
                    subscriptionId : subscriptionId,
                    multicastId : multicastId,
                    qos : subscriptionQos
                };

                var payloadCopy = Util.extendDeep({}, payload);
                var expectedSubscriptionRequest = new MulticastSubscriptionRequest(payloadCopy);

                receiveJoynrMessage({
                    type : JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
                    payload : payload,
                    expiryDate : expiryDateMs
                });

                expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalled();
                expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalledWith(
                        proxyId,
                        providerId,
                        expectedSubscriptionRequest,
                        jasmine.any(Function));

                checkSubscriptionReplyMessage(expiryDateMs);
            });

        }); // describe "no ttl uplift (default)"

        describe("with ttlUplift", function() {

            it("send request", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    request : "request"
                };

                dispatcherWithTtlUplift.sendRequest(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                        expiryDateWithTtlUplift);
            });

            it("send one way request", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    request : "oneWayRequest"
                };

                dispatcherWithTtlUplift.sendOneWayRequest(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY,
                        expiryDateWithTtlUplift);
            });

            it("send subscription request", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    subscriptionRequest : new SubscriptionRequest({
                        subscriptionId : "subscriptionId",
                        subscribedToName : "attributeName",
                        qos : subscriptionQos
                    })
                };

                dispatcherWithTtlUplift.sendSubscriptionRequest(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                        expiryDateWithTtlUplift);
            });

            it("send broadcast subscription request", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    subscriptionRequest : new BroadcastSubscriptionRequest({
                        subscriptionId : "subscriptionId",
                        subscribedToName : "broadcastEvent",
                        qos : subscriptionQos
                    })
                };

                dispatcherWithTtlUplift.sendBroadcastSubscriptionRequest(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                        expiryDateWithTtlUplift);
            });

            it("send multicast subscription request", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    subscriptionRequest : new MulticastSubscriptionRequest({
                        multicastId : "multicastId",
                        subscriptionId : "subscriptionId",
                        subscribedToName : "multicastEvent",
                        qos : subscriptionQos
                    })
                };

                dispatcherWithTtlUplift.sendBroadcastSubscriptionRequest(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
                        expiryDateWithTtlUplift);
            });

            it("send multicast subscription stop", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    multicastId : "multicastId",
                    subscriptionStop : new SubscriptionStop({
                        subscriptionId : "subscriptionId"
                    })
                };

                dispatcherWithTtlUplift.sendMulticastSubscriptionStop(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP,
                        expiryDateWithTtlUplift);
            });

            it("send subscription stop", function() {
                var settings = {
                    from : proxyId,
                    toDiscoveryEntry : providerDiscoveryEntry,
                    messagingQos : messagingQos,
                    subscriptionStop : new SubscriptionStop({
                        subscriptionId : "subscriptionId"
                    })
                };

                dispatcherWithTtlUplift.sendSubscriptionStop(settings);

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP,
                        expiryDateWithTtlUplift);
            });

            it("send publication", function() {
                var settings = {
                    from : proxyId,
                    to : providerId,
                    expiryDate : expiryDateMs
                };
                var publication = new SubscriptionPublication({
                    subscriptionId : "subscriptionId"
                });

                dispatcherWithTtlUplift.sendPublication(settings, "publication");

                checkMessageFromProxyWithTolerance(
                        JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION,
                        expiryDateWithTtlUplift);
            });

            it(
                    "send multicast publication",
                    function() {
                        var settings = {
                            from : providerId,
                            expiryDate : expiryDateMs
                        };
                        var multicastId = "multicastId";
                        var publication = new MulticastPublication({
                            multicastId : multicastId
                        });

                        dispatcherWithTtlUplift.sendMulticastPublication(settings, publication);

                        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
                        var msg =
                                clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
                        expect(msg.type).toEqual(JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST);
                        expect(msg.header.from).toEqual(providerId);
                        expect(msg.header.to).toEqual(multicastId);
                        expect(msg.header.expiryDate).toEqualWithPositiveTolerance(
                                expiryDateWithTtlUplift);
                    });

            it("request and reply", function() {
                var payload = {
                    methodName : "methodName"
                };

                receiveJoynrMessageTtlUplift({
                    type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                    payload : payload,
                    expiryDate : expiryDateMs + ttlUpliftMs
                });

                expect(requestReplyManager.handleRequest).toHaveBeenCalled();
                expect(requestReplyManager.handleRequest).toHaveBeenCalledWith(
                        providerId,
                        jasmine.any(Request),
                        jasmine.any(Function));

                checkRequestReplyMessage(expiryDateWithTtlUplift);
            });

            it("subscription expiry date and subscription reply", function() {
                var payload = {
                    subscribedToName : "attributeName",
                    subscriptionId : subscriptionId,
                    qos : subscriptionQos
                };

                var payloadCopy = Util.extendDeep({}, payload);
                var expectedSubscriptionRequest = new SubscriptionRequest(payloadCopy);
                expectedSubscriptionRequest.qos.expiryDateMs = expiryDateWithTtlUplift;

                receiveJoynrMessageTtlUplift({
                    type : JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                    payload : payload,
                    expiryDate : expiryDateMs + ttlUpliftMs
                });

                expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalled();
                expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalledWith(
                        proxyId,
                        providerId,
                        expectedSubscriptionRequest,
                        jasmine.any(Function));

                checkSubscriptionReplyMessage(expiryDateWithTtlUplift);
            });

            it("broadcast subscription expiry date and subscription reply", function() {
                var payload = {
                    subscribedToName : "broadcastEvent",
                    subscriptionId : subscriptionId,
                    qos : subscriptionQos
                };

                var payloadCopy = Util.extendDeep({}, payload);
                var expectedSubscriptionRequest = new BroadcastSubscriptionRequest(payloadCopy);
                expectedSubscriptionRequest.qos.expiryDateMs = expiryDateWithTtlUplift;

                receiveJoynrMessageTtlUplift({
                    type : JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
                    payload : payload,
                    expiryDate : expiryDateMs + ttlUpliftMs
                });

                expect(publicationManager.handleBroadcastSubscriptionRequest).toHaveBeenCalled();
                expect(publicationManager.handleBroadcastSubscriptionRequest).toHaveBeenCalledWith(
                        proxyId,
                        providerId,
                        expectedSubscriptionRequest,
                        jasmine.any(Function));

                checkSubscriptionReplyMessage(expiryDateWithTtlUplift);
            });

            it("multicast subscription expiry date and subscription reply", function() {
                var payload = {
                    subscribedToName : "multicastEvent",
                    subscriptionId : subscriptionId,
                    multicastId : multicastId,
                    qos : subscriptionQos
                };

                var payloadCopy = Util.extendDeep({}, payload);
                var expectedSubscriptionRequest = new MulticastSubscriptionRequest(payloadCopy);
                expectedSubscriptionRequest.qos.expiryDateMs = expiryDateWithTtlUplift;

                receiveJoynrMessageTtlUplift({
                    type : JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
                    payload : payload,
                    expiryDate : expiryDateMs + ttlUpliftMs
                });

                expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalled();
                expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalledWith(
                        proxyId,
                        providerId,
                        expectedSubscriptionRequest,
                        jasmine.any(Function));

                checkSubscriptionReplyMessage(expiryDateWithTtlUplift);
            });

        }); // describe "with ttlUplift"

    }); // describe ttlUpliftTest

}); // require
