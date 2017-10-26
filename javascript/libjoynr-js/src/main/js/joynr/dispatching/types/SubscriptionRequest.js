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
var Util = require("../../util/UtilInternal");
var Typing = require("../../util/Typing");
var PeriodicSubscriptionQos = require("../../proxy/PeriodicSubscriptionQos");
var OnChangeSubscriptionQos = require("../../proxy/OnChangeSubscriptionQos");
var OnChangeWithKeepAliveSubscriptionQos = require("../../proxy/OnChangeWithKeepAliveSubscriptionQos");

var defaultSettings = {
    qos: new PeriodicSubscriptionQos()
};

/**
 * @name SubscriptionRequest
 * @constructor
 * @param {Object} settings
 * @param {String}
 *            settings.subscriptionId Id of the new subscription
 * @param {String}
 *            settings.subscribedToName the name of the element to subscribe to
 * @param {SubscriptionQos}
 *            [settings.qos] the subscriptionQos
 */
function SubscriptionRequest(settings) {
    Typing.checkProperty(settings, "Object", "settings");
    Typing.checkProperty(settings.subscriptionId, "String", "settings.subscriptionId");
    Typing.checkProperty(settings.subscribedToName, "String", "settings.subscribedToName");
    Typing.checkPropertyIfDefined(
        settings.qos,
        ["Object", "OnChangeSubscriptionQos", "PeriodicSubscriptionQos", "OnChangeWithKeepAliveSubscriptionQos"],
        "settings.qos"
    );

    /**
     * @name SubscriptionRequest#subscriptionId
     * @type String
     */
    /**
     * @name SubscriptionRequest#qosName
     * @type OnChangeSubscriptionQos|PeriodicSubscriptionQos|OnChangeWithKeepAliveSubscriptionQos
     */
    /**
     * @name SubscriptionRequest#attributeName
     * @type String
     */
    Util.extend(this, defaultSettings, settings);

    /**
     * The joynr type name
     *
     * @name SubscriptionRequest#_typeName
     * @type String
     */

    Object.defineProperty(this, "_typeName", {
        value: "joynr.SubscriptionRequest",
        readable: true,
        writable: false,
        enumerable: true,
        configurable: false
    });

    return Object.freeze(this);
}

/**
 * The joynr type name
 *
 * @name Request#_typeName
 * @type String
 */
Object.defineProperty(SubscriptionRequest, "_typeName", {
    value: "joynr.SubscriptionRequest",
    readable: true,
    writable: false,
    enumerable: true,
    configurable: false
});

module.exports = SubscriptionRequest;
