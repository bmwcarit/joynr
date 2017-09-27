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
var Util = require('../../util/UtilInternal');
var Typing = require('../../util/Typing');
var OnChangeSubscriptionQos = require('../../proxy/OnChangeSubscriptionQos');
var BroadcastFilterParameters = require('../../proxy/BroadcastFilterParameters');

    var defaultSettings = {
        qos : new OnChangeSubscriptionQos()
    };

    /**
     * @name BroadcastSubscriptionRequest
     * @constructor
     * @param {String}
     *            settings.subscriptionId Id of the new subscription
     * @param {String}
     *            settings.subscribedToName the name of the element to subscribe to
     * @param {Object|SubscriptionQos}
     *            [settings.subscriptionQos] the subscriptionQos
     */
    function BroadcastSubscriptionRequest(settings) {
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.subscriptionId, "String", "settings.subscriptionId");
        Typing.checkProperty(settings.subscribedToName, "String", "settings.subscribedToName");
        Typing.checkPropertyIfDefined(settings.qos, [
            "Object",
            "OnChangeSubscriptionQos"
        ], "settings.qos");
        Typing.checkPropertyIfDefined(settings.filterParameters, [
            "Object",
            "BroadcastFilterParameters"
        ], "settings.filterParameters");

        /**
         * @name BroadcastSubscriptionRequest#subscriptionId
         * @type String
         */
        this.subscriptionId = settings.subscriptionId;
        /**
         * @name BroadcastSubscriptionRequest#subscribedToName
         * @type String
         */
        this.subscribedToName = settings.subscribedToName;
        /**
         * @name BroadcastSubscriptionRequest#qos
         * @type Object|OnChangeSubscriptionQos
         */
        this.qos = settings.qos || defaultSettings.qos;
        /**
         * @name BroadcastSubscriptionRequest#filterParameters
         * @type Object|BroadcastFilterParameters
         */
        if (settings.filterParameters !== undefined) {
            this.filterParameters = settings.filterParameters;
        }
        /**
         * The joynr type name
         *
         * @name BroadcastSubscriptionRequest#_typeName
         * @type String
         */
        Typing.augmentTypeName(this, "joynr");

        Object.defineProperty(this, "_typeName", {
            value : "joynr.BroadcastSubscriptionRequest",
            readable : true,
            writable : false,
            enumerable : true,
            configurable : false
        });

        return Object.freeze(this);
    }

    /**
     * The joynr type name
     *
     * @name Request#_typeName
     * @type String
     */
    Object.defineProperty(BroadcastSubscriptionRequest, "_typeName", {
        value : "joynr.BroadcastSubscriptionRequest",
        readable : true,
        writable : false,
        enumerable : true,
        configurable : false
    });
    module.exports = BroadcastSubscriptionRequest;
