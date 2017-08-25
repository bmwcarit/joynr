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

define("joynr/proxy/DiscoveryQos", [
    "joynr/types/ArbitrationStrategyCollection",
    "joynr/types/DiscoveryScope",
    "joynr/util/UtilInternal",
    "joynr/system/LoggerFactory"
], function(ArbitrationStrategyCollection, DiscoveryScope, Util, LoggerFactory) {

    var defaultSettings = {
        discoveryTimeoutMs : 10 * 60 * 1000, // 10 minutes
        discoveryRetryDelayMs : 10 * 1000, // 10 seconds
        arbitrationStrategy : ArbitrationStrategyCollection.LastSeen,
        cacheMaxAgeMs : 0,
        discoveryScope : DiscoveryScope.LOCAL_THEN_GLOBAL,
        providerMustSupportOnChange : false,
        additionalParameters : {}
    };

    /**
     * Constructor of DiscoveryQos object that is used in the generation of proxy objects
     *
     * @constructor
     * @name DiscoveryQos
     *
     * @param {Object} [settings] the settings object for the constructor call
     * @param {Number} [settings.discoveryTimeoutMs] for rpc calls to wait for arbitration to finish.
     * @param {Number} [settings.discoveryRetryDelayMs] the minimum delay between two arbitration retries
     * @param {Function} [settings.arbitrationStrategy] Strategy for choosing the appropriate provider from the list returned by the capabilities directory with the function signature "function(CapabilityInfos[])" that returns an array of CapabilityInfos
     * @param {Number} [settings.cacheMaxAgeMs] Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the local capabilities directory a lookup in the global capabilitiesDirectory will take place.
     * @param {DiscoveryScope} [settings.discoveryScope] default  is LOCAL_AND_GLOBAL
     * @param {Boolean} [settings.providerMustSupportOnChange] default false
     * @param {Object} [settings.additionalParameters] a map holding additional parameters in the form of key value pairs in the javascript object, e.g.: {"myKey": "myValue", "myKey2": 5}
     *
     * @returns {DiscoveryQos} a discovery Qos Object
     */
    function DiscoveryQos(settings) {
        if (!(this instanceof DiscoveryQos)) {
            // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
            return new DiscoveryQos(settings);
        }

        var log = LoggerFactory.getLogger("joynr.proxy.DiscoveryQos");

        settings = Util.extend({}, defaultSettings, settings);

        /**
         * @name DiscoveryQos#discoveryTimeoutMs
         * @type Number
         */
        this.discoveryTimeoutMs = settings.discoveryTimeoutMs;

        /**
         * @name DiscoveryQos#discoveryRetryDelayMs
         * @type Number
         */
        this.discoveryRetryDelayMs = settings.discoveryRetryDelayMs;

        /**
         * @name DiscoveryQos#arbitrationStrategy
         * @type Function
         * @see ArbitrationStrategyCollection
         */
        this.arbitrationStrategy = settings.arbitrationStrategy;

        /**
         * @name DiscoveryQos#cacheMaxAgeMs
         * @type Number
         */
        this.cacheMaxAgeMs = settings.cacheMaxAgeMs;

        /**
         * @name DiscoveryQos#discoveryScope
         */
        this.discoveryScope = settings.discoveryScope;

        /**
         * @name DiscoveryQos#providerMustSupportOnChange
         */
        this.providerMustSupportOnChange = settings.providerMustSupportOnChange;

        /**
         * @name DiscoveryQos#additionalParameters
         * @type Object
         */
        this.additionalParameters = settings.additionalParameters;
    }

    DiscoveryQos.setDefaultSettings = function(settings) {
        defaultSettings = Util.extend({}, defaultSettings, settings);
    };

    return DiscoveryQos;

});
