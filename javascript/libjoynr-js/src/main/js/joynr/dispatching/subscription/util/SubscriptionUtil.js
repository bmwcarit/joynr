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
/**
 * @exports SubscriptionUtil
 */
const SubscriptionUtil = {};
/**
 * @param {Map.<String, SubscriptionInformation>} subscriptions - Map&lt;String,
 *     SubscriptionInformation> containing the subscriptions to be serialized
 *
 * @returns {String} serialized subscriptions
 */
SubscriptionUtil.serializeSubscriptions = function(subscriptions) {
    const result = [];
    for (const subscriptionId in subscriptions) {
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
    const result = [];

    for (const subscriptionId in subscriptions) {
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
    let array;
    const result = {};
    if (JSON && JSON.parse) {
        try {
            array = JSON.parse(subscriptions);
        } catch (err) {
            throw new Error(err);
        }
    }

    for (const subscription in array) {
        if (array.hasOwnProperty(subscription)) {
            const object = array[subscription];
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
    let result = [];
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
SubscriptionUtil.checkFilterParameters = function(expectedFilterParameters, actualFilterParameters, broadcastName) {
    const result = {
        caughtErrors: []
    };
    if (
        actualFilterParameters === undefined ||
        actualFilterParameters === null ||
        Object.keys(actualFilterParameters).length === 0
    ) {
        return result;
    }
    const targetKeys = Object.keys(expectedFilterParameters);
    const sourceKeys = Object.keys(actualFilterParameters);
    for (let i = 0; i < targetKeys.length; i++) {
        if (sourceKeys.indexOf(targetKeys[i]) === -1) {
            result.caughtErrors.push(
                "Filter parameter " + targetKeys[i] + ' for broadcast "' + broadcastName + '" is not provided'
            );
        }
    }
    return result;
};

/**
 * @param {String} providerParticipantId - provider's participant ID
 * @param {String} multicastName - the name of the multicasts
 * @param {Array} paritions - partitions of this multicast
 */
SubscriptionUtil.createMulticastId = function(providerParticipantId, multicastName, partitions) {
    let i,
        multicastId = providerParticipantId + "/" + multicastName;
    if (partitions !== undefined) {
        for (i = 0; i < partitions.length; i++) {
            multicastId += "/" + partitions[i];
        }
    }
    return multicastId;
};

/**
 *
 * validates if the provided partitions for multicast publications only contains valid characters
 * @param {String} partition
 * @throws {Error} if partitions contains invalid characters
 */
SubscriptionUtil.validatePartitions = function(partitions) {
    let i, partition;
    if (partitions !== undefined) {
        for (i = 0; i < partitions.length; i++) {
            partition = partitions[i];
            if (
                !partition.match(SubscriptionUtil.VALID_PARTITION_REGEX) &&
                !(partition === SubscriptionUtil.SINGLE_POSITION_WILDCARD) &&
                !(i + 1 === partitions.length && partition === SubscriptionUtil.MULTI_LEVEL_WILDCARD)
            ) {
                throw new Error(
                    "Partition " +
                        partitions[i] +
                        " contains invalid characters.%n" +
                        "Must only contain a-z A-Z 0-9, or by a single position wildcard (+),%n" +
                        "or the last partition may be a multi-level wildcard (*)."
                );
            }
        }
    }
};

SubscriptionUtil.VALID_PARTITION_REGEX = /^[a-zA-Z0-9]+$/;
SubscriptionUtil.SINGLE_POSITION_WILDCARD = "+";
SubscriptionUtil.MULTI_LEVEL_WILDCARD = "*";
module.exports = SubscriptionUtil;
