/*global joynrTestRequire: true */

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

joynrTestRequire(
        "joynr/dispatching/TestDispatcher",
        [
            "joynr/dispatching/Dispatcher",
            "joynr/dispatching/subscription/PublicationManager",
            "joynr/dispatching/subscription/SubscriptionManager",
            "joynr/dispatching/RequestReplyManager",
            "joynr/messaging/inprocess/InProcessMessagingStub",
            "joynr/messaging/inprocess/InProcessMessagingSkeleton",
            "joynr/messaging/JoynrMessage",
            "joynr/dispatching/types/Request",
            "joynr/dispatching/types/Reply",
            "joynr/dispatching/types/SubscriptionRequest",
            "joynr/dispatching/types/SubscriptionReply",
            "joynr/dispatching/types/SubscriptionStop",
            "joynr/dispatching/types/SubscriptionPublication"
        ],
        function(
                Dispatcher,
                PublicationManager,
                SubscriptionManager,
                RequestReplyManager,
                InProcessMessagingStub,
                InProcessMessagingSkeleton,
                JoynrMessage,
                Request,
                Reply,
                SubscriptionRequest,
                SubscriptionReply,
                SubscriptionStop,
                SubscriptionPublication) {

            var providerId = "providerId";
            var proxyId = "proxyId";

            describe(
                    "libjoynr-js.joynr.dispatching.Dispatcher",
                    function() {

                        var dispatcher, requestReplyManager, subscriptionManager, publicationManager, clusterControllerMessagingStub, libjoynrMessageReceiver, subscriptionId =
                                "mySubscriptionId", requestReplyId = "requestReplyId";

                        /**
                         * Called before each test.
                         */
                        beforeEach(function() {
                            requestReplyManager = jasmine.createSpyObj("RequestReplyManager", [
                                "handleRequest",
                                "handleReply"
                            ]);
                            subscriptionManager = jasmine.createSpyObj("SubscriptionManager", [
                                "handleSubscriptionReply",
                                "handlePublication"
                            ]);
                            publicationManager = jasmine.createSpyObj("PublicationManager", [
                                "handleSubscriptionRequest",
                                "handleSubscriptionStop"
                            ]);
                            clusterControllerMessagingStub =
                                    jasmine.createSpy("inProcessMessagingStub");

                            libjoynrMessageReceiver =
                                    jasmine.createSpyObj(
                                            "inProcessMessagingReceiver",
                                            [ "registerListener"
                                            ]);

                            dispatcher =
                                    new Dispatcher(
                                            clusterControllerMessagingStub,
                                            libjoynrMessageReceiver);
                            dispatcher.registerRequestReplyManager(requestReplyManager);
                            dispatcher.registerSubscriptionManager(subscriptionManager);
                            dispatcher.registerPublicationManager(publicationManager);
                        });

                        it("is instantiable and of correct type", function() {
                            expect(Dispatcher).toBeDefined();
                            expect(typeof Dispatcher === "function").toBeTruthy();
                            expect(dispatcher).toBeDefined();
                            expect(dispatcher instanceof Dispatcher).toBeTruthy();
                            expect(dispatcher.registerRequestReplyManager).toBeDefined();
                            expect(typeof dispatcher.registerRequestReplyManager === "function")
                                    .toBeTruthy();
                            expect(dispatcher.registerSubscriptionManager).toBeDefined();
                            expect(typeof dispatcher.registerSubscriptionManager === "function")
                                    .toBeTruthy();
                            expect(dispatcher.registerPublicationManager).toBeDefined();
                            expect(typeof dispatcher.registerPublicationManager === "function")
                                    .toBeTruthy();
                            expect(dispatcher.sendRequest).toBeDefined();
                            expect(typeof dispatcher.sendRequest === "function").toBeTruthy();
                            expect(dispatcher.sendSubscriptionRequest).toBeDefined();
                            expect(typeof dispatcher.sendSubscriptionRequest === "function")
                                    .toBeTruthy();
                            expect(dispatcher.sendSubscriptionStop).toBeDefined();
                            expect(typeof dispatcher.sendSubscriptionStop === "function")
                                    .toBeTruthy();
                            expect(dispatcher.sendPublication).toBeDefined();
                            expect(typeof dispatcher.sendPublication === "function").toBeTruthy();
                            expect(dispatcher.receive).toBeDefined();
                            expect(typeof dispatcher.receive === "function").toBeTruthy();
                        });

                        it(
                                "forwards subscription request to Publication Manager",
                                function() {
                                    var payload = {
                                        subscribedToName : "attributeName",
                                        subscriptionId : subscriptionId
                                    };
                                    var joynrMessage =
                                            new JoynrMessage(
                                                    JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST);
                                    joynrMessage.setHeader(
                                            JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID,
                                            proxyId);
                                    joynrMessage.setHeader(
                                            JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID,
                                            providerId);
                                    joynrMessage.payload = JSON.stringify(payload);
                                    dispatcher.receive(joynrMessage);
                                    expect(publicationManager.handleSubscriptionRequest)
                                            .toHaveBeenCalled();
                                    expect(publicationManager.handleSubscriptionRequest)
                                            .toHaveBeenCalledWith(
                                                    proxyId,
                                                    providerId,
                                                    new SubscriptionRequest(payload));
                                });

                        it("forwards subscription reply to Subscription Manager", function() {
                            var payload = {
                                subscriptionId : subscriptionId
                            };
                            var joynrMessage =
                                    new JoynrMessage(
                                            JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY);
                            joynrMessage.payload = JSON.stringify(payload);
                            dispatcher.receive(joynrMessage);
                            expect(subscriptionManager.handleSubscriptionReply).toHaveBeenCalled();
                            expect(subscriptionManager.handleSubscriptionReply)
                                    .toHaveBeenCalledWith(new SubscriptionReply(payload));
                        });

                        it(
                                "forwards subscription stop to SubscriptionPublication Manager",
                                function() {
                                    var payload = {
                                        subscriptionId : subscriptionId
                                    };
                                    var joynrMessage =
                                            new JoynrMessage(
                                                    JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP);
                                    joynrMessage.payload = JSON.stringify(payload);
                                    dispatcher.receive(joynrMessage);
                                    expect(publicationManager.handleSubscriptionStop)
                                            .toHaveBeenCalled();
                                    expect(publicationManager.handleSubscriptionStop)
                                            .toHaveBeenCalledWith(new SubscriptionStop(payload));
                                });

                        it("forwards publication to Subscription Manager", function() {
                            var payload = {
                                subscriptionId : subscriptionId,
                                response : "myResponse"
                            };
                            var joynrMessage =
                                    new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION);
                            joynrMessage.payload = JSON.stringify(payload);
                            dispatcher.receive(joynrMessage);
                            expect(subscriptionManager.handlePublication).toHaveBeenCalled();
                            expect(subscriptionManager.handlePublication).toHaveBeenCalledWith(
                                    new SubscriptionPublication(payload));
                        });

                        it(
                                "forwards request to RequestReply Manager",
                                function() {
                                    var joynrMessage =
                                            new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
                                    joynrMessage.setHeader(
                                            JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID,
                                            providerId);
                                    joynrMessage.setHeader(
                                            JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID,
                                            proxyId);
                                    var request = new Request({
                                        methodName : "methodName"
                                    });
                                    joynrMessage.payload = JSON.stringify(request);
                                    dispatcher.receive(joynrMessage);
                                    expect(requestReplyManager.handleRequest).toHaveBeenCalled();
                                    expect(requestReplyManager.handleRequest.mostRecentCall.args[0])
                                            .toEqual(providerId);
                                    expect(requestReplyManager.handleRequest.mostRecentCall.args[1])
                                            .toEqual(request);
                                    expect(
                                            typeof requestReplyManager.handleRequest.mostRecentCall.args[2] === "function")
                                            .toBeTruthy();
                                });

                        it("forwards reply to RequestReply Manager", function() {
                            var joynrMessage =
                                    new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REPLY);
                            var reply = new Reply({
                                requestReplyId : requestReplyId,
                                response : []
                            });
                            joynrMessage.payload = JSON.stringify(reply);
                            dispatcher.receive(joynrMessage);
                            expect(requestReplyManager.handleReply).toHaveBeenCalled();
                            expect(requestReplyManager.handleReply).toHaveBeenCalledWith(reply);
                        });
                    });

        }); // require
