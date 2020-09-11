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
import * as Address from "../../generated/joynr/system/RoutingTypes/Address";
import DiscoveryEntry from "../../generated/joynr/types/DiscoveryEntry";
import * as ProviderQos from "../../generated/joynr/types/ProviderQos";
import ProviderScope from "../../generated/joynr/types/ProviderScope";
import Version from "../../generated/joynr/types/Version";
import loggingManager from "../system/LoggingManager";
import { JoynrProvider, JoynrProviderType } from "../types/JoynrProvider";
import * as UtilInternal from "../util/UtilInternal";
import MessageRouter = require("../messaging/routing/MessageRouter");
import ParticipantIdStorage = require("./ParticipantIdStorage");
import PublicationManager = require("../dispatching/subscription/PublicationManager");
import RequestReplyManager = require("../dispatching/RequestReplyManager");
import { DiscoveryStub } from "./interface/DiscoveryStub";

const log = loggingManager.getLogger("joynr.capabilities.CapabilitiesRegistrar");
let defaultExpiryIntervalMs = 6 * 7 * 24 * 60 * 60 * 1000; // 6 Weeks

interface RegistrationSettings {
    domain: string;
    provider: JoynrProvider;
    providerQos: ProviderQos;
    expiryDateMs?: number;
    loggingContext?: Record<string, any>;
    participantId?: string;
    awaitGlobalRegistration?: boolean;
}

interface RegistrationSettingsWithGbids extends RegistrationSettings {
    gbids?: string[];
}

interface MultipleBackendSettings {
    registerToAllBackends: boolean;
    gbids?: string[];
}

class CapabilitiesRegistrar {
    private started: boolean = true;
    private publicationManager: PublicationManager;
    private requestReplyManager: RequestReplyManager;
    private libjoynrMessagingAddress: Address;
    private participantIdStorage: ParticipantIdStorage;
    private messageRouter: MessageRouter;
    private discoveryStub: DiscoveryStub;
    /**
     * The Capabilities Registrar
     *
     * @constructor
     *
     * @param dependencies
     * @param dependencies.discoveryStub connects the inProcessStub to its skeleton
     * @param dependencies.messageRouter used to register nextHop for registered provider
     * @param dependencies.participantIdStorage connects the inProcessStub to its skeleton
     * @param dependencies.libjoynrMessagingAddress address to be used by the cluster controller to send incoming requests to this
     *            libjoynr's providers
     * @param dependencies.requestReplyManager passed on to providerAttribute, providerOperation and providerEvent
     * @param dependencies.publicationManager passed on to providerAttribute
     */
    public constructor(dependencies: {
        discoveryStub: DiscoveryStub;
        messageRouter: MessageRouter;
        participantIdStorage: ParticipantIdStorage;
        libjoynrMessagingAddress: Address;
        requestReplyManager: RequestReplyManager;
        publicationManager: PublicationManager;
    }) {
        this.discoveryStub = dependencies.discoveryStub;
        this.messageRouter = dependencies.messageRouter;
        this.participantIdStorage = dependencies.participantIdStorage;
        this.libjoynrMessagingAddress = dependencies.libjoynrMessagingAddress;
        this.requestReplyManager = dependencies.requestReplyManager;
        this.publicationManager = dependencies.publicationManager;
    }

    /**
     * Internal function used to throw exception in case of CapabilitiesRegistrar
     * is not used properly
     */
    private checkIfReady(): void {
        if (!this.started) {
            throw new Error("CapabilitiesRegistrar is already shut down");
        }
    }

    /**
     * Registers a provider so that it is publicly available
     *
     * If registration to local and global scope is requested by 'providerQos' parameter,
     * the provider is registered to all GBIDs configured in the cluster controller.
     *
     * The 'gbids' parameter can be provided to override the GBIDs selection in the cluster
     * controller. The global capabilities directory identified by the first selected GBID performs
     * the registration.
     *
     * @param settings the arguments object for this function call
     * @param settings.domain
     * @param settings.provider
     * @param settings.providerQos the Quality of Service parameters for provider registration
     * @param [settings.expiryDateMs] date in millis since epoch after which the discovery entry can be purged from all directories.
     *            Default value is one day.
     * @param [settings.loggingContext] optional logging context will be appended to logging messages created in the name of this proxy
     * @param [settings.participantId] optional. If not set, a globally unique UUID participantId will be generated, and persisted to
     *            localStorage. If set, the participantId must be unique in the context of the provider's scope, as set in the ProviderQos;
     *            The application setting the participantId is responsible for guaranteeing uniqueness.
     * @param [settings.awaitGlobalRegistration] optional. If provided and set to true registerProvider will wait until local and global
     *            registration succeeds or timeout is reached: otherwise registerProvider only waits for local registration.
     * @param settings.gbids Optional subset of GBIDs configured in the cluster controller for custom global
     * registration.
     *
     * @returns an A+ promise
     */
    public register(settings: RegistrationSettingsWithGbids): Promise<string> {
        return this.registerInternal(settings, { registerToAllBackends: false, gbids: settings.gbids });
    }

    /**
     * Registers a provider so that it is publicly available in all backends known to the cluster controller
     *
     * @param settings the arguments object for this function call
     * @param settings.domain
     * @param settings.provider
     * @param settings.providerQos the Quality of Service parameters for provider registration
     * @param [settings.expiryDateMs] date in millis since epoch after which the discovery entry can be purged from all directories.
     *            Default value is one day.
     * @param [settings.loggingContext] optional logging context will be appended to logging messages created in the name of this proxy
     * @param [settings.participantId] optional. If not set, a globally unique UUID participantId will be generated, and persisted to
     *            localStorage. If set, the participantId must be unique in the context of the provider's scope, as set in the ProviderQos;
     *            The application setting the participantId is responsible for guaranteeing uniqueness.
     * @param [settings.awaitGlobalRegistration] optional. If provided and set to true registerProvider will wait until local and global
     *            registration succeeds or timeout is reached: otherwise registerProvider only waits for local registration.
     *
     * @returns an A+ promise
     */
    public async registerInAllKnownBackends(settings: RegistrationSettings): Promise<string> {
        return this.registerInternal(settings, { registerToAllBackends: true });
    }

    private async registerInternal(
        {
            domain,
            provider,
            providerQos,
            expiryDateMs,
            loggingContext,
            participantId,
            awaitGlobalRegistration
        }: RegistrationSettings,
        gbIdSettings: MultipleBackendSettings
    ): Promise<string> {
        this.checkIfReady();

        const missingImplementations = provider.checkImplementation();

        if (missingImplementations.length > 0) {
            throw new Error(
                `provider: ${domain}/${provider.interfaceName}.v${
                    (provider.constructor as JoynrProviderType).MAJOR_VERSION
                } is missing: ${missingImplementations.toString()}`
            );
        }

        if (participantId === undefined || participantId === null) {
            // retrieve participantId if not passed in
            participantId = this.participantIdStorage.getParticipantId(domain, provider);
        } else {
            // store provided participantId
            this.participantIdStorage.setParticipantId(domain, provider, participantId);
        }

        if (loggingContext !== undefined) {
            log.warn("loggingContext is currently not supported");
        }

        if (awaitGlobalRegistration === undefined) {
            awaitGlobalRegistration = false;
        }

        if (typeof awaitGlobalRegistration !== "boolean") {
            const errText = "awaitGlobalRegistration must be boolean";
            log.warn(errText);
            return Promise.reject(new Error(errText));
        }

        // register provider at RequestReplyManager
        this.requestReplyManager.addRequestCaller(participantId, provider);

        // if provider has at least one attribute, add it as publication provider
        this.publicationManager.addPublicationProvider(participantId, provider);

        // register routing address at routingTable
        const isGloballyVisible = providerQos.scope === ProviderScope.GLOBAL;
        await this.messageRouter.addNextHop(participantId, this.libjoynrMessagingAddress, isGloballyVisible);

        // TODO: Must be later provided by the user or retrieved from somewhere
        const defaultPublicKeyId = "";

        try {
            const discoveryEntry = new DiscoveryEntry({
                providerVersion: new Version({
                    majorVersion: (provider.constructor as JoynrProviderType).MAJOR_VERSION,
                    minorVersion: (provider.constructor as JoynrProviderType).MINOR_VERSION
                }),
                domain,
                interfaceName: provider.interfaceName,
                participantId,
                qos: providerQos,
                publicKeyId: defaultPublicKeyId,
                expiryDateMs: expiryDateMs || Date.now() + defaultExpiryIntervalMs,
                lastSeenDateMs: Date.now()
            });

            if (gbIdSettings.registerToAllBackends) {
                await this.discoveryStub.addToAll(discoveryEntry, awaitGlobalRegistration);
            } else {
                await this.discoveryStub.add(discoveryEntry, awaitGlobalRegistration, gbIdSettings.gbids || []);
            }
        } catch (e) {
            this.messageRouter.removeNextHop(participantId).catch(UtilInternal.emptyFunction);
            throw e;
        }

        log.info(
            `Provider registered: participantId: ${participantId}, domain: ${domain}, interfaceName: ${
                provider.interfaceName
            }, majorVersion: ${(provider.constructor as JoynrProviderType).MAJOR_VERSION}`
        );
        return participantId;
    }

    /**
     * Registers a provider so that it is publicly available
     *
     * If registration to local and global scope is requested by 'providerQos' parameter,
     * the provider is registered to all GBIDs configured in the cluster controller.
     *
     * The 'gbids' parameter can be provided to override the GBIDs selection in the cluster
     * controller. The global capabilities directory identified by the first selected GBID performs
     * the registration.
     *
     * @param domain
     * @param provider
     * @param provider.interfaceName
     * @param providerQos the Quality of Service parameters for provider registration
     * @param [expiryDateMs] date in millis since epoch after which the discovery entry can be purged from all directories.
     *            Default value is one day.
     * @param [loggingContext] optional logging context will be appended to logging messages created in the name of this proxy
     * @param [participantId] optional. If not set, a globally unique UUID participantId will be generated, and persisted to localStorage.
     * @param [awaitGlobalRegistration] optional. If provided and set to true registerProvider will wait until local and global
     *            registration succeeds or timeout is reached: otherwise registerProvider only waits for local registration.
     * @param gbids: Optional subset of GBIDs configured in the cluster controller for custom global
     * registration.
     *
     * @returns an A+ promise
     */
    public async registerProvider(
        domain: string,
        provider: JoynrProvider,
        providerQos: ProviderQos,
        expiryDateMs?: number,
        loggingContext?: Record<string, any>,
        participantId?: string,
        awaitGlobalRegistration?: boolean,
        gbids?: string[]
    ): Promise<string> {
        return this.registerInternal(
            {
                domain,
                provider,
                providerQos,
                expiryDateMs,
                loggingContext,
                participantId,
                awaitGlobalRegistration
            },
            { registerToAllBackends: false, gbids }
        );
    }

    /**
     * Unregisters a provider so that it is not publicly available anymore
     *
     * @param domain
     * @param provider
     * @param provider.interfaceName
     * @returns an A+ promise
     */
    public async unregisterProvider(domain: string, provider: JoynrProvider): Promise<void> {
        this.checkIfReady();
        // retrieve participantId
        const participantId = this.participantIdStorage.getParticipantId(domain, provider);

        await this.discoveryStub.remove(participantId);

        // unregister routing address at routingTable
        await this.messageRouter.removeNextHop(participantId);

        // if provider has at least one attribute, remove it as publication
        // provider
        this.publicationManager.removePublicationProvider(participantId, provider);

        // unregister provider at RequestReplyManager
        this.requestReplyManager.removeRequestCaller(participantId);

        log.info(
            `Provider unregistered: participantId: ${participantId}, domain: ${domain}, interfaceName: ${
                provider.interfaceName
            }, majorVersion: ${(provider.constructor as JoynrProviderType).MAJOR_VERSION}`
        );
    }

    /**
     * Shutdown the capabilities registrar
     */
    public shutdown(): void {
        this.started = false;
    }

    public static setDefaultExpiryIntervalMs(delay: number): void {
        defaultExpiryIntervalMs = delay;
    }
}

export = CapabilitiesRegistrar;
