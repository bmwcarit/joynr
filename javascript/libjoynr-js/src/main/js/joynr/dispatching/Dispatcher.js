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
        "joynr/dispatching/Dispatcher",
        [
            "global/Promise",
            "joynr/dispatching/types/Request",
            "joynr/dispatching/types/Reply",
            "joynr/dispatching/types/OneWayRequest",
            "joynr/dispatching/types/BroadcastSubscriptionRequest",
            "joynr/dispatching/types/MulticastSubscriptionRequest",
            "joynr/dispatching/types/SubscriptionRequest",
            "joynr/dispatching/types/SubscriptionReply",
            "joynr/dispatching/types/SubscriptionStop",
            "joynr/dispatching/types/SubscriptionPublication",
            "joynr/dispatching/types/MulticastPublication",
            "joynr/messaging/JoynrMessage",
            "joynr/messaging/MessagingQosEffort",
            "joynr/messaging/inprocess/InProcessAddress",
            "joynr/start/settings/defaultMessagingSettings",
            "joynr/system/DiagnosticTags",
            "joynr/util/UtilInternal",
            "joynr/util/JSONSerializer",
            "joynr/util/Typing",
            "joynr/proxy/SubscriptionQos",
            "joynr/system/LoggerFactory"
        ],
        function(
                Promise,
                Request,
                Reply,
                OneWayRequest,
                BroadcastSubscriptionRequest,
                MulticastSubscriptionRequest,
                SubscriptionRequest,
                SubscriptionReply,
                SubscriptionStop,
                SubscriptionPublication,
                MulticastPublication,
                JoynrMessage,
                MessagingQosEffort,
                InProcessAddress,
                defaultMessagingSettings,
                DiagnosticTags,
                Util,
                JSONSerializer,
                Typing,
                SubscriptionQos,
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
            function Dispatcher(clusterControllerMessagingStub, securityManager, ttlUpLiftMs) {
                var log = LoggerFactory.getLogger("joynr.dispatching.Dispatcher");

                var requestReplyManager;
                var subscriptionManager;
                var publicationManager;
                var messageRouter;

                /**
                 * @name Dispatcher#upLiftTtl
                 * @function
                 * @private
                 *
                 * @param {expiryDate}
                 *            the expiry date in milliseconds
                 * @returns the expiry date with TTL_UPLIFT added as time delta
                 *
                 */
                function upLiftTtl(expiryDate) {
                    expiryDate +=
                            (ttlUpLiftMs !== undefined)
                                    ? ttlUpLiftMs
                                    : defaultMessagingSettings.TTL_UPLIFT;
                    if (expiryDate > Util.getMaxLongValue()) {
                        expiryDate = Util.getMaxLongValue();
                    }

                    return expiryDate;
                }

                /**
                 * @name Dispatcher#upLiftExpiryDateInSubscriptionRequest
                 * @function
                 * @private
                 *
                 * @param {subscriptionRequest}
                 *            the subscription request
                 * @returns the subscription request with qos.expiry date
                 *          with TTL_UPLIFT added as time delta
                 */
                function upLiftExpiryDateInSubscriptionRequest(subscriptionRequest) {
                    // if expiryDateMs == SubscriptionQos.NO_EXPIRY_DATE (=0), expiryDateMs must not be changed
                    if (subscriptionRequest.qos.expiryDateMs !== SubscriptionQos.NO_EXPIRY_DATE) {
                        subscriptionRequest.qos.expiryDateMs =
                                upLiftTtl(subscriptionRequest.qos.expiryDateMs);
                    }
                    return subscriptionRequest;
                }

                /**
                 * @name Dispatcher#parsePayload
                 * @function
                 * @private
                 *
                 * @param {Object}
                 *            joynrMessage the joynr message
                 * @returns payload if the payload is parsable JSON, this is parsed and returned as as an object; otherwise the payload itself
                 *          is returned
                 *
                 */
                function parsePayload(joynrMessage) {
                    try {
                        return JSON.parse(joynrMessage.payload);
                    } catch (e) {
                        // TODO handle error properly
                        log.error("error while deserializing: " + e);
                    }

                    // TODO: handle errors correctly with respect to return value
                    return joynrMessage.payload;
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
                 * @param {DiscoveryEntryWithMetaInfo}
                 *            settings.toDiscoveryEntry DiscoveryEntry of the receiver
                 * @param {MessagingQos}
                 *            settings.messagingQos the messaging Qos object for the ttl
                 */
                function sendJoynrMessage(joynrMessage, settings) {
                    // set headers
                    joynrMessage.creator = securityManager.getCurrentProcessUserId();
                    joynrMessage.from = settings.from;
                    joynrMessage.to = settings.toDiscoveryEntry.participantId;
                    joynrMessage.expiryDate =
                            upLiftTtl(Date.now() + settings.messagingQos.ttl).toString();
                    var effort = settings.messagingQos.effort;
                    if (effort !== MessagingQosEffort.NORMAL) {
                        joynrMessage.effort = effort.value;
                    }
                    if (settings.messagingQos.compress === true) {
                        joynrMessage.setCompress(true);
                    }

                    joynrMessage.setIsLocalMessage(settings.toDiscoveryEntry.isLocal);
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
                 * @name Dispatcher#registerMessageRouter
                 * @function
                 *
                 * @param {MessageRouter}
                 *            newMessageRouter
                 *
                 */
                this.registerMessageRouter = function registerMessageRouter(newMessageRouter) {
                    messageRouter = newMessageRouter;
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
                 * @param {DiscoveryEntryWithMetaInfo}
                 *            settings.toDiscoveryEntry DiscoveryEntry of the receiver
                 * @param {MessagingQos}
                 *            settings.messagingQos the messaging Qos object for the ttl
                 * @param {Request}
                 *            settings.request
                 * @returns {Object} A+ promise object
                 */
                this.sendRequest =
                        function sendRequest(settings) {
                            // Create a JoynrMessage with the Request
                            var requestMessage = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                                payload : JSONSerializer.stringify(settings.request)
                            });
                            if (settings.messagingQos.customHeaders) {
                                requestMessage
                                        .setCustomHeaders(settings.messagingQos.customHeaders);
                            }

                            log.info("calling " + settings.request.methodName + ".", DiagnosticTags
                                    .forRequest({
                                        request : settings.request,
                                        to : settings.toDiscoveryEntry.participantId,
                                        from : settings.from
                                    }));

                            return sendJoynrMessage(requestMessage, settings);
                        };

                /**
                 * @name Dispatcher#sendOneWayRequest
                 * @function
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {DiscoveryEntryWithMetaInfo}
                 *            settings.toDiscoveryEntry DiscoveryEntry of the receiver
                 * @param {MessagingQos}
                 *            settings.messagingQos the messaging Qos object for the ttl
                 * @param {OneWayRequest}
                 *            settings.request
                 * @returns {Object} A+ promise object
                 */
                this.sendOneWayRequest =
                        function sendOneWayRequest(settings) {
                            // Create a JoynrMessage with the OneWayRequest
                            var oneWayRequestMessage = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY,
                                payload : JSONSerializer.stringify(settings.request)
                            });
                            if (settings.messagingQos.customHeaders) {
                                oneWayRequestMessage
                                        .setCustomHeaders(settings.messagingQos.customHeaders);
                            }
                            log.info("calling " + settings.request.methodName + ".", DiagnosticTags
                                    .forOneWayRequest({
                                        request : settings.request,
                                        to : settings.toDiscoveryEntry.participantId,
                                        from : settings.from
                                    }));

                            return sendJoynrMessage(oneWayRequestMessage, settings);
                        };

                function getJoynrMessageType(subscriptionRequest) {
                    var type = Typing.getObjectType(subscriptionRequest);
                    switch (type) {
                        case "BroadcastSubscriptionRequest":
                            return JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
                        case "MulticastSubscriptionRequest":
                            return JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST;
                        default:
                            return JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST;
                    }
                }
                /**
                 * @name Dispatcher#sendSubscriptionRequest
                 * @function
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {DiscoveryEntryWithMetaInfo}
                 *            settings.toDiscoveryEntry DiscoveryEntry of the receiver
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
                                        to : settings.toDiscoveryEntry.participantId,
                                        from : settings.from
                                    }));

                            var requestMessage = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
                                payload : JSONSerializer.stringify(settings.subscriptionRequest)
                            });

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
                 * @param {DiscoveryEntryWithMetaInfo}
                 *            settings.toDiscoveryEntry DiscoveryEntry of the receiver
                 * @param {MessagingQos}
                 *            settings.messagingQos the messaging Qos object for the ttl
                 * @param {BroadcastSubscriptionRequest}
                 *            settings.subscriptionRequest
                 * @returns {Object}  promise object that is resolved when the request is sent by the messaging stub
                 */
                this.sendBroadcastSubscriptionRequest =
                        function sendBroadcastSubscriptionRequest(settings) {
                            var type = getJoynrMessageType(settings.subscriptionRequest);

                            var requestMessage = new JoynrMessage({
                                type : type,
                                payload : JSONSerializer.stringify(settings.subscriptionRequest)
                            });

                            if (type === JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST) {
                                log.info("broadcast subscription to "
                                    + settings.subscriptionRequest.subscribedToName, DiagnosticTags
                                        .forMulticastSubscriptionRequest({
                                            subscriptionRequest : settings.subscriptionRequest,
                                            to : settings.toDiscoveryEntry.participantId,
                                            from : settings.from
                                        }));
                                if (messageRouter !== undefined) {
                                    messageRouter
                                            .addMulticastReceiver({
                                                multicastId : settings.subscriptionRequest.multicastId,
                                                subscriberParticipantId : settings.from,
                                                providerParticipantId : settings.toDiscoveryEntry.participantId
                                            });
                                }
                            } else {
                                log.info("broadcast subscription to "
                                    + settings.subscriptionRequest.subscribedToName, DiagnosticTags
                                        .forBroadcastSubscriptionRequest({
                                            subscriptionRequest : settings.subscriptionRequest,
                                            to : settings.toDiscoveryEntry.participantId,
                                            from : settings.from
                                        }));

                            }
                            return sendJoynrMessage(requestMessage, settings);
                        };

                /**
                 * @name Dispatcher#sendMulticastSubscriptionStop
                 * @function
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {DiscoveryEntryWithMetaInfo}
                 *            settings.toDiscoveryEntry DiscoveryEntry of the receiver
                 * @param {String}
                 *            settings.multicastId of the multicast
                 * @param {SubscriptionStop}
                 *            settings.subscriptionStop
                 * @param {MessagingQos}
                 *            settings.messagingQos the messaging Qos object for the ttl
                 * @returns {Object} A+ promise object
                 */
                this.sendMulticastSubscriptionStop =
                        function sendMulticastSubscriptionStop(settings) {
                            var result = this.sendSubscriptionStop(settings);
                            if (messageRouter !== undefined) {
                                messageRouter.removeMulticastReceiver({
                                    multicastId : settings.multicastId,
                                    subscriberParticipantId : settings.from,
                                    providerParticipantId : settings.toDiscoveryEntry.participantId
                                });
                            }
                            return result;
                        };

                /**
                 * @name Dispatcher#sendSubscriptionStop
                 * @function
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {DiscoveryEntryWithMetaInfo}
                 *            settings.toDiscoveryEntry DiscoveryEntry of the receiver
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
                                        to : settings.toDiscoveryEntry.participantId,
                                        from : settings.from
                                    }));

                            var message = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP,
                                payload : JSONSerializer.stringify(settings.subscriptionStop)
                            });
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
                 * @param {Object}
                 *            settings.customHeaders custom headers from request
                 * @param {Reply|SubscriptionReply}
                 *            settings.reply the reply to be transmitted. It can either be a Reply or a SubscriptionReply object
                 */
                function sendReply(settings) {
                    // reply with the result in a JoynrMessage
                    var joynrMessage = new JoynrMessage({
                        type : settings.messageType,
                        payload : settings.reply,
                        header : {
                            from : settings.from,
                            to : settings.to,
                            expiryDate : settings.expiryDate
                        }
                    });

                    // set custom headers
                    joynrMessage.setCustomHeaders(settings.customHeaders);

                    clusterControllerMessagingStub.transmit(joynrMessage);
                }
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
                 * @param {Object}
                 *            settings.customHeaders custom headers from request
                 * @param {Reply}
                 *            reply
                 */
                function sendRequestReply(settings, reply) {
                    log.info("replying", DiagnosticTags.forReply({
                        reply : reply,
                        to : settings.to,
                        from : settings.from
                    }));

                    settings.reply = JSONSerializer.stringify(reply, reply.error !== undefined);
                    settings.messageType = JoynrMessage.JOYNRMESSAGE_TYPE_REPLY;
                    sendReply(settings);
                }
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
                 * @param {Object}
                 *            settings.customHeaders custom headers from request
                 * @param {SubscriptionReply}
                 *            subscriptionReply
                 */
                function sendSubscriptionReply(settings, subscriptionReply) {
                    log.info("replying", DiagnosticTags.forSubscriptionReply({
                        subscriptionReply : subscriptionReply,
                        to : settings.to,
                        from : settings.from
                    }));

                    settings.reply =
                            JSONSerializer.stringify(
                                    subscriptionReply,
                                    subscriptionReply.error !== undefined);
                    settings.messageType = JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY;
                    sendReply(settings);
                }

                function sendPublicationInternal(settings, type, publication) {
                    // create JoynrMessage for the publication
                    var publicationMessage = new JoynrMessage({
                        type : type,
                        payload : JSONSerializer.stringify(publication)
                    });

                    // set reply headers
                    publicationMessage.from = settings.from;
                    publicationMessage.to = settings.to;
                    publicationMessage.expiryDate = upLiftTtl(settings.expiryDate).toString();

                    clusterControllerMessagingStub.transmit(publicationMessage);
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
                 *            [publication.response]
                 * @param {?}
                 *            [publication.error]
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

                            sendPublicationInternal(
                                    settings,
                                    JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION,
                                    publication);
                        };

                /**
                 * @name Dispatcher#sendMulticastPublication
                 * @function
                 *
                 * @param {Object}
                 *            settings
                 * @param {String}
                 *            settings.from participantId of the sender
                 * @param {String}
                 *            settings.multicastName name of multicast
                 * @param {String[]}
                 *            partitions partitions for this multicast
                 * @param {Number}
                 *            settings.expiryDate time-to-live
                 * @param {SubscriptionPublication}
                 *            publication
                 * @param {?}
                 *            [publication.response]
                 * @param {?}
                 *            [publication.error]
                 * @param {String}
                 *            publication.multicastId
                 *
                 */
                this.sendMulticastPublication =
                        function sendMulticastPublication(settings, publication) {
                            log.info("publication", DiagnosticTags.forMulticastPublication({
                                publication : publication,
                                from : settings.from
                            }));

                            // Reply with the result in a JoynrMessage
                            var publicationMessage = new JoynrMessage({
                                type : JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST,
                                payload : JSONSerializer.stringify(publication),
                                header : {
                                    from : settings.from,
                                    to : publication.multicastId,
                                    expiryDate : upLiftTtl(settings.expiryDate).toString()
                                }
                            });

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
                            log.debug("received message with id \"" + joynrMessage.msgId + "\"");
                            switch (joynrMessage.type) {

                                case JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST:
                                    try {
                                        var request = new Request(parsePayload(joynrMessage));

                                        requestReplyManager.handleRequest(
                                                joynrMessage.to,
                                                request,
                                                function(reply) {
                                                    sendRequestReply({
                                                        from : joynrMessage.to,
                                                        to : joynrMessage.from,
                                                        expiryDate : joynrMessage.expiryDate,
                                                        customHeaders : joynrMessage
                                                                .getCustomHeaders()
                                                    }, reply);
                                                });
                                    } catch (errorInRequest) {
                                        // TODO handle error in handling the request
                                        log.error("error handling request: " + errorInRequest);
                                    }
                                    break;

                                case JoynrMessage.JOYNRMESSAGE_TYPE_REPLY:
                                    try {
                                        requestReplyManager.handleReply(new Reply(
                                                parsePayload(joynrMessage)));
                                    } catch (errorInReply) {
                                        // TODO handle error in handling the reply
                                        log.error("error handling reply: " + errorInReply);
                                    }
                                    break;

                                case JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY:
                                    try {
                                        requestReplyManager.handleOneWayRequest(
                                                joynrMessage.to,
                                                new OneWayRequest(parsePayload(joynrMessage)));
                                    } catch (errorInOneWayRequest) {
                                        log
                                                .error("error handling one way: "
                                                    + errorInOneWayRequest);
                                    }
                                    break;

                                case JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST:
                                    try {
                                        publicationManager
                                                .handleSubscriptionRequest(
                                                        joynrMessage.from,
                                                        joynrMessage.to,
                                                        upLiftExpiryDateInSubscriptionRequest(new SubscriptionRequest(
                                                                parsePayload(joynrMessage))),
                                                        function(subscriptionReply) {
                                                            sendSubscriptionReply(
                                                                    {
                                                                        from : joynrMessage.to,
                                                                        to : joynrMessage.from,
                                                                        expiryDate : joynrMessage.expiryDate,
                                                                        customHeaders : joynrMessage
                                                                                .getCustomHeaders()
                                                                    },
                                                                    subscriptionReply);
                                                        });
                                    } catch (errorInSubscriptionRequest) {
                                        // TODO handle error in handling the subscriptionRequest
                                        log.error("error handling subscriptionRequest: "
                                            + errorInSubscriptionRequest);
                                    }
                                    break;

                                case JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST:
                                    try {
                                        publicationManager
                                                .handleBroadcastSubscriptionRequest(
                                                        joynrMessage.from,
                                                        joynrMessage.to,
                                                        upLiftExpiryDateInSubscriptionRequest(new BroadcastSubscriptionRequest(
                                                                parsePayload(joynrMessage))),
                                                        function(subscriptionReply) {
                                                            sendSubscriptionReply(
                                                                    {
                                                                        from : joynrMessage.to,
                                                                        to : joynrMessage.from,
                                                                        expiryDate : joynrMessage.expiryDate,
                                                                        customHeaders : joynrMessage
                                                                                .getCustomHeaders()
                                                                    },
                                                                    subscriptionReply);
                                                        });
                                    } catch (errorInBroadcastSubscriptionRequest) {
                                        // TODO handle error in handling the subscriptionRequest
                                        log.error("error handling broadcastSubscriptionRequest: "
                                            + errorInBroadcastSubscriptionRequest);
                                    }
                                    break;

                                case JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST:
                                    try {
                                        publicationManager
                                                .handleMulticastSubscriptionRequest(
                                                        joynrMessage.from,
                                                        joynrMessage.to,
                                                        upLiftExpiryDateInSubscriptionRequest(new MulticastSubscriptionRequest(
                                                                parsePayload(joynrMessage))),
                                                        function(subscriptionReply) {
                                                            sendSubscriptionReply(
                                                                    {
                                                                        from : joynrMessage.to,
                                                                        to : joynrMessage.from,
                                                                        expiryDate : joynrMessage.expiryDate,
                                                                        customHeaders : joynrMessage
                                                                                .getCustomHeaders()
                                                                    },
                                                                    subscriptionReply);
                                                        });
                                    } catch (errorInMulticastSubscriptionRequest) {
                                        // TODO handle error in handling the subscriptionRequest
                                        log.error("error handling multicastSubscriptionRequest: "
                                            + errorInMulticastSubscriptionRequest);
                                    }
                                    break;

                                case JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY:
                                    try {
                                        subscriptionManager
                                                .handleSubscriptionReply(new SubscriptionReply(
                                                        parsePayload(joynrMessage)));
                                    } catch (errorInSubscriptionReply) {
                                        // TODO handle error in handling the subscriptionReply
                                        log.error("error handling subscriptionReply: "
                                            + errorInSubscriptionReply);
                                    }
                                    break;

                                case JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP:
                                    try {
                                        publicationManager
                                                .handleSubscriptionStop(new SubscriptionStop(
                                                        parsePayload(joynrMessage)));
                                    } catch (errorInSubscriptionStop) {
                                        // TODO handle error in handling the subscriptionStop
                                        log.error("error handling subscriptionStop: "
                                            + errorInSubscriptionStop);
                                    }
                                    break;

                                case JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION:
                                    try {
                                        subscriptionManager
                                                .handlePublication(new SubscriptionPublication(
                                                        parsePayload(joynrMessage)));
                                    } catch (errorInPublication) {
                                        // TODO handle error in handling the publication
                                        log.error("error handling publication: "
                                            + errorInPublication);
                                    }
                                    break;

                                case JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST:
                                    try {
                                        subscriptionManager
                                                .handleMulticastPublication(new MulticastPublication(
                                                        parsePayload(joynrMessage)));
                                    } catch (errorInMulticastPublication) {
                                        // TODO handle error in handling the multicast publication
                                        log.error("error handling multicast publication: "
                                            + errorInMulticastPublication);
                                    }
                                    break;

                                default:
                                    log.error("unknown JoynrMessage type : "
                                        + joynrMessage.type
                                        + ". Discarding message: "
                                        // TODO the js formatter is breaking this way, and jslint is
                                        // complaining.....
                                        + JSONSerializer.stringify(joynrMessage));
                                    break;
                            }
                            return Promise.resolve();
                        };
                /**
                 * Shutdown the dispatcher
                 *
                 * @function
                 * @name dispatcher#shutdown
                 */
                this.shutdown = function shutdown() {
                    log.debug("Dispatcher shut down");
                    /* do nothing, as either the managers on the layer above (RRM, PM, SM) or
                     * the message router on the layer below are implementing the
                     * correct handling when the runtime is shut down
                     */

                };
            }

            return Dispatcher;

        });
