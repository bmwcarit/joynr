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
    WebSocketLibjoynrProvisioning
} from "../../../../../javascript/libjoynr-js/src/main/js/joynr/start/interface/Provisioning";
import { log, prettyLog } from "./logging";
import joynr from "joynr";
import LocalStorage from "joynr/global/LocalStorageNode";
import RadioProxy from "../generated/js/joynr/vehicle/RadioProxy";
const provisioning: InProcessProvisioning & WebSocketLibjoynrProvisioning = require("./provisioning_common");
import OnChangeSubscriptionQos = require("joynr/joynr/proxy/OnChangeSubscriptionQos");
import RadioStation from "../generated/js/joynr/vehicle/RadioStation";
import Country from "../generated/js/joynr/vehicle/Country";
import showHelp from "./console_common";
import readline from "readline";

const persistencyLocation = "./radioLocalStorageConsumer";
const localStorage = new LocalStorage({ location: persistencyLocation, clearPersistency: false });
let subscriptionQosOnChange: OnChangeSubscriptionQos;

type Subscribable<T> = { [K in keyof T]: T[K] extends { subscribe: Function } ? K : never }[keyof T];

function runDemo(radioProxy: RadioProxy): Promise<void> {
    prettyLog("ATTRIBUTE GET: currentStation...");
    return radioProxy.currentStation
        .get()
        .catch(function(error: any) {
            prettyLog(`ATTRIBUTE GET: currentStation failed: ${error}`);
        })
        .then(function(value: any) {
            prettyLog(`ATTRIBUTE GET: currentStation returned: ${JSON.stringify(value)}`);
            prettyLog("RPC: radioProxy.addFavoriteStation(radioStation)...");
            return radioProxy.addFavoriteStation({
                newFavoriteStation: new RadioStation({
                    name: "runDemoFavoriteStation",
                    trafficService: true,
                    country: Country.GERMANY
                })
            });
        })
        .catch(function(error: any) {
            prettyLog(`RPC: radioProxy.addFavoriteStation(radioStation) failed: ${JSON.stringify(error)}`);
        })
        .then(function() {
            prettyLog("RPC: radioProxy.addFavoriteStation(radioStation) returned");
            prettyLog("radioProxy.shuffleStations()...");
            return radioProxy.shuffleStations();
        })
        .catch(function(error: any) {
            prettyLog(`RPC: radioProxy.shuffleStations() failed: ${JSON.stringify(error)}`);
        })
        .then(function() {
            prettyLog("RPC: radioProxy.shuffleStations() returned");
            prettyLog("ATTRIBUTE GET: currentStation after shuffle...");
            return radioProxy.currentStation.get();
        })
        .catch(function(error) {
            prettyLog(`ATTRIBUTE GET: currentStation failed: ${JSON.stringify(error)}`);
        })
        .then(function(value) {
            prettyLog(`ATTRIBUTE GET: currentStation returned: ${JSON.stringify(value)}`);
        });
}

function runInteractiveConsole(radioProxy: RadioProxy) {
    let currentStationSubscriptionId = localStorage.getItem("currentStationSubscriptionId");
    let multicastSubscriptionId = localStorage.getItem("multicastSubscriptionId");
    let multicastPSubscriptionId = localStorage.getItem("multicastPSubscriptionId");

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
        QUIT: {
            value: "q",
            description: "quit",
            options: {}
        },
        SHUFFLE_STATIONS: {
            value: "s",
            description: "shuffle stations",
            options: {}
        },
        ADD_FAVORITE_STATION: {
            value: "a",
            description: "add a Favorite Station",
            options: {
                NAME: "name"
            }
        },
        SUBSCRIBE: {
            value: "subscribe",
            description: "subscribe to current station",
            options: {}
        },
        MULTICAST: {
            value: "subscribeMulticast",
            description: "subscribe to weak signal multicast",
            options: {}
        },
        MULTICASTP: {
            value: "subscribeMulticastP",
            description: 'subscribe to weak signal multicast with partition "GERMANY"',
            options: {}
        },
        UNSUBSCRIBE: {
            value: "unsubscribe",
            description: "unsubscribe from all subscriptions",
            options: {}
        },
        GET_CURRENT_STATION: {
            value: "c",
            description: "get current station",
            options: {}
        }
    };

    rl.on("line", function(line) {
        const input = line.trim().split(" ");

        function subscribeHelper(settings: {
            subscribeToName: Subscribable<RadioProxy>;
            partitions?: string[];
            subscriptionId: string;
        }): Promise<string> {
            const partitionsString = settings.partitions ? JSON.stringify(settings.partitions) : "";
            function onReceiveCallback(value: any) {
                prettyLog(
                    `radioProxy.${settings.subscribeToName}${partitionsString}.subscribe.onReceive: ${JSON.stringify(
                        value
                    )}`
                );
            }

            function onErrorCallback(error: any) {
                prettyLog(`radioProxy.${settings.subscribeToName}${partitionsString}.subscribe.onError: ${error}`);
            }

            const parameters = {
                subscriptionQos: subscriptionQosOnChange,
                onReceive: onReceiveCallback,
                onError: onErrorCallback,
                partitions: settings.partitions,
                subscriptionId: settings.subscriptionId
            };

            return radioProxy[settings.subscribeToName]
                .subscribe(parameters)
                .then((subscriptionId: string) => {
                    prettyLog(
                        `radioProxy.${settings.subscribeToName}${partitionsString}.subscribe.done. ` +
                            `Subscription ID: ${subscriptionId}`
                    );
                    return subscriptionId;
                })
                .catch((error: any) => {
                    prettyLog(
                        `radioProxy.${settings.subscribeToName}${partitionsString}.subscribe.failed: ${JSON.stringify(
                            error
                        )}`
                    );
                    throw error;
                });
        }
        function unsubscribeHelper(settings: {
            subscribeToName: Subscribable<RadioProxy>;
            partitions?: string[];
            subscriptionId: string;
        }): Promise<void> {
            const partitionsString = settings.partitions ? JSON.stringify(settings.partitions) : "";
            return radioProxy[settings.subscribeToName]
                .unsubscribe({
                    subscriptionId: settings.subscriptionId
                })
                .then(() => {
                    prettyLog(
                        `radioProxy.${settings.subscribeToName}${partitionsString}.unsubscribe.done. ` +
                            `Subscription ID: ${settings.subscriptionId}`
                    );
                })
                .catch(function(error: any) {
                    prettyLog(
                        `radioProxy.${settings.subscribeToName}${partitionsString}.unsubscribe.fail. ` +
                            `Subscription ID: ${settings.subscriptionId} ERROR: ${error}`
                    );
                });
        }
        switch (input[0]) {
            case MODES.HELP.value:
                showHelp(MODES);
                break;
            case MODES.QUIT.value:
                rl.close();
                break;
            case MODES.ADD_FAVORITE_STATION.value:
                var newFavoriteStation = new RadioStation({
                    name: input[1] || "",
                    trafficService: true,
                    country: Country.GERMANY
                });

                radioProxy
                    .addFavoriteStation({ newFavoriteStation })
                    .then(function(returnValue: any) {
                        prettyLog(
                            `RPC: radioProxy.addFavoriteStation(${JSON.stringify(
                                newFavoriteStation
                            )}) returned: ${JSON.stringify(returnValue)}`
                        );
                    })
                    .catch(function(error: any) {
                        prettyLog(
                            `RPC: radioProxy.addFavoriteStation(${JSON.stringify(newFavoriteStation)}) failed: ${error}`
                        );
                    });
                break;
            case MODES.SHUFFLE_STATIONS.value:
                radioProxy
                    .shuffleStations()
                    .then(function() {
                        prettyLog("RPC: radioProxy.shuffleStations returned. ");
                    })
                    .catch(function(error: any) {
                        prettyLog(`RPC: radioProxy.shuffleStations failed: ${JSON.stringify(error)}`);
                    });
                break;
            case MODES.SUBSCRIBE.value:
                subscribeHelper({
                    subscribeToName: "currentStation",
                    subscriptionId: currentStationSubscriptionId
                }).then(function(suscriptionId: string) {
                    currentStationSubscriptionId = suscriptionId;
                    localStorage.setItem("currentStationSubscriptionId", suscriptionId);
                });
                break;
            case MODES.MULTICAST.value:
                subscribeHelper({
                    subscribeToName: "weakSignal",
                    subscriptionId: multicastSubscriptionId
                }).then(function(suscriptionId: string) {
                    multicastSubscriptionId = suscriptionId;
                    localStorage.setItem("multicastSubscriptionId", suscriptionId);
                });
                break;
            case MODES.MULTICASTP.value:
                subscribeHelper({
                    subscribeToName: "weakSignal",
                    partitions: ["GERMANY"],
                    subscriptionId: multicastPSubscriptionId
                }).then(function(suscriptionId: string) {
                    multicastPSubscriptionId = suscriptionId;
                    localStorage.setItem("multicastPSubscriptionId", suscriptionId);
                });
                break;
            case MODES.UNSUBSCRIBE.value:
                if (currentStationSubscriptionId !== undefined && currentStationSubscriptionId !== null) {
                    unsubscribeHelper({
                        subscribeToName: "currentStation",
                        subscriptionId: currentStationSubscriptionId
                    }).then(function() {
                        localStorage.removeItem(currentStationSubscriptionId);
                        currentStationSubscriptionId = undefined;
                    });
                }
                if (multicastSubscriptionId !== undefined && multicastSubscriptionId !== null) {
                    unsubscribeHelper({
                        subscribeToName: "weakSignal",
                        subscriptionId: multicastSubscriptionId
                    }).then(function() {
                        localStorage.removeItem(multicastSubscriptionId);
                        multicastSubscriptionId = undefined;
                    });
                }
                if (multicastPSubscriptionId !== undefined && multicastPSubscriptionId !== null) {
                    unsubscribeHelper({
                        subscribeToName: "weakSignal",
                        partitions: ["GERMANY"],
                        subscriptionId: multicastPSubscriptionId
                    }).then(function() {
                        localStorage.removeItem(multicastPSubscriptionId);
                        multicastPSubscriptionId = undefined;
                    });
                }
                break;
            case MODES.GET_CURRENT_STATION.value:
                radioProxy.currentStation
                    .get()
                    .then(function(currentStation) {
                        prettyLog(`RPC: radioProxy.getCurrentStation returned: ${JSON.stringify(currentStation)}`);
                    })
                    .catch(function(error) {
                        prettyLog(`RPC: radioProxy.getCurrentStation failed: ${error}`);
                    });
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
        res();
    });

    showHelp(MODES);
    rl.prompt();
    return promise;
}

(async () => {
    if (process.env.domain === undefined) {
        log("please pass a domain as argument");
        process.exit(0);
    }
    const domain = process.env.domain!;
    log(`domain: ${domain}`);

    provisioning.persistency = {
        location: persistencyLocation
    };

    if (process.env.runtime !== undefined) {
        if (process.env.runtime === "inprocess") {
            provisioning.brokerUri = process.env.brokerUri!;
            provisioning.bounceProxyBaseUrl = process.env.bounceProxyBaseUrl!;
            provisioning.bounceProxyUrl = `${provisioning.bounceProxyBaseUrl!}/bounceproxy/`;
            joynr.selectRuntime("inprocess");
        } else if (process.env.runtime === "websocket") {
            provisioning.ccAddress.host = process.env.cchost!;
            provisioning.ccAddress.port = (process.env.ccport as unknown) as number;
            joynr.selectRuntime("websocket.libjoynr");
        }
    }
    await localStorage.init();
    await joynr.load(provisioning);
    log("joynr started");
    const messagingQos = new joynr.messaging.MessagingQos({
        ttl: 60000
    });

    subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
        minIntervalMs: 50
    });
    const radioProxy = await joynr.proxyBuilder.build(RadioProxy, {
        domain,
        messagingQos
    });
    log("radio proxy build");
    await runDemo(radioProxy);
    await runInteractiveConsole(radioProxy);
    log("exiting...");
    await joynr.shutdown();
    log("shutdown completed...");
    process.exit(0);
})().catch(e => {
    log(`error running radioProxy: ${e}`);
    joynr.shutdown();
});
