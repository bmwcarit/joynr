/*eslint global-require: "off"*/
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
/* istanbul ignore file */

import JoynrObject from "./joynr/types/JoynrObject";

import JoynrRuntime from "./joynr/start/JoynrRuntime";
import JoynrApi from "./libjoynr-deps";

/**
 * copies all non private members and methods to joynr
 * methods need to be bound with this and can't be on the prototype
 *
 * @param joynr
 * @param runtime
 */
function wrapRuntime(joynr: any, runtime: JoynrRuntime): void {
    (Object.keys(runtime) as (keyof JoynrRuntime)[]).forEach(
        (key): void => {
            if (!key.startsWith("_")) {
                joynr[key] = runtime[key];
            }
        }
    );
}

interface TypedJoynrEnum<T> {
    name: T;
    value: T;
}

interface Capabilities {
    domain: string;
    interfaceName: string;
    providerQos: {
        customParameters: {
            name: string;
            value: string;
        }[];
        scope: TypedJoynrEnum<"LOCAL" | "GLOBAL">;
        priority: number;
        supportsOnChangeSubscriptions: boolean;
    };
    providerVersion: {
        majorVersion: number;
        minorVersion: number;
    };
    participantId: string;
}

interface DiscoveryQos {
    discoveryTimeoutMs: number;
    discoveryRetryDelayMs: number;
    discoveryExpiryIntervalMs: number;
}

interface Logging {
    configuration?: {
        loggers?: {
            root: {
                level: string;
            };
        };
        appenders?: {
            appender: {
                0: {
                    PatternLayout: {
                        pattern: string;
                    };
                };
            };
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

interface Persistency {
    /** clear persistent data during startup. Default value is false */
    clearPersistency?: boolean;
    /** Default is current dir */
    location?: string;
    /** Max local storage quota, in MB. Defaults to 5 MB. */
    quota?: number;
    /**
     * Default false. Persists RoutingTable entries and thus
     * allows the runtime to restart without help from the cc.
     */
    routingTable?: boolean;
    /**
     * Default true. Persists ParticipantIds of registered
     * providers and thus keeps them upon restart.
     */
    capabilities?: boolean;
    /**
     * Default true. Persists previously received subscriptionRequests and thus
     * allows publications to resume successfully upon restart.
     */
    publications?: boolean;
}

interface Provisioning {
    capabilities?: Capabilities[];
    discoveryQos?: DiscoveryQos;
    logging?: Logging;
    /** messaging qos used for joynr internal communication */
    internalMessagingQos?: {
        /**
         * round trip timeout ms for rpc requests
         * @default 60000
         */
        ttl: number;
    };
    messaging?: {
        /**
         * max queue size in KB bytes
         * @default 10000
         */
        maxQueueSizeInKBytes: number;
    };
    persistency?: Persistency;
    shutdownSettings?: {
        clearSubscriptionsEnabled: boolean;
        /** @default 1000 ms */
        clearSubscriptionsTimeoutMs: number;
    };
    ccAddress: {
        protocol: "ws" | "wss";
        port?: number;
        host?: string;
        /** @default "" */
        path?: string;
    };
    websocket?: {
        /**
         * time in milliseconds between websocket reconnect attempts
         * @default 1000
         */
        reconnectSleepTimeMs: number;
    };
}

type JoynrKeys =
    | "shutdown"
    | "terminateAllSubscriptions"
    | "registration"
    | "providerBuilder"
    | "proxyBuilder"
    | "participantIdStorage"
    | "logging"
    | "typeRegistry";

class Joynr extends JoynrApi implements Partial<Pick<JoynrRuntime, JoynrKeys>> {
    public shutdown?: (settings: any) => Promise<any>;
    public terminateAllSubscriptions?: (timeout?: number) => Promise<any>;

    private loaded: boolean = false;
    public JoynrObject = JoynrObject;
    public _selectedRuntime = "websocket.libjoynr";

    /**
     * @param provisioning
     * @return Promise object being resolved in case all libjoynr dependencies are loaded
     */
    public async load(provisioning: Provisioning): Promise<Joynr> {
        this.loaded = true;
        // eslint-disable-next-line @typescript-eslint/no-var-requires
        const Runtime = require("./joynr/Runtime");
        const runtime = new Runtime();
        try {
            await runtime.start(provisioning);
            wrapRuntime(this, runtime);

            // make sure the runtime is shutdown when process.exit(...)
            // gets called since otherwise the process might not
            // terminate. Ignore any exception thrown in case shutdown
            // had already been invoked manually before reaching this
            // point.
            if (typeof process === "object" && typeof process.on === "function") {
                process.on(
                    "exit",
                    (): void => {
                        try {
                            // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                            this.shutdown!({ clearSubscriptionsEnabled: false });
                        } catch (error) {
                            // ignore
                        }
                    }
                );
            }
            return this;
        } catch (error1) {
            return Promise.reject(error1);
        }
    }

    public selectRuntime(runtime: string): void {
        if (this.loaded) {
            throw new Error("joynr.selectRuntime: this method must be invoked before calling joynr.load()");
        }
        this._selectedRuntime = runtime;
    }
}

export = new Joynr();
