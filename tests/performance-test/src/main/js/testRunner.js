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

var testbase = require("test-base");
var PerformanceUtilities = require("./performanceutilities.js");
var Promise = require("bluebird").Promise;

var error = testbase.logging.error;
var log = testbase.logging.log;
var options = PerformanceUtilities.getCommandLineOptionsOrDefaults(process.env);
var measureMemory = options.measureMemory == "true";
var summary = [];
var benchmarks = null;
var Benchmarks = require("./Benchmarks");
var ProcessManager = require("./ProcessManager");

var testRunner = {
    displaySummary: function() {
        error("");
        error("Summary:");
        error("");
        summary.forEach(item => testRunner.logResults(item));
    },

    executeBenchmarks: function() {
        benchmarks = PerformanceUtilities.findBenchmarks(new Benchmarks(testRunner));
        return Promise.map(benchmarks, benchmark => testRunner.executeMultipleSubRuns(benchmark), {
            concurrency: 1
        }).then(testRunner.displaySummary);
    },

    executeSubRuns: function(benchmarkConfig, index) {
        var startTime;
        var numRuns = benchmarkConfig.numRuns;

        return ProcessManager.proxy
            .prepareBenchmark(benchmarkConfig)
            .then(() => {
                ProcessManager.provider.startMeasurement();
                ProcessManager.proxy.startMeasurement();
                startTime = Date.now();
                return ProcessManager.proxy.executeBenchmark(benchmarkConfig);
            })
            .then(function() {
                var elapsedTimeMs = Date.now() - startTime;
                log(
                    benchmarkConfig.name +
                        " " +
                        index +
                        " runs: " +
                        numRuns +
                        " took " +
                        elapsedTimeMs +
                        " ms. " +
                        numRuns / (elapsedTimeMs / 1000) +
                        " msgs/s"
                );
                let providerMeasurementPromise = ProcessManager.provider.stopMeasurement();
                let proxyMeasurementPromise = ProcessManager.proxy.stopMeasurement();
                return Promise.all([providerMeasurementPromise, proxyMeasurementPromise]).then(values => {
                    return { proxy: values[1], provider: values[0], time: elapsedTimeMs };
                });
            });
    },

    executeMultipleSubRuns: function(benchmarkConfig) {
        console.log("executeMultipleSubRuns");
        var numRuns = benchmarkConfig.numRuns;
        var testRuns = options.testRuns ? Number.parseInt(options.testRuns) : 1;
        var totalRuns = numRuns * testRuns;
        var totalLatency = 0;
        var testIndex = 0;
        var dummyArray = new Array(testRuns);
        var proxyUserTime = [];
        var proxySystemTime = [];
        var providerUserTime = [];
        var providerSystemTime = [];
        var proxyMemory = [];
        var providerMemory = [];
        var latency = [];

        var memInterval;
        var memSum = 0;
        var memTests = 0;

        return Promise.map(
            dummyArray,
            function() {
                testIndex++;
                return testRunner.executeSubRuns(benchmarkConfig, testIndex).then(function(result) {
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
                });
            },
            { concurrency: 1 }
        ).then(function() {
            totalLatency = latency.reduce((acc, curr) => acc + curr);
            var averageMsgPerSecond = totalRuns / (totalLatency / 1000);
            var variance = 0;
            var highestMsgPerSecond = -1;
            latency.map(time => numRuns / (time / 1000)).forEach(runMsgPerSecond => {
                variance += Math.pow(runMsgPerSecond - averageMsgPerSecond, 2);
                highestMsgPerSecond = Math.max(runMsgPerSecond, highestMsgPerSecond);
            });
            variance /= proxyUserTime.length;
            var deviation = Math.sqrt(variance).toFixed(2);
            highestMsgPerSecond = highestMsgPerSecond.toFixed(2);
            averageMsgPerSecond = averageMsgPerSecond.toFixed(2);

            var result = { time: {}, percentage: {} };

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
            result.time.totalTime = result.time.totalProviderTime + result.time.totalProxyTime;

            result.percentage.providerPercentage = result.time.totalProviderTime / totalLatency;
            result.percentage.proxyPercentage = result.time.totalProxyTime / totalLatency;

            result.memory = {};
            if (measureMemory) {
                result.memory.providerMemory = providerMemory.reduce((acc, curr) => acc + curr) / providerMemory.length;
                result.memory.proxyMemory = proxyMemory.reduce((acc, curr) => acc + curr) / proxyMemory.length;
            }

            testRunner.logResults(result);
            summary.push(result);
        });
    },
    logResults: function(result) {
        error("");
        error("Benchmark    : " + result.other.benchmarkName);
        error("total latency: " + result.other.totalLatency + " ms ");
        error("speed average: " + result.other.averageTime + " +/- " + result.other.deviation + " msgs/s: ");
        error("speed highest: " + result.other.highestMsgPerSecond + " msg/s");

        for (let key in result.time) {
            if (Object.prototype.hasOwnProperty.call(result.time, key)) {
                error(key + ": " + result.time[key].toFixed(0) + "ms");
            }
        }

        for (let key in result.percentage) {
            if (Object.prototype.hasOwnProperty.call(result.percentage, key)) {
                error(key + ": " + (result.percentage[key] * 100).toFixed(1) + "%");
            }
        }

        for (let key in result.memory) {
            if (Object.prototype.hasOwnProperty.call(result.memory, key)) {
                error(key + ": " + (result.memory[key] / 1048576.0).toFixed(2) + "MB");
            }
        }
    }
};
module.exports = testRunner;
