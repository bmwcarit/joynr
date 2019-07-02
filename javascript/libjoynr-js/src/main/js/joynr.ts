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

import { Provisioning } from "./joynr/start/interface/Provisioning";
import JoynrObject from "./joynr/types/JoynrObject";

import JoynrRuntime = require("./joynr/start/JoynrRuntime");
import JoynrApi = require("./libjoynr-deps");
import TypeRegistry = require("./joynr/start/TypeRegistry");
import ProxyBuilder = require("./joynr/proxy/ProxyBuilder");
import ProviderBuilder = require("./joynr/provider/ProviderBuilder");
import loggingManager = require("./joynr/system/LoggingManager");
import ParticipantIdStorage = require("./joynr/capabilities/ParticipantIdStorage");

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

type JoynrKeys =
    | "shutdown"
    | "terminateAllSubscriptions"
    | "registration"
    | "providerBuilder"
    | "proxyBuilder"
    | "participantIdStorage"
    | "logging"
    | "typeRegistry";

class Joynr extends JoynrApi implements Pick<JoynrRuntime, JoynrKeys> {
    public logging!: loggingManager;
    public participantIdStorage!: ParticipantIdStorage;
    public providerBuilder!: ProviderBuilder;
    public proxyBuilder!: ProxyBuilder;
    public registration: any;
    public typeRegistry!: TypeRegistry;
    public shutdown!: (settings?: any) => Promise<any>;
    public terminateAllSubscriptions!: (timeout?: number) => Promise<any>;

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
        const Runtime =
            this._selectedRuntime === "websocket.libjoynr"
                ? require("./joynr/start/WebSocketLibjoynrRuntime")
                : require("./joynr/start/InProcessRuntime");
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
type joynr = Joynr;
const joynr = new Joynr();
export = joynr;
