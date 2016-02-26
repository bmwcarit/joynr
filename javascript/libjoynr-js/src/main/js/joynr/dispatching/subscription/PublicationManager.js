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

define(
        "joynr/dispatching/subscription/PublicationManager",
        [
            "global/Promise",
            "joynr/proxy/SubscriptionQos",
            "joynr/dispatching/types/SubscriptionPublication",
            "joynr/dispatching/types/SubscriptionStop",
            "joynr/dispatching/types/SubscriptionInformation",
            "joynr/dispatching/types/Reply",
            "joynr/provider/ProviderEvent",
            "joynr/util/Typing",
            "joynr/dispatching/subscription/util/SubscriptionUtil",
            "joynr/exceptions/ProviderRuntimeException",
            "joynr/util/LongTimer",
            "joynr/util/UtilInternal",
            "joynr/system/LoggerFactory"
        ],
        function(
                Promise,
                SubscriptionQos,
                SubscriptionPublication,
                SubscriptionStop,
                SubscriptionInformation,
                Reply,
                ProviderEvent,
                Typing,
                SubscriptionUtil,
                ProviderRuntimeException,
                LongTimer,
                Util,
                LoggerFactory) {

            // TODO make MIN_PUBLICATION_INTERVAL configurable
            var MIN_PUBLICATION_INTERVAL = 50;

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
                var log =
                        LoggerFactory
                                .getLogger("joynr.dispatching.subscription.PublicationManager");

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

                // map: provider.id+attributeName -> subscriptionIds -> subscription
                var onChangeProviderAttributeToSubscriptions = {};

                // map: provider.id+eventName -> subscriptionIds -> subscription
                var onChangeProviderEventToSubscriptions = {};

                var subscriptionPersistenceKey =
                        PublicationManager.SUBSCRIPTIONS_STORAGE_PREFIX + "_" + joynrInstanceId;

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
                    var provider = participantIdToProvider[participantId], attribute;
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
                    var attribute =
                        getAttribute(
                                subscriptionInfo.providerParticipantId,
                                subscriptionInfo.subscribedToName), value;
                    try {
                        value = attribute.get();
                        if (Util.isPromise(value)) {
                            return value.catch(function(error) {
                                if (error instanceof ProviderRuntimeException) {
                                    throw error;
                                }
                                throw new ProviderRuntimeException({
                                    detailMessage: "getter method for attribute " +
                                        subscriptionInfo.subscribedToName + " reported an error"
                                });
                            });
                        }
                    } catch(error) {
                        if (error instanceof ProviderRuntimeException) {
                            return Promise.reject(error);
                        }
                        return Promise.reject(
                            new ProviderRuntimeException({
                                detailMessage: "getter method for attribute " +
                                    subscriptionInfo.subscribedToName + " reported an error"
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
                    log.debug("send Publication for subscriptionId "
                        + subscriptionInfo.subscriptionId
                        + " and attribute/event "
                        + subscriptionInfo.subscribedToName
                        + ": "
                        + value);
                    subscriptionInfo.lastPublication = Date.now();
                    var subscriptionPublication;

                    if (exception) {
                        subscriptionPublication =  new SubscriptionPublication({
                            error : exception,
                            subscriptionId : subscriptionInfo.subscriptionId
                        });
                    } else {
                        subscriptionPublication = new SubscriptionPublication({
                            response : value,
                            subscriptionId : subscriptionInfo.subscriptionId
                        });
                    }
                    dispatcher.sendPublication({
                        from : subscriptionInfo.providerParticipantId,
                        to : subscriptionInfo.proxyParticipantId,
                        expiryDate : (Date.now() + subscriptionInfo.qos.publicationTtlMs).toString()
                    }, subscriptionPublication
                    );
                }

                /**
                 * @name PublicationManager#getPeriod
                 * @private
                 */
                function getPeriod(subscriptionInfo) {
                    return subscriptionInfo.qos.maxInterval || subscriptionInfo.qos.period;
                }

                /**
                 * @name PublicationManager#prepareAttributePublication
                 * @private
                 */
                function prepareAttributePublication(subscriptionInfo, value, timer) {
                    var timeSinceLastPublication = Date.now() - subscriptionInfo.lastPublication;
                    if (subscriptionInfo.qos.minIntervalMs === undefined
                        || timeSinceLastPublication >= subscriptionInfo.qos.minIntervalMs) {
                        sendPublication(subscriptionInfo, value);
                        // if registered interval exists => reschedule it

                        if (subscriptionInfo.onChangeDebounce !== undefined) {
                            LongTimer.clearTimeout(subscriptionInfo.onChangeDebounce);
                            delete subscriptionInfo.onChangeDebounce;
                        }

                        // if there's an existing interval, clear it and restart
                        if (subscriptionInfo.subscriptionInterval !== undefined) {
                            LongTimer.clearTimeout(subscriptionInfo.subscriptionInterval);
                            subscriptionInfo.subscriptionInterval =
                                    timer(subscriptionInfo, getPeriod(subscriptionInfo));
                        }
                    } else {
                        if (subscriptionInfo.onChangeDebounce === undefined) {
                            subscriptionInfo.onChangeDebounce =
                                    timer(subscriptionInfo, subscriptionInfo.qos.minIntervalMs
                                        - timeSinceLastPublication, function() {
                                        delete subscriptionInfo.onChangeDebounce;
                                    });
                        }
                    }
                }

                /**
                 * @name PublicationManager#prepareBroadcastPublication
                 * @private
                 */
                function prepareBroadcastPublication(subscriptionInfo, value) {
                    var timeSinceLastPublication = Date.now() - subscriptionInfo.lastPublication;
                    if (subscriptionInfo.qos.minIntervalMs === undefined
                        || timeSinceLastPublication >= subscriptionInfo.qos.minIntervalMs) {
                        sendPublication(subscriptionInfo, value);
                    } else {
                        log.info("Two subsequent broadcasts of event "
                                + subscriptionInfo.subscribedToName
                                + " occured within minIntervalMs of subscription with id "
                                + subscriptionInfo.subscriptionId
                                + ". Event will not be sent to the subscribing client.");
                    }
                }

                /**
                 * This functions waits the delay time before pubslishing the value of the attribute
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
                        return LongTimer.setTimeout(function getAttributeFromProviderTimeout() {
                            if (callback !== undefined) {
                                callback();
                            }
                            getAttributeValue(subscriptionInfo).then(
                                function(value) {
                                    prepareAttributePublication(
                                        subscriptionInfo,
                                        value,
                                        triggerPublicationTimer);
                                    return value;
                            }).catch(function(exception) {
                                sendPublication(subscriptionInfo, undefined, exception);
                                return exception;
                            });
                        }, delay);
                    }
                }

                /**
                 * @name PublicationManager#getProviderIdAttributeKey
                 * @private
                 */
                function getProviderIdAttributeKey(providerId, attributeName) {
                    return providerId + "." + attributeName;
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
                    return Typing.getObjectType(providerAttribute)
                            .match(/^ProviderAttributeNotify/);
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
                    var subscriptionId, subscriptions =
                            getSubscriptionsForProviderAttribute(providerId, attributeName);
                    if (!subscriptions) {
                        log.error("ProviderAttribute "
                            + attributeName
                            + " for providerId "
                            + providerId
                            + " is not registered or notifiable");
                        // TODO: proper error handling for empty subscription map =>
                        // ProviderAttribute is not notifiable or not registered
                        return;
                    }

                    for (subscriptionId in subscriptions) {
                        if (subscriptions.hasOwnProperty(subscriptionId)) {
                            var subscriptionInfo = subscriptions[subscriptionId];
                            if (subscriptionInfo.qos.minIntervalMs !== undefined
                                && subscriptionInfo.qos.minIntervalMs > 0) {
                                prepareAttributePublication(subscriptionInfo, value, triggerPublicationTimer);
                            }
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
                 * @name PublicationManager#getProviderIdEventKey
                 * @private
                 */
                function getProviderIdEventKey(providerId, eventName) {
                    return providerId + "." + eventName;
                }

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
                    var publish;
                    var i;
                    var filterParameters;
                    var subscriptionId, subscriptions =
                        getSubscriptionsForProviderEvent(providerId, eventName);
                    var value = data.broadcastOutputParameters;
                    var filters = data.filters;
                    if (!subscriptions) {
                        log.error("ProviderEvent "
                            + eventName
                            + " for providerId "
                            + providerId
                            + " is not registered");
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
                                    if (subscriptionInfo.filterParameters &&
                                        subscriptionInfo.filterParameters.filterParameters) {
                                        publish = filters[i].filter(
                                            value,
                                            subscriptionInfo.filterParameters.filterParameters
                                        );
                                        // stop on first filter failure
                                        if (publish === false) {
                                            break;
                                        }
                                    }
                                }
                            }
                            if (publish) {
                                prepareBroadcastPublication(
                                    subscriptionInfo,
                                    value.outputParameters
                                );
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
                        publishEventValue(providerId, eventName, event, data);
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

                /**
                 * Removes a subscription, stops scheduled timers
                 * @name PublicationManager#removeSubscription
                 * @private
                 *
                 * @param {String}
                 *            subscriptionId
                 */
                function removeSubscription(subscriptionId) {
                    // make sure subscription info exists
                    var subscriptionInfo = subscriptionInfos[subscriptionId];
                    var pendingSubscriptions;
                    var pendingSubscription;
                    var subscriptionObject;
                    if (subscriptionInfo === undefined) {
                        log.warn("no subscription info found for subscriptionId " + subscriptionId);
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
                                    if (subscriptionObject !== undefined
                                        && subscriptionObject.subscriptionId === subscriptionInfo.subscriptionId) {
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

                    var subscriptions;

                    if (subscriptionInfo.subscriptionType === SubscriptionInformation.SUBSCRIPTION_TYPE_ATTRIBUTE) {
                        // This is an attribute subscription

                        var attributeName = subscriptionInfo.subscribedToName;

                        // find all subscriptions for the attribute/provider.id
                        subscriptions =
                                getSubscriptionsForProviderAttribute(provider.id, attributeName);
                        if (subscriptions === undefined) {
                            log.error("ProviderAttribute "
                                + attributeName
                                + " for providerId "
                                + provider.id
                                + " is not registered or notifiable");
                            // TODO: proper error handling for empty subscription map =>
                            // ProviderAttribute is not notifiable or not registered
                            return;
                        }
                    } else {
                        // subscriptionInfo.type === SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST
                        // This is a event subscription
                        var eventName = subscriptionInfo.subscribedToName;

                        // find all subscriptions for the event/provider.id
                        subscriptions =
                                getSubscriptionsForProviderEvent(provider.id, eventName);
                        if (subscriptions === undefined) {
                            log.error("ProviderEvent "
                                + eventName
                                + " for providerId "
                                + provider.id
                                + " is not registered or notifiable");
                            // TODO: proper error handling for empty subscription map =>
                            return;
                        }
                    }

                    // make sure subscription exists
                    var subscription = subscriptions[subscriptionId];
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

                    // remove subscription
                    delete subscriptions[subscriptionId];
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
                    var subscriptions, subscription, subscriptionObject, key =
                            getProviderIdAttributeKey(providerId, attributeName);

                    subscriptions = getSubscriptionsForProviderAttribute(providerId, attributeName);
                    if (subscriptions !== undefined) {
                        for (subscription in subscriptions) {
                            if (subscriptions.hasOwnProperty(subscription)) {
                                that.handleSubscriptionStop(new SubscriptionStop({
                                    subscriptionId : subscription
                                }));
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
                    var subscriptions, subscription, subscriptionObject, key =
                            getProviderIdEventKey(providerId, eventName);

                    subscriptions = getSubscriptionsForProviderEvent(providerId, eventName);
                    if (subscriptions !== undefined) {
                        for (subscription in subscriptions) {
                            if (subscriptions.hasOwnProperty(subscription)) {
                                that.handleSubscriptionStop(new SubscriptionStop({
                                    subscriptionId : subscription
                                }));
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
                this.hasSubscriptionsForProviderAttribute = function hasSubscriptionsForProviderAttribute(providerId, attributeName) {
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

                /**
                 * Handles SubscriptionRequests
                 *
                 * @name PublicationManager#handleSubscriptionRequest
                 * @function
                 *
                 * @param {SubscriptionRequest}
                 *            subscriptionRequest incoming subscriptionRequest subscriptionStop or
                 *            subscriptionReplyjoynrMessage containing a publication
                 * @throws {Error}
                 *             when no provider exists or the provider does not have the attribute
                 */
                this.handleSubscriptionRequest =
                        function handleSubscriptionRequest(
                                proxyParticipantId,
                                providerParticipantId,
                                subscriptionRequest) {
                            var subscriptionInterval;
                            var provider = participantIdToProvider[providerParticipantId];
                            // construct subscriptionInfo from subscriptionRequest and participantIds
                            var subscriptionInfo =
                                    new SubscriptionInformation(
                                            SubscriptionInformation.SUBSCRIPTION_TYPE_ATTRIBUTE,
                                            proxyParticipantId,
                                            providerParticipantId,
                                            subscriptionRequest);

                            var subscriptionId = subscriptionInfo.subscriptionId;

                            // in case the subscriptionId is already used in a prevsious
                            // subscription, remove this one
                            removeSubscription(subscriptionId);

                            // make sure the provider is registered
                            if (provider === undefined) {
                                log.error("Provider with participantId "
                                    + providerParticipantId
                                    + "not found. Queueing subscription request...");
                                queuedSubscriptionInfos[subscriptionId] = subscriptionInfo;
                                var pendingSubscriptions =
                                        queuedProviderParticipantIdToSubscriptionRequestsMapping[providerParticipantId];
                                if (pendingSubscriptions === undefined) {
                                    pendingSubscriptions = [];
                                    queuedProviderParticipantIdToSubscriptionRequestsMapping[providerParticipantId] =
                                            pendingSubscriptions;
                                }
                                pendingSubscriptions[pendingSubscriptions.length] =
                                        subscriptionInfo;
                                return;
                            }

                            // make sure the provider contains the attribute being subscribed to
                            var attributeName = subscriptionRequest.subscribedToName;
                            var attribute = provider[attributeName];
                            if (attribute === undefined) {
                                log.error("Provider: "
                                    + providerParticipantId
                                    + " misses attribute "
                                    + attributeName);
                                // TODO: proper error handling when the provider does not contain
                                // the attribute with the given name
                                return;
                            }

                            // make sure the provider attribute is a notifiable provider attribute
                            // (e.g.: ProviderAttributeNotify[Read][Write])
                            if (!providerAttributeIsNotifiable(attribute)) {
                                log.error("Attribute " + attributeName + " is not notifiable");
                                // TODO: proper error handling when the provider does not
                                // contain the attribute with the given name
                                return;
                            }

                            // make sure a ProviderAttribute is registered
                            var subscriptions =
                                    getSubscriptionsForProviderAttribute(provider.id, attributeName);
                            if (subscriptions === undefined) {
                                log.error("ProviderAttribute "
                                    + attributeName
                                    + " for providerId "
                                    + provider.id
                                    + " is not registered or notifiable");
                                // TODO: proper error handling for empty subscription map =>
                                // ProviderAttribute is not notifiable or not registered
                                return;
                            }

                            // if endDate is defined (also exclude default value 0 for
                            // the expiryDateMs qos-property)
                            if (subscriptionInfo.qos.expiryDateMs !== undefined
                                && subscriptionInfo.qos.expiryDateMs !== SubscriptionQos.NO_EXPIRY_DATE) {
                                var timeToEndDate = subscriptionRequest.qos.expiryDateMs - Date.now();

                                // if endDate lies in the past => don't add the subscription
                                if (timeToEndDate <= 0) {
                                    // log.warn("endDate lies in the past, discarding subscription
                                    // with id " + subscriptionId);
                                    // TODO: how should this warning be handled properly?
                                    return;
                                }

                                // schedule to remove subscription from internal maps
                                subscriptionInfo.endDateTimeout =
                                        LongTimer.setTimeout(function subscriptionReachedEndDate() {
                                            removeSubscription(subscriptionId);
                                        }, timeToEndDate);
                            }

                            // Set up publication interval if maxInterval is a number
                            //(not (is not a number)) ...
                            var period = getPeriod(subscriptionInfo);

                            if (!isNaN(period)) {
                                if (period < MIN_PUBLICATION_INTERVAL) {
                                    log.error("SubscriptionRequest error: period: "
                                        + period
                                        + "is smaller than MIN_PUBLICATION_INTERVAL: "
                                        + MIN_PUBLICATION_INTERVAL);
                                    // TODO: proper error handling when maxInterval is smaller than
                                    // MIN_PUBLICATION_INTERVAL
                                } else {
                                    // call the get method on the provider at the set interval
                                    subscriptionInfo.subscriptionInterval =
                                            triggerPublicationTimer(subscriptionInfo, period);
                                }
                            }

                            // save subscriptionInfo to subscriptionId => subscription and
                            // ProviderAttribute => subscription map
                            subscriptionInfos[subscriptionId] = subscriptionInfo;
                            subscriptions[subscriptionId] = subscriptionInfo;

                            persistency.setItem(subscriptionId, JSON.stringify(subscriptionInfo));
                            storeSubscriptions();

                            // publish value immediately
                            getAttributeValue(subscriptionInfo).then(
                                function(value) {
                                    prepareAttributePublication(
                                        subscriptionInfo,
                                            value,
                                            triggerPublicationTimer);
                                    return value;
                                }).catch(function(exception) {
                                    sendPublication(subscriptionInfo, undefined, exception);
                                    return exception;
                                });
                        };

                /**
                 * Handles EventSubscriptionRequests
                 *
                 * @name PublicationManager#handleEventSubscriptionRequest
                 * @function
                 *
                 * @param {EventSubscriptionRequest}
                 *            subscriptionRequest incoming subscriptionRequest subscriptionStop or
                 *            subscriptionReplyjoynrMessage containing a publication
                 * @throws {Error}
                 *             when no provider exists or the provider does not have the event
                 */
                this.handleEventSubscriptionRequest =
                        function handleEventSubscriptionRequest(
                                proxyParticipantId,
                                providerParticipantId,
                                subscriptionRequest) {
                            var subscriptionInterval;
                            var provider = participantIdToProvider[providerParticipantId];
                            // construct subscriptionInfo from subscriptionRequest and participantIds
                            var subscriptionInfo =
                                    new SubscriptionInformation(
                                            SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST,
                                            proxyParticipantId,
                                            providerParticipantId,
                                            subscriptionRequest);

                            var subscriptionId = subscriptionInfo.subscriptionId;

                            // in case the subscriptionId is already used in a previous
                            // subscription, remove this one
                            removeSubscription(subscriptionId);

                            // make sure the provider is registered
                            if (provider === undefined) {
                                log.error("Provider with participantId "
                                    + providerParticipantId
                                    + "not found. Queueing subscription request...");
                                queuedSubscriptionInfos[subscriptionId] = subscriptionInfo;
                                var pendingSubscriptions =
                                        queuedProviderParticipantIdToSubscriptionRequestsMapping[providerParticipantId];
                                if (pendingSubscriptions === undefined) {
                                    pendingSubscriptions = [];
                                    queuedProviderParticipantIdToSubscriptionRequestsMapping[providerParticipantId] =
                                            pendingSubscriptions;
                                }
                                pendingSubscriptions[pendingSubscriptions.length] =
                                        subscriptionInfo;
                                return;
                            }

                            // make sure the provider contains the event being subscribed to
                            var eventName = subscriptionRequest.subscribedToName;
                            var event = provider[eventName];
                            if (event === undefined) {
                                log.error("Provider: "
                                    + providerParticipantId
                                    + " misses event "
                                    + eventName);
                                // TODO: proper error handling when the provider does not contain
                                // the event with the given name
                                return;
                            }

                            // make sure a ProviderEvent is registered
                            var subscriptions =
                                    getSubscriptionsForProviderEvent(provider.id, eventName);
                            if (subscriptions === undefined) {
                                log.error("ProviderEvent "
                                    + eventName
                                    + " for providerId "
                                    + provider.id
                                    + " is not registered");
                                // TODO: proper error handling for empty subscription map =>
                                // ProviderEvent is not registered
                                return;
                            }

                            // if endDate is defined (also exclude default value 0 for
                            // the expiryDateMs qos-property)
                            if (subscriptionInfo.qos.expiryDateMs !== undefined
                                && subscriptionInfo.qos.expiryDateMs !== SubscriptionQos.NO_EXPIRY_DATE) {
                                var timeToEndDate = subscriptionRequest.qos.expiryDateMs - Date.now();

                                // if endDate lies in the past => don't add the subscription
                                if (timeToEndDate <= 0) {
                                    // log.warn("endDate lies in the past, discarding subscription
                                    // with id " + subscriptionId);
                                    // TODO: how should this warning be handled properly?
                                    return;
                                }

                                // schedule to remove subscription from internal maps
                                subscriptionInfo.endDateTimeout =
                                        LongTimer.setTimeout(function subscriptionReachedEndDate() {
                                            removeSubscription(subscriptionId);
                                        }, timeToEndDate);
                            }

                            // Set up publication interval if maxInterval is a number
                            //(not (is not a number)) ...
                            var period = getPeriod(subscriptionInfo);

                            // save subscriptionInfo to subscriptionId => subscription and
                            // ProviderEvent => subscription map
                            subscriptionInfos[subscriptionId] = subscriptionInfo;
                            subscriptions[subscriptionId] = subscriptionInfo;

                            persistency.setItem(subscriptionId, JSON.stringify(subscriptionInfo));
                            storeSubscriptions();
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
                this.removePublicationProvider =
                        function removePublicationProvider(participantId, provider) {
                            var propertyName, property;

                            // cycles over all provider members
                            for (propertyName in provider) {
                                if (provider.hasOwnProperty(propertyName)) {
                                    // checks whether the member is a notifiable provider attribute
                                    // and adds it if this is the case
                                    if (providerAttributeIsNotifiable(provider[propertyName])) {
                                        removePublicationAttribute(
                                                provider.id,
                                                propertyName,
                                                provider[propertyName]);
                                    }

                                    // checks whether the member is an event
                                    // and adds it if this is the case
                                    if (propertyIsProviderEvent(provider[propertyName])) {
                                        removePublicationEvent(
                                                provider.id,
                                                propertyName,
                                                provider[propertyName]);
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
                this.addPublicationProvider =
                        function addPublicationProvider(participantId, provider) {
                            var propertyName, property, pendingSubscriptions, pendingSubscription, subscriptionObject;

                            // stores the participantId to
                            participantIdToProvider[participantId] = provider;

                            // cycles over all provider members
                            for (propertyName in provider) {
                                if (provider.hasOwnProperty(propertyName)) {
                                    // checks whether the member is a notifiable provider attribute
                                    // and adds it if this is the case
                                    if (providerAttributeIsNotifiable(provider[propertyName])) {
                                        addPublicationAttribute(
                                                provider.id,
                                                propertyName,
                                                provider[propertyName]);
                                    }

                                    // checks whether the member is a event
                                    // and adds it if this is the case
                                    if (propertyIsProviderEvent(provider[propertyName])) {
                                        addPublicationEvent(
                                                provider.id,
                                                propertyName,
                                                provider[propertyName]);
                                    }
                                }
                            }

                            pendingSubscriptions =
                                    queuedProviderParticipantIdToSubscriptionRequestsMapping[participantId];
                            if (pendingSubscriptions !== undefined) {
                                for (pendingSubscription in pendingSubscriptions) {
                                    if (pendingSubscriptions.hasOwnProperty(pendingSubscription)) {
                                        subscriptionObject =
                                                pendingSubscriptions[pendingSubscription];
                                        delete pendingSubscriptions[pendingSubscription];

                                        if (subscriptionObject.filterParameters === undefined) {
                                            // call attribute subscription handler
                                            this.handleSubscriptionRequest(
                                                subscriptionObject.proxyParticipantId,
                                                subscriptionObject.providerParticipantId,
                                                subscriptionObject);
                                        } else {
                                            // call event subscription handler
                                            this.handleEventSubscriptionRequest(
                                                subscriptionObject.proxyParticipantId,
                                                subscriptionObject.providerParticipantId,
                                                subscriptionObject);
                                        }
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
                this.restore =
                        function restore() {
                            var subscriptions = persistency.getItem(subscriptionPersistenceKey);
                            if (subscriptions && JSON && JSON.parse) {
                                var subscriptionIds =
                                        SubscriptionUtil.deserializeSubscriptionIds(subscriptions), subscriptionId;
                                for (subscriptionId in subscriptionIds) {
                                    if (subscriptionIds.hasOwnProperty(subscriptionId)) {
                                        var item =
                                                persistency
                                                        .getItem(subscriptionIds[subscriptionId]), subscriptionInfo;
                                        if (item !== null && item !== undefined) {
                                            try {
                                                subscriptionInfo = JSON.parse(item);
                                                this.handleSubscriptionRequest(
                                                        subscriptionInfo.proxyParticipantId,
                                                        subscriptionInfo.providerParticipantId,
                                                        subscriptionInfo);
                                            } catch (err) {
                                                throw new Error(err);
                                            }
                                        }
                                    }
                                }
                            }
                        };

            }

            PublicationManager.SUBSCRIPTIONS_STORAGE_PREFIX = "subscriptions";
            return PublicationManager;
        });
