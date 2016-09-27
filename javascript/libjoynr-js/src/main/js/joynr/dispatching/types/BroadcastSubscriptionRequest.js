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

define("joynr/dispatching/types/BroadcastSubscriptionRequest", [
    "joynr/util/UtilInternal",
    "joynr/util/Typing",
    "joynr/proxy/OnChangeSubscriptionQos",
    "joynr/proxy/BroadcastFilterParameters"
], function(Util, Typing, OnChangeSubscriptionQos, BroadcastFilterParameters) {

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
        Util.checkProperty(settings, "Object", "settings");
        Util.checkProperty(settings.subscriptionId, "String", "settings.subscriptionId");
        Util.checkProperty(settings.subscribedToName, "String", "settings.subscribedToName");
        Util.checkPropertyIfDefined(settings.qos, [
            "Object",
            "OnChangeSubscriptionQos"
        ], "settings.qos");
        Util.checkPropertyIfDefined(settings.filterParameters, [
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
         * @name BroadcastSubscriptionRequest#subscribedToName
         * @type String
         */
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

        return Object.freeze(this);
    }

    return BroadcastSubscriptionRequest;

});
