/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
import * as Address from "../../../generated/joynr/system/RoutingTypes/Address";
import * as DiscoveryEntry from "../../../generated/joynr/types/DiscoveryEntry";
import * as DiscoveryEntryWithMetaInfo from "../../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
/**
 * The <code>CapabilityDiscovery</code> is a joynr internal interface. When the Arbitrator does a lookup for capabilities, this module is
 * queried. If a provider needs to be registered, this module selects the places to register at.
 */
import GlobalDiscoveryEntry from "../../../generated/joynr/types/GlobalDiscoveryEntry";

import DiscoveryQos from "../../proxy/DiscoveryQos";
import DiscoveryQosGen from "../../../generated/joynr/types/DiscoveryQos";
import DiscoveryScope from "../../../generated/joynr/types/DiscoveryScope";
import ProviderScope from "../../../generated/joynr/types/ProviderScope";
import GlobalCapabilitiesDirectoryProxy from "../../../generated/joynr/infrastructure/GlobalCapabilitiesDirectoryProxy";
import * as Typing from "../../util/Typing";
import LoggingManager from "../../system/LoggingManager";
import * as UtilInternal from "../../util/UtilInternal";
import ProviderRuntimeException from "../../exceptions/ProviderRuntimeException";
import * as CapabilitiesUtil from "../../util/CapabilitiesUtil";
import { DiscoveryStub } from "../interface/DiscoveryStub";
import ProxyBuilder = require("../../proxy/ProxyBuilder");
import MessageRouter = require("../../messaging/routing/MessageRouter");
import CapabilitiesStore = require("../CapabilitiesStore");
import JoynrRuntimeException = require("../../exceptions/JoynrRuntimeException");
import DiscoveryError from "../../../generated/joynr/types/DiscoveryError";
import ApplicationException from "../../exceptions/ApplicationException";

const TTL_30DAYS_IN_MS = 30 * 24 * 60 * 60 * 1000;
const log = LoggingManager.getLogger("joynr/capabilities/discovery/CapabilityDiscovery");

interface QueuedGlobalDiscoveryEntires {
    discoveryEntry: DiscoveryEntry;
    gbids: string[];
    resolve: (value: any) => void;
    reject: (error: any) => void;
}

interface QueuedGlobalLookups {
    domains: string[];
    interfaceName: string;
    ttl: number;
    capabilities: any[];
    gbids: string[];
    resolve: (value: any) => void;
    reject: (error: any) => void;
}

interface QueuedGlobalLookupsByParticipantId {
    participantId: string;
    ttl: number;
    gbids: string[];
    resolve: (value: any) => void;
    reject: (error: any) => void;
}

class CapabilityDiscovery implements DiscoveryStub {
    private globalCapabilitiesDomain: string;
    private proxyBuilder!: ProxyBuilder;
    private messageRouter!: MessageRouter;
    private globalCapabilitiesCache: CapabilitiesStore;
    private localCapabilitiesStore: CapabilitiesStore;
    private queuedGlobalLookups: QueuedGlobalLookups[];
    private queuedGlobalLookupsByParticipantId: QueuedGlobalLookupsByParticipantId[];

    private queuedGlobalDiscoveryEntries: QueuedGlobalDiscoveryEntires[];
    private globalAddressSerialized?: string;
    private knownGbids: string[];

    /**
     * The CapabilitiesDiscovery looks up the local and global capabilities directory
     *
     * @constructor
     *
     * @param localCapabilitiesStore the local capabilities store
     * @param globalCapabilitiesCache the cache for the global capabilities directory
     * @param globalCapabilitiesDomain the domain to communicate with the GlobalCapablitiesDirectory
     *                                     GlobalCapab
     * @param knownGbids known global backend identifiers provided by provisioning
     */
    public constructor(
        localCapabilitiesStore: CapabilitiesStore,
        globalCapabilitiesCache: CapabilitiesStore,
        globalCapabilitiesDomain: string,
        knownGbids: string[]
    ) {
        if (!knownGbids || knownGbids.length === 0) {
            throw new JoynrRuntimeException({
                detailMessage: `knownGbids needs the have a length of at least one ${knownGbids}`
            });
        }

        this.queuedGlobalDiscoveryEntries = [];
        this.queuedGlobalLookups = [];
        this.queuedGlobalLookupsByParticipantId = [];

        this.localCapabilitiesStore = localCapabilitiesStore;
        this.globalCapabilitiesCache = globalCapabilitiesCache;
        this.globalCapabilitiesDomain = globalCapabilitiesDomain;
        this.knownGbids = knownGbids;
    }

    /**
     * Set necessary dependencies
     *
     * @param messageRouter the message router
     * @param proxyBuilder the proxy builder used to create the GlobalCapabilitiesDirectoryProxy
     */
    public setDependencies(messageRouter: MessageRouter, proxyBuilder: ProxyBuilder): void {
        this.messageRouter = messageRouter;
        this.proxyBuilder = proxyBuilder;
    }

    /**
     * This method create a new global capabilities proxy with the provided ttl as messaging QoS
     *
     * @param ttl time to live of joynr messages triggered by the returning proxy
     *
     * @returns the newly created proxy
     */
    private getGlobalCapabilitiesDirectoryProxy(ttl: number): Promise<GlobalCapabilitiesDirectoryProxy> {
        return this.proxyBuilder
            .build(GlobalCapabilitiesDirectoryProxy, {
                domain: this.globalCapabilitiesDomain,
                messagingQos: {
                    ttl
                },
                discoveryQos: new DiscoveryQos({
                    discoveryScope: DiscoveryScope.GLOBAL_ONLY,
                    cacheMaxAgeMs: UtilInternal.getMaxLongValue()
                })
            })
            .catch(error => {
                throw new Error(`Failed to create global capabilities directory proxy: ${error}`);
            });
    }

    private lookupGlobal(
        domains: string[],
        interfaceName: string,
        ttl: number,
        capabilities: DiscoveryEntryWithMetaInfo[],
        gbids: string[]
    ): Promise<DiscoveryEntryWithMetaInfo[]> {
        return this.getGlobalCapabilitiesDirectoryProxy(ttl)
            .then(globalCapabilitiesDirectoryProxy =>
                globalCapabilitiesDirectoryProxy.lookup({
                    domains,
                    interfaceName,
                    gbids
                })
            )
            .then(opArgs => {
                const messageRouterPromises = [];
                const globalCapabilities = opArgs.result;
                let globalAddress;
                if (globalCapabilities === undefined) {
                    log.error("globalCapabilitiesDirectoryProxy.lookup() returns with missing result");
                } else {
                    for (let i = globalCapabilities.length - 1; i >= 0; i--) {
                        const globalDiscoveryEntry = globalCapabilities[i];
                        if (globalDiscoveryEntry.address === this.globalAddressSerialized) {
                            globalCapabilities.splice(i, 1);
                        } else {
                            try {
                                globalAddress = Typing.augmentTypes(JSON.parse(globalDiscoveryEntry.address));
                            } catch (e) {
                                log.error(
                                    `unable to use global discoveryEntry with unknown address type: ${
                                        globalDiscoveryEntry.address
                                    }`
                                );
                                continue;
                            }
                            // Update routing table
                            const isGloballyVisible = globalDiscoveryEntry.qos.scope === ProviderScope.GLOBAL;
                            messageRouterPromises.push(
                                this.messageRouter.addNextHop(
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
                return Promise.all(messageRouterPromises);
            })
            .then(() => capabilities);
    }

    /**
     * expects a capabilities array which is then filled with any that are found from the proxy
     *
     * @param domains - the domains
     * @param interfaceName - the interface name
     * @param ttl - time to live of joynr messages triggered by the returning proxy
     * @param capabilities - the capabilities array to be filled
     * @param gbids - array of knownGbids in which to look for providers
     *
     * @returns - the capabilities array filled with the capabilities found in the global capabilities directory
     */
    private lookupGlobalCapabilities(
        domains: string[],
        interfaceName: string,
        ttl: number,
        capabilities: any[],
        gbids: string[]
    ): Promise<any[]> {
        if (!this.globalAddressSerialized) {
            const deferred = UtilInternal.createDeferred();
            this.queuedGlobalLookups.push({
                domains,
                interfaceName,
                ttl,
                capabilities,
                gbids,
                resolve: deferred.resolve,
                reject: deferred.reject
            });
            return deferred.promise;
        } else {
            return this.lookupGlobal(domains, interfaceName, ttl, capabilities, gbids);
        }
    }

    private lookupGlobalByParticipantId(
        participantId: string,
        ttl: number,
        gbids: string[]
    ): Promise<DiscoveryEntryWithMetaInfo> {
        let result: DiscoveryEntryWithMetaInfo;
        return this.getGlobalCapabilitiesDirectoryProxy(ttl)
            .then(globalCapabilitiesDirectoryProxy =>
                globalCapabilitiesDirectoryProxy.lookup({
                    participantId,
                    gbids
                })
            )
            .then(opArgs => {
                const messageRouterPromises = [];
                const globalDiscoveryEntry = opArgs.result;
                let globalAddress;
                if (!globalDiscoveryEntry) {
                    log.error("globalCapabilitiesDirectoryProxy.lookup() returns with missing result");
                    return Promise.reject(
                        new ApplicationException({
                            detailMessage: "Got empty result from global lookup",
                            error: DiscoveryError.INTERNAL_ERROR
                        })
                    );
                } else {
                    if (globalDiscoveryEntry.address !== this.globalAddressSerialized) {
                        try {
                            globalAddress = Typing.augmentTypes(JSON.parse(globalDiscoveryEntry.address));
                        } catch (e) {
                            log.error(
                                `unable to use global discoveryEntry with unknown address type: ${
                                    globalDiscoveryEntry.address
                                }`
                            );
                            return Promise.reject(
                                new ApplicationException({
                                    detailMessage: "The address could not be deserialized, see error log message",
                                    error: DiscoveryError.INTERNAL_ERROR
                                })
                            );
                        }
                        // Update routing table
                        const isGloballyVisible = globalDiscoveryEntry.qos.scope === ProviderScope.GLOBAL;
                        messageRouterPromises.push(
                            this.messageRouter.addNextHop(
                                globalDiscoveryEntry.participantId,
                                globalAddress,
                                isGloballyVisible
                            )
                        );
                    }
                    result = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(false, globalDiscoveryEntry);
                }
                return Promise.all(messageRouterPromises);
            })
            .then(() => result);
    }

    private lookupGlobalCapabilitiesByParticipantId(participantId: string, ttl: number, gbids: string[]): Promise<any> {
        if (!this.globalAddressSerialized) {
            const deferred = UtilInternal.createDeferred();
            this.queuedGlobalLookupsByParticipantId.push({
                participantId,
                ttl,
                gbids,
                resolve: deferred.resolve,
                reject: deferred.reject
            });
            return deferred.promise;
        } else {
            return this.lookupGlobalByParticipantId(participantId, ttl, gbids);
        }
    }

    /**
     * @param discoveryEntry to be added to the global discovery directory
     * @param gbids global backend identifiers of backends to add discoveryEntries into
     * @returns an A+ promise
     */
    private addGlobal(discoveryEntry: DiscoveryEntry, gbids: string[]): Promise<void> {
        return this.getGlobalCapabilitiesDirectoryProxy(TTL_30DAYS_IN_MS)
            .then(globalCapabilitiesDirectoryProxy => {
                const discoveryEntryWithAddress = discoveryEntry as DiscoveryEntry & { address: string };
                // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                discoveryEntryWithAddress.address = this.globalAddressSerialized!;
                return globalCapabilitiesDirectoryProxy.add({
                    globalDiscoveryEntry: new GlobalDiscoveryEntry(discoveryEntryWithAddress),
                    gbids
                });
            })
            .catch(error => {
                log.error(`Error calling operation "add" of GlobalCapabilitiesDirectory because: ${error}`);
                throw error;
            });
    }

    /**
     * @param queuedDiscoveryEntry contains a discoveryEntry and the resolve
     * and reject functions from the original Promise created on add().
     */
    private addGlobalQueued(queuedDiscoveryEntry: QueuedGlobalDiscoveryEntires): void {
        this.addGlobal(queuedDiscoveryEntry.discoveryEntry, queuedDiscoveryEntry.gbids)
            .then(queuedDiscoveryEntry.resolve)
            .catch(queuedDiscoveryEntry.reject);
    }

    /**
     * This method removes a capability from the global capabilities directory.
     *
     * @param participantId to remove
     *
     * @returns an A+ promise
     */
    private removeParticipantIdFromGlobalCapabilitiesDirectory(participantId: string): Promise<void> {
        return this.getGlobalCapabilitiesDirectoryProxy(TTL_30DAYS_IN_MS)
            .then(globalCapabilitiesDirectoryProxy => {
                return globalCapabilitiesDirectoryProxy.remove({
                    participantId
                });
            })
            .catch(error => {
                throw new Error(`Error calling operation "remove" of GlobalCapabilitiesDirectory because: ${error}`);
            });
    }

    /**
     * This method is called when the global address has been created
     *
     * @param newGlobalAddress the address used to register discovery entries globally
     */
    public globalAddressReady(newGlobalAddress: Address): void {
        let parameters;
        this.globalAddressSerialized = JSON.stringify(newGlobalAddress);
        for (let i = 0; i < this.queuedGlobalDiscoveryEntries.length; i++) {
            this.addGlobalQueued(this.queuedGlobalDiscoveryEntries[i]);
        }
        this.queuedGlobalDiscoveryEntries = [];
        for (let i = 0; i < this.queuedGlobalLookups.length; i++) {
            parameters = this.queuedGlobalLookups[i];
            this.lookupGlobal(
                parameters.domains,
                parameters.interfaceName,
                parameters.ttl,
                parameters.capabilities,
                parameters.gbids
            )
                .then(parameters.resolve)
                .catch(parameters.reject);
        }
        this.queuedGlobalLookups = [];
        for (let i = 0; i < this.queuedGlobalLookupsByParticipantId.length; i++) {
            parameters = this.queuedGlobalLookupsByParticipantId[i];
            this.lookupGlobalByParticipantId(parameters.participantId, parameters.ttl, parameters.gbids)
                .then(parameters.resolve)
                .catch(parameters.reject);
        }
        this.queuedGlobalLookupsByParticipantId = [];
    }

    /**
     * This method queries the local and/or global capabilities directory according to the given discoveryStrategy given in the
     * DiscoveryQos object
     *
     * @param domains the domains
     * @param interfaceName the interface name
     * @param discoveryQos the DiscoveryQos giving the strategy for discovering a capability
     * @param gbids the backends in which the lookup shall be performed
     * @returns an A+ Promise object, that will provide an array of discovered capabilities, callback signatures:
     *          then({Array[GlobalDiscoveryEntry]} discoveredCaps).catch({Error} error)
     */
    public lookup(
        domains: string[],
        interfaceName: string,
        discoveryQos: DiscoveryQosGen,
        gbids: string[]
    ): Promise<DiscoveryEntryWithMetaInfo[]> {
        let localCapabilities, globalCapabilities;
        gbids = gbids.length > 0 ? gbids : this.knownGbids;

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
                localCapabilities = this.localCapabilitiesStore.lookup({
                    domains,
                    interfaceName
                });
                return Promise.resolve(
                    CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(true, localCapabilities)
                );

            // if anything local use it. Otherwise lookup global.
            case DiscoveryScope.LOCAL_THEN_GLOBAL.value:
                localCapabilities = this.localCapabilitiesStore.lookup({
                    domains,
                    interfaceName
                });
                if (localCapabilities.length > 0) {
                    return Promise.resolve(
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(true, localCapabilities)
                    );
                }
                globalCapabilities = this.globalCapabilitiesCache.lookup({
                    domains,
                    interfaceName,
                    cacheMaxAge: discoveryQos.cacheMaxAge
                });
                if (globalCapabilities.length > 0) {
                    return Promise.resolve(
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(false, globalCapabilities)
                    );
                }
                return this.lookupGlobalCapabilities(
                    domains,
                    interfaceName,
                    TTL_30DAYS_IN_MS,
                    localCapabilities,
                    gbids
                );

            // Use local results, but then lookup global
            case DiscoveryScope.LOCAL_AND_GLOBAL.value:
                localCapabilities = this.localCapabilitiesStore.lookup({
                    domains,
                    interfaceName
                });
                globalCapabilities = this.globalCapabilitiesCache.lookup({
                    domains,
                    interfaceName,
                    cacheMaxAge: discoveryQos.cacheMaxAge
                });
                if (globalCapabilities.length === 0) {
                    return this.lookupGlobalCapabilities(
                        domains,
                        interfaceName,
                        TTL_30DAYS_IN_MS,
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(true, localCapabilities),
                        gbids
                    );
                }
                return Promise.resolve(
                    CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(true, localCapabilities).concat(
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(false, globalCapabilities)
                    )
                );

            case DiscoveryScope.GLOBAL_ONLY.value:
                globalCapabilities = this.globalCapabilitiesCache.lookup({
                    domains,
                    interfaceName,
                    cacheMaxAge: discoveryQos.cacheMaxAge
                });
                if (globalCapabilities.length > 0) {
                    return Promise.resolve(
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(false, globalCapabilities)
                    );
                }
                return this.lookupGlobalCapabilities(
                    domains,
                    interfaceName,
                    TTL_30DAYS_IN_MS,
                    globalCapabilities,
                    gbids
                );
            default:
                return Promise.reject(
                    new ProviderRuntimeException({
                        detailMessage: `unknown discoveryScope value: ${discoveryQos.discoveryScope.value}`
                    })
                );
        }
    }

    /**
     * This method queries the local and/or global capabilities directory according to the given discoveryStrategy given in the
     * DiscoveryQos object
     *
     * @param participantId the participantId
     * @param discoveryQos the DiscoveryQos giving the strategy for discovering a capability
     * @param gbids the backends in which the lookup shall be performed
     * @returns an A+ Promise object, that will one single discovered capabilities, callback signatures:
     *          then({GlobalDiscoveryEntry} discoveredCap).catch({Error} error)
     */
    public lookupByParticipantId(
        participantId: string,
        discoveryQos: DiscoveryQosGen,
        gbids: string[]
    ): Promise<DiscoveryEntryWithMetaInfo> {
        let localCapabilities, globalCapabilities;
        gbids = gbids.length > 0 ? gbids : this.knownGbids;
        switch (discoveryQos.discoveryScope.value) {
            // only interested in local results
            case DiscoveryScope.LOCAL_ONLY.value:
                localCapabilities = this.localCapabilitiesStore.lookup({ participantId });
                if (localCapabilities.length === 0) {
                    log.debug(
                        `lookupByParticipantId(): no capability entry found in local capabilities store for participantId ${participantId}.`
                    );
                    return Promise.reject(DiscoveryError.NO_ENTRY_FOR_PARTICIPANT);
                }
                return Promise.resolve(
                    CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(true, localCapabilities[0])
                );

            case DiscoveryScope.LOCAL_THEN_GLOBAL.value:
            case DiscoveryScope.LOCAL_AND_GLOBAL.value:
                localCapabilities = this.localCapabilitiesStore.lookup({ participantId });
                if (localCapabilities.length > 0) {
                    return Promise.resolve(
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(true, localCapabilities[0])
                    );
                }

                globalCapabilities = this.globalCapabilitiesCache.lookup({
                    participantId,
                    cacheMaxAge: discoveryQos.cacheMaxAge
                });

                if (globalCapabilities.length > 0) {
                    return Promise.resolve(
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(false, globalCapabilities[0])
                    );
                }
                // no capability entry found in local or global cache
                // search for local entries in the global capabilities directory
                return this.lookupGlobalCapabilitiesByParticipantId(participantId, TTL_30DAYS_IN_MS, gbids);
            case DiscoveryScope.GLOBAL_ONLY.value:
                globalCapabilities = this.globalCapabilitiesCache.lookup({
                    participantId,
                    cacheMaxAge: discoveryQos.cacheMaxAge
                });
                if (globalCapabilities.length > 0) {
                    return Promise.resolve(
                        CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(false, globalCapabilities[0])
                    );
                }
                // no capability entry found found in local cache
                // search for global entries in the global capabilities directory
                return this.lookupGlobalCapabilitiesByParticipantId(participantId, TTL_30DAYS_IN_MS, gbids);
            default:
                log.debug(`lookupByParticipantId(): unknown discovery scope ${discoveryQos.discoveryScope.value}`);
                return Promise.reject(DiscoveryError.INTERNAL_ERROR);
        }
    }

    /**
     * This method adds a capability in the local and/or global capabilities directory according to
     * the given registration strategy. Global capabilities will be added to all known gbids.
     *
     * @param discoveryEntry DiscoveryEntry to add to all known backends
     * @returns Promise resolved on success.
     */
    public addToAll(discoveryEntry: DiscoveryEntry): Promise<void> {
        return this.addInternal(discoveryEntry, this.knownGbids);
    }

    private addInternal(discoveryEntry: DiscoveryEntry, gbids: string[]): Promise<void> {
        this.localCapabilitiesStore.add({
            discoveryEntry,
            remote: false
        });
        discoveryEntry.lastSeenDateMs = Date.now();
        if (discoveryEntry.qos.scope === ProviderScope.LOCAL) {
            return Promise.resolve();
        } else if (discoveryEntry.qos.scope === ProviderScope.GLOBAL) {
            if (!this.globalAddressSerialized) {
                const deferred = UtilInternal.createDeferred();
                this.queuedGlobalDiscoveryEntries.push({
                    discoveryEntry,
                    gbids,
                    resolve: deferred.resolve,
                    reject: deferred.reject
                });
                return deferred.promise;
            } else {
                return this.addGlobal(discoveryEntry, gbids);
            }
        }
        return Promise.reject(new Error(`Encountered unknown ProviderQos scope "${discoveryEntry.qos.scope}"`));
    }

    /**
     * This method adds a capability in the local and/or global capabilities directory according to the given registration
     * strategy.
     *
     * @param discoveryEntry DiscoveryEntry of the capability
     * @param _awaitGlobalRegistration webSocket-libjoynr specific property part of the DiscoveryStub interface
     * @param gbids identifiers of global backends to add capability into
     *
     * @returns an A+ promise
     */
    public add(
        discoveryEntry: DiscoveryEntry,
        _awaitGlobalRegistration?: boolean,
        gbids: string[] = []
    ): Promise<void> {
        return this.addInternal(discoveryEntry, gbids.length > 0 ? gbids : [this.knownGbids[0]]);
    }

    /**
     * This method sends a freshness update to the global capabilities directory to update
     * the lastSeenDateMs of the entries registered via the local cluster controller.
     *
     * @param clusterControllerId the channelId of the local cluster controller
     * @param ttlMs the time to live of the freshness update message
     *
     * @returns an A+ promise
     */
    public touch(clusterControllerId: string, ttlMs: number): Promise<void> {
        return this.getGlobalCapabilitiesDirectoryProxy(ttlMs)
            .then(globalCapabilitiesDirectoryProxy => {
                return globalCapabilitiesDirectoryProxy.touch({ clusterControllerId });
            })
            .catch(error => {
                throw new Error(`Error calling operation "touch" of GlobalCapabilitiesDirectory because: ${error}`);
            });
    }

    /**
     * This method removes a capability from the local and/or global capabilities directory according to the given registration
     * strategy.
     *
     * @param participantId to remove
     *
     * @returns an A+ promise
     */
    public remove(participantId: string): Promise<void> {
        const discoveryEntries = this.localCapabilitiesStore.lookup({
            participantId
        });
        this.localCapabilitiesStore.remove({
            participantId
        });
        if (discoveryEntries === undefined || discoveryEntries.length !== 1) {
            log.warn(
                `remove(): no capability entry found in local capabilities store for participantId ${participantId}. Trying to remove the capability from global directory`
            );
            return this.removeParticipantIdFromGlobalCapabilitiesDirectory(participantId);
        } else if (discoveryEntries[0].qos.scope === ProviderScope.LOCAL || discoveryEntries.length < 1) {
            return Promise.resolve();
        } else if (discoveryEntries[0].qos.scope === ProviderScope.GLOBAL) {
            return this.removeParticipantIdFromGlobalCapabilitiesDirectory(participantId);
        } else {
            return Promise.reject(
                new Error(`Encountered unknown ProviderQos scope "${discoveryEntries[0].qos.scope}"`)
            );
        }
    }
}

export = CapabilityDiscovery;
