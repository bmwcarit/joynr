/*jslint node: true es5: true */

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

var log = require("./logging.js").log;

/**
 * @param {RadioProxy}
 *            radioProxy
 * @param {String}
 *            attributeName
 * @param {String}
 *            subscriptionDescriptor a descriptor for the subscription type for logs
 * @param {SubscriptionQos}
 *            subscriptionQos
 * @param {Function}
 *            publicationFunction function is called on publication, signature: function(value)
 * @param {Array}
 *            subscriptionIds the subscription ids of all subscriptions to this attribute
 * @param {String}
 *            subscriptionIds.array subscription id for one subscription
 */
exports.subscribeAttribute = function subscribeAttribute(radioProxy, attributeName, subscriptionDescriptor, subscriptionQos, publicationFunction,
                            subscriptionIds, onDone) {
    radioProxy[attributeName].subscribe(
            {
                subscriptionQos : subscriptionQos,
                onReceive : function(value) {
                    log("radioProxy." + attributeName + ".subscribe(" + subscriptionDescriptor
                            + ").onReceive: " + value);
                    publicationFunction(value);
                },
                onError : function() {
                    log("radioProxy." + attributeName + ".subscribe(" + subscriptionDescriptor
                            + ").onError");
                }
            }).then(function(token) {
        subscriptionIds[token] = true;
        log("radioProxy." + attributeName + ".subscribe.done: " + token);
    }).catch(function(error) {
        log("radioProxy." + attributeName + ".subscribe.fail: " + error);
    }).always(function(){
        if (onDone) {
            onDone();
        }
    });
};

/**
 * @param {RadioProxy}
 *            radioProxy
 * @param {String}
 *            attributeName
 * @param {Object}
 *            subscriptionIds the subscription ids of all subscriptions to this attribute
 * @param {Boolean}
 *            subscriptionIds.SUBSCRIPTION_ID the key is the subscription id and the value is true
 * @param {String}
 *            subscriptionId the subscription id
 */
exports.unsubscribeAttributeSubscription =
    function unsubscribeAttributeSubscription(radioProxy,
                                              attributeName,
                                               subscriptionIds,
                                               subscriptionId,
                                               onDone) {
    log("radioProxy." + attributeName + ".unsubscribe(" + subscriptionId + ")");
    radioProxy[attributeName].unsubscribe({
        "subscriptionId" : subscriptionId
    }).then(function() {
        delete subscriptionIds[subscriptionId];
        log("radioProxy." + attributeName + ".unsubscribe(" + subscriptionId + ").done");
    }).catch(function(error) {
        log("radioProxy." + attributeName + ".unsubscribe(" + subscriptionId + ").fail: " + error);
    }).always(function(){
        if (onDone) {
            onDone();
        }
    });
};

/**
 * @param {RadioProxy}
 *            radioProxy
 * @param {String}
 *            attributeName
 * @param {Object}
 *            subscriptionIds the subscription ids of all subscriptions to this attribute
 * @param {Boolean}
 *            subscriptionIds.SUBSCRIPTION_ID the key is the subscription id and the value is true
 */
exports.unsubscribeAttributeSubscriptions =
    function unsubscribeAttributeSubscriptions(radioProxy,
                                                attributeName,
                                                subscriptionIds,
                                                onDone) {
    var subscriptionId;
    var validSubscriptionIds = [];
    var doneUnsubscriptions = 0;
    var checkDone;
    var countDone;
    var countAndCheckDone;
    if (Object.keys(subscriptionIds).length > 0) {
        for (subscriptionId in subscriptionIds) {
            if (subscriptionIds.hasOwnProperty(subscriptionId)) {
                validSubscriptionIds.push(subscriptionId);
            }
        }
    } else {
        log("there are no subscriptions for attribute " + attributeName + " to unsubscribe");
    }

    checkDone = function() {
        if(doneUnsubscriptions === validSubscriptionIds.length) {
            if (onDone) {
                onDone();
            }
        }
    };

    checkDone();

    countDone = function() {
        doneUnsubscriptions++;
    };

    countAndCheckDone = function(){
        countDone();
        checkDone();
    };

    for (subscriptionId in validSubscriptionIds) {
        if (validSubscriptionIds.hasOwnProperty(subscriptionId)) {
            exports.unsubscribeAttributeSubscription(radioProxy,
                                                     attributeName,
                                                     subscriptionIds,
                                                     validSubscriptionIds[subscriptionId], countAndCheckDone);
        }
    }
};