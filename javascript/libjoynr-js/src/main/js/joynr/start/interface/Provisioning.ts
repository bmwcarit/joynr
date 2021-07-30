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

import { DiscoveryEntryWithMetaInfoMembers } from "../../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
import { GlobalDiscoveryEntryMembers } from "../../../generated/joynr/types/GlobalDiscoveryEntry";

interface DiscoveryQos {
    discoveryTimeoutMs: number;
    discoveryRetryDelayMs: number;
    discoveryExpiryIntervalMs: number;
}

export interface Logging {
    configuration?: {
        loggers?: {
            root: {
                level: string;
            };
        };
        appenders?: {
            appender: {
                PatternLayout: {
                    pattern: string;
                };
            }[];
        };
    };
    /** supports only 1 appender */
    appenderClasses?: {
        [key: string]: {
            prototype: {
                append: Function;
            };
        };
    };
}

export interface Persistency {
    /** clear persistent data during startup. Default value is false */
    clearPersistency?: boolean;
    /** Default is current dir */
    location?: string;
    /**
     * Default true. Persists ParticipantIds of registered
     * providers and thus keeps them upon restart.
     */
    capabilities?: boolean;
}

export interface ShutdownSettings {
    clearSubscriptionsEnabled: boolean;
    /** @default 1000 ms */
    clearSubscriptionsTimeoutMs: number;
}

export interface Provisioning {
    discoveryQos?: DiscoveryQos;
    logging?: Logging;
    /**
     * @deprecated will be ignored
     */
    internalMessagingQos?: {
        ttl: number;
    };
    messaging?: {
        /**
         * max queue size in KB bytes
         * @default 10000
         */
        maxQueueSizeInKBytes: number;
        TTL_UPLIFT?: number;
    };
    persistency?: Persistency;
    shutdownSettings?: ShutdownSettings;
}

export interface InProcessProvisioning extends Provisioning {
    capabilities?: GlobalDiscoveryEntryMembers[];
    bounceProxyUrl: string;
    bounceProxyBaseUrl: string;
    brokerUri: string;
    channelId?: string;
    capabilitiesFreshnessUpdateIntervalMs?: number;
    mqtt: {
        qosLevel: 0 | 1 | 2;
    };
    /**
     * gbids known to the InProcess clustercontroller. Must have a length of at least one.
     * the first element is the default gbid
     **/
    gbids: string[];
}

export interface LibJoynrProvisioning extends Provisioning {
    capabilities?: DiscoveryEntryWithMetaInfoMembers[];
}

export interface WebSocketLibjoynrProvisioning extends LibJoynrProvisioning {
    keychain?: {
        tlsCert: string;
        tlsKey: string;
        tlsCa: string;
        ownerId: string;
    };
    ccAddress: {
        protocol: "ws" | "wss";
        port: number;
        host: string;
        /** @default "" */
        path: string;
    };
    websocket?: {
        /**
         * time in milliseconds between websocket reconnect attempts
         * @default 1000
         */
        reconnectSleepTimeMs: number;
    };
}

export interface UdsLibJoynrProvisioning extends LibJoynrProvisioning {
    uds?: {
        /**
         * socket path to the uds (server) address
         * @default "/var/run/joynr/cluster-controller.sock"
         */
        socketPath?: string;
        /**
         * clientId of the uds connection
         * @default "uuid value will be auto generated"
         */
        clientId?: string;
        /**
         * time in milliseconds between reconnect attempts for the uds connection
         * @default 500
         */
        connectSleepTimeMs?: number;
    };
}
