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

const log = require("./logging.js").log;
let joynr = require("joynr");
const readline = require("readline");

const runInteractiveConsole = function(radioProvider, onDone) {
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

    const showHelp = require("./console_common.js");
    rl.on("line", function(line) {
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
                radioProvider.shuffleStations({});
                break;
            case "":
                break;
            default:
                log(`unknown input: ${input}`);
                break;
        }
        rl.prompt();
    });

    rl.on("close", function() {
        if (onDone) {
            onDone()
                .then(function() {
                    process.exit(0);
                })
                .catch(function(error) {
                    console.log(`error while shutting down: ${error}`);
                    process.exit(1);
                });
        } else {
            process.exit(0);
        }
    });

    showHelp(MODES);
    rl.prompt();
};

if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(0);
}
const domain = process.env.domain;
log(`domain: ${domain}`);

const provisioning = require("./provisioning_common.js");

provisioning.persistency = {
    //clearPersistency : true,
    location: "./radioLocalStorageProvider"
};

if (process.env.runtime !== undefined) {
    if (process.env.runtime === "inprocess") {
        provisioning.brokerUri = process.env.brokerUri;
        provisioning.bounceProxyBaseUrl = process.env.bounceProxyBaseUrl;
        provisioning.bounceProxyUrl = `${provisioning.bounceProxyBaseUrl}/bounceproxy/`;
        joynr.selectRuntime("inprocess");
    } else if (process.env.runtime === "websocket") {
        provisioning.ccAddress.host = process.env.cchost;
        provisioning.ccAddress.port = process.env.ccport;
        joynr.selectRuntime("websocket.libjoynr");
    }
}

const RadioProvider = require("../generated/js/joynr/vehicle/RadioProvider.js");
const MyRadioProvider = require("./MyRadioProvider.js");
joynr.load(provisioning).then(function(loadedJoynr) {
    log("joynr started");
    joynr = loadedJoynr;

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

    joynr.registration
        .registerProvider(
            domain,
            radioProvider,
            providerQos,
            expiryDateMs,
            loggingContext,
            participantId,
            awaitGlobalRegistration
        )
        .then(function() {
            log("provider registered successfully");
            runInteractiveConsole(radioProviderImpl, function() {
                return joynr.registration
                    .unregisterProvider(domain, radioProvider)
                    .then(function() {
                        joynr.shutdown();
                    })
                    .catch(function() {
                        joynr.shutdown();
                    });
            });
            return null;
        })
        .catch(function(error) {
            log(`error registering provider: ${error.toString()}`);
            joynr.shutdown();
        });
    return loadedJoynr;
});
