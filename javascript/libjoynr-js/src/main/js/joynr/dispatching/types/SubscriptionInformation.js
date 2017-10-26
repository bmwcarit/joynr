/*jslint node: true */

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
var Typing = require("../../util/Typing");
var SubscriptionRequest = require("./SubscriptionRequest");

/**
 * @name SubscriptionInformation
 * @constructor
 * @param {String}
 *            proxyParticipantId Id of the proxy
 * @param {String}
 *            providerParticipantId Id of the provider
 * @param {Object|SubscriptionRequest}
 *            [request] the subscription request
 */
function SubscriptionInformation(subscriptionType, proxyParticipantId, providerParticipantId, request) {
    this.subscriptionType = subscriptionType;
    this.proxyParticipantId = proxyParticipantId;
    this.providerParticipantId = providerParticipantId;
    this.subscriptionId = request.subscriptionId;
    this.subscribedToName = request.subscribedToName;
    this.qos = request.qos;
    if (subscriptionType === SubscriptionInformation.SUBSCRIPTION_TYPE_MULTICAST) {
        this.multicastId = request.multicastId;
    } else {
        this.lastPublication = 0;
        if (subscriptionType === SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST) {
            this.filterParameters = request.filterParameters;
        }
    }

    /*
     * the following members may contain native timer objects, which cannot
     * be serialized via JSON.stringify(), hence they must be excluded
     */
    Object.defineProperty(this, "endDateTimeout", {
        enumerable: false,
        configurable: false,
        writable: true,
        value: undefined
    });
    Object.defineProperty(this, "subscriptionInterval", {
        enumerable: false,
        configurable: false,
        writable: true,
        value: undefined
    });

    /**
     * The joynr type name
     *
     * @name SubscriptionInformation#_typeName
     * @type String
     */
    Typing.augmentTypeName(this, "joynr");
}

SubscriptionInformation.SUBSCRIPTION_TYPE_ATTRIBUTE = "subscription_type_attribute";
SubscriptionInformation.SUBSCRIPTION_TYPE_BROADCAST = "subscription_type_broadcast";
SubscriptionInformation.SUBSCRIPTION_TYPE_MULTICAST = "subscription_type_multicast";

module.exports = SubscriptionInformation;
