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

import joynr from "joynr";
import EchoProxy from "../generated-javascript/joynr/tests/performance/EchoProxy";
import * as heapdump from "heapdump";
import testbase from "test-base";
import * as PerformanceUtilities from "./performanceutilities";
import { setup, benchmarks } from "./Benchmarks";

const error = testbase.logging.error;
const log = testbase.logging.log;
const options = PerformanceUtilities.getCommandLineOptionsOrDefaults();
const timeout = 600000;

const testType = options.testType;

log(`Using domain ${options.domain}`);
error(`test runs: ${options.testRuns}`);
error(`Using testType: ${testType}`);
const provisioning = PerformanceUtilities.getProvisioning(false);
provisioning.ccAddress.host = options.cchost;
provisioning.ccAddress.port = options.ccport;

let echoProxy: EchoProxy;

joynr
    .load(provisioning)
    .then(() => {
        log("joynr started");
    })
    .then(() => {
        const messagingQos = new joynr.messaging.MessagingQos({
            ttl: timeout
        });
        return joynr.proxyBuilder.build(EchoProxy, {
            domain: options.domain,
            messagingQos
        });
    })
    .then(echoProxy1 => {
        log("build proxy");
        echoProxy = echoProxy1;

        setup(echoProxy1);
        process.send!({ msg: "initialized" });
    })
    .catch(e => {
        throw e;
    });

let count = 0;
let totalBroadcastToReceive = 0;
function countReceivedBroadCasts() {
    count++;
    if (count === totalBroadcastToReceive) {
        process.send!({ msg: "receivedBroadcasts" });
    }
}

let broadCastsPreparedOnce = false;

function prepareBroadcast(amount: number) {
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

let cpuUsage: NodeJS.CpuUsage;
let memoryIntervalId: NodeJS.Timer;
const measureMemory = options.measureMemory == "true";
let totalMemory = 0;
let totalMemoryMeasurements = 0;
let testData: Record<string, any>;

let executeBenchmark: (name: string) => void;

switch (testType) {
    case "burst":
        executeBenchmark = function(name) {
            const promiseArray = testData.map((item: any) => benchmarks[name].testProcedure(item));
            Promise.all(promiseArray).then(() => process.send!({ msg: "executeBenchmarkFinished" }));
        };
        break;
    case "single":
        executeBenchmark = function(name) {
            testData
                .reduce((accumulator: Promise<any>, item: any) => {
                    return accumulator.then(() => benchmarks[name].testProcedure(item));
                }, Promise.resolve())
                .then(() => process.send!({ msg: "executeBenchmarkFinished" }));
        };
        break;
    case "immediate": {
        executeBenchmark = function(name) {
            const dataLength = testData.length;
            let finishedTests = 0;

            const testFinished = function() {
                finishedTests++;
                if (finishedTests === dataLength) {
                    process.send!({ msg: "executeBenchmarkFinished" });
                }
            };

            const pushSome = function() {
                if (testData.length > 0) {
                    const item = testData.pop();
                    benchmarks[name].testProcedure(item).then(testFinished);
                    setImmediate(pushSome);
                }
            };
            pushSome();
        };
        break;
    }
    default:
        throw new Error("unknown testType");
}

const handler = function(msg: { msg: string; config: { numRuns: any; name: string }; name: any; amount: number }) {
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
            var diff: any = process.cpuUsage(cpuUsage);
            if (measureMemory) {
                diff.averageMemory = totalMemory / totalMemoryMeasurements;
                clearInterval(memoryIntervalId);
            }
            process.send!({ msg: "gotMeasurement", data: diff });
            break;
        case "prepareBenchmark":
            testData = [];
            var numRuns = msg.config.numRuns;
            for (let i = 0; i < numRuns; i++) {
                testData.push(benchmarks[msg.config.name].generateData(i));
            }
            process.send!({ msg: "prepareBenchmarkFinished" });
            break;
        case "executeBenchmark":
            executeBenchmark(msg.config.name);
            break;
        case "takeHeapSnapShot":
            var fileName = msg.name;
            heapdump.writeSnapshot(fileName, (_err: any, filename?: string) => {
                error(`dump written to: ${filename}`);
            });
            break;
        case "subscribeBroadcast":
            prepareBroadcast(msg.amount).then(() => process.send!({ msg: "subscriptionFinished" }));
            break;
        default:
            throw new Error("unknown messageType");
            break;
    }
};
process.on("message", handler);
