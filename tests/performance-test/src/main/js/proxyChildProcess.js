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

let joynr = require("joynr");
const testbase = require("test-base");
const PerformanceUtilities = require("./performanceutilities.js");
PerformanceUtilities.overrideRequire();

const error = testbase.logging.error;
const log = testbase.logging.log;
const options = PerformanceUtilities.getCommandLineOptionsOrDefaults();
const timeout = 600000;
const Benchmarks = require("./Benchmarks");
let benchmarks;

const testType = options.testType;

log(`Using domain ${options.domain}`);
error(`test runs: ${options.testRuns}`);
error(`Using testType: ${testType}`);
const provisioning = PerformanceUtilities.getProvisioning(false);
provisioning.ccAddress.host = options.cchost;
provisioning.ccAddress.port = options.ccport;
joynr.selectRuntime("websocket.libjoynr");

const heapdump = require("heapdump");

let echoProxy;

joynr
    .load(provisioning)
    .then(loadedJoynr => {
        joynr = loadedJoynr;
        log("joynr started");
        return loadedJoynr;
    })
    .then(loadedJoynr => {
        const messagingQos = new loadedJoynr.messaging.MessagingQos({
            ttl: timeout
        });
        const EchoProxy = require("../generated-javascript/joynr/tests/performance/EchoProxy.js");
        return loadedJoynr.proxyBuilder.build(EchoProxy, {
            domain: options.domain,
            messagingQos
        });
    })
    .then(echoProxy1 => {
        log("build proxy");
        echoProxy = echoProxy1;
        benchmarks = new Benchmarks(echoProxy, joynr);
        process.send({ msg: "initialized" });
    })
    .catch(e => {
        throw e;
    });

let count = 0;
let totalBroadcastToReceive = 0;
function countReceivedBroadCasts() {
    count++;
    if (count === totalBroadcastToReceive) {
        process.send({ msg: "receivedBroadcasts" });
    }
}

let broadCastsPreparedOnce = false;

function prepareBroadcast(amount) {
    if (broadCastsPreparedOnce) {
        count = 0;
        return Promise.resolve();
    }
    broadCastsPreparedOnce = true;
    const promise = PerformanceUtilities.createPromise();
    totalBroadcastToReceive = amount;

    const subscriptionQosOnChange = new joynr.proxy.MulticastSubscriptionQos({ validityMs: 60000 });

    echoProxy.broadcastWithSinglePrimitiveParameter.subscribe({
        subscriptionQos: subscriptionQosOnChange,
        onReceive: countReceivedBroadCasts,
        onError: promise.reject,
        onSubscribed: promise.resolve
    });
    return promise.promise;
}

let cpuUsage;
let memoryIntervalId;
const measureMemory = options.measureMemory == "true";
let totalMemory = 0;
let totalMemoryMeasurements = 0;
let testData;

let executeBenchmark;

switch (testType) {
    case "burst":
        executeBenchmark = function(name) {
            const promiseArray = testData.map(item => benchmarks[name].testProcedure(item));
            Promise.all(promiseArray).then(() => process.send({ msg: "executeBenchmarkFinished" }));
        };
        break;
    case "single":
        executeBenchmark = function(name) {
            testData
                .reduce((accumulator, item) => {
                    return accumulator.then(() => benchmarks[name].testProcedure(item));
                }, Promise.resolve())
                .then(() => process.send({ msg: "executeBenchmarkFinished" }));
        };
        break;
    default:
        throw new Error("unknown testType");
}

const handler = function(msg) {
    switch (msg.msg) {
        case "terminate":
            joynr.shutdown();
            break;
        case "startMeasurement":
            if (measureMemory) {
                memoryIntervalId = setInterval(() => {
                    const memoryUsage = process.memoryUsage();
                    totalMemory += memoryUsage.rss;
                    totalMemoryMeasurements++;
                }, 500);
            }
            cpuUsage = process.cpuUsage();
            break;
        case "stopMeasurement":
            var diff = process.cpuUsage(cpuUsage);
            if (measureMemory) {
                diff.averageMemory = totalMemory / totalMemoryMeasurements;
                clearInterval(memoryIntervalId);
            }
            process.send({ msg: "gotMeasurement", data: diff });
            break;
        case "prepareBenchmark":
            testData = [];
            var numRuns = msg.config.numRuns;
            for (let i = 0; i < numRuns; i++) {
                testData.push(benchmarks[msg.config.name].generateData(i));
            }
            process.send({ msg: "prepareBenchmarkFinished" });
            break;
        case "executeBenchmark":
            executeBenchmark(msg.config.name);
            break;
        case "takeHeapSnapShot":
            var fileName = msg.name;
            heapdump.writeSnapshot(fileName, (err, filename) => {
                error(`dump written to: ${filename}`);
            });
            break;
        case "subscribeBroadcast":
            prepareBroadcast(msg.amount).then(() => process.send({ msg: "subscriptionFinished" }));
            break;
        default:
            throw new Error("unknown messageType");
            break;
    }
};
process.on("message", handler);
