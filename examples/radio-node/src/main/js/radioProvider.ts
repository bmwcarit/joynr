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

import {
    InProcessProvisioning,
    UdsLibJoynrProvisioning,
    WebSocketLibjoynrProvisioning
} from "joynr/joynr/start/interface/Provisioning";
import { log } from "./logging";

import joynr from "joynr";
import readline from "readline";
import showHelp from "./console_common";
import RadioProvider from "../generated/js/joynr/vehicle/RadioProvider";
import MyRadioProvider from "./MyRadioProvider";
import InProcessRuntime = require("joynr/joynr/start/InProcessRuntime");
import WebSocketLibjoynrRuntime from "joynr/joynr/start/WebSocketLibjoynrRuntime";

const provisioning: InProcessProvisioning &
    WebSocketLibjoynrProvisioning &
    UdsLibJoynrProvisioning = require("./provisioning_common");

const runInteractiveConsole = function(radioProvider: MyRadioProvider) {
    let res: Function;
    const promise = new Promise(resolve => {
        res = resolve;
    });

    const rl = readline.createInterface(process.stdin, process.stdout);
    rl.setPrompt(">> ");
    const MODES = {
        HELP: {
            value: "h",
            description: "help",
            options: {}
        },
        MULTICAST: {
            value: "w",
            description: "multicast weak signal",
            options: {}
        },
        MULTICASTP: {
            value: "p",
            description: "multicast weak signal with country of current station as partition",
            options: {}
        },
        QUIT: {
            value: "q",
            description: "quit",
            options: {}
        },
        SHUFFLE: {
            value: "shuffle",
            description: "shuffle current station",
            options: {}
        }
    };

    rl.on("line", line => {
        const input = line.trim().split(" ");
        switch (input[0]) {
            case MODES.HELP.value:
                showHelp(MODES);
                break;
            case MODES.QUIT.value:
                rl.close();
                break;
            case MODES.MULTICAST.value:
                radioProvider.fireWeakSignal();
                break;
            case MODES.MULTICASTP.value:
                radioProvider.fireWeakSignalWithPartition();
                break;
            case MODES.SHUFFLE.value:
                radioProvider.shuffleStations();
                break;
            case "":
                break;
            default:
                log(`unknown input: ${input}`);
                break;
        }
        rl.prompt();
    });

    rl.on("close", () => {
        res();
    });

    showHelp(MODES);
    rl.prompt();
    return promise;
};

(async () => {
    if (process.env.domain === undefined) {
        log("please pass a domain as argument");
        process.exit(0);
    }
    // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
    const domain = process.env.domain!;
    log(`domain: ${domain}`);

    provisioning.persistency = {
        //clearPersistency : true,
        location: "./radioLocalStorageProvider"
    };

    if (process.env.runtime !== undefined) {
        if (process.env.runtime === "inprocess") {
            if (process.env.brokerUri === undefined || process.env.bounceProxyBaseUrl === undefined) {
                log("please pass brokerUri and bounceProxyBaseUrl as argument");
                return process.exit(1);
            }
            provisioning.brokerUri = process.env.brokerUri!;
            provisioning.bounceProxyBaseUrl = process.env.bounceProxyBaseUrl!;
            provisioning.bounceProxyUrl = `${provisioning.bounceProxyBaseUrl}/bounceproxy/`;
            joynr.selectRuntime(InProcessRuntime);
        } else if (process.env.runtime === "websocket") {
            if (process.env.wshost === undefined || process.env.wsport === undefined) {
                log("please pass wshost and wsport as argument");
                return process.exit(1);
            }
            provisioning.ccAddress.host = process.env.wshost!;
            provisioning.ccAddress.port = (process.env.wsport as unknown) as number;
            joynr.selectRuntime(WebSocketLibjoynrRuntime);
        } else if (process.env.runtime === "uds") {
            if (!process.env.udspath || !process.env.udsclientid || !process.env.udsconnectsleeptimems) {
                log("please pass udspath, udsclientid, udsconnectsleeptimems as argument");
                return process.exit(1);
            }
            provisioning.uds = {
                socketPath: process.env.udspath,
                clientId: process.env.udsclientid,
                connectSleepTimeMs: Number(process.env.udsconnectsleeptimems)
            }
            // no selectRuntime: UdsLibJoynrRuntime is default
        }
    }

    await joynr.load(provisioning);
    log("joynr started");

    const providerQos = new joynr.types.ProviderQos({
        customParameters: [],
        priority: Date.now(),
        scope: joynr.types.ProviderScope.GLOBAL,
        supportsOnChangeSubscriptions: true
    });

    const radioProviderImpl = new MyRadioProvider();
    const radioProvider = joynr.providerBuilder.build(RadioProvider, radioProviderImpl);
    radioProviderImpl.setProvider(radioProvider);

    let expiryDateMs; // intentionally left undefined by default
    let loggingContext; // intentionally left undefined by default
    let participantId; // intentionally left undefined by default
    const awaitGlobalRegistration = true;

    await joynr.registration.registerProvider(
        domain,
        radioProvider,
        providerQos,
        expiryDateMs,
        loggingContext,
        participantId,
        awaitGlobalRegistration
    );
    log("provider registered successfully");
    await runInteractiveConsole(radioProviderImpl);
    await joynr.registration.unregisterProvider(domain, radioProvider);
    await joynr.shutdown();
    process.exit(0);
})().catch(async e => {
    log(`error running radioProvider: ${e}`);
    await joynr.shutdown();
    process.exit(1);
});
