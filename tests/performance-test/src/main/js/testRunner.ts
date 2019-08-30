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

import testbase from "test-base";

import * as PerformanceUtilities from "./performanceutilities";
import * as ProcessManager from "./ProcessManager";
import child_process from "child_process";
import path from "path";

const error = testbase.logging.error;
const log = testbase.logging.log;
const options = PerformanceUtilities.getCommandLineOptionsOrDefaults();
const measureMemory = options.measureMemory == "true";
const summary: any[] = [];
let benchmarkConfig = null;
const getCpuScript = path.join(__dirname, "getCpuTime.sh");
const ccPId = PerformanceUtilities.getCcPiD();

const testRunner = {
    displaySummary() {
        error("");
        error("Summary:");
        error("");
        summary.forEach(item => testRunner.logResults(item));
    },

    async executeBenchmarks() {
        benchmarkConfig = PerformanceUtilities.findBenchmarks();
        for (let i = 0; i < benchmarkConfig.length; i++) {
            const benchmark = benchmarkConfig[i];
            await testRunner.executeSubRunsWithWarmUp(benchmark);
        }
        return testRunner.displaySummary();
    },

    async executeSubRuns(benchmarkConfig: { name?: any; numRuns: any; numProxies: any; type?: any }, index: number) {
        let startTime: number;
        const numRuns = benchmarkConfig.numRuns;

        if (benchmarkConfig.type === "broadcast") {
            await ProcessManager.prepareBroadcasts(benchmarkConfig);
            return ProcessManager.executeBroadcasts(benchmarkConfig);
        }

        return ProcessManager.proxy
            .prepareBenchmark(benchmarkConfig)
            .then(() => {
                ProcessManager.provider.startMeasurement();
                ProcessManager.proxy.startMeasurement();
                startTime = Date.now();
                return ProcessManager.proxy.executeBenchmark(benchmarkConfig);
            })
            .then(() => {
                const elapsedTimeMs = Date.now() - startTime;
                log(
                    `${benchmarkConfig.name} ${index} runs: ${numRuns} took ${elapsedTimeMs} ms. ${numRuns /
                        (elapsedTimeMs / 1000)} msgs/s`
                );
                const providerMeasurementPromise = ProcessManager.provider.stopMeasurement();
                const proxyMeasurementPromise = ProcessManager.proxy.stopMeasurement();

                return Promise.all([providerMeasurementPromise, proxyMeasurementPromise]).then(values => {
                    return { proxy: values[1], provider: values[0], time: elapsedTimeMs };
                });
            });
    },

    executeSubRunsWithWarmUp(benchmarkConfig: { name: string; numRuns: number; numProxies: number; type: any }) {
        error(`warming up: ${benchmarkConfig.name}`);
        if (options.heapSnapShot == "true") {
            setTimeout(() => {
                ProcessManager.takeHeapSnapShot(`${Date.now()}start${benchmarkConfig.name}`);
            }, 500);
        }
        return testRunner
            .executeSubRuns(benchmarkConfig, -1)
            .then(() => testRunner.executeMultipleSubRuns(benchmarkConfig));
    },

    async executeMultipleSubRuns(benchmarkConfig: { name: string; numRuns: number; numProxies: number }) {
        const numRuns = benchmarkConfig.numRuns;
        const testRuns = options.testRuns ? Number.parseInt(options.testRuns) : 1;
        const totalRuns = numRuns * testRuns;
        let totalLatency = 0;
        const proxyUserTime: number[] = [];
        const proxySystemTime: number[] = [];
        const providerUserTime: number[] = [];
        const providerSystemTime: number[] = [];
        const proxyMemory: number[] = [];
        const providerMemory: number[] = [];
        const latency: number[] = [];

        const startCcCpu = Number(child_process.execFileSync(getCpuScript, [ccPId]).toString());
        console.log(`startCpu: ${startCcCpu}`);

        for (let i = 0; i <= testRuns; i++) {
            const result = await testRunner.executeSubRuns(benchmarkConfig, i);
            totalLatency += result.time;
            providerUserTime.push(result.provider.user);
            providerSystemTime.push(result.provider.system);
            proxyUserTime.push(result.proxy.user);
            proxySystemTime.push(result.proxy.system);
            latency.push(result.time);
            if (measureMemory) {
                providerMemory.push(result.provider.averageMemory);
                proxyMemory.push(result.proxy.averageMemory);
            }
        }

        const stopCCCpu = Number(child_process.execFileSync(getCpuScript, [ccPId]).toString());

        error("summary started");
        totalLatency = latency.reduce((acc, curr) => acc + curr);
        let averageMsgPerSecond = totalRuns / (totalLatency / 1000);
        let variance = 0;
        let highestMsgPerSecond = -1;
        latency.map(time => numRuns / (time / 1000)).forEach(runMsgPerSecond => {
            variance += Math.pow(runMsgPerSecond - averageMsgPerSecond, 2);
            highestMsgPerSecond = Math.max(runMsgPerSecond, highestMsgPerSecond);
        });
        variance /= proxyUserTime.length;
        const deviation = Math.sqrt(variance).toFixed(2);
        highestMsgPerSecond = Number(highestMsgPerSecond.toFixed(2));
        averageMsgPerSecond = Number(averageMsgPerSecond.toFixed(2));

        const result: any = { time: {}, percentage: {} };

        result.other = {
            averageTime: averageMsgPerSecond,
            deviation,
            highestMsgPerSecond,
            benchmarkName: benchmarkConfig.name,
            totalLatency
        };
        // cpu usage is in micro seconds -> divide by 1000
        result.time.totalProviderUserTime = providerUserTime.reduce((acc, curr) => acc + curr) / 1000.0;
        result.time.totalProviderSystemTime = providerSystemTime.reduce((acc, curr) => acc + curr) / 1000.0;
        result.time.totalProxyUserTime = proxyUserTime.reduce((acc, curr) => acc + curr) / 1000.0;
        result.time.totalProxySystemTime = proxySystemTime.reduce((acc, curr) => acc + curr) / 1000.0;
        result.time.totalProviderTime = result.time.totalProviderUserTime + result.time.totalProviderSystemTime;
        result.time.totalProxyTime = result.time.totalProxyUserTime + result.time.totalProxySystemTime;
        result.time.totalCCTime = (stopCCCpu - startCcCpu) * 10; // cctime is in number of ticks which is 10 ms long each
        result.time.totalTime = result.time.totalProviderTime + result.time.totalProxyTime;

        result.percentage.providerPercentage = result.time.totalProviderTime / totalLatency;
        result.percentage.proxyPercentage = result.time.totalProxyTime / totalLatency;
        result.percentage.totalCCPercentage = result.time.totalCCTime / totalLatency;

        result.memory = {};
        if (measureMemory) {
            result.memory.providerMemory = providerMemory.reduce((acc, curr) => acc + curr) / providerMemory.length;
            result.memory.proxyMemory = proxyMemory.reduce((acc, curr) => acc + curr) / proxyMemory.length;
        }

        testRunner.logResults(result);
        summary.push(result);
    },

    logResults(result: { time: any; percentage: any; other?: any; memory?: any }) {
        error("");
        error(`Benchmark    : ${result.other.benchmarkName}`);
        error(`total latency: ${result.other.totalLatency} ms `);
        error(`speed average: ${result.other.averageTime} +/- ${result.other.deviation} msgs/s: `);
        error(`speed highest: ${result.other.highestMsgPerSecond} msg/s`);

        for (const key in result.time) {
            if (Object.prototype.hasOwnProperty.call(result.time, key)) {
                error(`${key}: ${result.time[key].toFixed(0)}ms`);
            }
        }

        for (const key in result.percentage) {
            if (Object.prototype.hasOwnProperty.call(result.percentage, key)) {
                error(`${key}: ${(result.percentage[key] * 100).toFixed(1)}% of one cpu`);
            }
        }

        for (const key in result.memory) {
            if (Object.prototype.hasOwnProperty.call(result.memory, key)) {
                error(`${key}: ${(result.memory[key] / 1048576.0).toFixed(2)}MB`);
            }
        }
    }
};

export = testRunner;
