/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
import ChildProcessController = require("./ChildProcessController");

const options = PerformanceUtilities.getCommandLineOptionsOrDefaults();
const measureMemory = options.measureMemory == "true";

export const provider = new ChildProcessController("provider", undefined as any, undefined as any);
export const proxy = new ChildProcessController(
    "proxy",
    function(this: ChildProcessController, benchmarkConfig: any) {
        this.process.send({ msg: "prepareBenchmark", config: benchmarkConfig });
        this.prepareBenchmarkPromise = PerformanceUtilities.createPromise();
        return this.prepareBenchmarkPromise.promise;
    },
    function(this: ChildProcessController, benchmarkConfig: any) {
        this.process.send({ msg: "executeBenchmark", config: benchmarkConfig });
        this.executeBenchmarkPromise = PerformanceUtilities.createPromise();
        return this.executeBenchmarkPromise.promise;
    }
);

export function initializeChildProcesses() {
    const providerPromise = provider.initialize();
    const proxyPromise = proxy.initialize();

    const handler = () => {
        provider.shutdown();
        proxy.shutdown();
        process.exit(1);
    };
    process.on("SIGINT", handler);
    process.on("SIGTERM", handler);

    return Promise.all([providerPromise, proxyPromise]);
}

export function takeHeapSnapShot(name: string) {
    proxy.process.send({ msg: "takeHeapSnapShot", name: `./proxy${name}.heapsnapshot` });
    provider.process.send({ msg: "takeHeapSnapShot", name: `./provider${name}.heapsnapshot` });
}

let initializedBroadcasts = false;
let broadcastProxies: any[] = [];
let broadcastStarted: number;
export function initializeBroadcast(benchmarkConfig: { numProxies: any }) {
    if (initializedBroadcasts) {
        return Promise.resolve();
    }
    initializedBroadcasts = true;
    let count = benchmarkConfig.numProxies;
    broadcastProxies = [];
    const broadcastProxiesInitializedPromises = [];
    while (count--) {
        const broadcastProxy = new ChildProcessController("proxy", undefined as any, undefined as any);
        const broadCastProxyInitialized = broadcastProxy.initialize();
        broadcastProxies.push(broadcastProxy);
        broadcastProxiesInitializedPromises.push(broadCastProxyInitialized);
    }
    return Promise.all(broadcastProxiesInitializedPromises);
}

export async function prepareBroadcasts(benchmarkConfig: { numProxies: any }) {
    await initializeBroadcast(benchmarkConfig);
    return Promise.all(broadcastProxies.map(proxy => proxy.prepareForBroadcast(benchmarkConfig)));
}

export function _prepareBroadcastResults() {
    const timeMs = Date.now() - broadcastStarted;
    console.log(`broadcast took: ${timeMs}ms`);
    const providerFinished = provider.stopMeasurement();

    const broadcastProxiesFinished = broadcastProxies.map(proxy => proxy.stopMeasurement());
    const broadcastProxiesReduced = Promise.all(broadcastProxiesFinished).then(results => {
        let total;
        const numberOfProxies = broadcastProxiesFinished.length;
        if (measureMemory) {
            total = results.reduce(
                (acc, curr) => {
                    acc.user += curr.user;
                    acc.system += curr.system;
                    acc.averageMemory += curr.averageMemory;
                },
                { user: 0, system: 0, averageMemory: 0 }
            );
            total.averageMemory /= numberOfProxies;
        } else {
            total = results.reduce(
                (acc, curr) => {
                    acc.user += curr.user;
                    acc.system += curr.system;
                    return acc;
                },
                { user: 0, system: 0 }
            );
        }
        total.user /= numberOfProxies;
        total.system /= numberOfProxies;
        return total;
    });
    return Promise.all([providerFinished, broadcastProxiesReduced]).then(values => {
        return { proxy: values[1], provider: values[0], time: timeMs };
    });
}

export function executeBroadcasts(benchmarkConfig: { numRuns: any }) {
    provider.startMeasurement();
    let l = broadcastProxies.length;
    const promises = [];
    while (l--) {
        const broadcastProxy = broadcastProxies[l];
        broadcastProxy.startMeasurement();
        promises.push(broadcastProxy.broadcastsReceivedPromise.promise);
    }
    broadcastStarted = Date.now();
    provider.process.send({ msg: "fireBroadCast", amount: benchmarkConfig.numRuns });
    return Promise.all(promises).then(() => _prepareBroadcastResults());
}
