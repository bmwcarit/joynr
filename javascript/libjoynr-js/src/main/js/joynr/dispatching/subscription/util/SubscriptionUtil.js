/*global JSON: true */

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

define("joynr/dispatching/subscription/util/SubscriptionUtil", [], function() {
    /**
     * @exports SubscriptionUtil
     */
    var SubscriptionUtil = {};
    /**
     * @param {Map.<String, SubscriptionInformation>} subscriptions - Map&lt;String,
     *     SubscriptionInformation> containing the subscriptions to be serialized
     *
     * @returns {String} serialized subscriptions
     */
    SubscriptionUtil.serializeSubscriptions = function(subscriptions) {
        var result = [], subscriptionId;
        for (subscriptionId in subscriptions) {
            if (subscriptions.hasOwnProperty(subscriptionId)) {
                result[result.length] = subscriptions[subscriptionId];
            }
        }

        return JSON.stringify(result);
    };

    /**
     * @param {Map.<String, SubscriptionInformation>} subscriptions - Map&lt;String,
     *     SubscriptionInformation> containing the subscriptions to be serialized
     *
     * @returns {String} serialized subscriptionIds
     */
    SubscriptionUtil.serializeSubscriptionIds = function(subscriptions) {
        var result = [], subscriptionId;
        for (subscriptionId in subscriptions) {
            if (subscriptions.hasOwnProperty(subscriptionId)) {
                result[result.length] = subscriptionId;
            }
        }

        return JSON.stringify(result);
    };

    /**
     * @param {String} subscriptions - serialized subscriptions as String
     *
     * @returns {Map.<String, SubscriptionInformation>} deserialized subscriptions
     *     as Map.&lt;String, SubscriptionInformation>
     */
    SubscriptionUtil.deserializeSubscriptions = function(subscriptions) {
        var array, result = {}, subscription;
        if (JSON && JSON.parse) {
            try {
                array = JSON.parse(subscriptions);
            } catch (err) {
                throw new Error(err);
            }
        }

        for (subscription in array) {
            if (array.hasOwnProperty(subscription)) {
                var object = array[subscription];
                result[object.subscriptionId] = object;
            }
        }
        return result;
    };

    /**
     * @param {String} subscriptions - serialized subscriptions as String
     *
     * @returns {Array.<String>} deserialized subscriptionIds as Array of String
     */
    SubscriptionUtil.deserializeSubscriptionIds = function(subscriptions) {
        var result = [];
        if (JSON && JSON.parse) {
            try {
                result = JSON.parse(subscriptions);
            } catch (err) {
                throw new Error(err);
            }
        }

        return result;
    };

    /**
     * @param {Array} expectedFilterParameters - the expected filter parameters of a broadcast subscription
     * @param {Array} actualFilterParameters - the actual filter parameters of a broadcast subscription
     * @param {String} broadcastName - the name of the checked broadcast
     *
     * @returns {Object} an object containing possible caughtErrors if the actualFilterParameters do not match
     *                   the expected filter parameters
     */
    SubscriptionUtil.checkFilterParameters =
            function(expectedFilterParameters, actualFilterParameters, broadcastName) {
                var i, result = {
                    caughtErrors : []
                };
                if (actualFilterParameters === undefined
                    || actualFilterParameters === null
                    || Object.keys(actualFilterParameters).length === 0) {
                    return result;
                }
                var targetKeys = Object.keys(expectedFilterParameters);
                var sourceKeys = Object.keys(actualFilterParameters);
                for (i = 0; i < targetKeys.length; i++) {
                    if (sourceKeys.indexOf(targetKeys[i]) === -1) {
                        result.caughtErrors.push("Filter parameter "
                            + targetKeys[i]
                            + " for broadcast \""
                            + broadcastName
                            + "\" is not provided");
                    }
                }
                return result;
            };

    return SubscriptionUtil;
});
