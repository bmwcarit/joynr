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

var ProcessManager = {};

function ChildProcessStuff(type) {
    this.file = type === "provider" ? "providerChildProcess.js" : "proxyChildProcess.js";
}
ChildProcessStuff.prototype.initialize = function() {
    const port = PerformanceUtilities.getRandomInt(1000, 9000);
    const config = process.env.debug == "true" ? { execArgv: ["--inspect-brk=" + port] } : {};

    this.process = child_process.fork("src/main/js/" + this.file, [], config);
    this.ready = PerformanceUtilities.createPromise();
    var that = this;

    this.process.on("message", function(msg) {
        if (msg.msg === "initialized") {
            that.ready.resolve();
        } else if (msg.msg === "gotMeasurement") {
            that.measurementPromise.resolve(msg.data);
        } else if (msg.msg === "prepareBenchmarkFinished") {
            that.prepareBenchmarkPromise.resolve();
        } else if (msg.msg === "executeBenchmarkFinished") {
            that.executeBenchmarkPromise.resolve();
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

module.exports = ProcessManager;
