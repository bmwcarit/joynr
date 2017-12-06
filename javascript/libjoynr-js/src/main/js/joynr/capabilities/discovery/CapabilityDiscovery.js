/*jslint es5: true, node: true, continue: true */
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
/**
 * The <code>CapabilityDiscovery</code> is a joynr internal interface. When the Arbitrator does a lookup for capabilities, this module is
 * queried. If a provider needs to be registered, this module selects the places to register at.
 */
var Promise = require("../../../global/Promise");
var GlobalDiscoveryEntry = require("../../../joynr/types/GlobalDiscoveryEntry");
var DiscoveryQos = require("../../proxy/DiscoveryQos");
var DiscoveryScope = require("../../../joynr/types/DiscoveryScope");
var ProviderScope = require("../../../joynr/types/ProviderScope");
var GlobalCapabilitiesDirectoryProxy = require("../../infrastructure/GlobalCapabilitiesDirectoryProxy");
var TypeRegistrySingleton = require("../../../joynr/types/TypeRegistrySingleton");
var Typing = require("../../util/Typing");
var LoggerFactory = require("../../system/LoggerFactory");
var Util = require("../../util/UtilInternal");
var ProviderRuntimeException = require("../../exceptions/ProviderRuntimeException");
var CapabilitiesUtil = require("../../util/CapabilitiesUtil");

/**
 * The CapabilitiesDiscovery looks up the local and global capabilities directory
 *
 * @constructor
 * @name CapabilityDiscovery
 *
 * @param {CapabilitiesStore}
 *            localCapabilitiesStore the local capabilities store
 * @param {CapabilitiesStore}
 *            globalCapabilitiesCache the cache for the global capabilities directory
 * @param {MessageRouter}
 *            messageRouter the message router
 * @param {ProxyBuilder}
 *            proxyBuilder the proxy builder used to create the GlobalCapabilitiesDirectoryProxy
 * @param {String}
 *            globalCapabilitiesDomain the domain to communicate with the GlobalCapablitiesDirectory
 *                                     GlobalCapab
 *
 */
function CapabilityDiscovery(
    localCapabilitiesStore,
    globalCapabilitiesCache,
    messageRouter,
    proxyBuilder,
    globalCapabilitiesDomain
) {
    var log = LoggerFactory.getLogger("joynr/capabilities/discovery/CapabilityDiscovery");
    var TTL_30DAYS_IN_MS = 30 * 24 * 60 * 60 * 1000;
    var globalAddress, globalAddressSerialized;
    var typeRegistry = TypeRegistrySingleton.getInstance();
    var queuedGlobalDiscoveryEntries = [];
    var queuedGlobalLookups = [];

    if (
        !localCapabilitiesStore ||
        !localCapabilitiesStore.lookup ||
        !localCapabilitiesStore.add ||
        !localCapabilitiesStore.remove
    ) {
        throw new Error("constructor of CapabilityDiscovery requires the localCapabilitiesStore as argument");
    }

    if (
        !globalCapabilitiesCache ||
        !globalCapabilitiesCache.lookup ||
        !globalCapabilitiesCache.add ||
        !globalCapabilitiesCache.remove
    ) {
        throw new Error("constructor of CapabilityDiscovery requires the globalCapabilitiesCache as argument");
    }

    if (!messageRouter || !messageRouter.addNextHop) {
        throw new Error("constructor of CapabilityDiscovery requires the messageRouter as argument");
    }

    if (proxyBuilder === undefined) {
        throw new Error("constructor of CapabilityDiscovery requires the proxyBuilder as argument");
    }

    if (globalCapabilitiesDomain === undefined) {
        throw new Error("constructor of CapabilityDiscovery requires the globalCapabilitiesDomain");
    }

    /**
     * This method create a new global capabilities proxy with the provided ttl as messaging QoS
     *
     * @function
     * @name CapabilityDiscovery#getGlobalCapabilitiesDirectoryProxy
     *
     * @param {Number}
     *            ttl time to live of joynr messages triggered by the returning proxy
     *
     * @returns {GlobalCapabilitiesDirectoryProxy} the newly created proxy
     *
     */
    function getGlobalCapabilitiesDirectoryProxy(ttl) {
        return proxyBuilder
            .build(GlobalCapabilitiesDirectoryProxy, {
                domain: globalCapabilitiesDomain,
                messagingQos: {
                    ttl: ttl
                },
                discoveryQos: new DiscoveryQos({
                    discoveryScope: DiscoveryScope.GLOBAL_ONLY,
                    cacheMaxAgeMs: Util.getMaxLongValue()
                })
            })
            .catch(function(error) {
                throw new Error("Failed to create global capabilities directory proxy: " + error);
            });
    }

    function lookupGlobal(domains, interfaceName, ttl, capabilities) {
        return getGlobalCapabilitiesDirectoryProxy(ttl).then(function(globalCapabilitiesDirectoryProxy) {
            return globalCapabilitiesDirectoryProxy
                .lookup({
                    domains: domains,
                    interfaceName: interfaceName
                })
                .then(function(opArgs) {
                    var i,
                        messageRouterPromises = [],
                        globalCapabilities = opArgs.result;
                    var globalAddress;
                    if (globalCapabilities === undefined) {
                        log.error("globalCapabilitiesDirectoryProxy.lookup() returns with missing result");
                    } else {
                        for (i = globalCapabilities.length - 1; i >= 0; i--) {
                            var globalDiscoveryEntry = globalCapabilities[i];
                            if (globalDiscoveryEntry.address === globalAddressSerialized) {
                                globalCapabilities.splice(i, 1);
                            } else {
                                try {
                                    globalAddress = Typing.augmentTypes(
                                        JSON.parse(globalDiscoveryEntry.address),
                                        typeRegistry
                                    );
                                } catch (e) {
                                    log.error(
                                        "unable to use global discoveryEntry with unknown address type: " +
                                            globalDiscoveryEntry.address
                                    );
                                    continue;
                                }
                                // Update routing table
                                var isGloballyVisible = globalDiscoveryEntry.qos.scope === ProviderScope.GLOBAL;
                                messageRouterPromises.push(
                                    messageRouter.addNextHop(
                                        globalDiscoveryEntry.participantId,
                                        globalAddress,
                                        isGloballyVisible
                                    )
                                );
                                capabilities.push(
                                    CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(false, globalDiscoveryEntry)
                                );
                            }
                        }
                    }
                    return Promise.all(messageRouterPromises).then(function() {
                        return capabilities;
                    });
                });
        });
    }
    /**
     * expects a capabilities array which is then filled with any that are found from the proxy
     *
     * @function
     * @name CapabilityDiscovery#lookupGlobalCapabilities
     *
     * @param {String} domains - the domains
     * @param {String} interfaceName - the interface name
     * @param {Number} ttl - time to live of joynr messages triggered by the returning proxy
     * @param {Array} capabilities - the capabilities array to be filled
     *
     * @returns {Array} - the capabilities array filled with the capabilities found in the global capabilities directory
     */
    function lookupGlobalCapabilities(domains, interfaceName, ttl, capabilities) {
        var promise;
        if (!globalAddressSerialized) {
            var deferred = Util.createDeferred();
            queuedGlobalLookups.push({
                domains: domains,
                interfaceName: interfaceName,
                ttl: ttl,
                capabilities: capabilities,
                resolve: deferred.resolve,
                reject: deferred.reject
            });
            promise = deferred.promise;
        } else {
            promise = lookupGlobal(domains, interfaceName, ttl, capabilities);
        }
        return promise;
    }

    /**
     *
     * @param discoveryEntry to be added to the global discovery directory
     * @returns {Object} an A+ promise
     */
    function addGlobal(discoveryEntry) {
        return getGlobalCapabilitiesDirectoryProxy(TTL_30DAYS_IN_MS).then(function(globalCapabilitiesDirectoryProxy) {
            discoveryEntry.address = globalAddressSerialized;
            return globalCapabilitiesDirectoryProxy
                .add({
                    globalDiscoveryEntry: new GlobalDiscoveryEntry(discoveryEntry)
                })
                .catch(function(error) {
                    throw new Error('Error calling operation "add" of GlobalCapabilitiesDirectory because: ' + error);
                });
        });
    }

    /**
     *
     * @param {Object} queuedDiscoveryEntry contains a discoveryEntry and the resolve
     * and reject functions from the original Promise created on add().
     */
    function addGlobalQueued(queuedDiscoveryEntry) {
        addGlobal(queuedDiscoveryEntry.discoveryEntry)
            .then(queuedDiscoveryEntry.resolve)
            .catch(queuedDiscoveryEntry.reject);
    }

    /**
     * This method is called when the global address has been created
     *
     * @function
     * @name CapabilityDiscovery#globalAddressReady
     *
     * @param {Address}
     *            globalAddress the address used to register discovery entries globally
     */
    this.globalAddressReady = function globalAddressReady(newGlobalAddress) {
        var i, parameters;
        globalAddress = newGlobalAddress;
        globalAddressSerialized = JSON.stringify(newGlobalAddress);
        for (i = 0; i < queuedGlobalDiscoveryEntries.length; i++) {
            addGlobalQueued(queuedGlobalDiscoveryEntries[i]);
        }
        queuedGlobalDiscoveryEntries = [];
        for (i = 0; i < queuedGlobalLookups.length; i++) {
            parameters = queuedGlobalLookups[i];
            lookupGlobal(parameters.domains, parameters.interfaceName, parameters.ttl, parameters.capabilities)
                .then(parameters.resolve)
                .catch(parameters.reject);
        }
        queuedGlobalLookups = [];
    };

    /**
     * This method queries the local and/or global capabilities directory according to the given discoveryStrategy given in the
     * DiscoveryQos object
     *
     * @function
     * @name CapabilityDiscovery#lookup
     *
     * @param {String}
     *            domains the domains
     * @param {String}
     *            interfaceName the interface name
     * @param {DiscoveryQos}
     *            discoveryQos the DiscoveryQos giving the strategy for discovering a capability
     * @param {DiscoveryScope}
     *            dDiscoveryQos.discoveryScope the strategy to discover capabilities
     * @returns {Object} an A+ Promise object, that will provide an array of discovered capabilities, callback signatures:
     *          then({Array[GlobalDiscoveryEntry]} discoveredCaps).catch({Error} error)
     */
    this.lookup = function lookup(domains, interfaceName, discoveryQos) {
        var localCapabilities, globalCapabilities;

        if (domains.length !== 1) {
            return Promise.reject(
                new ProviderRuntimeException({
                    detailMessage: "Cluster-controller does not yet support multi-proxy lookups."
                })
            );
        }

        switch (discoveryQos.discoveryScope.value) {
            // only interested in local results
            case DiscoveryScope.LOCAL_ONLY.value:
                localCapabilities = localCapabilitiesStore.lookup({
                    domains: domains,
                    interfaceName: interfaceName
                });
                return Promise.resolve(
                    CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(true, localCapabilities)
                );

            // if anything local use it. Otherwise lookup global.
            case DiscoveryScope.LOCAL_THEN_GLOBAL.value:
                localCapabilities = localCapabilitiesStore.lookup({
                    domains: domains,
                    interfaceName: interfaceName
                });
                if (localCapabilities.length > 0) {
                    return Promise.resolve(
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(true, localCapabilities)
                    );
                }
                globalCapabilities = globalCapabilitiesCache.lookup({
                    domains: domains,
                    interfaceName: interfaceName,
                    cacheMaxAge: discoveryQos.cacheMaxAge
                });
                if (globalCapabilities.length > 0) {
                    return Promise.resolve(
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(false, globalCapabilities)
                    );
                }
                return lookupGlobalCapabilities(domains, interfaceName, TTL_30DAYS_IN_MS, localCapabilities);

            // Use local results, but then lookup global
            case DiscoveryScope.LOCAL_AND_GLOBAL.value:
                localCapabilities = localCapabilitiesStore.lookup({
                    domains: domains,
                    interfaceName: interfaceName
                });
                globalCapabilities = globalCapabilitiesCache.lookup({
                    domains: domains,
                    interfaceName: interfaceName,
                    cacheMaxAge: discoveryQos.cacheMaxAge
                });
                if (globalCapabilities.length === 0) {
                    return lookupGlobalCapabilities(
                        domains,
                        interfaceName,
                        TTL_30DAYS_IN_MS,
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(true, localCapabilities)
                    );
                }
                return Promise.resolve(
                    CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(true, localCapabilities).concat(
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(false, globalCapabilities)
                    )
                );

            case DiscoveryScope.GLOBAL_ONLY.value:
                globalCapabilities = globalCapabilitiesCache.lookup({
                    domains: domains,
                    interfaceName: interfaceName,
                    cacheMaxAge: discoveryQos.cacheMaxAge
                });
                if (globalCapabilities.length > 0) {
                    return Promise.resolve(
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(false, globalCapabilities)
                    );
                }
                return lookupGlobalCapabilities(domains, interfaceName, TTL_30DAYS_IN_MS, globalCapabilities);
        }
    };

    /**
     * This method adds a capability in the local and/or global capabilities directory according to the given registration
     * strategy.
     *
     * @function
     * @name CapabilityDiscovery#add
     *
     * @param {String}
     *            domain of the capability
     * @param {String}
     *            interfaceName of the capability
     * @param {String}
     *            participantId of the capability
     * @param {ProviderQos}
     *            providerQos of the capability
     * @param {Array}
     *            array of communication middlewares
     *
     * @returns {Object} an A+ promise
     */
    this.add = function add(discoveryEntry) {
        localCapabilitiesStore.add({
            discoveryEntry: discoveryEntry,
            remote: false
        });
        var promise;
        discoveryEntry.lastSeenDateMs = Date.now();
        if (discoveryEntry.qos.scope === ProviderScope.LOCAL) {
            promise = Promise.resolve();
        } else if (discoveryEntry.qos.scope === ProviderScope.GLOBAL) {
            if (!globalAddressSerialized) {
                var deferred = Util.createDeferred();
                queuedGlobalDiscoveryEntries.push({
                    discoveryEntry: discoveryEntry,
                    resolve: deferred.resolve,
                    reject: deferred.reject
                });
                promise = deferred.promise;
            } else {
                promise = addGlobal(discoveryEntry);
            }
        } else {
            promise = Promise.reject(
                new Error('Encountered unknown ProviderQos scope "' + discoveryEntry.qos.scope + '"')
            );
        }
        return promise;
    };

    /**
     * This method sends a freshness update to the global capabilities directory to update
     * the lastSeenDateMs of the entries registered via the local cluster controller.
     *
     * @param clusterControllerId the channelId of the local cluster controller
     * @param ttlMs the time to live of the freshness update message
     *
     * @returns {Object} an A+ promise
     */
    this.touch = function touch(clusterControllerId, ttlMs) {
        return getGlobalCapabilitiesDirectoryProxy(ttlMs)
            .then(function(globalCapabilitiesDirectoryProxy) {
                return globalCapabilitiesDirectoryProxy.touch({ clusterControllerId: clusterControllerId });
            })
            .catch(function(error) {
                throw new Error('Error calling operation "touch" of GlobalCapabilitiesDirectory because: ' + error);
            });
    };

    /**
     * This method removes a capability from the global capabilities directory.
     *
     * @function
     * @name CapabilityDiscovery#removeParticipantIdFromGlobalCapabilitiesDirectory
     *
     * @param {String}
     *            participantId to remove
     *
     * @returns {Object} an A+ promise
     */
    function removeParticipantIdFromGlobalCapabilitiesDirectory(participantId) {
        return getGlobalCapabilitiesDirectoryProxy(TTL_30DAYS_IN_MS).then(function(globalCapabilitiesDirectoryProxy) {
            return globalCapabilitiesDirectoryProxy
                .remove({
                    participantId: participantId
                })
                .catch(function(error) {
                    throw new Error(
                        'Error calling operation "remove" of GlobalCapabilitiesDirectory because: ' + error
                    );
                });
        });
    }

    /**
     * This method removes a capability from the local and/or global capabilities directory according to the given registration
     * strategy.
     *
     * @function
     * @name CapabilityDiscovery#remove
     *
     * @param {String}
     *            participantId to remove
     *
     * @returns {Object} an A+ promise
     */
    this.remove = function remove(participantId) {
        var discoveryEntries = localCapabilitiesStore.lookup({
            participantId: participantId
        });
        var promise;

        localCapabilitiesStore.remove({
            participantId: participantId
        });
        if (discoveryEntries === undefined || discoveryEntries.length !== 1) {
            log.warn(
                "remove(): no capability entry found in local capabilities store for participantId " +
                    participantId +
                    ". Trying to remove the capability from global directory"
            );
            promise = removeParticipantIdFromGlobalCapabilitiesDirectory(participantId);
        } else {
            if (discoveryEntries[0].qos.scope === ProviderScope.LOCAL || discoveryEntries.length < 1) {
                promise = Promise.resolve();
            } else if (discoveryEntries[0].qos.scope === ProviderScope.GLOBAL) {
                promise = removeParticipantIdFromGlobalCapabilitiesDirectory(participantId);
            } else {
                promise = Promise.reject(
                    new Error('Encountered unknown ProviderQos scope "' + discoveryEntries[0].qos.scope + '"')
                );
            }
        }
        return promise;
    };
}

module.exports = CapabilityDiscovery;
