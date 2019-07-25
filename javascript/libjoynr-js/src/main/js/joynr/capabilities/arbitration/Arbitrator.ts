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
import * as DiscoveryEntry from "../../../generated/joynr/types/DiscoveryEntry";
import * as DiscoveryEntryWithMetaInfo from "../../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
import * as Version from "../../../generated/joynr/types/Version";

import * as UtilInternal from "../../util/UtilInternal";
import DiscoveryException from "../../exceptions/DiscoveryException";
import NoCompatibleProviderFoundException from "../../exceptions/NoCompatibleProviderFoundException";
import LongTimer from "../../util/LongTimer";
import CapabilityDiscovery = require("../discovery/CapabilityDiscovery");
import DiscoveryQosGen = require("../../../generated/joynr/types/DiscoveryQos");
import DiscoveryQos = require("../../proxy/DiscoveryQos");

/**
 * checks if the provided discoveryEntry supports onChange subscriptions if required
 *
 * @param discoveryEntry - the discovery entry to check
 * @param providerMustSupportOnChange - filter only entries supporting onChange subscriptions
 */
function checkSupportsOnChangeSubscriptions(
    discoveryEntry: DiscoveryEntry,
    providerMustSupportOnChange: boolean
): boolean {
    if (providerMustSupportOnChange === undefined || !providerMustSupportOnChange) {
        return true;
    }
    if (discoveryEntry.qos === undefined) {
        return false;
    }
    return discoveryEntry.qos.supportsOnChangeSubscriptions;
}

function addToListIfNotExisting(list: Version[], providerVersion: Version): void {
    for (let j = 0; j < list.length; ++j) {
        if (
            list[j].majorVersion === providerVersion.majorVersion &&
            list[j].minorVersion === providerVersion.minorVersion
        ) {
            return;
        }
    }
    list.push(providerVersion);
}

interface DiscoverCapabilitiesDeferred {
    pending: boolean;
    reject: Function;
    resolve: Function;
    id: number;
    incompatibleVersionsFound: Version[];
    discoveryTimeoutMsId: number | string;
    discoveryRetryTimer?: number | string;
    errorMsg?: string;
}

/**
 * Tries to discover capabilities with given domains, interfaceName and discoveryQos within the localCapDir as long as the deferred's state is pending
 *
 * @param capabilityDiscoveryStub - the capabilites discovery module
 * @param domains - the domains
 * @param interfaceName - the interfaceName
 * @param applicationDiscoveryQos
 * @param proxyVersion
 * @param deferred
 * @returns a Promise/A+ object, that will provide an array of discovered capabilities
 */
function discoverCapabilities(
    capabilityDiscoveryStub: CapabilityDiscovery,
    domains: string[],
    interfaceName: string,
    applicationDiscoveryQos: DiscoveryQos,
    proxyVersion: Version,
    deferred: DiscoverCapabilitiesDeferred
): void {
    function discoveryCapabilitiesRetry(): void {
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

    // discover caps from local capabilities directory
    capabilityDiscoveryStub
        .lookup(
            domains,
            interfaceName,
            new DiscoveryQosGen({
                discoveryScope: applicationDiscoveryQos.discoveryScope,
                cacheMaxAge: applicationDiscoveryQos.cacheMaxAgeMs,
                discoveryTimeout: applicationDiscoveryQos.discoveryTimeoutMs,
                providerMustSupportOnChange: applicationDiscoveryQos.providerMustSupportOnChange
            })
        )
        .then(discoveredCaps => {
            // filter caps according to chosen arbitration strategy
            const arbitratedCaps = applicationDiscoveryQos.arbitrationStrategy(discoveredCaps);
            const versionCompatibleArbitratedCaps = [];

            // if deferred is still pending => discoveryTimeoutMs is not expired yet
            if (deferred.pending) {
                // if there are caps found
                deferred.incompatibleVersionsFound = [];
                let providerVersion;

                for (let i = 0; i < arbitratedCaps.length; i++) {
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
                        addToListIfNotExisting(deferred.incompatibleVersionsFound, providerVersion);
                    }
                }
                if (versionCompatibleArbitratedCaps.length > 0) {
                    // report the discovered & arbitrated caps
                    deferred.pending = false;
                    deferred.resolve(versionCompatibleArbitratedCaps);
                } else {
                    deferred.discoveryRetryTimer = LongTimer.setTimeout(
                        discoveryCapabilitiesRetry,
                        applicationDiscoveryQos.discoveryRetryDelayMs
                    );
                }
            }
        })
        .catch((error: any) => {
            if (deferred.pending) {
                if (error.message) {
                    deferred.errorMsg = error.message;
                }
                deferred.discoveryRetryTimer = LongTimer.setTimeout(
                    discoveryCapabilitiesRetry,
                    applicationDiscoveryQos.discoveryRetryDelayMs
                );
            }
        });
}

class Arbitrator {
    private started: boolean = true;
    private arbitrationId: number = 0;
    private pendingArbitrations: any = {};
    private capabilityDiscoveryStub: CapabilityDiscovery;
    private staticCapabilities?: DiscoveryEntryWithMetaInfo[];
    /**
     * An arbitrator looks up all capabilities for given domains and an interface and uses the provides arbitraionStrategy passed in the
     * discoveryQos to choose one or more for the calling proxy
     *
     * @constructor
     *
     * @param capabilityDiscoveryStub the capability discovery
     * @param staticCapabilities the capabilities the arbitrator will use to resolve capabilities in case of static arbitration
     */
    public constructor(
        capabilityDiscoveryStub: CapabilityDiscovery,
        staticCapabilities?: DiscoveryEntryWithMetaInfo[]
    ) {
        this.staticCapabilities = staticCapabilities;
        this.capabilityDiscoveryStub = capabilityDiscoveryStub;
    }

    /**
     * Starts the arbitration process
     *
     * @param settings the settings object
     * @param settings.domains the domains to discover the provider
     * @param settings.interfaceName the interfaceName to discover the provider
     * @param settings.discoveryQos
     * @param [settings.staticArbitration] shall the arbitrator use staticCapabilities or contact the discovery provider
     * @param [settings.proxyVersion] the version of the proxy object
     * @returns a A+ Promise object, that will provide asynchronously an array of arbitrated capabilities
     */
    public async startArbitration(settings: {
        domains: string[];
        interfaceName: string;
        discoveryQos: DiscoveryQos;
        staticArbitration?: boolean;
        proxyVersion: Version;
    }): Promise<any[]> {
        if (!this.started) {
            return Promise.reject(new Error("Arbitrator is already shut down"));
        }

        this.arbitrationId++;

        if (settings.staticArbitration && this.staticCapabilities) {
            return this.discoverStaticCapabilities(
                settings.domains,
                settings.interfaceName,
                settings.discoveryQos,
                settings.proxyVersion
            );
        } else {
            return this.discoverCapabilitiesWrapper(settings);
        }
    }

    private discoverCapabilitiesWrapper(settings: {
        domains: string[];
        interfaceName: string;
        discoveryQos: DiscoveryQos;
        staticArbitration?: boolean;
        proxyVersion: Version;
    }): Promise<any[]> {
        const startArbitrationDeferred = UtilInternal.createDeferred();

        const deferred: DiscoverCapabilitiesDeferred = {
            id: this.arbitrationId,
            incompatibleVersionsFound: [],
            pending: true,
            resolve: (args: any) => {
                LongTimer.clearTimeout(deferred.discoveryTimeoutMsId);
                delete this.pendingArbitrations[deferred.id];
                startArbitrationDeferred.resolve(args);
            },
            reject: (args: any) => {
                LongTimer.clearTimeout(deferred.discoveryTimeoutMsId);
                delete this.pendingArbitrations[deferred.id];
                startArbitrationDeferred.reject(args);
            },
            discoveryTimeoutMsId: LongTimer.setTimeout(() => {
                deferred.pending = false;
                delete this.pendingArbitrations[deferred.id];

                if (deferred.incompatibleVersionsFound.length > 0) {
                    const message = `no compatible provider found within discovery timeout for domains "${JSON.stringify(
                        settings.domains
                    )}", interface "${settings.interfaceName}" with discoveryQos "${JSON.stringify(
                        settings.discoveryQos
                    )}"`;
                    startArbitrationDeferred.reject(
                        new NoCompatibleProviderFoundException({
                            detailMessage: message,
                            discoveredVersions: deferred.incompatibleVersionsFound,
                            interfaceName: settings.interfaceName
                        })
                    );
                } else {
                    startArbitrationDeferred.reject(
                        new DiscoveryException({
                            detailMessage: `no provider found within discovery timeout for domains "${JSON.stringify(
                                settings.domains
                            )}", interface "${settings.interfaceName}" with discoveryQos "${JSON.stringify(
                                settings.discoveryQos
                            )}"${deferred.errorMsg !== undefined ? `. Error: ${deferred.errorMsg}` : ""}`
                        })
                    );
                }
            }, settings.discoveryQos.discoveryTimeoutMs)
        };

        this.pendingArbitrations[deferred.id] = deferred;
        discoverCapabilities(
            this.capabilityDiscoveryStub,
            settings.domains,
            settings.interfaceName,
            settings.discoveryQos,
            settings.proxyVersion,
            deferred
        );

        return startArbitrationDeferred.promise;
    }

    private async discoverStaticCapabilities(
        domains: string[],
        interfaceName: string,
        discoveryQos: DiscoveryQos,
        proxyVersion: Version
    ): Promise<any[]> {
        try {
            const arbitratedCaps = [];

            if (this.staticCapabilities === undefined) {
                return Promise.reject(new Error("Exception while arbitrating: static capabilities are missing"));
            } else {
                for (let i = 0; i < this.staticCapabilities.length; ++i) {
                    const capability = this.staticCapabilities[i];
                    if (
                        domains.indexOf(capability.domain) !== -1 &&
                        interfaceName === capability.interfaceName &&
                        capability.providerVersion.majorVersion === proxyVersion.majorVersion &&
                        capability.providerVersion.minorVersion >= proxyVersion.minorVersion &&
                        checkSupportsOnChangeSubscriptions(capability, discoveryQos.providerMustSupportOnChange)
                    ) {
                        arbitratedCaps.push(capability);
                    }
                }
                return discoveryQos.arbitrationStrategy(arbitratedCaps);
            }
        } catch (e) {
            return Promise.reject(new Error(`Exception while arbitrating: ${e}`));
        }
    }

    /**
     * Shutdown the Arbitrator
     */
    public shutdown(): void {
        for (const id in this.pendingArbitrations) {
            if (this.pendingArbitrations.hasOwnProperty(id)) {
                const pendingArbitration = this.pendingArbitrations[id];
                if (pendingArbitration.discoveryTimeoutMsId !== undefined) {
                    LongTimer.clearTimeout(pendingArbitration.discoveryTimeoutMsId);
                }
                if (pendingArbitration.discoveryRetryTimer !== undefined) {
                    LongTimer.clearTimeout(pendingArbitration.discoveryRetryTimer);
                }
                pendingArbitration.reject(new Error("Arbitration is already shut down"));
            }
        }
        this.pendingArbitrations = {};
        this.started = false;
    }
}

export = Arbitrator;
