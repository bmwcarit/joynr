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
var ProxyAttribute = require('./ProxyAttribute');
module.exports =
        (function(ProxyAttribute) {

    /**
     * Constructor of ProxyAttribute* object that is used in the generation of proxy objects
     *
     * @constructor
     * @name ProxyAttributeNotifyRead
     * @param {Object}
     *            parent is the proxy object that contains this attribute
     * @param {String}
     *            parent.fromParticipantId of the proxy itself
     * @param {String}
     *            parent.toParticipantId of the provider being addressed
     * @param {Object}
     *            settings the settings object for this function call
     * @param {DiscoveryQos}
     *            settings.discoveryQos the Quality of Service parameters for arbitration
     * @param {MessagingQos}
     *            settings.messagingQos the Quality of Service parameters for messaging
     * @param {String}
     *            attributeName the name of the attribute
     * @param {String}
     *            attributeType the type of the attribute
     */
    function ProxyAttributeNotifyRead(parent, settings, attributeName, attributeType) {
        if (!(this instanceof ProxyAttributeNotifyRead)) {
            // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
            return new ProxyAttributeNotifyRead(parent, settings, attributeName, attributeType);
        }

        var proxyAttribute =
                new ProxyAttribute(parent, settings, attributeName, attributeType, "NOTIFYREAD");

        /**
         * Getter for attribute
         *
         * @name ProxyAttributeNotifyRead#get
         * @function
         *
         * @param {Object}
         *            [settings] the settings object for this function call
         * @returns {Object} returns an A+ promise object
         */
        this.get = function get(settings) {
            return proxyAttribute.get(settings);
        };

        /**
         * Subscription to attribute
         *
         * @name ProxyAttributeNotifyRead#subscribe
         * @function
         *
         * @param {Object}
         *            settings the settings object for this function call
         *
         * @param {Object|SubscriptionQos}
         *            settings.subscriptionQos the subscription quality of service object
         *
         * @param {string}
         *            settings.subscriptionId optional parameter subscriptionId to reuse a
         *            preexisting identifier for this concrete subscription request
         *
         * @param {Function}
         *            settings.onReceive this function is called if the attribute has been published
         *            successfully, method signature: "void onReceive({?}value)"
         * @param {Function}
         *            settings.onError this function is called if a publication of the attribute
         *            value was missed, method signature: "void onError({Error} error)"
         * @param {Function}
         *            settings.onSubscribed the callback to inform once the subscription request has
         *            been delivered successfully
         * @returns {Object} returns an A+ promise object. This object additionally holds the unique
         *            subscriptionId, used for unsubscribing from the attribute in the key
         *            subscriptionId
         */
        this.subscribe = function subscribe(settings) {
            return proxyAttribute.subscribe(settings);
        };

        /**
         * Unsubscribe from the attribute
         *
         * @name ProxyAttributeNotifyRead#unsubscribe
         * @function
         *
         * @param {Object}
         *            settings the settings object for this function call
         * @param {Object}
         *            subscriptionId the subscription token retrieved from the subscribe function
         * @returns {Object} returns an A+ promise
         *          "then(function(){..}).catch(error(function ({string}error){..})"
         * @throws {string}
         *             if the subscription does not exist
         * @see ProxyAttributeNotifyRead#subscribe
         */
        this.unsubscribe = function unsubscribe(settings) {
            return proxyAttribute.unsubscribe(settings);
        };

        return Object.freeze(this);
    }

    return ProxyAttributeNotifyRead;

        }(ProxyAttribute));