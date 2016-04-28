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
        "joynr/dispatching/Dispatcher",
        [
            "global/Promise",
            "joynr/dispatching/types/Request",
            "joynr/dispatching/types/Reply",
            "joynr/dispatching/types/OneWayRequest",
            "joynr/dispatching/types/SubscriptionRequest",
            "joynr/dispatching/types/SubscriptionReply",
            "joynr/dispatching/types/SubscriptionStop",
            "joynr/dispatching/types/SubscriptionPublication",
            "joynr/messaging/JoynrMessage",
            "joynr/messaging/inprocess/InProcessAddress",
            "joynr/system/DiagnosticTags",
            "joynr/util/UtilInternal",
            "joynr/util/JSONSerializer",
            "joynr/system/LoggerFactory"
        ],
        function(
                Promise,
                Request,
                Reply,
                OneWayRequest,
                SubscriptionRequest,
                SubscriptionReply,
                SubscriptionStop,
                SubscriptionPublication,
                JoynrMessage,
                InProcessAddress,
                DiagnosticTags,
                Util,
                JSONSerializer,
                LoggerFactory) {

            /**
             * @name Dispatcher
             * @constructor
             *
             * @param {MessagingStub}
             *            clusterControllerMessagingStub for sending outgoing joynr messages
             * @param {PlatformSecurityManager}
             *            securityManager for setting the creator user ID header
             */
            function Dispatcher(clusterControllerMessagingStub, securityManager) {
                var log = LoggerFactory.getLogger("joynr.dispatching.Dispatcher");

                var requestReplyManager;
                var subscriptionManager;
                var publicationManager;

                /**
                 * @name Dispatcher#parsePayload
                 * @function
                 * @private
                 *
                 * @param {String}
                 *            payload of a JoynrMessage
                 * @returns payload if the payload is parsable JSON, this is parsed and returned as as an object; otherwise the payload itself
                 *          is returned
                 *
                 */
                function parsePayload(payload) {
                    if (typeof payload !== "string") {
                        // TODO handle error properly
                        log.error("payload is not of type string, cannot deserialize!");
                    }

                    try {
                        return JSON.parse(payload);
                    } catch (e) {
                        // TODO handle error properly
                        log.error("error while deserializing: " + e);
                    }

                    // TODO: handle errors correctly with respect to return value
                    return payload;
                }

                /**
                 * @name Dispatcher#sendJoynrMessage
                 * @function
                 * @private
                 *
                 * @param {JoynrMessage}
                 *            joynrMessage
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {String}
                 *            settings.to participantId of the receiver
                 * @param {MessagingQos}
                 *            settings.messagingQos the messaging Qos object for the ttl
                 */
                function sendJoynrMessage(joynrMessage, settings) {
                    // set headers
                    joynrMessage.creator = securityManager.getCurrentProcessUserId();
                    joynrMessage.from = settings.from;
                    joynrMessage.to = settings.to;
                    var expiryDate = Date.now() + settings.messagingQos.ttl;
                    if (expiryDate > Util.getMaxLongValue()) {
                        expiryDate = Util.getMaxLongValue();
                    }

                    joynrMessage.expiryDate = expiryDate.toString();
                    // send message
                    return clusterControllerMessagingStub.transmit(joynrMessage);
                }

                /**
                 * @name Dispatcher#registerRequestReplyManager
                 * @function
                 *
                 * @param {RequestReplyManager}
                 *            requestReplyManager handles incoming and outgoing requests and replies
                 *
                 */
                this.registerRequestReplyManager =
                        function registerRequestReplyManager(newRequestReplyManager) {
                            requestReplyManager = newRequestReplyManager;
                        };

                /**
                 * @name Dispatcher#registerSubscriptionManager
                 * @function
                 *
                 * @param {SubscriptionManager}
                 *            subscriptionManager sends subscription requests; handles incoming publications and incoming replies to
                 *            subscription requests
                 */
                this.registerSubscriptionManager =
                        function registerSubscriptionManager(newSubscriptionManager) {
                            subscriptionManager = newSubscriptionManager;
                        };

                /**
                 * @name Dispatcher#registerPublicationManager
                 * @function
                 *
                 * @param {PublicationManager}
                 *            publicationManager sends publications; handles incoming subscription start and stop requests
                 */
                this.registerPublicationManager =
                        function registerPublicationManager(newPublicationManager) {
                            publicationManager = newPublicationManager;
                        };

                /**
                 * @name Dispatcher#sendRequest
                 * @function
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {String}
                 *            settings.to participantId of the receiver
                 * @param {MessagingQos}
                 *            settings.messagingQos the messaging Qos object for the ttl
                 * @param {Request}
                 *            settings.request
                 * @returns {Object} A+ promise object
                 */
                this.sendRequest =
                        function sendRequest(settings) {
                            // Create a JoynrMessage with the Request
                            var requestMessage =
                                    new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
                            requestMessage.payload = JSONSerializer.stringify(settings.request);

                            log.info("calling "
                                + settings.request.methodName
                                + ". Request: "
                                + requestMessage.payload, DiagnosticTags.forRequest({
                                request : settings.request,
                                to : settings.to,
                                from : settings.from
                            }));

                            return sendJoynrMessage(requestMessage, settings);
                        };

                /**
                 * @name Dispatcher#sendSubscriptionRequest
                 * @function
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {String}
                 *            settings.to participantId of the receiver
                 * @param {MessagingQos}
                 *            settings.messagingQos the messaging Qos object for the ttl
                 * @param {SubscriptionRequest}
                 *            settings.subscriptionRequest
                 * @returns {Object}  promise object that is resolved when the request is sent by the messaging stub
                 */
                this.sendSubscriptionRequest =
                        function sendSubscriptionRequest(settings) {
                            log.info("subscription to "
                                + settings.subscriptionRequest.subscribedToName, DiagnosticTags
                                    .forSubscriptionRequest({
                                        subscriptionRequest : settings.subscriptionRequest,
                                        to : settings.to,
                                        from : settings.from
                                    }));

                            var requestMessage =
                                    new JoynrMessage(
                                            JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST);
                            // requestMessage.setHeader("subscriptionID", settings.subscriptionRequest.subscriptionId);
                            requestMessage.payload =
                                    JSONSerializer.stringify(settings.subscriptionRequest);

                            return sendJoynrMessage(requestMessage, settings);
                        };

                /**
                 * @name Dispatcher#sendBroadcastSubscriptionRequest
                 * @function
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {String}
                 *            settings.to participantId of the receiver
                 * @param {MessagingQos}
                 *            settings.messagingQos the messaging Qos object for the ttl
                 * @param {BroadcastSubscriptionRequest}
                 *            settings.subscriptionRequest
                 * @returns {Object}  promise object that is resolved when the request is sent by the messaging stub
                 */
                this.sendBroadcastSubscriptionRequest =
                        function sendSubscriptionRequest(settings) {
                            log.info("broadcast subscription to "
                                + settings.subscriptionRequest.subscribedToName, DiagnosticTags
                                    .forSubscriptionRequest({
                                        subscriptionRequest : settings.subscriptionRequest,
                                        to : settings.to,
                                        from : settings.from
                                    }));

                            var requestMessage =
                                    new JoynrMessage(
                                            JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST);
                            // requestMessage.setHeader("subscriptionID", settings.subscriptionRequest.subscriptionId);
                            requestMessage.payload =
                                    JSONSerializer.stringify(settings.subscriptionRequest);

                            return sendJoynrMessage(requestMessage, settings);
                        };
                /**
                 * @name Dispatcher#sendSubscriptionStop
                 * @function
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {String}
                 *            settings.to participantId of the receiver
                 * @param {SubscriptionStop}
                 *            settings.subscriptionStop
                 * @param {MessagingQos}
                 *            settings.messagingQos the messaging Qos object for the ttl
                 * @returns {Object} A+ promise object
                 */
                this.sendSubscriptionStop =
                        function sendSubscriptionStop(settings) {
                            log.info("subscription stop "
                                + settings.subscriptionStop.subscriptionId, DiagnosticTags
                                    .forSubscriptionStop({
                                        subscriptionId : settings.subscriptionStop.subscriptionId,
                                        to : settings.to,
                                        from : settings.from
                                    }));

                            var message =
                                    new JoynrMessage(
                                            JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP);
                            message.payload = JSONSerializer.stringify(settings.subscriptionStop);
                            return sendJoynrMessage(message, settings);
                        };

                /**
                 * @private
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {String}
                 *            settings.to participantId of the receiver
                 * @param {Number}
                 *            settings.expiryDate time-to-live
                 * @param {MessagingQos}
                 *            settings.messagingQos quality-of-service parameters such as time-to-live
                 * @param {String}
                 *            settings.replyChannelId channelId of receiver, as received in request
                 * @param {Reply}
                 *            reply
                 */
                function sendReply(settings, reply) {
                    log.info("replying", DiagnosticTags.forReply({
                        reply : reply,
                        to : settings.to,
                        from : settings.from
                    }));

                    // Reply with the result in a JoynrMessage
                    var replyMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REPLY);

                    // set reply headers
                    replyMessage.from = settings.from;
                    replyMessage.to = settings.to;
                    replyMessage.expiryDate = settings.expiryDate;

                    /*
                     * in case the reply contains an error, do not perform any special string replacement
                     */
                    replyMessage.payload =
                            JSONSerializer.stringify(reply, reply.error !== undefined);
                    clusterControllerMessagingStub.transmit(replyMessage);
                }

                /**
                 * @name Dispatcher#sendPublication
                 * @function
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {String}
                 *            settings.to participantId of the receiver
                 * @param {Number}
                 *            settings.expiryDate time-to-live
                 * @param {SubscriptionPublication}
                 *            publication
                 * @param {?}
                 *            publication.response
                 * @param {String}
                 *            publication.subscriptionId
                 *
                 */
                this.sendPublication =
                        function sendPublication(settings, publication) {
                            log.info("publication", DiagnosticTags.forPublication({
                                publication : publication,
                                to : settings.to,
                                from : settings.from
                            }));

                            // Reply with the result in a JoynrMessage
                            var publicationMessage =
                                    new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION);

                            // set reply headers
                            publicationMessage.from = settings.from;
                            publicationMessage.to = settings.to;
                            publicationMessage.expiryDate = settings.expiryDate;

                            publicationMessage.payload = JSONSerializer.stringify(publication);
                            clusterControllerMessagingStub.transmit(publicationMessage);
                        };

                /**
                 * receives a new JoynrMessage that has to be routed to one of the managers
                 *
                 * @name Dispatcher#receive
                 * @function
                 * @param {JoynrMessage}
                 *            joynrMessage being routed
                 */
                this.receive =
                        function receive(joynrMessage) {
                            return new Promise(
                                    function(resolve, reject) {
                                        log.debug("received message with the following payload: "
                                            + joynrMessage.payload);
                                        switch (joynrMessage.type) {

                                            case JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST:
                                                try {
                                                    var request =
                                                            new Request(
                                                                    parsePayload(joynrMessage.payload));

                                                    requestReplyManager
                                                            .handleRequest(
                                                                    joynrMessage.to,
                                                                    request,
                                                                    function(reply) {
                                                                        sendReply(
                                                                                {
                                                                                    from : joynrMessage.to,
                                                                                    to : joynrMessage.from,
                                                                                    expiryDate : joynrMessage.expiryDate
                                                                                },
                                                                                reply);
                                                                    });
                                                    resolve();
                                                } catch (errorInRequest) {
                                                    // TODO handle error in handling the request
                                                    log.error("error handling request: "
                                                        + errorInRequest);
                                                    reject(new Error("error handling request: "
                                                        + errorInRequest));
                                                }
                                                break;

                                            case JoynrMessage.JOYNRMESSAGE_TYPE_REPLY:
                                                try {
                                                    var settings =
                                                            Util
                                                                    .extend(
                                                                            parsePayload(joynrMessage.payload),
                                                                            {
                                                                                requestReplyId : joynrMessage.requestReplyId
                                                                            });
                                                    requestReplyManager.handleReply(new Reply(
                                                            settings));
                                                    resolve();
                                                } catch (errorInReply) {
                                                    // TODO handle error in handling the reply
                                                    log.error("error handling reply: "
                                                        + errorInReply);
                                                    reject(new Error("error handling reply: "
                                                        + errorInReply));
                                                }
                                                break;

                                            case JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY:
                                                try {
                                                    requestReplyManager.handleOneWayRequest({
                                                        payload : new OneWayRequest(
                                                                parsePayload(joynrMessage.payload))
                                                    });
                                                    resolve();
                                                } catch (errorInOneWayRequest) {
                                                    // TODO do we have to do any error handling on a one way
                                                    // other than log it?
                                                    log.error("error handling one way: "
                                                        + errorInOneWayRequest);
                                                    reject(new Error("error handling one way: "
                                                        + errorInOneWayRequest));
                                                }
                                                break;

                                            case JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST:
                                                try {
                                                    publicationManager
                                                            .handleSubscriptionRequest(
                                                                    joynrMessage.from,
                                                                    joynrMessage.to,
                                                                    new SubscriptionRequest(
                                                                            parsePayload(joynrMessage.payload)));
                                                    resolve();
                                                } catch (errorInSubscriptionRequest) {
                                                    // TODO handle error in handling the subscriptionRequest
                                                    log
                                                            .error("error handling subscriptionRequest: "
                                                                + errorInSubscriptionRequest);
                                                    reject(new Error(
                                                            "error handling subscriptionRequest: "
                                                                + errorInSubscriptionRequest));
                                                }
                                                break;

                                            case JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST:
                                                try {
                                                    publicationManager
                                                            .handleEventSubscriptionRequest(
                                                                    joynrMessage.from,
                                                                    joynrMessage.to,
                                                                    new SubscriptionRequest(
                                                                            parsePayload(joynrMessage.payload)));
                                                    resolve();
                                                } catch (errorInEventSubscriptionRequest) {
                                                    // TODO handle error in handling the subscriptionRequest
                                                    log
                                                            .error("error handling eventSubscriptionRequest: "
                                                                + errorInEventSubscriptionRequest);
                                                    reject(new Error(
                                                            "error handling eventSubscriptionRequest: "
                                                                + errorInEventSubscriptionRequest));
                                                }
                                                break;

                                            case JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY:
                                                try {
                                                    subscriptionManager
                                                            .handleSubscriptionReply(new SubscriptionReply(
                                                                    parsePayload(joynrMessage.payload)));
                                                    resolve();
                                                } catch (errorInSubscriptionReply) {
                                                    // TODO handle error in handling the subscriptionReply
                                                    log.error("error handling subscriptionReply: "
                                                        + errorInSubscriptionReply);
                                                    reject(new Error(
                                                            "error handling subscriptionReply: "
                                                                + errorInSubscriptionReply));
                                                }
                                                break;

                                            case JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP:
                                                try {
                                                    publicationManager
                                                            .handleSubscriptionStop(new SubscriptionStop(
                                                                    parsePayload(joynrMessage.payload)));
                                                    resolve();
                                                } catch (errorInSubscriptionStop) {
                                                    // TODO handle error in handling the subscriptionStop
                                                    log.error("error handling subscriptionStop: "
                                                        + errorInSubscriptionStop);
                                                    reject(new Error(
                                                            "error handling subscriptionStop: "
                                                                + errorInSubscriptionStop));
                                                }
                                                break;

                                            case JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION:
                                                try {
                                                    subscriptionManager
                                                            .handlePublication(new SubscriptionPublication(
                                                                    parsePayload(joynrMessage.payload)));
                                                    resolve();
                                                } catch (errorInPublication) {
                                                    // TODO handle error in handling the publication
                                                    log.error("error handling publication: "
                                                        + errorInPublication);
                                                    reject(new Error("error handling publication: "
                                                        + errorInPublication));
                                                }
                                                break;

                                            default:
                                                log.error("unknown JoynrMessage type: "
                                                    + joynrMessage.type
                                                    + ". Discarding message: "
                                                    // TODO the js formatter is breaking this way, and jslint is
                                                    // complaining.....
                                                    + JSONSerializer.stringify(joynrMessage));
                                                reject(new Error("unknown JoynrMessage type: "
                                                    + joynrMessage.type
                                                    + ". Discarding message: "
                                                    + JSONSerializer.stringify(joynrMessage)));
                                                break;
                                        }
                                    });
                        };
            }

            return Dispatcher;

        });
