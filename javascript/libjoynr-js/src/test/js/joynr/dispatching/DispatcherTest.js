/*global fail: true */
/*jslint es5: true, nomen: true */

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
            "joynr/dispatching/types/SubscriptionRequest",
            "joynr/dispatching/types/SubscriptionReply",
            "joynr/dispatching/types/SubscriptionStop",
            "joynr/dispatching/types/SubscriptionPublication",
            "joynr/tests/testTypes/TestEnum",
            "joynr/types/TypeRegistrySingleton"
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
                SubscriptionRequest,
                SubscriptionReply,
                SubscriptionStop,
                SubscriptionPublication,
                TestEnum,
                TypeRegistrySingleton) {

            var providerId = "providerId";
            var proxyId = "proxyId";

            describe(
                    "libjoynr-js.joynr.dispatching.Dispatcher",
                    function() {

                        var dispatcher, requestReplyManager, subscriptionManager, publicationManager, clusterControllerMessagingStub, securityManager, subscriptionId =
                                "mySubscriptionId", requestReplyId = "requestReplyId";

                        /**
                         * Called before each test.
                         */
                        beforeEach(function(done) {
                            requestReplyManager = jasmine.createSpyObj("RequestReplyManager", [
                                "handleOneWayRequest",
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

                        it(
                                "forwards subscription request to Publication Manager",
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
                                    dispatcher.receive(joynrMessage);
                                    expect(publicationManager.handleSubscriptionRequest)
                                            .toHaveBeenCalled();
                                    expect(publicationManager.handleSubscriptionRequest)
                                            .toHaveBeenCalledWith(
                                                    proxyId,
                                                    providerId,
                                                    new SubscriptionRequest(payload));
                                    done();
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

                        it("forwards publication to Subscription Manager", function(done) {
                            var payload = {
                                subscriptionId : subscriptionId,
                                response : "myResponse"
                            };
                            var joynrMessage = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION,
                                payload : JSON.stringify(payload)
                            });
                            dispatcher.receive(joynrMessage);
                            expect(subscriptionManager.handlePublication).toHaveBeenCalled();
                            expect(subscriptionManager.handlePublication).toHaveBeenCalledWith(
                                    new SubscriptionPublication(payload));
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
                                        to : "to",
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
                                        to : "to",
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
                                        to : "to",
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
                                        to : "to",
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
                    });

        }); // require
