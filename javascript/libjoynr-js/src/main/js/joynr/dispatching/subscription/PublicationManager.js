/*jslint es5: true, node: true, node: true */
/*global triggerPublicationTimer: true */
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
var Promise = require("../../../global/Promise");
var SubscriptionQos = require("../../proxy/SubscriptionQos");
var PeriodicSubscriptionQos = require("../../proxy/PeriodicSubscriptionQos");
var BroadcastSubscriptionRequest = require("../types/BroadcastSubscriptionRequest");
var SubscriptionRequest = require("../types/SubscriptionRequest");
var MulticastPublication = require("../types/MulticastPublication");
var SubscriptionPublication = require("../types/SubscriptionPublication");
var SubscriptionReply = require("../types/SubscriptionReply");
var SubscriptionStop = require("../types/SubscriptionStop");
var SubscriptionInformation = require("../types/SubscriptionInformation");
var Reply = require("../types/Reply");
var ProviderEvent = require("../../provider/ProviderEvent");
var Typing = require("../../util/Typing");
var SubscriptionUtil = require("./util/SubscriptionUtil");
var SubscriptionException = require("../../exceptions/SubscriptionException");
var ProviderRuntimeException = require("../../exceptions/ProviderRuntimeException");
var JSONSerializer = require("../../util/JSONSerializer");
var LongTimer = require("../../util/LongTimer");
var Util = require("../../util/UtilInternal");
var LoggerFactory = require("../../system/LoggerFactory");

/**
 * The PublicationManager is responsible for handling subscription requests.
 *
 * @name PublicationManager
 * @constructor
 *
 * @param {Dispatcher}
 *            dispatcher
 *
 * @param {String}
 *            joynrInstanceId: the Id of the actual joynr instance
 */
function PublicationManager(dispatcher, persistency, joynrInstanceId) {
    var log = LoggerFactory.getLogger("joynr.dispatching.subscription.PublicationManager");

    // map: key is the provider's participantId, value is the provider object
    var participantIdToProvider = {};

    var that = this;

    var attributeObserverFunctions = {};

    var eventObserverFunctions = {};

    // map: subscriptionId to SubscriptionRequest
    var subscriptionInfos = {};

    // map: subscriptionId to SubscriptionRequest
    var queuedSubscriptionInfos = {};

    // queued subscriptions for deferred providers
    var queuedProviderParticipantIdToSubscriptionRequestsMapping = {};

    // map: providerId+attributeName -> subscriptionIds -> subscription
    var onChangeProviderAttributeToSubscriptions = {};

    // map: providerId+eventName -> subscriptionIds -> subscription
    var onChangeProviderEventToSubscriptions = {};

    var multicastSubscriptions = {};

    var subscriptionPersistenceKey = PublicationManager.SUBSCRIPTIONS_STORAGE_PREFIX + "_" + joynrInstanceId;

    var started = true;

    function isReady() {
        return started;
    }

    /**
     * Stores subscriptions
     * @name PublicationManager#storeSubscriptions
     * @private
     *
     */
    function storeSubscriptions() {
        var item = SubscriptionUtil.serializeSubscriptionIds(subscriptionInfos);
        persistency.setItem(subscriptionPersistenceKey, item);
    }

    /**
     * Helper function to get attribute object based on provider's participantId and
     * attribute name
     * @name PublicationManager#getAttribute
     * @private
     *
     * @param {String}
     *            participantId the participantId of the provider
     * @param {String}
     *            attributeName the attribute name
     * @returns {ProviderAttribute} the provider attribute
     */
    function getAttribute(participantId, attributeName) {
        var provider = participantIdToProvider[participantId];
        return provider[attributeName];
    }

    /**
     * Helper function to get attribute value. In case the attribute getter function of
     * the provider returns a promise object this function waits until the promise object
     * is resolved to resolve the promise object return by this function
     * @name PublicationManager#getAttributeValue
     * @private
     *
     * @param {Object}
     *            subscriptionInfo the subscription information
     * @param {String}
     *            subscriptionInfo.providerParticipantId the participantId of the provider
     * @param {String}
     *            subscriptionInfo.subscribedToName the attribute to be published
     * @returns an A+ promise object which provides attribute value upon success and an
     *            error upon failure
     */
    function getAttributeValue(subscriptionInfo) {
        function promiseCatchHandler(error) {
            if (error instanceof ProviderRuntimeException) {
                throw error;
            }
            throw new ProviderRuntimeException({
                detailMessage: "getter method for attribute " + subscriptionInfo.subscribedToName + " reported an error"
            });
        }

        var attribute = getAttribute(subscriptionInfo.providerParticipantId, subscriptionInfo.subscribedToName),
            value;
        try {
            value = attribute.get();
            if (Util.isPromise(value)) {
                return value.catch(promiseCatchHandler);
            }
        } catch (error) {
            if (error instanceof ProviderRuntimeException) {
                return Promise.reject(error);
            }
            return Promise.reject(
                new ProviderRuntimeException({
                    detailMessage:
                        "getter method for attribute " + subscriptionInfo.subscribedToName + " reported an error"
                })
            );
        }
        return Promise.resolve(value);
    }

    /**
     * @name PublicationManager#sendPublication
     * @private
     */
    function sendPublication(subscriptionInfo, value, exception) {
        log.debug(
            "send Publication for subscriptionId " +
                subscriptionInfo.subscriptionId +
                " and attribute/event " +
                subscriptionInfo.subscribedToName +
                ": " +
                value
        );
        subscriptionInfo.lastPublication = Date.now();
        var subscriptionPublication;

        if (exception) {
            subscriptionPublication = new SubscriptionPublication({
                error: exception,
                subscriptionId: subscriptionInfo.subscriptionId
            });
        } else {
            subscriptionPublication = new SubscriptionPublication({
                response: value,
                subscriptionId: subscriptionInfo.subscriptionId
            });
        }
        dispatcher.sendPublication(
            {
                from: subscriptionInfo.providerParticipantId,
                to: subscriptionInfo.proxyParticipantId,
                expiryDate: Date.now() + subscriptionInfo.qos.publicationTtlMs
            },
            subscriptionPublication
        );
    }

    /**
     * @name PublicationManager#getPeriod
     * @private
     */
    function getPeriod(subscriptionInfo) {
        return subscriptionInfo.qos.maxIntervalMs || subscriptionInfo.qos.periodMs;
    }

    /**
     * @name PublicationManager#prepareAttributePublication
     * @private
     */
    function prepareAttributePublication(subscriptionInfo, value, timer) {
        var timeSinceLastPublication = Date.now() - subscriptionInfo.lastPublication;
        if (
            subscriptionInfo.qos.minIntervalMs === undefined ||
            timeSinceLastPublication >= subscriptionInfo.qos.minIntervalMs
        ) {
            sendPublication(subscriptionInfo, value);
            // if registered interval exists => reschedule it

            if (subscriptionInfo.onChangeDebounce !== undefined) {
                LongTimer.clearTimeout(subscriptionInfo.onChangeDebounce);
                delete subscriptionInfo.onChangeDebounce;
            }

            // if there's an existing interval, clear it and restart
            if (subscriptionInfo.subscriptionInterval !== undefined) {
                LongTimer.clearTimeout(subscriptionInfo.subscriptionInterval);
                subscriptionInfo.subscriptionInterval = timer(subscriptionInfo, getPeriod(subscriptionInfo));
            }
        } else if (subscriptionInfo.onChangeDebounce === undefined) {
            subscriptionInfo.onChangeDebounce = timer(
                subscriptionInfo,
                subscriptionInfo.qos.minIntervalMs - timeSinceLastPublication,
                function() {
                    delete subscriptionInfo.onChangeDebounce;
                }
            );
        }
    }

    /**
     * @name PublicationManager#prepareBroadcastPublication
     * @private
     */
    function prepareBroadcastPublication(subscriptionInfo, value) {
        var timeSinceLastPublication = Date.now() - subscriptionInfo.lastPublication;
        if (
            subscriptionInfo.qos.minIntervalMs === undefined ||
            timeSinceLastPublication >= subscriptionInfo.qos.minIntervalMs
        ) {
            sendPublication(subscriptionInfo, value);
        } else {
            log.info(
                "Two subsequent broadcasts of event " +
                    subscriptionInfo.subscribedToName +
                    " occured within minIntervalMs of subscription with id " +
                    subscriptionInfo.subscriptionId +
                    ". Event will not be sent to the subscribing client."
            );
        }
    }

    function PublicationTimerCallback(subscriptionInfo, callback) {
        if (callback !== undefined) {
            callback();
        }

        function getAttributeValueSuccess(value) {
            prepareAttributePublication(subscriptionInfo, value, triggerPublicationTimer);
            return value;
        }

        function getAttributeValueFailure(exception) {
            sendPublication(subscriptionInfo, undefined, exception);
            return exception;
        }

        getAttributeValue(subscriptionInfo)
            .then(getAttributeValueSuccess)
            .catch(getAttributeValueFailure);
    }

    /**
     * This functions waits the delay time before publishing the value of the attribute
     * specified in the subscription information
     *
     * @name PublicationManager#triggerPublicationTimer
     * @private
     *
     * @param {Object}
     *            subscriptionInfo the subscription information
     * @param {String}
     *            subscriptionInfo.providerParticipantId the participantId of the provider
     * @param {String}
     *            subscriptionInfo.subscribedToName the attribute to be published
     * @param {Number}
     *            delay the delay to wait for the publication
     * @param {Function}
     *            callback to be invoked by this function once the timer has been exceeded
     */
    function triggerPublicationTimer(subscriptionInfo, delay, callback) {
        if (!isNaN(delay)) {
            return LongTimer.setTimeout(PublicationTimerCallback, delay, subscriptionInfo, callback);
            // the last two arguments get to PublicationTimerCallback
        }
        // TODO: what kind of recursive design is this? PublicationTimerCallback calls triggerPublicationTimer on Success ...
        // why not use Intervals?

        // TODO:why is nothing returned here in the else case?
    }

    /**
     * @name PublicationManager#getProviderIdAttributeKey
     * @private
     */
    function getProviderIdAttributeKey(providerId, attributeName) {
        return providerId + "." + attributeName;
    }

    /**
     * @name PublicationManager#getProviderIdEventKey
     * @private
     */
    function getProviderIdEventKey(providerId, eventName) {
        return providerId + "." + eventName;
    }

    /**
     * Gives the list of subscriptions for the given providerId and attribute name
     * @name PublicationManager#getSubscriptionsForProviderAttribute
     * @private
     *
     * @param {String}
     *            providerId
     * @param {String}
     *            attributeName
     * @returns {Array} a list of subscriptions
                  e.g. [{ participantId : "...", attributeName : "..." }]
     */
    function getSubscriptionsForProviderAttribute(providerId, attributeName) {
        var key = getProviderIdAttributeKey(providerId, attributeName);
        // make sure the mapping exists, so that subscriptions can register here
        if (onChangeProviderAttributeToSubscriptions[key] === undefined) {
            onChangeProviderAttributeToSubscriptions[key] = {};
        }
        return onChangeProviderAttributeToSubscriptions[key];
    }

    /**
     * @name PublicationManager#resetSubscriptionsForProviderAttribute
     * @private
     */
    function resetSubscriptionsForProviderAttribute(providerId, attributeName) {
        var key = getProviderIdAttributeKey(providerId, attributeName);
        delete onChangeProviderAttributeToSubscriptions[key];
    }

    /**
     * Checks whether the provider attribute is notifiable (=is even interesting to the
     * PublicationManager)
     * @name PublicationManager#providerAttributeIsNotifiable
     * @private
     *
     * @param {ProviderAttribute}
     *            the provider attribute to check for Notifiability
     * @returns {Boolean} if the provider attribute is notifiable
     */
    function providerAttributeIsNotifiable(providerAttribute) {
        return typeof providerAttribute.isNotifiable === "function" && providerAttribute.isNotifiable();
    }

    /**
     * Checks whether the provider property is an event (=is even interesting to the
     * PublicationManager)
     * @name PublicationManager#propertyIsProviderEvent
     * @private
     *
     * @param {providerProperty}
     *            the provider attribute to check for Notifiability
     * @returns {Boolean} if the provider attribute is notifiable
     */
    function propertyIsProviderEvent(providerProperty) {
        return providerProperty instanceof ProviderEvent;
    }

    /**
     * @name PublicationManager#publishAttributeValue
     * @private
     * @param {String}
     *            providerId
     * @param {String}
     *            attributeName
     * @param {ProviderAttribute}
     *            attribute
     * @param {?}
     *            value
     */
    function publishAttributeValue(providerId, attributeName, attribute, value) {
        if (!isReady()) {
            throw new Error(
                'attribute publication for providerId "' +
                    providerId +
                    "and attribute " +
                    attributeName +
                    " is not forwarded to subscribers, as the publication manager is " +
                    "already shut down"
            );
        }
        var subscriptionId,
            subscriptions = getSubscriptionsForProviderAttribute(providerId, attributeName);
        if (!subscriptions) {
            log.error(
                "ProviderAttribute " +
                    attributeName +
                    " for providerId " +
                    providerId +
                    " is not registered or notifiable"
            );
            // TODO: proper error handling for empty subscription map =>
            // ProviderAttribute is not notifiable or not registered
            return;
        }

        for (subscriptionId in subscriptions) {
            if (subscriptions.hasOwnProperty(subscriptionId)) {
                var subscriptionInfo = subscriptions[subscriptionId];
                prepareAttributePublication(subscriptionInfo, value, triggerPublicationTimer);
            }
        }
    }

    /**
     * Adds a notifiable provider attribute to the internal map if it does not exist =>
     * onChange subscriptions can be registered here
     *
     * @name PublicationManager#addPublicationAttribute
     * @private
     *
     * @param {String}
     *            providerId
     * @param {String}
     *            attributeName
     */
    function addPublicationAttribute(providerId, attributeName, attribute) {
        var key = getProviderIdAttributeKey(providerId, attributeName);

        attributeObserverFunctions[key] = function(value) {
            publishAttributeValue(providerId, attributeName, attribute, value);
        };
        attribute.registerObserver(attributeObserverFunctions[key]);
    }

    // Broadcast specific implementation

    /**
     * Gives the list of subscriptions for the given providerId and event name
     * @name PublicationManager#getSubscriptionsForProviderEvent
     * @private
     *
     * @param {String}
     *            providerId
     * @param {String}
     *            eventName
     * @returns {Array} a list of subscriptions
                  e.g. [{ participantId : "...", eventName : "..." }]
     */
    function getSubscriptionsForProviderEvent(providerId, eventName) {
        var key = getProviderIdEventKey(providerId, eventName);
        // make sure the mapping exists, so that subscriptions can register here
        if (onChangeProviderEventToSubscriptions[key] === undefined) {
            onChangeProviderEventToSubscriptions[key] = {};
        }
        return onChangeProviderEventToSubscriptions[key];
    }

    function prepareMulticastPublication(providerId, eventName, partitions, outputParameters) {
        var multicastId = SubscriptionUtil.createMulticastId(providerId, eventName, partitions);
        var publication = new MulticastPublication({
            response: outputParameters,
            multicastId: multicastId
        });
        dispatcher.sendMulticastPublication(
            {
                from: providerId,
                expiryDate: Date.now() + SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS //TODO: what should be the ttl?
            },
            publication
        );
    }

    /**
     * @name PublicationManager#publishEventValue
     * @private
     * @param {String}
     *            providerId
     * @param {String}
     *            eventName
     * @param {ProviderEvent}
     *            event
     * @param {?}
     *            value
     */
    function publishEventValue(providerId, eventName, event, data) {
        var value = data.broadcastOutputParameters;
        if (!isReady()) {
            throw new Error(
                'event publication for providerId "' +
                    providerId +
                    "and eventName " +
                    eventName +
                    " is not forwarded to subscribers, as the publication manager is " +
                    "already shut down"
            );
        }
        if (!event.selective) {
            //handle multicast
            prepareMulticastPublication(providerId, eventName, data.partitions, value.outputParameters);
            return;
        }
        var publish;
        var i;
        var filterParameters;
        var subscriptionId,
            subscriptions = getSubscriptionsForProviderEvent(providerId, eventName);
        var filters = data.filters;
        if (!subscriptions) {
            log.error("ProviderEvent " + eventName + " for providerId " + providerId + " is not registered");
            // TODO: proper error handling for empty subscription map =>
            // ProviderEvent is not registered
            return;
        }

        for (subscriptionId in subscriptions) {
            if (subscriptions.hasOwnProperty(subscriptionId)) {
                var subscriptionInfo = subscriptions[subscriptionId];
                // if any filters present, check them
                publish = true;
                if (filters && filters.length > 0) {
                    for (i = 0; i < filters.length; i++) {
                        if (subscriptionInfo.filterParameters && subscriptionInfo.filterParameters.filterParameters) {
                            publish = filters[i].filter(value, subscriptionInfo.filterParameters.filterParameters);
                            // stop on first filter failure
                            if (publish === false) {
                                break;
                            }
                        }
                    }
                }
                if (publish) {
                    prepareBroadcastPublication(subscriptionInfo, value.outputParameters);
                }
            }
        }
    }

    /**
     * Adds a provider event to the internal map if it does not exist =>
     * onChange subscriptions can be registered here
     *
     * @name PublicationManager#addPublicationEvent
     * @private
     *
     * @param {String}
     *            providerId
     * @param {String}
     *            eventName
     */
    function addPublicationEvent(providerId, eventName, event) {
        var key = getProviderIdEventKey(providerId, eventName);

        eventObserverFunctions[key] = function(data) {
            publishEventValue(providerId, eventName, event, data || {});
        };
        event.registerObserver(eventObserverFunctions[key]);
    }

    /**
     * @name PublicationManager#resetSubscriptionsForProviderEvent
     * @private
     */
    function resetSubscriptionsForProviderEvent(providerId, eventName) {
        var key = getProviderIdEventKey(providerId, eventName);
        delete onChangeProviderEventToSubscriptions[key];
    }

    // End of broadcast specific implementation

    function addRequestToMulticastSubscriptions(multicastId, subscriptionId) {
        var i, subscriptions;
        if (multicastSubscriptions[multicastId] === undefined) {
            multicastSubscriptions[multicastId] = [];
        }
        subscriptions = multicastSubscriptions[multicastId];
        for (i = 0; i < subscriptions.length; i++) {
            if (subscriptions[i] === subscriptionId) {
                return;
            }
        }
        subscriptions.push(subscriptionId);
    }

    function removeRequestFromMulticastSubscriptions(multicastId, subscriptionId) {
        if (multicastId !== undefined && multicastSubscriptions[multicastId] !== undefined) {
            var i;
            for (i = 0; i < multicastSubscriptions[multicastId].length; i++) {
                if (multicastSubscriptions[multicastId][i] === subscriptionId) {
                    multicastSubscriptions[multicastId].splice(i, 1);
                    break;
                }
            }
            if (multicastSubscriptions[multicastId].length === 0) {
                delete multicastSubscriptions[multicastId];
            }
        }
    }

    /**
     * Removes a subscription, stops scheduled timers
     * @name PublicationManager#removeSubscription
     * @private
     *
     * @param {String}
     *            subscriptionId
     * @param {Boolean}
     *            silent suppress log outputs if subscription cannot be found
     */
    function removeSubscription(subscriptionId, silent) {
        // make sure subscription info exists
        var subscriptionInfo = subscriptionInfos[subscriptionId];
        var pendingSubscriptions;
        var pendingSubscription;
        var subscriptionObject;
        if (subscriptionInfo === undefined) {
            if (silent !== true) {
                log.warn("no subscription info found for subscriptionId " + subscriptionId);
            }
            // TODO: proper handling for a non-existent subscription

            //check if a subscriptionRequest is queued
            subscriptionInfo = queuedSubscriptionInfos[subscriptionId];

            if (subscriptionInfo === undefined) {
                return;
            }

            pendingSubscriptions =
                queuedProviderParticipantIdToSubscriptionRequestsMapping[subscriptionInfo.providerParticipantId];
            if (pendingSubscriptions !== undefined) {
                for (pendingSubscription in pendingSubscriptions) {
                    if (pendingSubscriptions.hasOwnProperty(pendingSubscription)) {
                        subscriptionObject = pendingSubscriptions[pendingSubscription];
                        if (
                            subscriptionObject !== undefined &&
                            subscriptionObject.subscriptionId === subscriptionInfo.subscriptionId
                        ) {
                            delete pendingSubscriptions[pendingSubscription];
                        }
                    }
                }
            }
            delete queuedSubscriptionInfos[subscriptionId];
            return;
        }
        var providerParticipantId = subscriptionInfo.providerParticipantId;

        // make sure the provider exists for the given participantId
        var provider = participantIdToProvider[providerParticipantId];
        if (provider === undefined) {
            log.error("no provider found for " + providerParticipantId);
            // TODO: proper error handling for a non-existent provider
            return;
        }

        var subscription;

        if (subscriptionInfo.subscriptionType === SubscriptionInformation.SUBSCRIPTION_TYPE_ATTRIBUTE) {
            // This is an attribute subscription

            var attributeName = subscriptionInfo.subscribedToName;

            var attributeSubscriptions = getSubscriptionsForProviderAttribute(providerParticipantId, attributeName);
            if (attributeSubscriptions === undefined) {
                log.error(
                    "ProviderAttribute " +
                        attributeName +
                        " for providerId " +
                        providerParticipantId +
                        " is not registered or notifiable"
                );
                // TODO: proper error handling for empty subscription map =>
                // ProviderAttribute is not notifiable or not registered
                return;
            }
            subscription = attributeSubscriptions[subscriptionId];
            delete attributeSubscriptions[subscriptionId];
            if (Object.keys(attributeSubscriptions).length === 0) {
                resetSubscriptionsForProviderAttribute(providerParticipantId, attributeName);
            }
        } else {
            // subscriptionInfo.type === SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST
            // This is a event subscription
            var eventName = subscriptionInfo.subscribedToName;

            // find all subscriptions for the event/providerParticipantId
            var eventSubscriptions = getSubscriptionsForProviderEvent(providerParticipantId, eventName);
            if (eventSubscriptions === undefined) {
                log.error(
                    "ProviderEvent " +
                        eventName +
                        " for providerId " +
                        providerParticipantId +
                        " is not registered or notifiable"
                );
                // TODO: proper error handling for empty subscription map =>
                return;
            }
            subscription = eventSubscriptions[subscriptionId];
            delete eventSubscriptions[subscriptionId];
            if (Object.keys(eventSubscriptions).length === 0) {
                resetSubscriptionsForProviderEvent(providerParticipantId, eventName);
            }
        }

        // make sure subscription exists
        if (subscription === undefined) {
            log.error("no subscription found for subscriptionId " + subscriptionId);
            // TODO: proper error handling when subscription does not exist
            return;
        }

        // clear subscription interval if it exists
        if (subscription.subscriptionInterval !== undefined) {
            LongTimer.clearTimeout(subscription.subscriptionInterval);
        }

        // clear endDate timeout if it exists
        if (subscription.endDateTimeout !== undefined) {
            LongTimer.clearTimeout(subscription.endDateTimeout);
        }

        removeRequestFromMulticastSubscriptions(subscription.multicastId, subscriptionId);

        delete subscriptionInfos[subscriptionId];

        persistency.removeItem(subscriptionId);
        storeSubscriptions();
    }

    /**
     * @name PublicationManager#handleSubscriptionStop
     * @function
     *
     * @param {SubscriptionStop}
     *            subscriptionStop incoming subscriptionStop
     */
    this.handleSubscriptionStop = function handleSubscriptionStop(subscriptionStop) {
        removeSubscription(subscriptionStop.subscriptionId);
    };

    /**
     * Removes a notifiable provider attribute from the internal map if it exists
     * @name PublicationManager#removePublicationAttribute
     * @function
     *
     * @param {String}
     *            providerId
     * @param {String}
     *            attributeName
     */
    function removePublicationAttribute(providerId, attributeName, attribute) {
        var subscriptions,
            subscription,
            subscriptionObject,
            key = getProviderIdAttributeKey(providerId, attributeName);

        subscriptions = getSubscriptionsForProviderAttribute(providerId, attributeName);
        if (subscriptions !== undefined) {
            for (subscription in subscriptions) {
                if (subscriptions.hasOwnProperty(subscription)) {
                    that.handleSubscriptionStop(
                        new SubscriptionStop({
                            subscriptionId: subscription
                        })
                    );
                }
            }
            resetSubscriptionsForProviderAttribute();
        }

        attribute.unregisterObserver(attributeObserverFunctions[key]);
        attributeObserverFunctions[key] = undefined;
    }

    /**
     * Removes a provider event from the internal map if it exists
     * @name PublicationManager#removePublicationEvent
     * @function
     *
     * @param {String}
     *            providerId
     * @param {String}
     *            eventName
     */
    function removePublicationEvent(providerId, eventName, event) {
        var subscriptions,
            subscription,
            subscriptionObject,
            key = getProviderIdEventKey(providerId, eventName);

        subscriptions = getSubscriptionsForProviderEvent(providerId, eventName);
        if (subscriptions !== undefined) {
            for (subscription in subscriptions) {
                if (subscriptions.hasOwnProperty(subscription)) {
                    that.handleSubscriptionStop(
                        new SubscriptionStop({
                            subscriptionId: subscription
                        })
                    );
                }
            }
            resetSubscriptionsForProviderEvent();
        }

        event.unregisterObserver(eventObserverFunctions[key]);
        eventObserverFunctions[key] = undefined;
    }

    /**
     * @name PublicationManager#hasSubscriptionsForProviderEvent
     * @function
     *
     *
     * @param {String}
     *            providerId
     * @param {String}
     *            eventName
     * @returns true if a subscription exists for the given event
     */
    this.hasSubscriptionsForProviderEvent = function hasSubscriptionsForProviderEvent(providerId, eventName) {
        var subscriptions = getSubscriptionsForProviderEvent(providerId, eventName);
        var subscriptionId;
        if (subscriptions !== undefined) {
            for (subscriptionId in subscriptions) {
                if (subscriptions.hasOwnProperty(subscriptionId)) {
                    return true;
                }
            }
        }
        return false;
    };

    /**
     * @name PublicationManager#hasSubscriptionsForProviderAttribute
     * @function
     *
     *
     * @param {String}
     *            providerId
     * @param {String}
     *            attributeName
     * @returns true if a subscription exists for the given attribute
     */
    this.hasSubscriptionsForProviderAttribute = function hasSubscriptionsForProviderAttribute(
        providerId,
        attributeName
    ) {
        var subscriptions = getSubscriptionsForProviderAttribute(providerId, attributeName);
        var subscriptionId;
        if (subscriptions !== undefined) {
            for (subscriptionId in subscriptions) {
                if (subscriptions.hasOwnProperty(subscriptionId)) {
                    return true;
                }
            }
        }
        return false;
    };

    /* the parameter "callbackDispatcher" is optional, as in case of restoring
     * subscriptions, no reply must be sent back via the dispatcher
     */
    function callbackDispatcherAsync(reply, callbackDispatcher) {
        function asyncCallbackDispatcher() {
            callbackDispatcher(new SubscriptionReply(reply));
        }

        if (callbackDispatcher !== undefined) {
            LongTimer.setTimeout(asyncCallbackDispatcher, 0);
        }
    }

    /**
     * Handles SubscriptionRequests
     *
     * @name PublicationManager#handleSubscriptionRequest
     * @function
     *
     * @param {SubscriptionRequest}
     *            proxyParticipantId - participantId of proxy consuming the attribute publications
     *            providerParticipantId - participantId of provider producing the attribute publications
     *            subscriptionRequest incoming subscriptionRequest
     *            callbackDispatcher callback function to inform the caller about the handling result
     * @throws {Error}
     *             when no provider exists or the provider does not have the attribute
     */
    this.handleSubscriptionRequest = function handleSubscriptionRequest(
        proxyParticipantId,
        providerParticipantId,
        subscriptionRequest,
        callbackDispatcher
    ) {
        var exception;
        var timeToEndDate = 0;
        var attributeName = subscriptionRequest.subscribedToName;
        var subscriptionId = subscriptionRequest.subscriptionId;

        // if endDate is defined (also exclude default value 0 for
        // the expiryDateMs qos-property)
        if (
            subscriptionRequest.qos !== undefined &&
            subscriptionRequest.qos.expiryDateMs !== undefined &&
            subscriptionRequest.qos.expiryDateMs !== SubscriptionQos.NO_EXPIRY_DATE
        ) {
            timeToEndDate = subscriptionRequest.qos.expiryDateMs - Date.now();

            // if endDate lies in the past => don't add the subscription
            if (timeToEndDate <= 0) {
                exception = new SubscriptionException({
                    detailMessage:
                        "error handling subscription request: " +
                        JSONSerializer.stringify(subscriptionRequest) +
                        ". expiryDateMs " +
                        subscriptionRequest.qos.expiryDateMs +
                        "for ProviderAttribute " +
                        attributeName +
                        " for providerId " +
                        providerParticipantId +
                        " lies in the past",
                    subscriptionId: subscriptionId
                });
                log.error(exception.detailMessage);
                callbackDispatcherAsync(
                    {
                        error: exception,
                        subscriptionId: subscriptionId
                    },
                    callbackDispatcher
                );
                return;
            }
        }

        if (!isReady()) {
            exception = new SubscriptionException({
                detailMessage:
                    "error handling subscription request: " +
                    JSONSerializer.stringify(subscriptionRequest) +
                    "and provider ParticipantId " +
                    providerParticipantId +
                    ": joynr runtime already shut down",
                subscriptionId: subscriptionRequest.subscriptionId
            });
            log.debug(exception.detailMessage);
            callbackDispatcherAsync(
                {
                    error: exception,
                    subscriptionId: subscriptionRequest.subscriptionId
                },
                callbackDispatcher
            );
            return;
        }
        var provider = participantIdToProvider[providerParticipantId];
        // construct subscriptionInfo from subscriptionRequest and participantIds
        var subscriptionInfo = new SubscriptionInformation(
            SubscriptionInformation.SUBSCRIPTION_TYPE_ATTRIBUTE,
            proxyParticipantId,
            providerParticipantId,
            subscriptionRequest
        );

        // in case the subscriptionId is already used in a previous
        // subscription, remove this one
        removeSubscription(subscriptionId, true);

        // make sure the provider is registered
        if (provider === undefined) {
            log.info(
                "Provider with participantId " + providerParticipantId + "not found. Queueing subscription request..."
            );
            queuedSubscriptionInfos[subscriptionId] = subscriptionInfo;
            var pendingSubscriptions = queuedProviderParticipantIdToSubscriptionRequestsMapping[providerParticipantId];
            if (pendingSubscriptions === undefined) {
                pendingSubscriptions = [];
                queuedProviderParticipantIdToSubscriptionRequestsMapping[providerParticipantId] = pendingSubscriptions;
            }
            pendingSubscriptions[pendingSubscriptions.length] = subscriptionInfo;
            return;
        }

        // make sure the provider contains the attribute being subscribed to
        var attribute = provider[attributeName];
        if (attribute === undefined) {
            exception = new SubscriptionException({
                detailMessage:
                    "error handling subscription request: " +
                    JSONSerializer.stringify(subscriptionRequest) +
                    ".Provider: " +
                    providerParticipantId +
                    " misses attribute " +
                    attributeName,
                subscriptionId: subscriptionId
            });
            log.error(exception.detailMessage);
            callbackDispatcherAsync(
                {
                    error: exception,
                    subscriptionId: subscriptionId
                },
                callbackDispatcher
            );
            return;
        }

        // make sure the provider attribute is a notifiable provider attribute
        // (e.g.: ProviderAttributeNotify[Read][Write])
        if (!providerAttributeIsNotifiable(attribute)) {
            exception = new SubscriptionException({
                detailMessage:
                    "error handling subscription request: " +
                    JSONSerializer.stringify(subscriptionRequest) +
                    ". Provider: " +
                    providerParticipantId +
                    " attribute " +
                    attributeName +
                    " is not notifiable",
                subscriptionId: subscriptionId
            });
            log.error(exception.detailMessage);
            callbackDispatcherAsync(
                {
                    error: exception,
                    subscriptionId: subscriptionId
                },
                callbackDispatcher
            );
            return;
        }

        // make sure a ProviderAttribute is registered
        var subscriptions = getSubscriptionsForProviderAttribute(providerParticipantId, attributeName);
        if (subscriptions === undefined) {
            exception = new SubscriptionException({
                detailMessage:
                    "error handling subscription request: " +
                    JSONSerializer.stringify(subscriptionRequest) +
                    ". ProviderAttribute " +
                    attributeName +
                    " for providerId " +
                    providerParticipantId +
                    " is not registered or notifiable",
                subscriptionId: subscriptionId
            });
            log.error(exception.detailMessage);
            callbackDispatcherAsync(
                {
                    error: exception,
                    subscriptionId: subscriptionId
                },
                callbackDispatcher
            );
            return;
        }

        // Set up publication interval if maxIntervalMs is a number
        //(not (is not a number)) ...
        var periodMs = getPeriod(subscriptionInfo);

        if (!isNaN(periodMs)) {
            if (periodMs < PeriodicSubscriptionQos.MIN_PERIOD_MS) {
                exception = new SubscriptionException({
                    detailMessage:
                        "error handling subscription request: " +
                        JSONSerializer.stringify(subscriptionRequest) +
                        ". periodMs " +
                        periodMs +
                        " is smaller than PeriodicSubscriptionQos.MIN_PERIOD_MS " +
                        PeriodicSubscriptionQos.MIN_PERIOD_MS,
                    subscriptionId: subscriptionId
                });
                log.error(exception.detailMessage);
                callbackDispatcherAsync(
                    {
                        error: exception,
                        subscriptionId: subscriptionId
                    },
                    callbackDispatcher
                );
                return;
            }
        }

        function subscriptionReachedEndDate() {
            removeSubscription(subscriptionId);
        }

        if (timeToEndDate > 0) {
            // schedule to remove subscription from internal maps
            subscriptionInfo.endDateTimeout = LongTimer.setTimeout(subscriptionReachedEndDate, timeToEndDate);
        }

        // call the get method on the provider at the set interval
        subscriptionInfo.subscriptionInterval = triggerPublicationTimer(subscriptionInfo, periodMs);

        // save subscriptionInfo to subscriptionId => subscription and
        // ProviderAttribute => subscription map
        subscriptionInfos[subscriptionId] = subscriptionInfo;
        subscriptions[subscriptionId] = subscriptionInfo;

        persistency.setItem(subscriptionId, JSON.stringify(subscriptionInfo));
        storeSubscriptions();

        function getAttributeValueSuccess(value) {
            prepareAttributePublication(subscriptionInfo, value, triggerPublicationTimer);
            return value;
        }

        function getAttributeValueException(getValueException) {
            sendPublication(subscriptionInfo, undefined, getValueException);
            return getValueException;
        }

        // publish value immediately
        getAttributeValue(subscriptionInfo)
            .then(getAttributeValueSuccess)
            .catch(getAttributeValueException);

        callbackDispatcherAsync({ subscriptionId: subscriptionId }, callbackDispatcher);
    };

    function handleBroadcastSubscriptionRequestInternal(
        proxyParticipantId,
        providerParticipantId,
        subscriptionRequest,
        callbackDispatcher,
        multicast
    ) {
        var requestType = multicast ? "multicast" : "broadcast" + " subscription request";
        var exception;
        var timeToEndDate = 0;
        var eventName = subscriptionRequest.subscribedToName;
        var subscriptionId = subscriptionRequest.subscriptionId;

        // if endDate is defined (also exclude default value 0 for
        // the expiryDateMs qos-property)
        if (
            subscriptionRequest.qos !== undefined &&
            subscriptionRequest.qos.expiryDateMs !== undefined &&
            subscriptionRequest.qos.expiryDateMs !== SubscriptionQos.NO_EXPIRY_DATE
        ) {
            timeToEndDate = subscriptionRequest.qos.expiryDateMs - Date.now();

            // if endDate lies in the past => don't add the subscription
            if (timeToEndDate <= 0) {
                exception = new SubscriptionException({
                    detailMessage:
                        "error handling " +
                        requestType +
                        ": " +
                        JSONSerializer.stringify(subscriptionRequest) +
                        ". expiryDateMs " +
                        subscriptionRequest.qos.expiryDateMs +
                        "for ProviderEvent " +
                        eventName +
                        " for providerId " +
                        providerParticipantId +
                        " lies in the past",
                    subscriptionId: subscriptionId
                });
                log.error(exception.detailMessage);
                callbackDispatcherAsync(
                    {
                        error: exception,
                        subscriptionId: subscriptionId
                    },
                    callbackDispatcher
                );
                return;
            }
        }

        if (!isReady()) {
            exception = new SubscriptionException({
                detailMessage:
                    "error handling " +
                    requestType +
                    ": " +
                    JSONSerializer.stringify(subscriptionRequest) +
                    "and provider ParticipantId " +
                    providerParticipantId +
                    ": joynr runtime already shut down",
                subscriptionId: subscriptionRequest.subscriptionId
            });
            log.debug(exception.detailMessage);
            callbackDispatcherAsync(
                {
                    error: exception,
                    subscriptionId: subscriptionRequest.subscriptionId
                },
                callbackDispatcher
            );
            return;
        }
        var provider = participantIdToProvider[providerParticipantId];
        // construct subscriptionInfo from subscriptionRequest and participantIds

        var subscriptionInfo = new SubscriptionInformation(
            multicast
                ? SubscriptionInformation.SUBSCRIPTION_TYPE_MULTICAST
                : SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST,
            proxyParticipantId,
            providerParticipantId,
            subscriptionRequest
        );

        // in case the subscriptionId is already used in a previous
        // subscription, remove this one
        removeSubscription(subscriptionId, true);

        // make sure the provider is registered
        if (provider === undefined) {
            log.warn(
                "Provider with participantId " + providerParticipantId + "not found. Queueing " + requestType + "..."
            );
            queuedSubscriptionInfos[subscriptionId] = subscriptionInfo;
            var pendingSubscriptions = queuedProviderParticipantIdToSubscriptionRequestsMapping[providerParticipantId];
            if (pendingSubscriptions === undefined) {
                pendingSubscriptions = [];
                queuedProviderParticipantIdToSubscriptionRequestsMapping[providerParticipantId] = pendingSubscriptions;
            }
            pendingSubscriptions[pendingSubscriptions.length] = subscriptionInfo;
            return;
        }

        // make sure the provider contains the event being subscribed to
        var event = provider[eventName];
        if (event === undefined) {
            exception = new SubscriptionException({
                detailMessage:
                    "error handling " +
                    requestType +
                    ": " +
                    JSONSerializer.stringify(subscriptionRequest) +
                    ". Provider: " +
                    providerParticipantId +
                    " misses event " +
                    eventName,
                subscriptionId: subscriptionId
            });
            log.error(exception.detailMessage);
            callbackDispatcherAsync(
                {
                    error: exception,
                    subscriptionId: subscriptionId
                },
                callbackDispatcher
            );
            return;
        }

        // make sure a ProviderEvent is registered
        var subscriptions = getSubscriptionsForProviderEvent(providerParticipantId, eventName);
        if (subscriptions === undefined) {
            exception = new SubscriptionException({
                detailMessage:
                    "error handling " +
                    requestType +
                    ": " +
                    JSONSerializer.stringify(subscriptionRequest) +
                    ". ProviderEvent " +
                    eventName +
                    " for providerId " +
                    providerParticipantId +
                    " is not registered",
                subscriptionId: subscriptionId
            });
            log.error(exception.detailMessage);
            callbackDispatcherAsync(
                {
                    error: exception,
                    subscriptionId: subscriptionId
                },
                callbackDispatcher
            );
            return;
        }

        if (multicast) {
            var multicastId = subscriptionInfo.multicastId;
            if (event.selective) {
                exception = new SubscriptionException({
                    detailMessage:
                        "error handling multicast subscription request: " +
                        JSONSerializer.stringify(subscriptionRequest) +
                        ". Provider: " +
                        providerParticipantId +
                        " event " +
                        eventName +
                        " is marked as selective, which is not allowed for multicasts",
                    subscriptionId: subscriptionId
                });
                log.error(exception.detailMessage);
                callbackDispatcherAsync(
                    {
                        error: exception,
                        subscriptionId: subscriptionId
                    },
                    callbackDispatcher
                );
                return;
            }
            addRequestToMulticastSubscriptions(multicastId, subscriptionId);
        } else {
            var checkResult = event.checkFilterParameters(subscriptionRequest.filterParameters);
            if (checkResult.caughtErrors.length !== 0) {
                exception = new SubscriptionException({
                    detailMessage:
                        "The incoming subscription request does not contain the expected filter parameters to subscribe to broadcast " +
                        eventName +
                        " for providerId " +
                        providerParticipantId +
                        ": " +
                        JSON.stringify(checkResult.caughtErrors),
                    subscriptionId: subscriptionId
                });
                log.error(exception.detailMessage);
                callbackDispatcherAsync(
                    {
                        error: exception,
                        subscriptionId: subscriptionId
                    },
                    callbackDispatcher
                );
                return;
            }
        }

        if (timeToEndDate > 0) {
            // schedule to remove subscription from internal maps
            subscriptionInfo.endDateTimeout = LongTimer.setTimeout(removeSubscription, timeToEndDate, subscriptionId);
        }

        // save subscriptionInfo to subscriptionId => subscription and
        // ProviderEvent => subscription map
        subscriptionInfos[subscriptionId] = subscriptionInfo;
        subscriptions[subscriptionId] = subscriptionInfo;

        persistency.setItem(subscriptionId, JSON.stringify(subscriptionInfo));
        storeSubscriptions();
        callbackDispatcherAsync(
            {
                subscriptionId: subscriptionId
            },
            callbackDispatcher
        );
    }
    /**
     * Handles BroadcastSubscriptionRequests
     *
     * @name PublicationManager#handleBroadcastSubscriptionRequest
     * @function
     *
     * @param {BroadcastSubscriptionRequest}
     *            proxyParticipantId - participantId of proxy consuming the broadcast
     *            providerParticipantId - participantId of provider producing the broadcast
     *            subscriptionRequest incoming subscriptionRequest
     *            callbackDispatcher callback function to inform the caller about the handling result
     * @throws {Error}
     *             when no provider exists or the provider does not have the event
     */
    this.handleBroadcastSubscriptionRequest = function handleBroadcastSubscriptionRequest(
        proxyParticipantId,
        providerParticipantId,
        subscriptionRequest,
        callbackDispatcher
    ) {
        return handleBroadcastSubscriptionRequestInternal(
            proxyParticipantId,
            providerParticipantId,
            subscriptionRequest,
            callbackDispatcher,
            false
        );
    };

    /**
     * Handles MulticastSubscriptionRequests
     *
     * @name PublicationManager#handleMulticastSubscriptionRequest
     * @function
     *
     * @param {MulticastSubscriptionRequest}
     *            providerParticipantId - participantId of provider producing the multicast
     *            subscriptionRequest incoming subscriptionRequest
     *            callbackDispatcher callback function to inform the caller about the handling result
     * @throws {Error}
     *             when no provider exists or the provider does not have the event
     */
    this.handleMulticastSubscriptionRequest = function handleMulticastSubscriptionRequest(
        proxyParticipantId,
        providerParticipantId,
        subscriptionRequest,
        callbackDispatcher
    ) {
        return handleBroadcastSubscriptionRequestInternal(
            proxyParticipantId,
            providerParticipantId,
            subscriptionRequest,
            callbackDispatcher,
            true
        );
    };

    /**
     * Unregisters all attributes of a provider to handle subscription requests
     *
     * @name PublicationManager#removePublicationProvider
     * @function
     *
     * @param {String}
     *            participantId of the provider handling the subsription
     */
    this.removePublicationProvider = function removePublicationProvider(participantId, provider) {
        if (!isReady()) {
            throw new Error("PublicationManager is already shut down");
        }
        var propertyName, property;

        // cycles over all provider members
        for (propertyName in provider) {
            if (provider.hasOwnProperty(propertyName)) {
                // checks whether the member is a notifiable provider attribute
                // and adds it if this is the case
                if (providerAttributeIsNotifiable(provider[propertyName])) {
                    removePublicationAttribute(participantId, propertyName, provider[propertyName]);
                }

                // checks whether the member is an event
                // and adds it if this is the case
                if (propertyIsProviderEvent(provider[propertyName])) {
                    removePublicationEvent(participantId, propertyName, provider[propertyName]);
                }
            }
        }

        // stores the participantId to
        participantIdToProvider[participantId] = undefined;
    };

    /**
     * Registers all attributes of a provider to handle subscription requests
     *
     * @name PublicationManager#addPublicationProvider
     * @function
     *
     * @param {String}
     *            participantId of the provider handling the subsription
     * @param {Provider}
     *            provider
     */
    this.addPublicationProvider = function addPublicationProvider(participantId, provider) {
        var propertyName, property, pendingSubscriptions, pendingSubscription, subscriptionObject;
        if (!isReady()) {
            throw new Error("PublicationManager is already shut down");
        }

        // stores the participantId to
        participantIdToProvider[participantId] = provider;

        // cycles over all provider members
        for (propertyName in provider) {
            if (provider.hasOwnProperty(propertyName)) {
                // checks whether the member is a notifiable provider attribute
                // and adds it if this is the case
                if (providerAttributeIsNotifiable(provider[propertyName])) {
                    addPublicationAttribute(participantId, propertyName, provider[propertyName]);
                }

                // checks whether the member is a event
                // and adds it if this is the case
                if (propertyIsProviderEvent(provider[propertyName])) {
                    addPublicationEvent(participantId, propertyName, provider[propertyName]);
                }
            }
        }

        pendingSubscriptions = queuedProviderParticipantIdToSubscriptionRequestsMapping[participantId];
        if (pendingSubscriptions !== undefined) {
            for (pendingSubscription in pendingSubscriptions) {
                if (pendingSubscriptions.hasOwnProperty(pendingSubscription)) {
                    subscriptionObject = pendingSubscriptions[pendingSubscription];
                    delete pendingSubscriptions[pendingSubscription];

                    /*jslint nomen:true*/
                    if (subscriptionObject.subscriptionType === SubscriptionInformation.SUBSCRIPTION_TYPE_ATTRIBUTE) {
                        // call attribute subscription handler
                        this.handleSubscriptionRequest(
                            subscriptionObject.proxyParticipantId,
                            subscriptionObject.providerParticipantId,
                            subscriptionObject
                        );
                    } else {
                        // call broadcast subscription handler
                        if (
                            subscriptionObject.subscriptionType === SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST
                        ) {
                            this.handleBroadcastSubscriptionRequest(
                                subscriptionObject.proxyParticipantId,
                                subscriptionObject.providerParticipantId,
                                subscriptionObject
                            );
                        } else {
                            this.handleMulticastSubscriptionRequest(
                                subscriptionObject.proxyParticipantId,
                                subscriptionObject.providerParticipantId,
                                subscriptionObject
                            );
                        }
                    }
                    /*jslint nomen:false*/
                }
            }
        }
    };

    /**
     * Loads subscriptions
     *
     * @name PublicationManager#restore
     * @function
     */
    this.restore = function restore(callbackAsync) {
        if (!isReady()) {
            throw new Error("PublicationManager is already shut down");
        }

        var subscriptions = persistency.getItem(subscriptionPersistenceKey);
        if (subscriptions && JSON && JSON.parse) {
            var subscriptionIds = SubscriptionUtil.deserializeSubscriptionIds(subscriptions),
                subscriptionId;
            for (subscriptionId in subscriptionIds) {
                if (subscriptionIds.hasOwnProperty(subscriptionId)) {
                    var item = persistency.getItem(subscriptionIds[subscriptionId]),
                        subscriptionInfo;
                    if (item !== null && item !== undefined) {
                        try {
                            subscriptionInfo = JSON.parse(item);
                            /*jslint nomen:true*/
                            if (
                                subscriptionInfo.subscriptionType ===
                                SubscriptionInformation.SUBSCRIPTION_TYPE_ATTRIBUTE
                            ) {
                                // call attribute subscription handler
                                this.handleSubscriptionRequest(
                                    subscriptionInfo.proxyParticipantId,
                                    subscriptionInfo.providerParticipantId,
                                    subscriptionInfo,
                                    callbackAsync
                                );
                            } else {
                                // call broadcast subscription handler
                                if (
                                    subscriptionInfo.subscriptionType ===
                                    SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST
                                ) {
                                    this.handleBroadcastSubscriptionRequest(
                                        subscriptionInfo.proxyParticipantId,
                                        subscriptionInfo.providerParticipantId,
                                        subscriptionInfo,
                                        callbackAsync
                                    );
                                } else {
                                    this.handleMulticastSubscriptionRequest(
                                        subscriptionInfo.proxyParticipantId,
                                        subscriptionInfo.providerParticipantId,
                                        subscriptionInfo,
                                        callbackAsync
                                    );
                                }
                            }
                            /*jslint nomen:false*/
                        } catch (err) {
                            throw new Error(err);
                        }
                    }
                }
            }
        }
    };

    this.hasMulticastSubscriptions = function() {
        return Object.keys(multicastSubscriptions).length > 0;
    };

    this.hasSubscriptions = function() {
        var hasSubscriptionInfos = Object.keys(subscriptionInfos).length > 0;

        var hasQueuedSubscriptionInfos = Object.keys(queuedSubscriptionInfos).length > 0;

        var hasQueuedProviderParticipantIdToSubscriptionRequestsMapping =
            Object.keys(queuedProviderParticipantIdToSubscriptionRequestsMapping).length > 0;

        var hasOnChangeProviderAttributeToSubscriptions =
            Object.keys(onChangeProviderAttributeToSubscriptions).length > 0;

        var hasOnChangeProviderEventToSubscriptions = Object.keys(onChangeProviderEventToSubscriptions).length > 0;

        return (
            hasSubscriptionInfos ||
            hasQueuedSubscriptionInfos ||
            hasQueuedProviderParticipantIdToSubscriptionRequestsMapping ||
            hasOnChangeProviderAttributeToSubscriptions ||
            hasOnChangeProviderEventToSubscriptions ||
            this.hasMulticastSubscriptions()
        );
    };

    /**
     * Shutdown the publication manager
     *
     * @function
     * @name PublicationManager#shutdown
     */
    this.shutdown = function shutdown() {
        var subscriptionId;
        for (subscriptionId in subscriptionInfos) {
            if (subscriptionInfos.hasOwnProperty(subscriptionId)) {
                var subscriptionInfo = subscriptionInfos[subscriptionId];
                if (subscriptionInfo.subscriptionInterval !== undefined) {
                    LongTimer.clearTimeout(subscriptionInfo.subscriptionInterval);
                }
                if (subscriptionInfo.onChangeDebounce !== undefined) {
                    LongTimer.clearTimeout(subscriptionInfo.onChangeDebounce);
                }
            }
        }
        started = false;
    };
}

PublicationManager.SUBSCRIPTIONS_STORAGE_PREFIX = "subscriptions";
module.exports = PublicationManager;
