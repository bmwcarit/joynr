/*jslint node: true */

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

var child_process = require("child_process");
var exitHook = require("exit-hook");
var PerformanceUtilities = require("./performanceutilities");

var options = PerformanceUtilities.getCommandLineOptionsOrDefaults();
var measureMemory = options.measureMemory == "true";

var ProcessManager = {};

function ChildProcessStuff(type) {
    this.file = type === "provider" ? "providerChildProcess.js" : "proxyChildProcess.js";
}
ChildProcessStuff.prototype.initialize = function() {
    const port = PerformanceUtilities.getRandomInt(1000, 9000);
    const config = process.env.debug == "true" ? { execArgv: ["--inspect-brk=" + port] } : {};
    config.env = Object.create(process.env);

    this.process = child_process.fork("src/main/js/" + this.file, [], config);
    this.ready = PerformanceUtilities.createPromise();
    var that = this;

    this.process.on("message", function(msg) {
        switch (msg.msg) {
            case "initialized":
                that.ready.resolve();
                break;
            case "gotMeasurement":
                that.measurementPromise.resolve(msg.data);
                break;
            case "prepareBenchmarkFinished":
                that.prepareBenchmarkPromise.resolve();
                break;
            case "executeBenchmarkFinished":
                that.executeBenchmarkPromise.resolve();
                break;
            case "subscriptionFinished":
                that.subsciptionFinishedPromise.resolve();
                break;
            case "receivedBroadcasts":
                that.broadcastsReceivedPromise.resolve();
                break;
            default:
                throw new Error("unknown MessageType" + JSON.stringify(msg));
                break;
        }
    });

    return this.ready.promise;
};

ChildProcessStuff.prototype.shutdown = function() {
    this.process.send({ msg: "terminate" });
};

ChildProcessStuff.prototype.startMeasurement = function() {
    this.process.send({ msg: "startMeasurement" });
};

ChildProcessStuff.prototype.stopMeasurement = function() {
    this.process.send({ msg: "stopMeasurement" });
    this.measurementPromise = PerformanceUtilities.createPromise();
    return this.measurementPromise.promise;
};

ChildProcessStuff.prototype.prepareForBroadcast = function(benchmarkConfig) {
    this.process.send({ msg: "subscribeBroadcast", amount: benchmarkConfig.numRuns });
    this.subsciptionFinishedPromise = PerformanceUtilities.createPromise();
    this.broadcastsReceivedPromise = PerformanceUtilities.createPromise();
    return this.subsciptionFinishedPromise.promise;
};

ProcessManager.provider = new ChildProcessStuff("provider");
ProcessManager.proxy = new ChildProcessStuff("proxy");

ProcessManager.proxy.prepareBenchmark = function(benchmarkConfig) {
    this.process.send({ msg: "prepareBenchmark", config: benchmarkConfig });
    this.prepareBenchmarkPromise = PerformanceUtilities.createPromise();
    return this.prepareBenchmarkPromise.promise;
};

ProcessManager.proxy.executeBenchmark = function(benchmarkConfig) {
    this.process.send({ msg: "executeBenchmark", config: benchmarkConfig });
    this.executeBenchmarkPromise = PerformanceUtilities.createPromise();
    return this.executeBenchmarkPromise.promise;
};

ProcessManager.initializeChildProcesses = function() {
    let providerPromise = ProcessManager.provider.initialize();
    let proxyPromise = ProcessManager.proxy.initialize();

    exitHook(function() {
        ProcessManager.provider.shutdown();
        ProcessManager.proxy.shutdown();
    });
    return Promise.all([providerPromise, proxyPromise]);
};

ProcessManager.takeHeapSnapShot = function(name) {
    this.proxy.process.send({ msg: "takeHeapSnapShot", name: "./proxy" + name + ".heapsnapshot" });
    this.provider.process.send({ msg: "takeHeapSnapShot", name: "./provider" + name + ".heapsnapshot" });
};

var initializedBroadcasts = false;

ProcessManager.initializeBroadcast = function(benchmarkConfig) {
    if (initializedBroadcasts) {
        return Promise.resolve();
    }
    initializedBroadcasts = true;
    let count = benchmarkConfig.numProxies;
    this.broadcastProxies = [];
    let broadcastProxiesInitializedPromises = [];
    while (count--) {
        let broadcastProxy = new ChildProcessStuff("proxy");
        let broadCastProxyInitialized = broadcastProxy.initialize();
        this.broadcastProxies.push(broadcastProxy);
        broadcastProxiesInitializedPromises.push(broadCastProxyInitialized);
    }
    return Promise.all(broadcastProxiesInitializedPromises);
};

ProcessManager.prepareBroadcasts = function(benchmarkConfig) {
    return ProcessManager.initializeBroadcast(benchmarkConfig).then(() => {
        return Promise.all(this.broadcastProxies.map(proxy => proxy.prepareForBroadcast(benchmarkConfig)));
    });
};

ProcessManager._prepareBroadcastResults = function() {
    let timeMs = Date.now() - this.broadcastStarted;
    console.log("broadcast took: " + timeMs + "ms");
    let providerFinished = this.provider.stopMeasurement();

    let broadcastProxiesFinished = this.broadcastProxies.map(proxy => proxy.stopMeasurement());
    let broadcastProxiesReduced = Promise.all(broadcastProxiesFinished).then(results => {
        let total;
        let numberOfProxies = broadcastProxiesFinished.length;
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
};

ProcessManager.executeBroadcasts = function(benchmarkConfig) {
    this.provider.startMeasurement();
    let l = this.broadcastProxies.length;
    let promises = [];
    while (l--) {
        let broadcastProxy = this.broadcastProxies[l];
        broadcastProxy.startMeasurement();
        promises.push(broadcastProxy.broadcastsReceivedPromise.promise);
    }
    this.broadcastStarted = Date.now();
    this.provider.process.send({ msg: "fireBroadCast", amount: benchmarkConfig.numRuns });
    return Promise.all(promises).then(() => ProcessManager._prepareBroadcastResults());
};

module.exports = ProcessManager;
