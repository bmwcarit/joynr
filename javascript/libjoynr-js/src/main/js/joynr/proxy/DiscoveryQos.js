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

define(
        "joynr/proxy/DiscoveryQos",
        [
            "joynr/types/ArbitrationStrategyCollection",
            "joynr/types/DiscoveryScope",
            "joynr/util/UtilInternal",
            "joynr/system/LoggerFactory"
        ],
        function(ArbitrationStrategyCollection, DiscoveryScope, Util, LoggerFactory) {

            var defaultSettings = {
                discoveryTimeoutMs : 30000,
                discoveryRetryDelayMs : 1000,
                arbitrationStrategy : ArbitrationStrategyCollection.HighestPriority,
                cacheMaxAgeMs : 0,
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
             * @param {Number} [settings.discoveryTimeoutMs] for rpc calls to wait for arbitration to finish.
             * @param {Number} [settings.discoveryRetryDelayMs] the minimum delay between two arbitration retries
             * @param {Function} [settings.arbitrationStrategy] Strategy for choosing the appropriate provider from the list returned by the capabilities directory with the function signature "function(CapabilityInfos[])" that returns an array of CapabilityInfos
             * @param {Number} [settings.cacheMaxAgeMs] Maximum age of entries in the localCapabilitiesDirectory. If this value filters out all entries of the local capabilities directory a lookup in the global capabilitiesDirectory will take place.
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

                var log = LoggerFactory.getLogger("joynr.proxy.DiscoveryQos");

                if (settings && !(settings instanceof DiscoveryQos)) {
                    if (settings.discoveryTimeout !== undefined) {
                        log
                                .warn("DiscoveryQos has been invoked with deprecated settings member \"discoveryTimeout\". "
                                    + "By 2017-01-01, the discovery timeout can only be specified with member \"discoveryTimeoutMs\".");
                        settings.discoveryTimeoutMs = settings.discoveryTimeout;
                        settings.discoveryTimeout = undefined;
                    }

                    if (settings.discoveryRetryDelay !== undefined) {
                        log
                                .warn("DiscoveryQos has been invoked with deprecated settings member \"discoveryRetryDelay\". "
                                    + "By 2017-01-01, the discovery retry delay can only be specified with member \"discoveryRetryDelayMs\".");
                        settings.discoveryRetryDelayMs = settings.discoveryRetryDelay;
                        settings.discoveryRetryDelay = undefined;
                    }

                    if (settings.cacheMaxAge !== undefined) {
                        log
                                .warn("DiscoveryQos has been invoked with deprecated settings member \"cacheMaxAge\". "
                                    + "By 2017-01-01, the cache max age can only be specified with member \"cacheMaxAgeMs\".");
                        settings.cacheMaxAgeMs = settings.cacheMaxAge;
                        settings.cacheMaxAge = undefined;
                    }
                }

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
                 * @name DiscoveryQos#additionalParameters
                 * @type Object
                 */
                this.additionalParameters = settings.additionalParameters;

                /**
                 * @name DiscoveryQos#cacheMaxAge
                 * @type Number
                 * @deprecated cacheMaxAge will be removed by 2017-01-01, please use cacheMaxAgeMs instead
                 */
                Object.defineProperty(this, "cacheMaxAge", {
                    get : function() {
                        return this.cacheMaxAgeMs;
                    },
                    set : function(val) {
                        this.cacheMaxAgeMs = val;
                    }
                });

                /**
                 * @name DiscoveryQos#discoveryRetryDelay
                 * @type Number
                 * @deprecated discoveryRetryDelay will be removed by 2017-01-01, please use discoveryRetryDelayMs instead
                 */
                Object.defineProperty(this, "discoveryRetryDelay", {
                    get : function() {
                        return this.discoveryRetryDelayMs;
                    },
                    set : function(val) {
                        this.discoveryRetryDelayMs = val;
                    }
                });

                /**
                 * @name DiscoveryQos#discoveryTimeout
                 * @type Number
                 * @deprecated discoveryTimeout will be removed by 2017-01-01, please use discoveryTimeoutMs instead
                 */
                Object.defineProperty(this, "discoveryTimeout", {
                    get : function() {
                        return this.discoveryTimeoutMs;
                    },
                    set : function(val) {
                        this.discoveryTimeoutMs = val;
                    }
                });
            }

            return DiscoveryQos;

        });
