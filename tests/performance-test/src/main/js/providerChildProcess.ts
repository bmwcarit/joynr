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

import * as PerformanceUtilities from "./performanceutilities";
import heapdump from "heapdump";

const options = PerformanceUtilities.getCommandLineOptionsOrDefaults();

import EchoProvider from "../generated-javascript/joynr/tests/performance/EchoProvider";
import * as EchoProviderImpl from "./EchoProviderImpl";

const domain = options.domain;

import joynr from "joynr";
joynr.selectRuntime("websocket.libjoynr");

import testbase from "test-base";
const provisioning = PerformanceUtilities.getProvisioning(true);
const log = testbase.logging.log;
const error = testbase.logging.error;
log(`domain: ${domain}`);

joynr.load(provisioning).then(() => {
    log("joynr started");

    const providerQos = new joynr.types.ProviderQos({
        customParameters: [],
        priority: Date.now(),
        scope: joynr.types.ProviderScope.LOCAL,
        supportsOnChangeSubscriptions: true
    });

    const echoProvider = joynr.providerBuilder.build(EchoProvider, EchoProviderImpl.implementation);

    return joynr.registration
        .registerProvider(domain, echoProvider, providerQos)
        .then(() => {
            log("provider registered successfully");
            process.send!({
                msg: "initialized"
            });
        })
        .catch((error: any) => {
            log(`error registering provider: ${error}`);
        });
});

function fireBroadcasts(numberOfBroadCasts: number) {
    const implementation = EchoProviderImpl.implementation;

    for (let i = 0; i < numberOfBroadCasts; i++) {
        const stringOut = `boom${i}`;
        const outputParameters = implementation.broadcastWithSinglePrimitiveParameter!.createBroadcastOutputParameters();
        outputParameters.setStringOut(stringOut);
        implementation.broadcastWithSinglePrimitiveParameter!.fire(outputParameters);
    }
}

let cpuUsage: NodeJS.CpuUsage;
let memoryIntervalId: NodeJS.Timer;
const measureMemory = options.measureMemory == "true";
let totalMemory = 0;
let totalMemoryMeasurements = 0;
const handler = function(msg: { msg: string; name: any; amount: any }) {
    if (msg.msg === "terminate") {
        joynr.shutdown();
    } else if (msg.msg === "startMeasurement") {
        if (measureMemory) {
            memoryIntervalId = setInterval(() => {
                const memoryUsage = process.memoryUsage();
                totalMemory += memoryUsage.rss;
                totalMemoryMeasurements++;
            }, 500);
        }
        cpuUsage = process.cpuUsage();
    } else if (msg.msg === "stopMeasurement") {
        const diff: NodeJS.CpuUsage & { averageMemory?: number } = process.cpuUsage(cpuUsage);
        if (measureMemory) {
            diff.averageMemory = totalMemory / totalMemoryMeasurements;
            clearInterval(memoryIntervalId);
        }
        process.send!({ msg: "gotMeasurement", data: diff });
    } else if (msg.msg === "takeHeapSnapShot") {
        const fileName = msg.name;
        heapdump.writeSnapshot(fileName, (_err, filename) => {
            error(`dump written to: ${filename}`);
        });
    } else if (msg.msg === "fireBroadCast") {
        const numberOfBroadCasts = msg.amount;
        fireBroadcasts(numberOfBroadCasts);
    }
};
process.on("message", handler);
