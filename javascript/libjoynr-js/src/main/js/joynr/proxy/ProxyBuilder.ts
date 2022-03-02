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
import Address from "../../generated/joynr/system/RoutingTypes/Address";

import { nanoid } from "nanoid";
import DiscoveryQos from "./DiscoveryQos";
import JoynrRuntimeException from "../exceptions/JoynrRuntimeException";
import MessagingQos from "../messaging/MessagingQos";
import TypeRegistrySingleton from "../../joynr/types/TypeRegistrySingleton";
import Version from "../../generated/joynr/types/Version";
import * as Typing from "../util/Typing";
import LoggingManager from "../system/LoggingManager";
import Arbitrator from "../capabilities/arbitration/Arbitrator";
import TypeRegistry from "../start/TypeRegistry";
import RequestReplyManager from "../dispatching/RequestReplyManager";
import SubscriptionManager from "../dispatching/subscription/SubscriptionManager";
import PublicationManager from "../dispatching/subscription/PublicationManager";
import MessageRouter from "../messaging/routing/MessageRouter";

const log = LoggingManager.getLogger("joynr.proxy.ProxyBuilder");

interface JoynrProxy {
    proxyParticipantId: string;
    domain: string;
    interfaceName: string;
    providerDiscoveryEntry: any;
}

interface JoynrProxyConstructor<T extends JoynrProxy> {
    new (...args: any[]): T;
    getUsedJoynrtypes(): any[];
    MAJOR_VERSION: number;
    MINOR_VERSION: number;
}

const typeRegistry = TypeRegistrySingleton.getInstance();

class ProxyBuilder {
    private messageRouter: MessageRouter;
    private libjoynrMessagingAddress: Address;
    private proxyDependencies: any;
    private arbitrator: Arbitrator;
    /**
     * @constructor
     *
     * @param proxyDependencies injected for the children of the proxyBuilder and its children
     * @param proxyDependencies.arbitrator used by the proxyBuilder to find the correct provider
     * @param proxyDependencies.requestReplyManager passed on to proxyAttribute and
     *            proxyOperation
     * @param proxyDependencies.subscriptionManager passed on to proxyAttribute and
     *            proxyOperation
     * @param proxyDependencies.publicationManager passed on to proxyAttribute and
     *            proxyOperation
     * @param dependencies injected for the proxyBuilder
     * @param dependencies.messageRouter the message router
     * @param dependencies.libjoynrMessagingAddress address to this libjoynr's message receiver
     * @param dependencies.typeRegistry the typeRegistry being able to augment raw objects with
     *            type information
     */
    public constructor(
        proxyDependencies: {
            arbitrator: Arbitrator;
            requestReplyManager: RequestReplyManager;
            subscriptionManager: SubscriptionManager;
            publicationManager: PublicationManager;
        },
        dependencies: {
            messageRouter: MessageRouter;
            libjoynrMessagingAddress: Address;
            typeRegistry: TypeRegistry;
        }
    ) {
        this.arbitrator = proxyDependencies.arbitrator;
        this.proxyDependencies = proxyDependencies;
        this.messageRouter = dependencies.messageRouter;
        this.libjoynrMessagingAddress = dependencies.libjoynrMessagingAddress;
    }

    /**
     * A function that constructs, arbitrates a object and provides the result and error using
     * an A+ promise
     *
     * @param ProxyConstructor - the constructor function of the generated Proxy that
     *            creates a new proxy instance
     * @param settings - the settings object that is passed to the constructor when building
     *            the object
     * @param settings.domain - the domain on which the provider should be looked for
     * @param settings.discoveryQos - the settings object determining arbitration
     *            parameters
     * @param settings.messagingQos - the settings object determining messaging quality of
     *            service parameters
     * @param settings.loggingContext - optional logging context will be appended to logging
     *            messages created in the name of this proxy
     * @param settings.staticArbitration - true if staticArbitration shall be used
     * @param settings.gbids - optional global backend identifiers of backends to lookup capabilities
     *
     * @returns Promise
     *  - resolved with the created proxy
     *  - rejected with either DiscoveryException or NoCompatibleProviderFoundException
     */
    public async build<T extends JoynrProxy>(
        ProxyConstructor: JoynrProxyConstructor<T>,
        settings: {
            domain: string;
            discoveryQos?: DiscoveryQos | Partial<DiscoveryQos.Settings>;
            messagingQos?: MessagingQos | Partial<MessagingQos.Settings>;
            loggingContext?: Record<string, any>;
            staticArbitration?: boolean;
            gbids?: string[];
        }
    ): Promise<T> {
        // eslint-disable-next-line prefer-const
        let { domain, discoveryQos, messagingQos, loggingContext, staticArbitration, gbids } = settings;

        // augment Qos objects if they're missing
        discoveryQos = new DiscoveryQos(discoveryQos);
        messagingQos = new MessagingQos(messagingQos);

        // check if objects are there and of correct type
        Typing.checkProperty(domain, "String", "settings.domain");
        Typing.checkProperty(discoveryQos, DiscoveryQos, "settings.discoveryQos");
        Typing.checkProperty(messagingQos, MessagingQos, "settings.messagingQos");

        const proxyParticipantId = nanoid();

        const proxy = new ProxyConstructor({
            domain,
            discoveryQos,
            messagingQos,
            loggingContext,
            proxyParticipantId,
            dependencies: this.proxyDependencies
        });

        ProxyConstructor.getUsedJoynrtypes().forEach(joynrType => {
            typeRegistry.addType(joynrType);
        });

        const proxyVersion = new Version({
            majorVersion: ProxyConstructor.MAJOR_VERSION,
            minorVersion: ProxyConstructor.MINOR_VERSION
        });
        const arbitratedCaps = await this.arbitrator.startArbitration({
            domains: [proxy.domain],
            interfaceName: proxy.interfaceName,
            // typecast may be removed after Arbitrator typings are done
            discoveryQos: discoveryQos as any,
            staticArbitration: staticArbitration as boolean,
            proxyVersion,
            gbids
        });

        if (settings.loggingContext !== undefined) {
            log.warn("loggingContext is currently not supported");
        }
        let isGloballyVisible = false;
        if (arbitratedCaps && arbitratedCaps.length > 0) {
            proxy.providerDiscoveryEntry = arbitratedCaps[0];
            if (!arbitratedCaps[0].isLocal) {
                isGloballyVisible = true;
            }
        }

        return this.messageRouter
            .addNextHop(proxy.proxyParticipantId, this.libjoynrMessagingAddress, isGloballyVisible)
            .then(() => {
                this.messageRouter.setToKnown(proxy.providerDiscoveryEntry.participantId);
                log.info(
                    `Proxy created, proxy participantId: ${
                        proxy.proxyParticipantId
                    }, provider discoveryEntry: ${JSON.stringify(proxy.providerDiscoveryEntry)}`
                );
                return proxy;
            })
            .catch((error: any) => {
                const errorMsg = `Proxy creation for domain ${proxy.domain}, interface ${
                    proxy.interfaceName
                }, major version ${ProxyConstructor.MAJOR_VERSION}, proxyParticipantId ${
                    proxy.proxyParticipantId
                } failed. Exception occurred while registering the address to MessageRouter. Error: ${error.stack}`;
                log.debug(errorMsg);
                throw new JoynrRuntimeException({ detailMessage: errorMsg });
            });
    }
}

export = ProxyBuilder;
