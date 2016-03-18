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

define("joynr/proxy/DiscoveryQos", [
    "joynr/types/ArbitrationStrategyCollection",
    "joynr/types/DiscoveryScope",
    "joynr/util/UtilInternal"
], function(ArbitrationStrategyCollection, DiscoveryScope, Util) {

    var defaultSettings = {
        discoveryTimeout : 30000,
        discoveryRetryDelay : 1000,
        arbitrationStrategy : ArbitrationStrategyCollection.HighestPriority,
        cacheMaxAge : 0,
        discoveryScope : DiscoveryScope.LOCAL_THEN_GLOBAL,
        additionalParameters : {}
    };

    /**
     * Constructor of DiscoveryQos object that is used in the generation of proxy objects
     *
     * @constructor
     * @name DiscoveryQos
     *
     * @param {Object} [settings] the settings object for the constructor call
     * @param {Number} [settings.discoveryTimeout] for rpc calls to wait for arbitration to finish.
     * @param {Number} [settings.discoveryRetryDelay] the minimum delay between two arbitration retries
     * @param {Function} [settings.arbitrationStrategy] Strategy for choosing the appropriate provider from the list returned by the capabilities directory with the function signature "function(CapabilityInfos[])" that returns an array of CapabilityInfos
     * @param {Number} [settings.cacheMaxAge] Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the local capabilities directory a lookup in the global capabilitiesDirectory will take place.
     * @param {DiscoveryScope} [settings.discoveryScope] default  is LOCAL_AND_GLOBAL
     * @param {Object} [settings.additionalParameters] a map holding additional parameters in the form of key value pairs in the javascript object, e.g.: {"myKey": "myValue", "myKey2": 5}
     *
     * @returns {DiscoveryQos} a discovery Qos Object
     */
    function DiscoveryQos(settings) {
        if (!(this instanceof DiscoveryQos)) {
            // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
            return new DiscoveryQos(settings);
        }

        settings = Util.extend({}, defaultSettings, settings);

        /**
         * @name DiscoveryQos#discoveryTimeout
         * @type Number
         */
        this.discoveryTimeout = settings.discoveryTimeout;

        /**
         * @name DiscoveryQos#discoveryRetryDelay
         * @type Number
         */
        this.discoveryRetryDelay = settings.discoveryRetryDelay;

        /**
         * @name DiscoveryQos#arbitrationStrategy
         * @type Function
         * @see ArbitrationStrategyCollection
         */
        this.arbitrationStrategy = settings.arbitrationStrategy;

        /**
         * @name DiscoveryQos#cacheMaxAge
         * @type Number
         */
        this.cacheMaxAge = settings.cacheMaxAge;

        /**
         * @name DiscoveryQos#discoveryScope
         */
        this.discoveryScope = settings.discoveryScope;

        /**
         * @name DiscoveryQos#additionalParameters
         * @type Object
         */
        this.additionalParameters = settings.additionalParameters;
    }

    return DiscoveryQos;

});
