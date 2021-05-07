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
import DiscoveryError from "../../../generated/joynr/types/DiscoveryError";
import * as Version from "../../../generated/joynr/types/Version";
import LoggingManager from "../../system/LoggingManager";
import { Deferred } from "../../util/UtilInternal";

import * as UtilInternal from "../../util/UtilInternal";
import DiscoveryException from "../../exceptions/DiscoveryException";
import NoCompatibleProviderFoundException from "../../exceptions/NoCompatibleProviderFoundException";
import LongTimer from "../../util/LongTimer";
import { DiscoveryStub } from "../interface/DiscoveryStub";
import DiscoveryQosGen = require("../../../generated/joynr/types/DiscoveryQos");
import DiscoveryQos = require("../../proxy/DiscoveryQos");
import ApplicationException = require("../../exceptions/ApplicationException");
import * as ArbitrationStrategyCollection from "../../../joynr/types/ArbitrationStrategyCollection";
import { FIXED_PARTICIPANT_PARAMETER } from "../../types/ArbitrationConstants";
import { isEqual } from "lodash";

const log = LoggingManager.getLogger("joynr.capabilities.arbitration.Arbitrator");

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

class Arbitrator {
    private started: boolean = true;
    private arbitrationId: number = 0;
    private pendingArbitrations: Record<string, Deferred> = {};
    private capabilityDiscoveryStub: DiscoveryStub;
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
    public constructor(capabilityDiscoveryStub: DiscoveryStub, staticCapabilities?: DiscoveryEntryWithMetaInfo[]) {
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
     *
     * @returns Promise
     *  - resolved with an array of arbitrated capabilities
     *  - rejected with either DiscoveryException or NoCompatibleProviderFoundException
     */
    public async startArbitration(settings: {
        domains: string[];
        interfaceName: string;
        discoveryQos: DiscoveryQos;
        staticArbitration?: boolean;
        proxyVersion: Version;
        gbids?: string[];
    }): Promise<any[]> {
        if (!this.started) {
            return Promise.reject(new Error("Arbitrator is already shut down"));
        }

        this.arbitrationId++;

        log.debug(
            `Arbitration started for domains ${settings.domains}, interface ${settings.interfaceName}, gbids ${
                settings.gbids
            }.`
        );
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

    /**
     * checks if the provided discoveryEntry supports onChange subscriptions if required
     *
     * @param discoveryEntry - the discovery entry to check
     * @param providerMustSupportOnChange - filter only entries supporting onChange subscriptions
     */
    private static checkSupportsOnChangeSubscriptions(
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

    private discoverCapabilitiesWrapper(settings: {
        domains: string[];
        interfaceName: string;
        discoveryQos: DiscoveryQos;
        staticArbitration?: boolean;
        proxyVersion: Version;
        gbids?: string[];
    }): Promise<DiscoveryEntryWithMetaInfo[]> {
        const id = this.arbitrationId;
        const deferred = UtilInternal.createDeferred();

        this.pendingArbitrations[id] = deferred;

        this.discoverCapabilities({
            capabilityDiscoveryStub: this.capabilityDiscoveryStub,
            domains: settings.domains,
            interfaceName: settings.interfaceName,
            discoveryQos: settings.discoveryQos,
            proxyVersion: settings.proxyVersion,
            gbids: settings.gbids || []
        })
            .then(args => {
                delete this.pendingArbitrations[id];
                deferred.resolve(args);
            })
            .catch(error => {
                delete this.pendingArbitrations[id];
                deferred.reject(error);
            });

        return deferred.promise;
    }

    /**
     * Tries to discover capabilities with given domains, interfaceName and discoveryQos within the localCapDir as long as the deferred's state is pending
     *
     * @param capabilityDiscoveryStub - the capabilites discovery module
     * @param domains - the domains
     * @param interfaceName - the interfaceName
     * @param applicationDiscoveryQos
     * @param proxyVersion
     * @param gbids global backend identifiers of backends to look for capabilities
     * @returns a Promise/A+ object, that will provide an array of discovered capabilities
     */
    private async discoverCapabilities({
        capabilityDiscoveryStub,
        domains,
        interfaceName,
        discoveryQos,
        proxyVersion,
        gbids
    }: {
        capabilityDiscoveryStub: DiscoveryStub;
        domains: string[];
        interfaceName: string;
        discoveryQos: DiscoveryQos;
        proxyVersion: Version;
        gbids: string[];
    }): Promise<DiscoveryEntryWithMetaInfo[]> {
        // discover caps from local capabilities directory
        let incompatibleVersionsFound: Version[] = [];
        const arbitrationDeadline = Date.now() + discoveryQos.discoveryTimeoutMs;
        const discoveryRetryDelayMs = discoveryQos.discoveryRetryDelayMs;
        let errorMsg: string | null = null;
        let firstLoop = true;
        let participantId: string;
        const isArbitrationStrategyFixedParticipant = isEqual(
            discoveryQos.arbitrationStrategy,
            ArbitrationStrategyCollection.FixedParticipant
        );
        participantId = "";
        if (isArbitrationStrategyFixedParticipant) {
            if (!discoveryQos.additionalParameters.hasOwnProperty(FIXED_PARTICIPANT_PARAMETER)) {
                throw new Error(
                    "parameter FIXED_PARTICIPANT_PARAMETER does not exist in DiscoveryQos.additionalParameters"
                );
            }
            participantId = discoveryQos.additionalParameters[FIXED_PARTICIPANT_PARAMETER];
        }

        do {
            if (!firstLoop) {
                // eslint-disable-next-line promise/avoid-new
                await new Promise(resolve => {
                    LongTimer.setTimeout(resolve, discoveryRetryDelayMs);
                });
            }
            firstLoop = false;
            incompatibleVersionsFound = [];
            let discoveredCaps: DiscoveryEntryWithMetaInfo[];
            try {
                if (isArbitrationStrategyFixedParticipant) {
                    discoveredCaps = [];
                    discoveredCaps.push(
                        await capabilityDiscoveryStub.lookupByParticipantId(
                            participantId,
                            new DiscoveryQosGen({
                                discoveryScope: discoveryQos.discoveryScope,
                                cacheMaxAge: discoveryQos.cacheMaxAgeMs,
                                discoveryTimeout: arbitrationDeadline - Date.now(),
                                providerMustSupportOnChange: discoveryQos.providerMustSupportOnChange
                            }),
                            gbids
                        )
                    );
                    if (discoveredCaps.length > 0) {
                        if (discoveredCaps[0].interfaceName !== interfaceName) {
                            const errorMsg = `Interface "${
                                discoveredCaps[0].interfaceName
                            }" of discovered provider does not match proxy's interface "${interfaceName}".`;
                            log.error(errorMsg);
                            return Promise.reject(errorMsg);
                        }
                    }
                } else {
                    discoveredCaps = await capabilityDiscoveryStub.lookup(
                        domains,
                        interfaceName,
                        new DiscoveryQosGen({
                            discoveryScope: discoveryQos.discoveryScope,
                            cacheMaxAge: discoveryQos.cacheMaxAgeMs,
                            discoveryTimeout: arbitrationDeadline - Date.now(),
                            providerMustSupportOnChange: discoveryQos.providerMustSupportOnChange
                        }),
                        gbids
                    );
                }

                const versionCompatibleCaps: DiscoveryEntryWithMetaInfo[] = [];

                for (let i = 0; i < discoveredCaps.length; i++) {
                    const providerVersion = discoveredCaps[i].providerVersion;

                    if (
                        Arbitrator.checkSupportsOnChangeSubscriptions(
                            discoveredCaps[i],
                            discoveryQos.providerMustSupportOnChange
                        )
                    ) {
                        if (
                            providerVersion.majorVersion === proxyVersion.majorVersion &&
                            providerVersion.minorVersion >= proxyVersion.minorVersion
                        ) {
                            versionCompatibleCaps.push(discoveredCaps[i]);
                        } else {
                            addToListIfNotExisting(incompatibleVersionsFound, providerVersion);
                        }
                    }
                }
                // filter caps according to chosen arbitration strategy
                const arbitratedCaps = discoveryQos.arbitrationStrategy(versionCompatibleCaps);
                if (arbitratedCaps.length > 0) {
                    // report the discovered & arbitrated caps
                    return arbitratedCaps;
                }
            } catch (error) {
                if (error instanceof ApplicationException) {
                    errorMsg = `Discovery failed due to ${error.error.name}`;
                    if (
                        error.error !== DiscoveryError.NO_ENTRY_FOR_PARTICIPANT &&
                        error.error !== DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS
                    ) {
                        log.error(
                            `Discovery attempt for domains ${domains}, interface ${interfaceName}, gbids ${gbids} failed due to DiscoveryError: ${
                                error.error.name
                            }. Attempting no retry`
                        );
                        break;
                    } else {
                        log.info(
                            `Discovery attempt for domains ${domains}, interface ${interfaceName}, gbids ${gbids} failed due to DiscoveryError: ${
                                error.error.name
                            }. Attempting retry in ${discoveryQos.discoveryRetryDelayMs} ms`
                        );
                    }
                } else if (error.message) {
                    log.info(
                        `Discovery attempt for domains ${domains}, interface ${interfaceName} failed due to DiscoveryError: ${
                            error.name
                        }. Attempting retry in ${discoveryQos.discoveryRetryDelayMs} ms`
                    );
                    errorMsg = `${error.name} : ${error.message}`;
                }
            }
        } while (arbitrationDeadline - (Date.now() + discoveryRetryDelayMs) > 0);

        if (incompatibleVersionsFound.length > 0) {
            let message: string;
            if (gbids && gbids.length > 0) {
                message = `no compatible provider found within discovery timeout for domains "${JSON.stringify(
                    domains
                )}", interface "${interfaceName}", gbids "${JSON.stringify(gbids)}" with discoveryQos "${JSON.stringify(
                    discoveryQos
                )}"`;
            } else {
                message = `no compatible provider found within discovery timeout for domains "${JSON.stringify(
                    domains
                )}", interface "${interfaceName}" with discoveryQos "${JSON.stringify(discoveryQos)}"`;
            }

            return Promise.reject(
                new NoCompatibleProviderFoundException({
                    detailMessage: message,
                    discoveredVersions: incompatibleVersionsFound,
                    interfaceName
                })
            );
        } else {
            let message: string;
            if (gbids && gbids.length > 0) {
                message = `no provider found within discovery timeout for domains "${JSON.stringify(
                    domains
                )}", interface "${interfaceName}", gbids "${JSON.stringify(gbids)}" with discoveryQos "${JSON.stringify(
                    discoveryQos
                )}"${errorMsg !== undefined ? `. Error: ${errorMsg}` : ""}`;
            } else {
                message = `no provider found within discovery timeout for domains "${JSON.stringify(
                    domains
                )}", interface "${interfaceName}" with discoveryQos "${JSON.stringify(discoveryQos)}"${
                    errorMsg !== undefined ? `. Error: ${errorMsg}` : ""
                }`;
            }
            return Promise.reject(
                new DiscoveryException({
                    detailMessage: message
                })
            );
        }
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
                        Arbitrator.checkSupportsOnChangeSubscriptions(
                            capability,
                            discoveryQos.providerMustSupportOnChange
                        )
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
                pendingArbitration.reject(new Error("Arbitration is already shut down"));
            }
        }
        this.pendingArbitrations = {};
        this.started = false;
    }
}

export = Arbitrator;
