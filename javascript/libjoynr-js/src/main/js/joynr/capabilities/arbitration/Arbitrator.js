/*jslint es5: true*/

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
        "joynr/capabilities/arbitration/Arbitrator",
        [
            "global/Promise",
            "joynr/types/DiscoveryQos",
            "joynr/util/UtilInternal",
            "joynr/util/LongTimer"
        ],
        function(Promise, DiscoveryQos, Util, LongTimer) {
            function discoverStaticCapabilities(
                    capabilities,
                    domain,
                    interfaceName,
                    discoveryQos,
                    deferred) {
                try {
                    var i, capability, arbitratedCaps = [];

                    deferred.pending = false;
                    if (capabilities === undefined) {
                        deferred.reject(new Error("Exception while arbitrating: static capabilities are missing"));
                    } else {
                        for (i = 0; i < capabilities.length; ++i) {
                            capability = capabilities[i];
                            if (domain === capability.domain
                                    && interfaceName === capability.interfaceName) {
                                arbitratedCaps.push(capability);
                            }
                        }
                        deferred.resolve(discoveryQos.arbitrationStrategy(arbitratedCaps));
                    }
                } catch (e) {
                    deferred.pending = false;
                    deferred.reject(new Error("Exception while arbitrating: " + e));
                }
            }

            /**
             * Tries to discover capabilities with given domain, interfaceName and discoveryQos within the localCapDir as long as the deferred's state is pending
             * @private
             *
             * @param {CapabilityDiscovery} capabilityDiscoveryStub - the capabilites discovery module
             * @param {String} domain - the domain
             * @param {String} interfaceName - the interfaceName
             * @param {joynr.capabilities.discovery.DiscoveryQos} discoveryQos - the discoveryQos object determining the arbitration strategy and timeouts
             * @returns {Object} a Promise/A+ object, that will provide an array of discovered capabilities
             */
            function discoverCapabilities(
                    capabilityDiscoveryStub,
                    domain,
                    interfaceName,
                    applicationDiscoveryQos,
                    deferred) {
                // discover caps from local capabilities directory
                capabilityDiscoveryStub.lookup(domain, interfaceName, new DiscoveryQos({
                    discoveryScope : applicationDiscoveryQos.discoveryScope,
                    cacheMaxAge : applicationDiscoveryQos.cacheMaxAge
                })).then(
                        function(discoveredCaps) {
                            // filter caps according to chosen arbitration strategy
                            var arbitratedCaps =
                                    applicationDiscoveryQos.arbitrationStrategy(discoveredCaps);
                            // if deferred is still pending => discoveryTimeout is not expired yet
                            if (deferred.pending) {
                                // if there are caps found
                                if (arbitratedCaps.length > 0) {
                                    // report the discovered & arbitrated caps
                                    deferred.pending = false;
                                    deferred.resolve(arbitratedCaps);
                                } else {
                                    // retry discovery in discoveryRetryDelay ms
                                    LongTimer.setTimeout(function discoveryCapabilitiesRetry() {
                                        discoverCapabilities(
                                                capabilityDiscoveryStub,
                                                domain,
                                                interfaceName,
                                                applicationDiscoveryQos,
                                                deferred);
                                    }, applicationDiscoveryQos.discoveryRetryDelay);
                                }
                            }
                        }).catch(function(error) {
                            deferred.pending = false;
                            deferred.reject(error);
                        });
            }

            /**
             * An arbitrator looks up all capabilities for a given domain and interface and uses the provides arbitraionStrategy passed in the
             * discoveryQos to choose one or more for the calling proxy
             *
             * @name Arbitrator
             * @constructor
             *
             * @param {CapabilityDiscovery} capabilityDiscoveryStub the capability discovery
             * @param {Array} capabilities the capabilities the arbitrator will use to resolve capabilities in case of static arbitration
             * @returns {Arbitrator} an Arbitrator instance
             */
            function Arbitrator(capabilityDiscoveryStub, staticCapabilities) {
                if (!(this instanceof Arbitrator)) {
                    // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
                    return new Arbitrator(capabilityDiscoveryStub, staticCapabilities);
                }

                /**
                 * Starts the arbitration process
                 *
                 * @name Arbitrator#startArbitration
                 * @function
                 *
                 * @param {Object} settings the settings object
                 * @param {String} settings.domain the domain to discover the provider
                 * @param {String} settings.interfaceName the interfaceName to discover the provider
                 * @param {DiscoveryQos} settings.discoveryQos
                 * @param {Boolean} [settings.staticArbitration] shall the arbitrator use staticCapabilities or contact the discovery provider
                 * @returns {Object} a A+ Promise object, that will provide asynchronously an array of arbitrated capabilities
                 */
                this.startArbitration =
                        function startArbitration(settings) {
                            settings = Util.extendDeep({}, settings);
                            return new Promise(
                                    function(resolve, reject) {
                                        var deferred = {
                                            resolve : resolve,
                                            reject : reject,
                                            pending : true
                                        };
                                        if (settings.staticArbitration && staticCapabilities) {
                                            discoverStaticCapabilities(
                                                    staticCapabilities,
                                                    settings.domain,
                                                    settings.interfaceName,
                                                    settings.discoveryQos,
                                                    deferred);
                                        } else {
                                            var discoveryTimeoutId =
                                                    LongTimer
                                                            .setTimeout(
                                                                    function discoveryCapabilitiesTimeOut() {
                                                                        deferred.pending = false;
                                                                        reject(new Error(
                                                                                "no provider found within discovery timeout for domain \""
                                                                                    + settings.domain
                                                                                    + "\", interface \""
                                                                                    + settings.interfaceName
                                                                                    + "\" with discoveryQos \""
                                                                                    + JSON
                                                                                            .stringify(settings.discoveryQos)
                                                                                    + "\""));
                                                                    },
                                                                    settings.discoveryQos.discoveryTimeout);
                                            var resolveWrapper = function(args) {
                                                LongTimer.clearTimeout(discoveryTimeoutId);
                                                resolve(args);
                                            };
                                            deferred.resolve = resolveWrapper;
                                            discoverCapabilities(
                                                    capabilityDiscoveryStub,
                                                    settings.domain,
                                                    settings.interfaceName,
                                                    settings.discoveryQos,
                                                    deferred);
                                        }
                                    });
                        };
            }

            return Arbitrator;
        });
