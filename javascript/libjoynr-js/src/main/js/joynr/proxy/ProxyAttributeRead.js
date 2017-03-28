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

define("joynr/proxy/ProxyAttributeRead", [ "joynr/proxy/ProxyAttribute"
], function(ProxyAttribute) {

    /**
     * Constructor of ProxyAttribute* object that is used in the generation of proxy objects
     *
     * @constructor
     * @name ProxyAttributeRead
     *
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
    function ProxyAttributeRead(parent, settings, attributeName, attributeType) {
        if (!(this instanceof ProxyAttributeRead)) {
            // in case someone calls constructor without new keyword
            // (e.g. var c = Constructor({..}))
            return new ProxyAttributeRead(parent, settings, attributeName, attributeType);
        }

        var proxyAttribute =
                new ProxyAttribute(parent, settings, attributeName, attributeType, "READ");

        /**
         * Getter for attribute
         *
         * @name ProxyAttributeRead#get
         * @function
         *
         * @param {Object}
         *            [settings] the settings object for this function call
         * @returns {Object} returns an A+ promise object that will alternatively accept callback
         *            functions through its setters
         *          "then(function ({?}value){..}).catch(function ({string}error){..})"
         */
        this.get = function get(settings) {
            return proxyAttribute.get(settings);
        };

        return Object.freeze(this);
    }

    return ProxyAttributeRead;

});