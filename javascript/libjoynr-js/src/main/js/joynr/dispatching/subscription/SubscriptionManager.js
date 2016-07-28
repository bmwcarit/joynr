/*jslint es5: true */

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

define("joynr/dispatching/subscription/SubscriptionManager", [
    "global/Promise",
    "joynr/messaging/MessagingQos",
    "joynr/start/settings/defaultMessagingSettings",
    "joynr/proxy/SubscriptionQos",
    "joynr/dispatching/types/SubscriptionStop",
    "joynr/dispatching/types/SubscriptionRequest",
    "joynr/dispatching/types/BroadcastSubscriptionRequest",
    "joynr/dispatching/subscription/SubscriptionListener",
    "joynr/util/LongTimer",
    "joynr/system/LoggerFactory",
    "uuid",
    "joynr/util/UtilInternal",
    "joynr/util/Typing",
    "joynr/types/TypeRegistrySingleton",
    "joynr/exceptions/PublicationMissedException",
    "joynr/util/JSONSerializer"
], function(
        Promise,
        MessagingQos,
        defaultMessagingSettings,
        SubscriptionQos,
        SubscriptionStop,
        SubscriptionRequest,
        BroadcastSubscriptionRequest,
        SubscriptionListener,
        LongTimer,
        LoggerFactory,
        uuid,
        Util,
        Typing,
        TypeRegistrySingleton,
        PublicationMissedException,
        JSONSerializer) {
    /**
     * @name SubscriptionManager
     * @constructor
     * @param {Dispatcher}
     *            dispatcher
     */
    function SubscriptionManager(dispatcher) {
        var log = LoggerFactory.getLogger("joynr.dispatching.subscription.SubscriptionManager");
        var typeRegistry = TypeRegistrySingleton.getInstance();
        if (!(this instanceof SubscriptionManager)) {
            // in case someone calls constructor without new keyword (e.g. var c =
            // Constructor({..}))
            return new SubscriptionManager(dispatcher);
        }

        // stores subscriptionId - SubscriptionInfo pairs
        var subscriptionInfos = {};
        // stores subscriptionId - SubscriptionListener
        var subscriptionListeners = {};
        // stores the object which is returned by setTimeout mapped to the subscriptionId
        var publicationCheckTimerIds = {};
        var subscriptionReplyCallers = {};

        /**
         * @param {String}
         *            subscriptionId Id of the subscription to check
         * @returns {Number} time of last received publication
         */
        function getLastPublicationTime(subscriptionId) {
            return subscriptionInfos[subscriptionId].lastPublicationTime_ms;
        }

        function setLastPublicationTime(subscriptionId, time_ms) {
            subscriptionInfos[subscriptionId].lastPublicationTime_ms = time_ms;
        }

        /**
         * @param {String}
         *            subscriptionId Id of the subscription to check
         * @param {Number}
         *            delay_ms Delay to the next publication check.
         * @returns {Boolean} true if subscription is expired, false if end date is not reached.
         */
        function subscriptionEnds(subscriptionId, delay_ms) {
            if (subscriptionInfos[subscriptionId] === undefined) {
                log.warn("subscriptionEnds has been called with unresolved subscriptionId \""
                    + subscriptionId
                    + "\"");
                return true;
            }
            var expiryDateMs = subscriptionInfos[subscriptionId].qos.expiryDateMs;
            // log.debug("Checking subscription end for subscriptionId: " + subscriptionId + "
            // expiryDateMs: " + expiryDateMs + "
            // current time: " + Date.now());
            var ends = expiryDateMs <= Date.now() + delay_ms;
            // if (ends === true) {
            // log.info("Subscription end date reached for id: " + subscriptionId);
            // }
            return ends;
        }

        /**
         * @param {String}
         *            subscriptionId Id of the subscription to check
         * @param {Number}
         *            alertAfterIntervalMs maximum delay between two incoming publications
         */
        function checkPublication(subscriptionId, alertAfterIntervalMs) {
            var subscriptionListener = subscriptionListeners[subscriptionId];
            var timeSinceLastPublication = Date.now() - getLastPublicationTime(subscriptionId);
            // log.debug("timeSinceLastPublication : " + timeSinceLastPublication + "
            // alertAfterIntervalMs: " + alertAfterIntervalMs);
            if (alertAfterIntervalMs > 0 && timeSinceLastPublication >= alertAfterIntervalMs) {
                // log.warn("publication missed for subscription id: " + subscriptionId);
                if (subscriptionListener.onError) {
                    var publicationMissedException = new PublicationMissedException({
                        detailMessage : "alertAfterIntervalMs period exceeded without receiving publication",
                        subscriptionId : subscriptionId
                    });
                    subscriptionListener.onError(publicationMissedException);
                }
            }

            var delay_ms;
            if (timeSinceLastPublication > alertAfterIntervalMs) {
                delay_ms = alertAfterIntervalMs;
            } else {
                delay_ms = alertAfterIntervalMs - timeSinceLastPublication;
            }
            if (!subscriptionEnds(subscriptionId, delay_ms)) {

                // log.debug("Rescheduling checkPublication with delay: " + delay_ms);
                publicationCheckTimerIds[subscriptionId] =
                        LongTimer.setTimeout(function checkPublicationDelay() {
                            checkPublication(subscriptionId, alertAfterIntervalMs);
                        }, delay_ms);
            }
        }

        function calculateTtl(subscriptionQos) {
            var ttl;
            if (subscriptionQos.expiryDateMs === SubscriptionQos.NO_EXPIRY_DATE) {
                return defaultMessagingSettings.MAX_MESSAGING_TTL_MS;
            }
            ttl = subscriptionQos.expiryDateMs - Date.now();
            if (ttl > defaultMessagingSettings.MAX_MESSAGING_TTL_MS) {
                return defaultMessagingSettings.MAX_MESSAGING_TTL_MS;
            }
            return ttl;
        }

        function storeSubscriptionRequest(settings, subscriptionRequest) {
            var onReceiveWrapper;
            if (settings.attributeType !== undefined) {
                onReceiveWrapper = function(response) {
                    settings.onReceive(Typing.augmentTypes(response[0], typeRegistry, settings.attributeType));
                };
            } else {
                onReceiveWrapper = function(response) {
                    var responseKey;
                    for (responseKey in response) {
                        if (response.hasOwnProperty(responseKey)) {
                            response[responseKey] = Typing.augmentTypes(response[responseKey],
                                                                        typeRegistry,
                                                                        settings.broadcastParameter[responseKey].type);
                        }
                    }
                    settings.onReceive(response);
                };
            }
            subscriptionListeners[subscriptionRequest.subscriptionId] = new SubscriptionListener({
                onReceive : onReceiveWrapper,
                onError : settings.onError,
                onSubscribed : settings.onSubscribed
            });
            var subscriptionInfo = Util.extend({
                proxyId : settings.proxyId,
                providerId : settings.providerId,
                lastPublicationTime_ms : 0
            }, subscriptionRequest);

            subscriptionInfos[subscriptionRequest.subscriptionId] = subscriptionInfo;

            var alertAfterIntervalMs = subscriptionRequest.qos.alertAfterIntervalMs;
            if (alertAfterIntervalMs !== undefined && alertAfterIntervalMs > 0) {
                publicationCheckTimerIds[subscriptionRequest.subscriptionId] =
                        LongTimer.setTimeout(
                                function checkPublicationAlertAfterInterval() {
                                    checkPublication(
                                            subscriptionRequest.subscriptionId,
                                            alertAfterIntervalMs);
                                },
                                alertAfterIntervalMs);
            }

        }

        /**
         * This callback is called when a publication is received
         * @callback SubscriptionManager~onReceive
         * @param {Object} publication received
         */
        /**
         * This callback is called if there was an error with the subscription
         * @callback SubscriptionManager~onError
         * @param {Error} error
         */

        /**
         * @name SubscriptionManager#registerSubscription
         * @function
         * @param {Object}
         *            settings
         * @param {String}
         *            settings.proxyId participantId of the sender
         * @param {String}
         *            settings.providerId participantId of the receiver
         * @param {String}
         *            settings.attributeType the type of the subscribing attribute
         * @param {String}
         *            settings.attributeName the attribute name to subscribe to
         * @param {SubscriptionQos}
         *            [settings.qos] the subscriptionQos
         * @param {String}
         *            settings.subscriptionId optional parameter subscriptionId to reuse a
         *            preexisting identifier for this concrete subscription request
         * @param {SubscriptionManager~onReceive}
         *            settings.onReceive the callback for received publications.
         * @param {SubscriptionManager~onError}
         *            settings.onError the callback for missing publication alerts or when an
         *            error occurs.
         * @param {SubscriptionManager~onSubscribed}
         *            settings.onSubscribed the callback to inform once the subscription request has
         *            been delivered successfully
         * @returns an A promise object which provides the subscription token upon success and
         *          an error upon failure
         */
        this.registerSubscription =
                function registerSubscription(settings) {
                    return new Promise(function(resolve, reject) {
                        var subscriptionId = settings.subscriptionId || uuid();
                        // log.debug("Registering Subscription Id " + subscriptionId);

                        if (settings.onError === undefined){
                            log.warn("Warning: subscription for attribute \"" + settings.attributeName + "\" has been done without error callback function. You will not be informed about missed publications. Please specify the \"onError\" parameter while subscribing!");
                        }
                        if (settings.onReceive === undefined){
                            log.warn("Warning: subscription for attribute \"" + settings.attributeName + "\" has been done without receive callback function. You will not be informed about incoming publications. Please specify the \"onReceive\" parameter while subscribing!");
                        }
                        var subscriptionRequest = new SubscriptionRequest({
                            subscriptionId : subscriptionId,
                            subscribedToName : settings.attributeName,
                            qos : settings.qos
                        });

                        var messagingQos = new MessagingQos({
                            ttl : calculateTtl(subscriptionRequest.qos)
                        });

                        subscriptionReplyCallers[subscriptionId] = {
                            resolve : resolve,
                            reject : reject
                        };

                        dispatcher.sendSubscriptionRequest({
                            from : settings.proxyId,
                            to : settings.providerId,
                            messagingQos : messagingQos,
                            subscriptionRequest : subscriptionRequest
                        }).catch(function(error) {
                            delete subscriptionReplyCallers[subscriptionId];
                            reject(error);
                            return;
                        });

                        storeSubscriptionRequest(settings, subscriptionRequest);
                    });
                };

        /**
         * @name SubscriptionManager#registerBroadcastSubscription
         * @function
         * @param {Object}
         *            parameters
         * @param {String}
         *            parameters.proxyId participantId of the sender
         * @param {String}
         *            parameters.providerId participantId of the receiver
         * @param {String}
         *            parameters.broadcastName the name of the broadcast being subscribed to
         * @param {String[]}
         *            parameters.broadcastParameter the parameter meta information of the broadcast being subscribed to
         * @param {SubscriptionQos}
         *            [parameters.subscriptionQos] the subscriptionQos
         * @param {BroadcastFilterParameters}
         *            [parameters.filterParameters] filter parameters used to indicate interest in
         *            only a subset of broadcasts that might be sent.
         * @param {String}
         *            parameters.subscriptionId optional parameter subscriptionId to reuse a
         *            pre-existing identifier for this concrete subscription request
         * @param {SubscriptionManager~onReceive}
         *            parameters.onReceive is called when a broadcast is received.
         * @param {SubscriptionManager~onError}
         *            parameters.onError is called when an error occurs with the broadcast
         * @param {SubscriptionManager~onSubscribed}
         *            parameters.onSubscribed the callback to inform once the subscription request has
         *            been delivered successfully
         * @returns a promise object which provides the subscription token upon success and an error
         *          upon failure
         */
        this.registerBroadcastSubscription = function(parameters) {
            var messagingQos;

            return new Promise(function(resolve, reject) {
                var subscriptionRequest = new BroadcastSubscriptionRequest({
                    subscriptionId : parameters.subscriptionId || uuid(),
                    subscribedToName : parameters.broadcastName,
                    qos : parameters.subscriptionQos,
                    filterParameters : parameters.filterParameters
                });

                messagingQos = new MessagingQos({
                    ttl : calculateTtl(subscriptionRequest.qos)
                });

                subscriptionReplyCallers[subscriptionRequest.subscriptionId] = {
                    resolve : resolve,
                    reject : reject
                };

                dispatcher.sendBroadcastSubscriptionRequest({
                    from : parameters.proxyId,
                    to : parameters.providerId,
                    messagingQos : messagingQos,
                    subscriptionRequest : subscriptionRequest
                }).catch(function(error) {
                    delete subscriptionReplyCallers[subscriptionRequest.subscriptionId];
                    reject(error);
                    return;
                });
                storeSubscriptionRequest(parameters, subscriptionRequest);
            });
        };

        /**
         * @name SubscriptionManager#handleSubscriptionReply
         * @function
         * @param subscriptionReply
         *            {SubscriptionReply} incoming subscriptionReply
         */
        this.handleSubscriptionReply = function handleSubscriptionReply(subscriptionReply) {
            var subscriptionReplyCaller = subscriptionReplyCallers[subscriptionReply.subscriptionId];
            var subscriptionListener = subscriptionListeners[subscriptionReply.subscriptionId];

            if (subscriptionReplyCaller === undefined && subscriptionListener === undefined) {
                log
                        .error("error handling subscription reply, because subscriptionReplyCaller and subscriptionListener could not be found: "
                            + JSONSerializer.stringify(subscriptionReply, undefined, 4));
                return;
            }

            try {
                if (subscriptionReply.error) {
                    if (!(subscriptionReply.error instanceof Error)) {
                        subscriptionReply.error = Typing.augmentTypes(
                                subscriptionReply.error,
                                typeRegistry);
                    }
                    if (subscriptionReplyCaller !== undefined) {
                        subscriptionReplyCaller.reject(subscriptionReply.error);
                    }
                    if (subscriptionListener !== undefined && subscriptionListener.onError !== undefined) {
                        subscriptionListener.onError(subscriptionReply.error);
                    }
                } else {
                    if (subscriptionReplyCaller !== undefined) {
                        subscriptionReplyCaller.resolve(subscriptionReply.subscriptionId);
                    }
                    if (subscriptionListener !== undefined && subscriptionListener.onSubscribed !== undefined) {
                        subscriptionListener.onSubscribed(subscriptionReply.subscriptionId);
                    }
                }
            } catch (e) {
                log.error("exception thrown during handling subscription reply "
                    + JSONSerializer.stringify(subscriptionReply, undefined, 4)
                    + ":\n"
                    + e.stack);
            }
            delete subscriptionReplyCallers[subscriptionReply.subscriptionId];
        };

        /**
         * @name SubscriptionManager#handlePublication
         * @function
         * @param publication
         *            {SubscriptionPublication} incoming publication
         */
        this.handlePublication =
                function handlePublication(publication) {
                    if (subscriptionInfos[publication.subscriptionId] === undefined) {
                        throw new Error("Publication cannot be handled, as no subscription with "
                                + "subscriptionId " + publication.subscriptionId + " is known.");
                    }
                    setLastPublicationTime(publication.subscriptionId, Date.now());
                    var subscriptionListener = subscriptionListeners[publication.subscriptionId];
                    if (publication.error) {
                        if (subscriptionListener.onError) {
                            subscriptionListener.onError(publication.error);
                        } else {
                            throw new Error("no subscription error handler registered for publication "
                                + JSONSerializer.stringify(publication));
                        }
                    } else if (publication.response) {
                        if (subscriptionListener.onReceive) {
                            subscriptionListener.onReceive(publication.response);
                        } else {
                            throw new Error("no subscription listener registered for publication "
                                + JSONSerializer.stringify(publication));
                        }
                    }
                };

        /**
         * @name SubscriptionManager#unregisterSubscription
         * @function
         * @param {Object}
         *            settings
         * @param {MessagingQos}
         *            settings.messagingQos the messaging Qos object for the ttl
         * @param {String}
         *            settings.subscriptionId of the subscriptionId to stop
         * @returns {Object} A promise object
         */
        this.unregisterSubscription = function unregisterSubscription(settings) {
            var subscriptionInfo = subscriptionInfos[settings.subscriptionId];
            var errorMessage;
            if (subscriptionInfo === undefined) {
                errorMessage = "Cannot find subscription with id: " + settings.subscriptionId;
                log.error(errorMessage);
                return Promise.reject(new Error(errorMessage));
            }

            var subscriptionStop = new SubscriptionStop({
                subscriptionId : settings.subscriptionId
            });

            var promise = dispatcher.sendSubscriptionStop({
                from : subscriptionInfo.proxyId,
                to : subscriptionInfo.providerId,
                messagingQos : settings.messagingQos,
                subscriptionStop : subscriptionStop
            });

            if (publicationCheckTimerIds[settings.subscriptionId] !== undefined) {
                LongTimer.clearTimeout(publicationCheckTimerIds[settings.subscriptionId]);
                delete publicationCheckTimerIds[settings.subscriptionId];
            }

            delete subscriptionInfos[settings.subscriptionId];
            delete subscriptionListeners[settings.subscriptionId];
            if (subscriptionReplyCallers[settings.subscriptionId] !== undefined) {
                delete subscriptionReplyCallers[settings.subscriptionId];
            }

            return promise;
        };

    }

    return SubscriptionManager;
});
