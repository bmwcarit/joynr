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
    WebSocketLibjoynrProvisioning,
    UdsLibJoynrProvisioning
} from "joynr/joynr/start/interface/Provisioning";
import { log, prettyLog } from "./logging";
import joynr from "joynr";
import LocalStorage from "joynr/global/LocalStorageNode";
import RadioProxy from "../generated/js/joynr/vehicle/RadioProxy";

// eslint-disable-next-line @typescript-eslint/no-var-requires
const provisioning: InProcessProvisioning &
    WebSocketLibjoynrProvisioning &
    UdsLibJoynrProvisioning = require("./provisioning_common");
import OnChangeSubscriptionQos = require("joynr/joynr/proxy/OnChangeSubscriptionQos");
import RadioStation from "../generated/js/joynr/vehicle/RadioStation";
import Country from "../generated/js/joynr/vehicle/Country";
import showHelp from "./console_common";
import readline from "readline";
import InProcessRuntime = require("joynr/joynr/start/InProcessRuntime");
import WebSocketLibjoynrRuntime from "joynr/joynr/start/WebSocketLibjoynrRuntime";

const persistencyLocation = "./radioLocalStorageConsumer";
const localStorage = new LocalStorage({ location: persistencyLocation, clearPersistency: false });
let subscriptionQosOnChange: OnChangeSubscriptionQos;

type Subscribable<T> = { [K in keyof T]: T[K] extends { subscribe: Function } ? K : never }[keyof T];

function runDemo(radioProxy: RadioProxy): Promise<void> {
    prettyLog("ATTRIBUTE GET: currentStation...");
    return radioProxy.currentStation
        .get()
        .catch((error: any) => {
            prettyLog(`ATTRIBUTE GET: currentStation failed: ${error}`);
        })
        .then((value: any) => {
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
        .catch((error: any) => {
            prettyLog(`RPC: radioProxy.addFavoriteStation(radioStation) failed: ${JSON.stringify(error)}`);
        })
        .then(() => {
            prettyLog("RPC: radioProxy.addFavoriteStation(radioStation) returned");
            prettyLog("radioProxy.shuffleStations()...");
            return radioProxy.shuffleStations();
        })
        .catch((error: any) => {
            prettyLog(`RPC: radioProxy.shuffleStations() failed: ${JSON.stringify(error)}`);
        })
        .then(() => {
            prettyLog("RPC: radioProxy.shuffleStations() returned");
            prettyLog("ATTRIBUTE GET: currentStation after shuffle...");
            return radioProxy.currentStation.get();
        })
        .catch(error => {
            prettyLog(`ATTRIBUTE GET: currentStation failed: ${JSON.stringify(error)}`);
        })
        .then(value => {
            prettyLog(`ATTRIBUTE GET: currentStation returned: ${JSON.stringify(value)}`);
        });
}

function runInteractiveConsole(radioProxy: RadioProxy): Promise<void> {
    let currentStationSubscriptionId = localStorage.getItem("currentStationSubscriptionId");
    let multicastSubscriptionId = localStorage.getItem("multicastSubscriptionId");
    let multicastPSubscriptionId = localStorage.getItem("multicastPSubscriptionId");

    let res: Function;
    // eslint-disable-next-line promise/avoid-new
    const promise = new Promise<void>(resolve => {
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

    function subscribeHelper(settings: {
        subscribeToName: Subscribable<RadioProxy>;
        partitions?: string[];
        subscriptionId: string;
    }): Promise<string> {
        const partitionsString = settings.partitions ? JSON.stringify(settings.partitions) : "";

        const parameters = {
            subscriptionQos: subscriptionQosOnChange,
            onReceive: (value: any) => {
                prettyLog(
                    `radioProxy.${settings.subscribeToName}${partitionsString}.subscribe.onReceive: ${JSON.stringify(
                        value
                    )}`
                );
            },
            onError: (error: any) => {
                prettyLog(`radioProxy.${settings.subscribeToName}${partitionsString}.subscribe.onError: ${error}`);
            },
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
            .catch((error: any) => {
                prettyLog(
                    `radioProxy.${settings.subscribeToName}${partitionsString}.unsubscribe.fail. ` +
                        `Subscription ID: ${settings.subscriptionId} ERROR: ${error}`
                );
            });
    }

    rl.on("line", line => {
        const input = line.trim().split(" ");
        switch (input[0]) {
            case MODES.HELP.value:
                showHelp(MODES);
                break;
            case MODES.QUIT.value:
                rl.close();
                break;
            case MODES.ADD_FAVORITE_STATION.value:
                // eslint-disable-next-line no-case-declarations
                const newFavoriteStation = new RadioStation({
                    name: input[1] || "",
                    trafficService: true,
                    country: Country.GERMANY
                });

                radioProxy
                    .addFavoriteStation({ newFavoriteStation })
                    .then((returnValue: any) => {
                        prettyLog(
                            `RPC: radioProxy.addFavoriteStation(${JSON.stringify(
                                newFavoriteStation
                            )}) returned: ${JSON.stringify(returnValue)}`
                        );
                    })
                    .catch((error: any) => {
                        prettyLog(
                            `RPC: radioProxy.addFavoriteStation(${JSON.stringify(newFavoriteStation)}) failed: ${error}`
                        );
                    });
                break;
            case MODES.SHUFFLE_STATIONS.value:
                radioProxy
                    .shuffleStations()
                    .then(() => {
                        prettyLog("RPC: radioProxy.shuffleStations returned. ");
                    })
                    .catch((error: any) => {
                        prettyLog(`RPC: radioProxy.shuffleStations failed: ${JSON.stringify(error)}`);
                    });
                break;
            case MODES.SUBSCRIBE.value:
                subscribeHelper({
                    subscribeToName: "currentStation",
                    subscriptionId: currentStationSubscriptionId
                })
                    .then((suscriptionId: string) => {
                        currentStationSubscriptionId = suscriptionId;
                        localStorage.setItem("currentStationSubscriptionId", suscriptionId);
                    })
                    .catch((error: any) => prettyLog(error));
                break;
            case MODES.MULTICAST.value:
                subscribeHelper({
                    subscribeToName: "weakSignal",
                    subscriptionId: multicastSubscriptionId
                })
                    .then((suscriptionId: string) => {
                        multicastSubscriptionId = suscriptionId;
                        localStorage.setItem("multicastSubscriptionId", suscriptionId);
                    })
                    .catch((error: any) => prettyLog(error));
                break;
            case MODES.MULTICASTP.value:
                subscribeHelper({
                    subscribeToName: "weakSignal",
                    partitions: ["GERMANY"],
                    subscriptionId: multicastPSubscriptionId
                })
                    .then((suscriptionId: string) => {
                        multicastPSubscriptionId = suscriptionId;
                        localStorage.setItem("multicastPSubscriptionId", suscriptionId);
                    })
                    .catch((error: any) => prettyLog(error));
                break;
            case MODES.UNSUBSCRIBE.value:
                if (currentStationSubscriptionId !== undefined && currentStationSubscriptionId !== null) {
                    unsubscribeHelper({
                        subscribeToName: "currentStation",
                        subscriptionId: currentStationSubscriptionId
                    })
                        .then(() => {
                            localStorage.removeItem(currentStationSubscriptionId);
                            currentStationSubscriptionId = undefined;
                        })
                        .catch((error: any) => prettyLog(error));
                }
                if (multicastSubscriptionId !== undefined && multicastSubscriptionId !== null) {
                    unsubscribeHelper({
                        subscribeToName: "weakSignal",
                        subscriptionId: multicastSubscriptionId
                    })
                        .then(() => {
                            localStorage.removeItem(multicastSubscriptionId);
                            multicastSubscriptionId = undefined;
                        })
                        .catch((error: any) => prettyLog(error));
                }
                if (multicastPSubscriptionId !== undefined && multicastPSubscriptionId !== null) {
                    unsubscribeHelper({
                        subscribeToName: "weakSignal",
                        partitions: ["GERMANY"],
                        subscriptionId: multicastPSubscriptionId
                    })
                        .then(() => {
                            localStorage.removeItem(multicastPSubscriptionId);
                            multicastPSubscriptionId = undefined;
                        })
                        .catch((error: any) => prettyLog(error));
                }
                break;
            case MODES.GET_CURRENT_STATION.value:
                radioProxy.currentStation
                    .get()
                    .then(currentStation => {
                        prettyLog(`RPC: radioProxy.getCurrentStation returned: ${JSON.stringify(currentStation)}`);
                    })
                    .catch(error => {
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

    rl.on("close", () => {
        res();
    });

    showHelp(MODES);
    rl.prompt();
    return promise;
}

(async () => {
    if (process.env.domain === undefined) {
        log("please pass a domain as argument");
        process.exit(1);
    }
    if (process.env.runtime === undefined) {
        log("please pass a runtime as argument");
        process.exit(1);
    }
    // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
    const domain = process.env.domain!;
    log(`domain: ${domain}`);

    provisioning.persistency = {
        location: persistencyLocation
    };

    if (process.env.runtime === "inprocess") {
        if (process.env.brokerUri === undefined || process.env.bounceProxyBaseUrl === undefined) {
            log("please pass brokerUri and bounceProxyBaseUrl as argument");
            return process.exit(1);
        }
        provisioning.brokerUri = process.env.brokerUri;
        provisioning.bounceProxyBaseUrl = process.env.bounceProxyBaseUrl;
        provisioning.bounceProxyUrl = `${process.env.bounceProxyBaseUrl}/bounceproxy/`;
        joynr.selectRuntime(InProcessRuntime);
    } else if (process.env.runtime === "websocket") {
        if (process.env.wshost === undefined || process.env.wsport === undefined) {
            log("please pass wshost and wsport as argument");
            return process.exit(1);
        }
        provisioning.ccAddress.host = process.env.wshost;
        provisioning.ccAddress.port = Number(process.env.wsport);
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
