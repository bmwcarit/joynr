/*jslint es5: true, nomen: true, node: true */

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
var Promise = require("../../../global/Promise");
var DiscoveryQos = require("../../../joynr/types/DiscoveryQos");
var Util = require("../../util/UtilInternal");
var DiscoveryException = require("../../exceptions/DiscoveryException");
var NoCompatibleProviderFoundException = require("../../exceptions/NoCompatibleProviderFoundException");
var LongTimer = require("../../util/LongTimer");

/**
 * checks if the provided discoveryEntry supports onChange subscriptions if required
 *
 * @name Arbitrator#checkSupportsOnChangeSubscriptions
 * @function
 *
 * @param {DiscoveryEntry} discoveryEntry - the discovery entry to check
 * @param {Boolean} providerMustSupportOnChange - filter only entries supporting onChange subscriptions
 */
function checkSupportsOnChangeSubscriptions(discoveryEntry, providerMustSupportOnChange) {
    if (providerMustSupportOnChange === undefined || !providerMustSupportOnChange) {
        return true;
    }
    if (discoveryEntry.qos === undefined) {
        return false;
    }
    return discoveryEntry.qos.supportsOnChangeSubscriptions;
}

function discoverStaticCapabilities(capabilities, domains, interfaceName, discoveryQos, proxyVersion, deferred) {
    try {
        var i,
            capability,
            arbitratedCaps = [];

        deferred.pending = false;
        if (capabilities === undefined) {
            deferred.reject(new Error("Exception while arbitrating: static capabilities are missing"));
        } else {
            for (i = 0; i < capabilities.length; ++i) {
                capability = capabilities[i];
                if (
                    domains.indexOf(capability.domain) !== -1 &&
                    interfaceName === capability.interfaceName &&
                    capability.providerVersion.MAJOR_VERSION === proxyVersion.MAJOR_VERSION &&
                    capability.providerVersion.MINOR_VERSION === proxyVersion.MINOR_VERSION &&
                    checkSupportsOnChangeSubscriptions(capability, discoveryQos.providerMustSupportOnChange)
                ) {
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
 * Tries to discover capabilities with given domains, interfaceName and discoveryQos within the localCapDir as long as the deferred's state is pending
 * @private
 *
 * @param {CapabilityDiscovery} capabilityDiscoveryStub - the capabilites discovery module
 * @param {String} domains - the domains
 * @param {String} interfaceName - the interfaceName
 * @param {joynr.capabilities.discovery.DiscoveryQos} discoveryQos - the discoveryQos object determining the arbitration strategy and timeouts
 * @returns {Object} a Promise/A+ object, that will provide an array of discovered capabilities
 */
function discoverCapabilities(
    capabilityDiscoveryStub,
    domains,
    interfaceName,
    applicationDiscoveryQos,
    proxyVersion,
    deferred
) {
    function discoveryCapabilitiesRetry() {
        delete deferred.discoveryRetryTimer;
        discoverCapabilities(
            capabilityDiscoveryStub,
            domains,
            interfaceName,
            applicationDiscoveryQos,
            proxyVersion,
            deferred
        );
    }

    function retryCapabilityDiscovery(errorMsg) {
        // retry discovery in discoveryRetryDelayMs ms
        if (errorMsg) {
            deferred.errorMsg = errorMsg;
        }
        deferred.discoveryRetryTimer = LongTimer.setTimeout(
            discoveryCapabilitiesRetry,
            applicationDiscoveryQos.discoveryRetryDelayMs
        );
    }

    function capabilitiesDiscovered(discoveredCaps) {
        // filter caps according to chosen arbitration strategy
        var arbitratedCaps = applicationDiscoveryQos.arbitrationStrategy(discoveredCaps);
        var versionCompatibleArbitratedCaps = [];
        var i;
        // if deferred is still pending => discoveryTimeoutMs is not expired yet
        if (deferred.pending) {
            // if there are caps found
            deferred.incompatibleVersionsFound = [];
            var providerVersion;

            var addtoListIfNotExisting = function(list, providerVersion) {
                var j;
                for (j = 0; j < list.length; ++j) {
                    if (
                        list[j].majorVersion === providerVersion.majorVersion &&
                        list[j].minorVersion === providerVersion.minorVersion
                    ) {
                        return;
                    }
                }
                list.push(providerVersion);
            };

            for (i = 0; i < arbitratedCaps.length; i++) {
                providerVersion = arbitratedCaps[i].providerVersion;
                if (
                    providerVersion.majorVersion === proxyVersion.majorVersion &&
                    providerVersion.minorVersion >= proxyVersion.minorVersion &&
                    checkSupportsOnChangeSubscriptions(
                        arbitratedCaps[i],
                        applicationDiscoveryQos.providerMustSupportOnChange
                    )
                ) {
                    versionCompatibleArbitratedCaps.push(arbitratedCaps[i]);
                } else {
                    addtoListIfNotExisting(deferred.incompatibleVersionsFound, providerVersion);
                }
            }
            if (versionCompatibleArbitratedCaps.length > 0) {
                // report the discovered & arbitrated caps
                deferred.pending = false;
                deferred.resolve(versionCompatibleArbitratedCaps);
            } else {
                retryCapabilityDiscovery();
            }
        }
        return null;
    }

    function capabilitiesDiscoveredError(error) {
        if (deferred.pending) {
            retryCapabilityDiscovery(error.message);
        }
    }

    // discover caps from local capabilities directory
    capabilityDiscoveryStub
        .lookup(
            domains,
            interfaceName,
            new DiscoveryQos({
                discoveryScope: applicationDiscoveryQos.discoveryScope,
                cacheMaxAge: applicationDiscoveryQos.cacheMaxAgeMs,
                discoveryTimeout: applicationDiscoveryQos.discoveryTimeoutMs,
                providerMustSupportOnChange: applicationDiscoveryQos.providerMustSupportOnChange
            })
        )
        .then(capabilitiesDiscovered)
        .catch(capabilitiesDiscoveredError);
}

/**
 * An arbitrator looks up all capabilities for given domains and an interface and uses the provides arbitraionStrategy passed in the
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

    this._staticCapabilities = staticCapabilities;
    this._capabilityDiscoveryStub = capabilityDiscoveryStub;
    this._pendingArbitrations = {};
    this._arbitrationId = 0;
    this._started = true;
}

/**
 * Starts the arbitration process
 *
 * @name Arbitrator#startArbitration
 * @function
 *
 * @param {Object} settings the settings object
 * @param {String} settings.domains the domains to discover the provider
 * @param {String} settings.interfaceName the interfaceName to discover the provider
 * @param {DiscoveryQos} settings.discoveryQos
 * @param {Boolean} [settings.staticArbitration] shall the arbitrator use staticCapabilities or contact the discovery provider
 * @param {Version} [settings.proxyVersion] the version of the proxy object
 * @returns {Object} a A+ Promise object, that will provide asynchronously an array of arbitrated capabilities
 */
Arbitrator.prototype.startArbitration = function startArbitration(settings) {
    var that = this;

    settings = Util.extendDeep({}, settings);

    function arbitrationResolver(resolve, reject) {
        if (!that._started) {
            reject(new Error("Arbitrator is already shut down"));
            return;
        }
        this._arbitrationId++;
        var deferred = {
            id: this._arbitrationId,
            resolve: resolve,
            reject: reject,
            incompatibleVersionsFound: [],
            pending: true
        };

        function discoveryCapabilitiesTimeOutHandler() {
            deferred.pending = false;
            delete that._pendingArbitrations[deferred.id];

            if (deferred.incompatibleVersionsFound.length > 0) {
                var message =
                    'no compatible provider found within discovery timeout for domains "' +
                    JSON.stringify(settings.domains) +
                    '", interface "' +
                    settings.interfaceName +
                    '" with discoveryQos "' +
                    JSON.stringify(settings.discoveryQos) +
                    '"';
                reject(
                    new NoCompatibleProviderFoundException({
                        detailMessage: message,
                        discoveredVersions: deferred.incompatibleVersionsFound,
                        interfaceName: settings.interfaceName
                    })
                );
            } else {
                reject(
                    new DiscoveryException({
                        detailMessage:
                            'no provider found within discovery timeout for domains "' +
                            JSON.stringify(settings.domains) +
                            '", interface "' +
                            settings.interfaceName +
                            '" with discoveryQos "' +
                            JSON.stringify(settings.discoveryQos) +
                            '"' +
                            (deferred.errorMsg !== undefined ? ". Error: " + deferred.errorMsg : "")
                    })
                );
            }
        }

        if (settings.staticArbitration && that._staticCapabilities) {
            discoverStaticCapabilities(
                that._staticCapabilities,
                settings.domains,
                settings.interfaceName,
                settings.discoveryQos,
                settings.proxyVersion,
                deferred
            );
        } else {
            that._pendingArbitrations[deferred.id] = deferred;
            deferred.discoveryTimeoutMsId = LongTimer.setTimeout(
                discoveryCapabilitiesTimeOutHandler.bind(this),
                settings.discoveryQos.discoveryTimeoutMs
            );
            var resolveWrapper = function(args) {
                LongTimer.clearTimeout(deferred.discoveryTimeoutMsId);
                delete that._pendingArbitrations[deferred.id];
                resolve(args);
            };
            var rejectWrapper = function(args) {
                LongTimer.clearTimeout(deferred.discoveryTimeoutMsId);
                delete that._pendingArbitrations[deferred.id];
                reject(args);
            };
            deferred.resolve = resolveWrapper;
            deferred.reject = rejectWrapper;
            discoverCapabilities(
                that._capabilityDiscoveryStub,
                settings.domains,
                settings.interfaceName,
                settings.discoveryQos,
                settings.proxyVersion,
                deferred
            );
        }
    }

    return new Promise(arbitrationResolver);
};

/**
 * Shutdown the Arbitrator
 *
 * @function
 * @name Arbitrator#shutdown
 */
Arbitrator.prototype.shutdown = function shutdown() {
    var id;
    for (id in this._pendingArbitrations) {
        if (this._pendingArbitrations.hasOwnProperty(id)) {
            var pendingArbitration = this._pendingArbitrations[id];
            if (pendingArbitration.discoveryTimeoutMsId !== undefined) {
                LongTimer.clearTimeout(pendingArbitration.discoveryTimeoutMsId);
            }
            if (pendingArbitration.discoveryRetryTimer !== undefined) {
                LongTimer.clearTimeout(pendingArbitration.discoveryRetryTimer);
            }
            pendingArbitration.reject(new Error("Arbitration is already shut down"));
        }
    }
    this._pendingArbitrations = {};
    this._started = false;
};

module.exports = Arbitrator;
