/*jslint node: true */

/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

const PerformanceUtilities = require("./performanceutilities");
const heapdump = require("heapdump");

const options = PerformanceUtilities.getCommandLineOptionsOrDefaults();
PerformanceUtilities.overrideRequire();

const EchoProvider = require("../generated-javascript/joynr/tests/performance/EchoProvider.js");
const EchoProviderImpl = require("./EchoProviderImpl.js");

const domain = options.domain;

let joynr = require("joynr");
joynr.selectRuntime("websocket.libjoynr");

const testbase = require("test-base");
const provisioning = PerformanceUtilities.getProvisioning(true);
const log = testbase.logging.log;
const error = testbase.logging.error;
log(`domain: ${domain}`);

joynr
    .load(provisioning)
    .then(loadedJoynr => {
        log("joynr started");
        joynr = loadedJoynr;

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
                process.send({
                    msg: "initialized"
                });
            })
            .catch(error => {
                log(`error registering provider: ${error.toString()}`);
            });

        return loadedJoynr;
    })
    .catch(error => {
        throw error;
    });

function fireBroadcasts(numberOfBroadCasts) {
    const implementation = EchoProviderImpl.implementation;

    for (let i = 0; i < numberOfBroadCasts; i++) {
        const stringOut = `boom${i}`;
        const outputParameters = implementation.broadcastWithSinglePrimitiveParameter.createBroadcastOutputParameters();
        outputParameters.setStringOut(stringOut);
        implementation.broadcastWithSinglePrimitiveParameter.fire(outputParameters);
    }
}

let cpuUsage;
let memoryIntervalId;
const measureMemory = options.measureMemory == "true";
let totalMemory = 0;
let totalMemoryMeasurements = 0;
const handler = function(msg) {
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
        const diff = process.cpuUsage(cpuUsage);
        if (measureMemory) {
            diff.averageMemory = totalMemory / totalMemoryMeasurements;
            clearInterval(memoryIntervalId);
        }
        process.send({ msg: "gotMeasurement", data: diff });
    } else if (msg.msg === "takeHeapSnapShot") {
        const fileName = msg.name;
        heapdump.writeSnapshot(fileName, (err, filename) => {
            error(`dump written to: ${filename}`);
        });
    } else if (msg.msg === "fireBroadCast") {
        const numberOfBroadCasts = msg.amount;
        fireBroadcasts(numberOfBroadCasts);
    }
};
process.on("message", handler);
