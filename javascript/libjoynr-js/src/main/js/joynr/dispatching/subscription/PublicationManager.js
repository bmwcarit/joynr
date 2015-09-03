/*jslint es5: true */

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

define(
        "joynr/dispatching/subscription/PublicationManager",
        [
            "global/Promise",
            "joynr/proxy/SubscriptionQos",
            "joynr/dispatching/types/SubscriptionPublication",
            "joynr/dispatching/types/SubscriptionStop",
            "joynr/dispatching/types/SubscriptionInformation",
            "joynr/dispatching/types/Reply",
            "joynr/util/Typing",
            "joynr/dispatching/subscription/util/SubscriptionUtil",
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
                Typing,
                SubscriptionUtil,
                LongTimer,
                Util,
                LoggerFactory) {

            // TODO make MIN_PUBLICATION_INTERVAL configurable
            var MIN_PUBLICATION_INTERVAL = 100;

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
                // map: subscriptionId to SubscriptionRequest
                var subscriptionInfos = {};

                // map: subscriptionId to SubscriptionRequest
                var queuedSubscriptionInfos = {};

                // queued subscriptions for deferred providers
                var queuedProviderParticipantIdToSubscriptionRequestsMapping = {};

                // map: provider.id+attributeName -> subscriptionIds -> subscription
                var onChangeProviderAttributeToSubscriptions = {};

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
                    value = attribute.get();
                    if (Util.isPromise(value)) {
                        return value.catch(function(error) {
                            throw new Error("Error occured when calling getter \""
                                    + value
                                    + "\" on the provider: " + error.stack);
                        });
                    }
                    return Promise.resolve(value);
                }

                /**
                 * @name PublicationManager#sendPublication
                 * @private
                 */
                function sendPublication(subscriptionInfo, value) {
                    log.debug("send Publication for subscriptionId "
                        + subscriptionInfo.subscriptionId
                        + " and attribute/broadcast "
                        + subscriptionInfo.subscribedToName
                        + ": "
                        + value);
                    subscriptionInfo.lastPublication = Date.now();
                    dispatcher.sendPublication({
                        from : subscriptionInfo.providerParticipantId,
                        to : subscriptionInfo.proxyParticipantId,
                        expiryDate : Date.now() + subscriptionInfo.qos.publicationTtl
                    }, new SubscriptionPublication({
                        response : value,
                        subscriptionId : subscriptionInfo.subscriptionId
                    }));
                }

                /**
                 * @name PublicationManager#getPeriod
                 * @private
                 */
                function getPeriod(subscriptionInfo) {
                    return subscriptionInfo.qos.maxInterval || subscriptionInfo.qos.period;
                }

                /**
                 * @name PublicationManager#preparePublication
                 * @private
                 */
                function preparePublication(subscriptionInfo, value, timer) {
                    var timeSinceLastPublication = Date.now() - subscriptionInfo.lastPublication;
                    if (subscriptionInfo.qos.minInterval === undefined
                        || timeSinceLastPublication >= subscriptionInfo.qos.minInterval) {
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
                                    timer(subscriptionInfo, subscriptionInfo.qos.minInterval
                                        - timeSinceLastPublication, function() {
                                        delete subscriptionInfo.onChangeDebounce;
                                    });
                        }
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
                                        preparePublication(
                                                subscriptionInfo,
                                                [value],
                                                triggerPublicationTimer);
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
                            if (subscriptionInfo.qos.minInterval !== undefined
                                && subscriptionInfo.qos.minInterval > 0) {
                                preparePublication(subscriptionInfo, [value], triggerPublicationTimer);
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
                    var attributeName = subscriptionInfo.subscribedToName;

                    // make sure the provider exists for the given participantId
                    var provider = participantIdToProvider[providerParticipantId];
                    if (provider === undefined) {
                        log.error("no provider found for " + providerParticipantId);
                        // TODO: proper error handling for a non-existent provider
                        return;
                    }

                    // find all subscriptions for the attribute/provider.id
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
                            // the expiryDate qos-property)
                            if (subscriptionInfo.qos.expiryDate !== undefined
                                && subscriptionInfo.qos.expiryDate !== SubscriptionQos.NO_EXPIRY_DATE) {
                                var timeToEndDate = subscriptionRequest.qos.expiryDate - Date.now();

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
                                        preparePublication(
                                                subscriptionInfo,
                                                [value],
                                                triggerPublicationTimer);
                                    });
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
                                }
                            }

                            // stores the participantId to
                            participantIdToProvider[participantId] = undefined;

                        };

                /**
                 * Registers a all attributes of a provider to handle subscription requests
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
                                        this.handleSubscriptionRequest(
                                                subscriptionObject.proxyParticipantId,
                                                subscriptionObject.providerParticipantId,
                                                subscriptionObject);
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
