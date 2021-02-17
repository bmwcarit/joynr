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
import * as DiscoveryEntryWithMetaInfo from "../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
import Arbitrator from "../capabilities/arbitration/Arbitrator";
import { DiscoveryStub } from "../capabilities/interface/DiscoveryStub";

import ProviderBuilder from "../provider/ProviderBuilder";
import ProxyBuilder from "../proxy/ProxyBuilder";
import CapabilitiesRegistrar from "../capabilities/CapabilitiesRegistrar";
import ParticipantIdStorage from "../capabilities/ParticipantIdStorage";
import RequestReplyManager from "../dispatching/RequestReplyManager";
import PublicationManager from "../dispatching/subscription/PublicationManager";
import SubscriptionManager from "../dispatching/subscription/SubscriptionManager";
import Dispatcher from "../dispatching/Dispatcher";
import PlatformSecurityManager from "../security/PlatformSecurityManagerNode";
import MessageRouter, { MessageRouterSettings } from "../messaging/routing/MessageRouter";
import MessageQueue from "../messaging/routing/MessageQueue";
import InProcessMessagingSkeleton from "../messaging/inprocess/InProcessMessagingSkeleton";
import InProcessMessagingStub from "../messaging/inprocess/InProcessMessagingStub";
import InProcessAddress from "../messaging/inprocess/InProcessAddress";
import DiscoveryQos from "../proxy/DiscoveryQos";
import TypeRegistrySingleton from "../types/TypeRegistrySingleton";
import nanoid from "nanoid";
import loggingManager from "../system/LoggingManager";
import { Provisioning, ShutdownSettings } from "./interface/Provisioning";
import defaultLibjoynrSettings from "./settings/defaultLibjoynrSettings";
import LocalStorage from "../../global/LocalStorageNode";
import MemoryStorage from "../../global/MemoryStorage";
import LoggingManager = require("../system/LoggingManager");
import TypeRegistry = require("./TypeRegistry");
import { Persistency } from "../../global/interface/Persistency";
import JoynrStates = require("./JoynrStates");
import * as UtilInternal from "../util/UtilInternal";
import JoynrRuntimeException from "../exceptions/JoynrRuntimeException";

const log = loggingManager.getLogger("joynr.start.JoynrRuntime");
type Omit<T, K> = Pick<T, Exclude<keyof T, K>>;

class JoynrRuntime<T extends Provisioning> {
    protected provisioning!: T;
    protected shutdownSettings?: ShutdownSettings;
    protected discovery!: DiscoveryStub;
    protected persistencyConfig: any;
    protected persistency!: Persistency;
    protected joynrState: typeof JoynrStates[keyof typeof JoynrStates];
    protected multicastSkeletons: Record<string, any> = {};
    protected dispatcher!: Dispatcher;
    protected onFatalRuntimeError: (error: JoynrRuntimeException) => void;
    protected subscriptionManager!: SubscriptionManager;
    protected publicationManager!: PublicationManager;
    protected requestReplyManager!: RequestReplyManager;
    protected messageRouter!: MessageRouter;
    protected arbitrator!: Arbitrator;

    /**
     * @name JoynrRuntime#logging
     * @type LoggingManager
     */
    public logging: LoggingManager;

    /**
     * @name JoynrRuntime#participantIdStorage
     * @type ParticipantIdStorage
     */
    public participantIdStorage?: ParticipantIdStorage;

    /**
     * @name JoynrRuntime#proxyBuilder
     * @type ProxyBuilder
     */
    public proxyBuilder?: ProxyBuilder;

    /**
     * @name JoynrRuntime#providerBuilder
     * @type ProviderBuilder
     */
    public providerBuilder?: ProviderBuilder;

    /**
     * @name JoynrRuntime#registration
     * @type CapabilitiesRegistrar
     */
    public registration?: CapabilitiesRegistrar;

    /**
     * @name JoynrRuntime#typeRegistry
     * @type TypeRegistry
     */
    public typeRegistry: TypeRegistry;

    public constructor(onFatalRuntimeError: (error: JoynrRuntimeException) => void) {
        // all methods apart from start need to be bound here.
        this.shutdown = this.shutdown.bind(this);
        this.terminateAllSubscriptions = this.terminateAllSubscriptions.bind(this);

        this.onFatalRuntimeError = onFatalRuntimeError;

        this.typeRegistry = TypeRegistrySingleton.getInstance();

        this.logging = loggingManager;

        this.joynrState = JoynrStates.SHUTDOWN;
    }

    protected initializePersistency(provisioning: T): Promise<void> {
        const persistencyProvisioning = Object.assign(
            {},
            defaultLibjoynrSettings.persistencySettings,
            provisioning.persistency
        );

        let persistencyPromise;
        if (
            persistencyProvisioning.routingTable ||
            persistencyProvisioning.capabilities ||
            persistencyProvisioning.publications
        ) {
            this.persistency = new LocalStorage({
                clearPersistency: persistencyProvisioning.clearPersistency,
                location: persistencyProvisioning.location
            });
            persistencyPromise = this.persistency.init();
        } else {
            persistencyPromise = Promise.resolve();
        }

        this.persistencyConfig = {
            routingTable: persistencyProvisioning.routingTable ? this.persistency : undefined,
            capabilities: persistencyProvisioning.capabilities ? this.persistency : new MemoryStorage(),
            publications: persistencyProvisioning.publications ? this.persistency : undefined
        };

        return persistencyPromise;
    }

    protected createMessageRouter(
        provisioning: T,
        messageRouterSettings: Omit<MessageRouterSettings, "persistency" | "multicastSkeletons" | "messageQueue">
    ): void {
        if (UtilInternal.checkNullUndefined(this.persistencyConfig)) {
            throw new Error("Call initializePersistency before createMessageRouter.");
        }

        const messageQueueSettings: { maxQueueSizeInKBytes?: number } = {};
        if (provisioning.messaging !== undefined && provisioning.messaging.maxQueueSizeInKBytes !== undefined) {
            messageQueueSettings.maxQueueSizeInKBytes = provisioning.messaging.maxQueueSizeInKBytes;
        }

        (messageRouterSettings as MessageRouterSettings).persistency = this.persistencyConfig.routingTable;
        (messageRouterSettings as MessageRouterSettings).multicastSkeletons = this.multicastSkeletons;
        (messageRouterSettings as MessageRouterSettings).messageQueue = new MessageQueue(messageQueueSettings);

        this.messageRouter = new MessageRouter(messageRouterSettings as MessageRouterSettings);
    }

    protected initializeComponents(
        provisioning: T,
        joynrInstanceId: string,
        discovery: DiscoveryStub,
        externalMessagingStub: InProcessMessagingStub,
        typedCapabilities?: DiscoveryEntryWithMetaInfo[]
    ): void {
        if (UtilInternal.checkNullUndefined(this.messageRouter)) {
            throw new Error("Call createMessageRouter before initializePersistency.");
        }

        this.discovery = discovery;

        const ttlUpLiftMs =
            provisioning.messaging && provisioning.messaging.TTL_UPLIFT ? provisioning.messaging.TTL_UPLIFT : undefined;
        this.dispatcher = new Dispatcher(externalMessagingStub, new PlatformSecurityManager(), ttlUpLiftMs);

        const libjoynrMessagingSkeleton = new InProcessMessagingSkeleton();
        libjoynrMessagingSkeleton.registerListener(this.dispatcher.receive);

        this.requestReplyManager = new RequestReplyManager(this.dispatcher);
        this.subscriptionManager = new SubscriptionManager(this.dispatcher);
        this.publicationManager = new PublicationManager(
            this.dispatcher,
            this.persistencyConfig.publications,
            joynrInstanceId
        );

        this.dispatcher.registerRequestReplyManager(this.requestReplyManager);
        this.dispatcher.registerSubscriptionManager(this.subscriptionManager);
        this.dispatcher.registerPublicationManager(this.publicationManager);
        this.dispatcher.registerMessageRouter(this.messageRouter);

        this.participantIdStorage = new ParticipantIdStorage(this.persistencyConfig.capabilities, nanoid);

        this.registration = new CapabilitiesRegistrar({
            discoveryStub: this.discovery,
            messageRouter: this.messageRouter,
            requestReplyManager: this.requestReplyManager,
            publicationManager: this.publicationManager,
            libjoynrMessagingAddress: new InProcessAddress(libjoynrMessagingSkeleton),
            participantIdStorage: this.participantIdStorage
        });

        // typedCapabilites can be undefined in case of InProcessRuntime
        this.arbitrator = new Arbitrator(this.discovery, typedCapabilities);

        this.providerBuilder = new ProviderBuilder({
            typeRegistry: this.typeRegistry
        });

        this.proxyBuilder = new ProxyBuilder(
            {
                arbitrator: this.arbitrator,
                requestReplyManager: this.requestReplyManager,
                subscriptionManager: this.subscriptionManager,
                publicationManager: this.publicationManager
            },
            {
                messageRouter: this.messageRouter,
                libjoynrMessagingAddress: new InProcessAddress(libjoynrMessagingSkeleton),
                typeRegistry: this.typeRegistry
            }
        );
    }

    /**
     * Starts up the libjoynr instance
     *
     * @returns an A+ promise object, reporting when libjoynr startup is
     *          completed or has failed
     * @throws {Error} if libjoynr is not in SHUTDOWN state
     */
    public start(provisioning: T): void {
        if (this.joynrState !== JoynrStates.SHUTDOWN) {
            throw new Error(`Cannot start libjoynr because it's currently "${this.joynrState}"`);
        }
        this.joynrState = JoynrStates.STARTING;

        if (!provisioning) {
            throw new Error("Constructor has been invoked without provisioning");
        }

        if (provisioning.logging) {
            this.logging.configure(provisioning.logging);
        }

        this.shutdownSettings = provisioning.shutdownSettings;
        this.provisioning = provisioning;

        if (provisioning.discoveryQos) {
            const discoveryQos = provisioning.discoveryQos;

            if (discoveryQos.discoveryExpiryIntervalMs) {
                CapabilitiesRegistrar.setDefaultExpiryIntervalMs(discoveryQos.discoveryExpiryIntervalMs);
            }

            const discoveryQosSettings: Partial<DiscoveryQos.Settings> = {};

            if (discoveryQos.discoveryRetryDelayMs) {
                discoveryQosSettings.discoveryRetryDelayMs = discoveryQos.discoveryRetryDelayMs;
            }
            if (discoveryQos.discoveryTimeoutMs) {
                discoveryQosSettings.discoveryTimeoutMs = discoveryQos.discoveryTimeoutMs;
            }

            DiscoveryQos.setDefaultSettings(discoveryQosSettings);
        }
    }

    /**
     *  Sends subscriptionStop messages for all active subscriptions.
     *
     *  @param timeout {number} optional timeout defaulting to 0 = no timeout
     *  @returns - resolved after all SubscriptionStop messages are sent.
     *  - rejected in case of any issues or timeout occurs.
     */
    public terminateAllSubscriptions(timeout = 0): Promise<any> {
        return this.subscriptionManager.terminateSubscriptions(timeout);
    }

    /**
     * Shuts down libjoynr
     * @param settings.clearSubscriptionsTimeoutMs {number} time in ms till clearSubscriptionsPromise will be rejected
     *  if it's not resolved yet
     * @param settings.clearSubscriptionsEnabled {boolean} clear all subscriptions before shutting down.
     *  Set this to false in process.exit handler as this is not synchronous.
     *
     * @returns - resolved after successful shutdown
     * - rejected in case of any issues
     */
    public async shutdown(settings?: ShutdownSettings): Promise<void> {
        if (this.joynrState !== JoynrStates.STARTED && this.joynrState !== JoynrStates.STARTING) {
            throw new Error(`Cannot shutdown libjoynr because it's currently "${this.joynrState}"`);
        }
        this.joynrState = JoynrStates.SHUTTINGDOWN;

        const shutdownSettings = Object.assign(
            {},
            defaultLibjoynrSettings.shutdownSettings,
            this.shutdownSettings,
            settings || {}
        );

        if (shutdownSettings.clearSubscriptionsEnabled) {
            await this.subscriptionManager
                .terminateSubscriptions(shutdownSettings.clearSubscriptionsTimeoutMs)
                .catch((e: any) => {
                    log.error(`could not shutdown joynr in time due to ${e}`);
                });
        }

        ([
            "registration",
            "arbitrator",
            "messageRouter",
            "requestReplyManager",
            "publicationManager",
            "subscriptionManager",
            "dispatcher",
            "typeRegistry"
        ] as (
            | "registration"
            | "arbitrator"
            | "messageRouter"
            | "requestReplyManager"
            | "publicationManager"
            | "subscriptionManager"
            | "dispatcher"
            | "typeRegistry")[]).forEach(component => {
            if (this[component]) {
                // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                this[component]!.shutdown();
            }
        });

        if (this.persistency !== undefined) await this.persistency.shutdown();

        this.joynrState = JoynrStates.SHUTDOWN;
        log.debug("joynr shut down");
    }
}

export = JoynrRuntime;
