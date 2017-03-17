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

define(
        [
            "joynr/util/Util",
            "joynr/dispatching/Dispatcher",
            "joynr/dispatching/subscription/PublicationManager",
            "joynr/dispatching/subscription/SubscriptionManager",
            "joynr/dispatching/RequestReplyManager",
            "joynr/messaging/inprocess/InProcessMessagingStub",
            "joynr/messaging/inprocess/InProcessMessagingSkeleton",
            "joynr/messaging/JoynrMessage",
            "joynr/messaging/MessagingQos",
            "joynr/messaging/MessagingQosEffort",
            "joynr/dispatching/types/OneWayRequest",
            "joynr/dispatching/types/Request",
            "joynr/dispatching/types/Reply",
            "joynr/dispatching/types/BroadcastSubscriptionRequest",
            "joynr/dispatching/types/MulticastSubscriptionRequest",
            "joynr/dispatching/types/SubscriptionRequest",
            "joynr/dispatching/types/SubscriptionReply",
            "joynr/dispatching/types/SubscriptionStop",
            "joynr/dispatching/types/MulticastPublication",
            "joynr/dispatching/types/SubscriptionPublication",
            "joynr/tests/testTypes/TestEnum",
            "joynr/types/TypeRegistrySingleton",
            "joynr/types/DiscoveryEntryWithMetaInfo",
            "joynr/types/Version",
            "joynr/types/ProviderQos",
            "uuid"
        ],
        function(
                Util,
                Dispatcher,
                PublicationManager,
                SubscriptionManager,
                RequestReplyManager,
                InProcessMessagingStub,
                InProcessMessagingSkeleton,
                JoynrMessage,
                MessagingQos,
                MessagingQosEffort,
                OneWayRequest,
                Request,
                Reply,
                BroadcastSubscriptionRequest,
                MulticastSubscriptionRequest,
                SubscriptionRequest,
                SubscriptionReply,
                SubscriptionStop,
                MulticastPublication,
                SubscriptionPublication,
                TestEnum,
                TypeRegistrySingleton,
                DiscoveryEntryWithMetaInfo,
                Version,
                ProviderQos,
                uuid) {

            var providerId = "providerId";
            var proxyId = "proxyId";
            var toDiscoveryEntry, globalToDiscoveryEntry;

            describe(
                    "libjoynr-js.joynr.dispatching.Dispatcher",
                    function() {

                        var dispatcher, requestReplyManager, subscriptionManager, publicationManager, messageRouter, clusterControllerMessagingStub, securityManager;
                        var subscriptionId = "mySubscriptionId-" + uuid();
                        var multicastId = "multicastId-" + uuid();
                        var requestReplyId = "requestReplyId";

                        /**
                         * Called before each test.
                         */
                        beforeEach(function(done) {
                            toDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
                                providerVersion : new Version({majorVersion : 0, minorVersion : 23}),
                                domain : "testProviderDomain",
                                interfaceName : "interfaceName",
                                participantId : providerId,
                                qos : new ProviderQos(),
                                lastSeenDateMs : Date.now(),
                                expiryDateMs : Date.now() + 60000,
                                publicKeyId : "publicKeyId",
                                isLocal : true
                            });
                            globalToDiscoveryEntry = new DiscoveryEntryWithMetaInfo(toDiscoveryEntry);
                            globalToDiscoveryEntry.isLocal = false;
                            requestReplyManager = jasmine.createSpyObj("RequestReplyManager", [
                                "handleOneWayRequest",
                                "handleRequest",
                                "handleReply"
                            ]);
                            subscriptionManager = jasmine.createSpyObj("SubscriptionManager", [
                                "handleSubscriptionReply",
                                "handleMulticastPublication",
                                "handlePublication"
                            ]);
                            var sendSubscriptionReply = function(
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
                                handleMulticastSubscriptionRequest : sendSubscriptionReply,
                                handleSubscriptionStop : function() {}
                            };
                            spyOn(publicationManager, "handleSubscriptionRequest").and.callThrough();
                            spyOn(publicationManager, "handleMulticastSubscriptionRequest").and.callThrough();
                            spyOn(publicationManager, "handleSubscriptionStop");

                            messageRouter = jasmine.createSpyObj("MessageRouter", [
                                "addMulticastReceiver",
                                "removeMulticastReceiver"
                            ]);
                            clusterControllerMessagingStub =
                                    jasmine.createSpyObj(
                                            "ClusterControllerMessagingStub",
                                            [ "transmit"
                                            ]);

                            securityManager =
                                    jasmine.createSpyObj(
                                            "SecurityManager",
                                            [ "getCurrentProcessUserId"
                                            ]);

                            dispatcher =
                                    new Dispatcher(clusterControllerMessagingStub, securityManager);
                            dispatcher.registerRequestReplyManager(requestReplyManager);
                            dispatcher.registerSubscriptionManager(subscriptionManager);
                            dispatcher.registerPublicationManager(publicationManager);
                            dispatcher.registerMessageRouter(messageRouter);

                            /*
                             * Make sure 'TestEnum' is properly registered as a type.
                             * Just requiring the module is insufficient since the
                             * automatically generated code called async methods.
                             * Execution might be still in progress.
                             */
                            TypeRegistrySingleton.getInstance().getTypeRegisteredPromise("joynr.tests.testTypes.TestEnum", 1000).then(function() {
                                done();
                                return null;
                            }).catch(fail);
                        });

                        it(
                                "is instantiable and of correct type",
                                function(done) {
                                    expect(Dispatcher).toBeDefined();
                                    expect(typeof Dispatcher === "function").toBeTruthy();
                                    expect(dispatcher).toBeDefined();
                                    expect(dispatcher instanceof Dispatcher).toBeTruthy();
                                    expect(dispatcher.registerRequestReplyManager).toBeDefined();
                                    expect(
                                            typeof dispatcher.registerRequestReplyManager === "function")
                                            .toBeTruthy();
                                    expect(dispatcher.registerSubscriptionManager).toBeDefined();
                                    expect(
                                            typeof dispatcher.registerSubscriptionManager === "function")
                                            .toBeTruthy();
                                    expect(dispatcher.registerPublicationManager).toBeDefined();
                                    expect(
                                            typeof dispatcher.registerPublicationManager === "function")
                                            .toBeTruthy();
                                    expect(dispatcher.sendRequest).toBeDefined();
                                    expect(typeof dispatcher.sendRequest === "function")
                                            .toBeTruthy();
                                    expect(dispatcher.sendOneWayRequest).toBeDefined();
                                    expect(typeof dispatcher.sendOneWayRequest === "function")
                                            .toBeTruthy();
                                    expect(dispatcher.sendBroadcastSubscriptionRequest).toBeDefined();
                                    expect(typeof dispatcher.sendBroadcastSubscriptionRequest === "function")
                                            .toBeTruthy();
                                    expect(dispatcher.sendSubscriptionRequest).toBeDefined();
                                    expect(typeof dispatcher.sendSubscriptionRequest === "function")
                                            .toBeTruthy();
                                    expect(dispatcher.sendSubscriptionStop).toBeDefined();
                                    expect(typeof dispatcher.sendSubscriptionStop === "function")
                                            .toBeTruthy();
                                    expect(dispatcher.sendPublication).toBeDefined();
                                    expect(typeof dispatcher.sendPublication === "function")
                                            .toBeTruthy();
                                    expect(dispatcher.receive).toBeDefined();
                                    expect(typeof dispatcher.receive === "function").toBeTruthy();
                                    done();
                                });

                        function receiveJoynrMessage(parameters){
                            var joynrMessage = new JoynrMessage({
                                type : parameters.type,
                                payload : JSON.stringify(parameters.payload)
                            });
                            joynrMessage.setHeader(
                                    JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID,
                                    proxyId);
                            joynrMessage.setHeader(
                                    JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID,
                                    providerId);
                            dispatcher.receive(joynrMessage);
                        }

                        it(
                                "forwards subscription request to Publication Manager",
                                function() {
                                    var payload = {
                                        subscribedToName : "attributeName",
                                        subscriptionId : subscriptionId
                                    };
                                    receiveJoynrMessage({
                                        type : JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                                        payload : payload
                                    });
                                    expect(publicationManager.handleSubscriptionRequest)
                                            .toHaveBeenCalled();
                                    expect(publicationManager.handleSubscriptionRequest)
                                            .toHaveBeenCalledWith(
                                                    proxyId,
                                                    providerId,
                                                    new SubscriptionRequest(payload),
                                                    jasmine.any(Function));
                                });

                        it(
                                "forwards multicast subscription request to Publication Manager",
                                function() {
                                    var payload = {
                                        subscribedToName : "multicastEvent",
                                        subscriptionId : subscriptionId,
                                        multicastId : multicastId
                                    };
                                    receiveJoynrMessage({
                                        type : JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
                                        payload : payload
                                    });
                                    expect(publicationManager.handleMulticastSubscriptionRequest)
                                            .toHaveBeenCalled();
                                    expect(publicationManager.handleMulticastSubscriptionRequest)
                                            .toHaveBeenCalledWith(
                                                    proxyId,
                                                    providerId,
                                                    new MulticastSubscriptionRequest(payload),
                                                    jasmine.any(Function));
                                    expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
                                    var sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
                                    expect(sentMessage.payload).toMatch("{\"subscriptionId\":\"" + subscriptionId + "\",\"_typeName\":\"joynr.SubscriptionReply\"}");
                                });

                        it("forwards subscription reply to Subscription Manager", function(done) {
                            var payload = {
                                subscriptionId : subscriptionId
                            };
                            var joynrMessage = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY,
                                payload : JSON.stringify(payload)
                            });
                            dispatcher.receive(joynrMessage);
                            expect(subscriptionManager.handleSubscriptionReply).toHaveBeenCalled();
                            expect(subscriptionManager.handleSubscriptionReply)
                                    .toHaveBeenCalledWith(new SubscriptionReply(payload));
                            done();
                        });

                        it(
                                "forwards subscription stop to SubscriptionPublication Manager",
                                function(done) {
                                    var payload = {
                                        subscriptionId : subscriptionId
                                    };
                                    var joynrMessage = new JoynrMessage({
                                        type : JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP,
                                        payload : JSON.stringify(payload)
                                    });
                                    dispatcher.receive(joynrMessage);
                                    expect(publicationManager.handleSubscriptionStop)
                                            .toHaveBeenCalled();
                                    expect(publicationManager.handleSubscriptionStop)
                                            .toHaveBeenCalledWith(new SubscriptionStop(payload));
                                    done();
                                });

                        function receivePublication(parameters) {
                            var joynrMessage = new JoynrMessage({
                                type : parameters.type,
                                payload : JSON.stringify(parameters.payload)
                            });
                            dispatcher.receive(joynrMessage);
                        }

                        it("forwards publication to Subscription Manager", function(done) {
                            var payload = {
                                subscriptionId : subscriptionId,
                                response : "myResponse"
                            };
                            receivePublication({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION,
                                payload : payload
                            });
                            expect(subscriptionManager.handlePublication).toHaveBeenCalled();
                            expect(subscriptionManager.handlePublication).toHaveBeenCalledWith(
                                    new SubscriptionPublication(payload));
                            done();
                        });

                        it("forwards multicast publication to Subscription Manager", function(done) {
                            var payload = {
                                multicastId : multicastId,
                                response : "myResponse"
                            };
                            receivePublication({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST,
                                payload : payload
                            });
                            expect(subscriptionManager.handleMulticastPublication).toHaveBeenCalled();
                            expect(subscriptionManager.handleMulticastPublication).toHaveBeenCalledWith(
                                    new MulticastPublication(payload));
                            done();
                        });

                        it(
                                "forwards request to RequestReply Manager",
                                function(done) {
                                    var request = new Request({
                                        methodName : "methodName"
                                    });
                                    var joynrMessage = new JoynrMessage({
                                        type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                                        payload : JSON.stringify(request)
                                    });
                                    joynrMessage.setHeader(
                                            JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID,
                                            providerId);
                                    joynrMessage.setHeader(
                                            JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID,
                                            proxyId);
                                    dispatcher.receive(joynrMessage);
                                    expect(requestReplyManager.handleRequest).toHaveBeenCalled();
                                    expect(
                                            requestReplyManager.handleRequest.calls.mostRecent().args[0])
                                            .toEqual(providerId);
                                    expect(
                                            requestReplyManager.handleRequest.calls.mostRecent().args[1])
                                            .toEqual(request);
                                    expect(
                                            typeof requestReplyManager.handleRequest.calls
                                                    .mostRecent().args[2] === "function")
                                            .toBeTruthy();
                                    done();
                                });

                        it(
                                "forwards one-way request to RequestReply Manager",
                                function(done) {
                                    var oneWayRequest = new OneWayRequest({
                                        methodName : "methodName"
                                    });
                                    var joynrMessage = new JoynrMessage({
                                        type : JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY,
                                        payload : JSON.stringify(oneWayRequest)
                                    });
                                    joynrMessage.setHeader(
                                            JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID,
                                            providerId);
                                    joynrMessage.setHeader(
                                            JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID,
                                            proxyId);
                                    dispatcher.receive(joynrMessage);
                                    expect(requestReplyManager.handleOneWayRequest)
                                            .toHaveBeenCalled();
                                    expect(
                                            requestReplyManager.handleOneWayRequest.calls
                                                    .mostRecent().args[0]).toEqual(providerId);
                                    expect(
                                            requestReplyManager.handleOneWayRequest.calls
                                                    .mostRecent().args[1]).toEqual(oneWayRequest);
                                    done();
                                });

                        it("forwards reply to RequestReply Manager", function(done) {
                            var reply = new Reply({
                                requestReplyId : requestReplyId,
                                response : []
                            });
                            var joynrMessage = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_REPLY,
                                payload : JSON.stringify(reply)
                            });
                            dispatcher.receive(joynrMessage);
                            expect(requestReplyManager.handleReply).toHaveBeenCalled();
                            expect(requestReplyManager.handleReply).toHaveBeenCalledWith(reply);
                            done();
                        });

                        function sendAndCheckBroadcastSubscriptionRequest(request, expectedType) {
                            var sentMessage;
                            var messagingQos = new MessagingQos();
                            dispatcher.sendBroadcastSubscriptionRequest({
                                from : "from",
                                toDiscoveryEntry : toDiscoveryEntry,
                                messagingQos : messagingQos,
                                subscriptionRequest : request
                            });
                            expect(clusterControllerMessagingStub.transmit)
                            .toHaveBeenCalled();
                            sentMessage =
                                clusterControllerMessagingStub.transmit.calls
                                .mostRecent().args[0];
                            expect(sentMessage.type).toEqual(
                                    expectedType);
                        }

                        it(
                                "is able to send multicast subscription request",
                                function() {
                                    var multicastId = "multicastId";
                                    sendAndCheckBroadcastSubscriptionRequest(new MulticastSubscriptionRequest({
                                        subscribedToName : "multicastEvent",
                                        subscriptionId : "subscriptionId",
                                        multicastId : multicastId
                                    }), JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST);
                                    expect(messageRouter.addMulticastReceiver).toHaveBeenCalled();
                                    expect(messageRouter.addMulticastReceiver.calls.argsFor(0)[0].multicastId).toEqual(multicastId);
                                });

                        it(
                                "is able to send selective broadcast subscription request",
                                function() {
                                    sendAndCheckBroadcastSubscriptionRequest(new BroadcastSubscriptionRequest({
                                        subscribedToName : "broadcastEvent",
                                        subscriptionId : "subscriptionId"
                                    }), JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST);
                                });

                        function setsIsLocalMessageInSubscriptionRequest(subscriptionRequest, sendMethod) {
                            var sentMessage;
                            var messagingQos = new MessagingQos();

                            sendMethod({
                                from : "from",
                                toDiscoveryEntry : toDiscoveryEntry,
                                messagingQos : messagingQos,
                                subscriptionRequest : subscriptionRequest
                            });
                            expect(clusterControllerMessagingStub.transmit)
                                    .toHaveBeenCalled();
                            sentMessage = clusterControllerMessagingStub.transmit.calls
                                        .mostRecent().args[0];
                            expect(sentMessage.isLocalMessage).toEqual(true);

                            sendMethod({
                                from : "from",
                                toDiscoveryEntry : globalToDiscoveryEntry,
                                messagingQos : messagingQos,
                                subscriptionRequest : subscriptionRequest
                            });
                            expect(clusterControllerMessagingStub.transmit)
                                    .toHaveBeenCalled();
                            sentMessage = clusterControllerMessagingStub.transmit.calls
                                        .mostRecent().args[0];
                            expect(sentMessage.isLocalMessage).toEqual(false);
                        }

                        it("sets isLocalMessage in request messages", function(done) {
                            var sentMessage;
                            var messagingQos = new MessagingQos();

                            var request = new Request({
                                methodName : "methodName"
                            });
                            dispatcher.sendRequest({
                                from : "from",
                                toDiscoveryEntry : toDiscoveryEntry,
                                messagingQos : messagingQos,
                                request : request
                            });
                            expect(clusterControllerMessagingStub.transmit)
                                    .toHaveBeenCalled();
                            sentMessage = clusterControllerMessagingStub.transmit.calls
                                        .mostRecent().args[0];
                            expect(sentMessage.isLocalMessage).toEqual(true);

                            dispatcher.sendRequest({
                                from : "from",
                                toDiscoveryEntry : globalToDiscoveryEntry,
                                messagingQos : messagingQos,
                                request : request
                            });
                            expect(clusterControllerMessagingStub.transmit)
                                    .toHaveBeenCalled();
                            sentMessage = clusterControllerMessagingStub.transmit.calls
                                        .mostRecent().args[0];
                            expect(sentMessage.isLocalMessage).toEqual(false);

                            done();
                        });

                        it("sets isLocalMessage in subscription request messages", function(done) {
                            var subscriptionRequestPayload = {
                                subscribedToName : "subscribeToName",
                                subscriptionId : subscriptionId
                            };
                            var subscriptionRequest = new SubscriptionRequest(subscriptionRequestPayload);
                            setsIsLocalMessageInSubscriptionRequest(subscriptionRequest, dispatcher.sendSubscriptionRequest);

                            var broadcastSubscriptionRequest = new BroadcastSubscriptionRequest(subscriptionRequestPayload);
                            setsIsLocalMessageInSubscriptionRequest(broadcastSubscriptionRequest, dispatcher.sendBroadcastSubscriptionRequest);

                            subscriptionRequestPayload.multicastId = multicastId;
                            var multicastSubscriptionRequest = new MulticastSubscriptionRequest(subscriptionRequestPayload);
                            setsIsLocalMessageInSubscriptionRequest(multicastSubscriptionRequest, dispatcher.sendBroadcastSubscriptionRequest);

                            done();
                        });

                        it(
                                "enriches requests with custom headers",
                                function(done) {
                                    var sentMessage;
                                    var request = new Request({
                                        methodName : "methodName"
                                    });
                                    var messagingQos = new MessagingQos();
                                    var headerKey = "key";
                                    var headerValue = "value";
                                    messagingQos.putCustomMessageHeader(headerKey, headerValue);
                                    dispatcher.sendRequest({
                                        from : "from",
                                        toDiscoveryEntry : toDiscoveryEntry,
                                        messagingQos : messagingQos,
                                        request : request
                                    });
                                    expect(clusterControllerMessagingStub.transmit)
                                            .toHaveBeenCalled();
                                    sentMessage =
                                            clusterControllerMessagingStub.transmit.calls
                                                    .mostRecent().args[0];
                                    expect(sentMessage.getCustomHeaders()[headerKey]).toEqual(
                                            headerValue);
                                    done();
                                });

                        it(
                                "enriches requests with effort header",
                                function(done) {
                                    var sentMessage;
                                    var request = new Request({
                                        methodName : "methodName"
                                    });
                                    var messagingQos = new MessagingQos();
                                    messagingQos.effort = MessagingQosEffort.BEST_EFFORT;
                                    dispatcher.sendRequest({
                                        from : "from",
                                        toDiscoveryEntry : toDiscoveryEntry,
                                        messagingQos : messagingQos,
                                        request : request
                                    });
                                    expect(clusterControllerMessagingStub.transmit)
                                            .toHaveBeenCalled();
                                    sentMessage =
                                            clusterControllerMessagingStub.transmit.calls
                                                    .mostRecent().args[0];
                                    expect(sentMessage.effort).toEqual(MessagingQosEffort.BEST_EFFORT.value);
                                    done();
                                });

                        it(
                                "enriches one way requests with custom headers",
                                function(done) {
                                    var sentMessage;
                                    var request = new OneWayRequest({
                                        methodName : "methodName"
                                    });
                                    var messagingQos = new MessagingQos();
                                    var headerKey = "key";
                                    var headerValue = "value";
                                    messagingQos.putCustomMessageHeader(headerKey, headerValue);
                                    dispatcher.sendOneWayRequest({
                                        from : "from",
                                        toDiscoveryEntry : toDiscoveryEntry,
                                        messagingQos : messagingQos,
                                        request : request
                                    });
                                    expect(clusterControllerMessagingStub.transmit)
                                            .toHaveBeenCalled();
                                    sentMessage =
                                            clusterControllerMessagingStub.transmit.calls
                                                    .mostRecent().args[0];
                                    expect(sentMessage.getCustomHeaders()[headerKey]).toEqual(
                                            headerValue);
                                    done();
                                });

                        it(
                                "enriches replies with custom headers from request",
                                function(done) {
                                    var sentRequestMessage, sentReplyMessage;
                                    var request = new Request({
                                        methodName : "methodName"
                                    });
                                    var messagingQos = new MessagingQos();
                                    var headerKey = "key";
                                    var headerValue = "value";
                                    messagingQos.putCustomMessageHeader(headerKey, headerValue);
                                    dispatcher.sendRequest({
                                        from : "from",
                                        toDiscoveryEntry : toDiscoveryEntry,
                                        messagingQos : messagingQos,
                                        request : request
                                    });
                                    expect(clusterControllerMessagingStub.transmit)
                                            .toHaveBeenCalled();
                                    sentRequestMessage =
                                            clusterControllerMessagingStub.transmit.calls
                                                    .mostRecent().args[0];
                                    // get ready for an incoming request: when handleRequest is called, pass an empty reply back.
                                    requestReplyManager.handleRequest.and.callFake(function(
                                            to,
                                            request,
                                            callback) {
                                        callback(new Reply());
                                    });
                                    // now simulate receiving the request message, as if it had been transmitted
                                    // this will be passed on to the mock requestReplyManager
                                    dispatcher.receive(sentRequestMessage);
                                    sentReplyMessage =
                                            clusterControllerMessagingStub.transmit.calls
                                                    .mostRecent().args[0];
                                    expect(sentReplyMessage.getCustomHeaders()[headerKey]).toEqual(
                                            headerValue);
                                    done();
                                });

                        it(
                                "sends subscription reply on subscription request",
                                function(done) {
                                    var payload = {
                                        subscribedToName : "attributeName",
                                        subscriptionId : subscriptionId
                                    };
                                    var joynrMessage = new JoynrMessage({
                                        type : JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                                        payload : JSON.stringify(payload)
                                    });
                                    joynrMessage.setHeader(
                                            JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID,
                                            proxyId);
                                    joynrMessage.setHeader(
                                            JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID,
                                            providerId);

                                    var subscriptionReplyPayload = {
                                            subscriptionId : subscriptionId
                                    };
                                    var subscriptionReply = new SubscriptionReply(payload);

                                    /*
                                     * The dispatcher.receive() based on the message type calls
                                     * publicationManager.handleSubscriptionRequest()
                                     * and hands over a callback that invokes sendSubscriptionReply().
                                     * The resulting message is finally sent out using
                                     * clusterControllerMessagingStub.transmit().
                                     */

                                    publicationManager.handleSubscriptionRequest.and.callFake(function(proxyId, providerId, subscriptionRequest, callback) {
                                        callback(subscriptionReply);
                                    });

                                    dispatcher.receive(joynrMessage);

                                    /*
                                     * Note: We can directly expect stuff here only just because
                                     * the jasmine spies do not emulate async action which would
                                     * otherwise occur in real code.
                                     */
                                    expect(publicationManager.handleSubscriptionRequest)
                                            .toHaveBeenCalled();
                                    expect(publicationManager.handleSubscriptionRequest)
                                            .toHaveBeenCalledWith(
                                                    proxyId,
                                                    providerId,
                                                    new SubscriptionRequest(payload),
                                                    jasmine.any(Function));

                                    expect(clusterControllerMessagingStub.transmit)
                                            .toHaveBeenCalled();
                                    var sentMessage =
                                            clusterControllerMessagingStub.transmit.calls
                                                    .mostRecent().args[0];
                                    expect(sentMessage.type).toEqual(
                                            JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY);
                                    expect(sentMessage.payload).toEqual(
                                            JSON.stringify(subscriptionReply));

                                    done();
                                });
                    });

        }); // require
