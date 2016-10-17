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

define("joynr/dispatching/types/MulticastSubscriptionRequest", [
    "joynr/util/UtilInternal",
    "joynr/util/Typing",
    "joynr/proxy/OnChangeSubscriptionQos",
    "joynr/proxy/BroadcastFilterParameters"
], function(Util, Typing, OnChangeSubscriptionQos, BroadcastFilterParameters) {

    var defaultSettings = {
        qos : new OnChangeSubscriptionQos()
    };

    /**
     * @name MulticastSubscriptionRequest
     * @constructor
     * @param {String}
     *            settings.subscriptionId Id of the new subscription
     * @param {String}
     *            settings.subscribedToName the name of the element to subscribe to
     * @param {Object|SubscriptionQos}
     *            [settings.subscriptionQos] the subscriptionQos
     */
    function MulticastSubscriptionRequest(settings) {
        Util.checkProperty(settings, "Object", "settings");
        Util.checkProperty(settings.multicastId, "String", "settings.multicastId");
        Util.checkProperty(settings.subscriptionId, "String", "settings.subscriptionId");
        Util.checkProperty(settings.subscribedToName, "String", "settings.subscribedToName");
        Util.checkPropertyIfDefined(settings.qos, [
            "Object",
            "OnChangeSubscriptionQos"
        ], "settings.qos");

        /**
         * @name MulticastSubscriptionRequest#multicastId
         * @type String
         */
        this.multicastId = settings.multicastId;
        /**
         * @name MulticastSubscriptionRequest#subscriptionId
         * @type String
         */
        this.subscriptionId = settings.subscriptionId;
        /**
         * @name MulticastSubscriptionRequest#subscribedToName
         * @type String
         */
        this.subscribedToName = settings.subscribedToName;
        /**
         * @name MulticastSubscriptionRequest#qos
         * @type Object|OnChangeSubscriptionQos
         */
        this.qos = settings.qos || defaultSettings.qos;
        /**
         * The joynr type name
         *
         * @name MulticastSubscriptionRequest#_typeName
         * @type String
         */
        Object.defineProperty(this, "_typeName", {
            value : "joynr.MulticastSubscriptionRequest",
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
    Object.defineProperty(MulticastSubscriptionRequest, "_typeName", {
        value : "joynr.MulticastSubscriptionRequest",
        readable : true,
        writable : false,
        enumerable : true,
        configurable : false
    });

    return MulticastSubscriptionRequest;

});
