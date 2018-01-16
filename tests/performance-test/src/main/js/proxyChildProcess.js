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

var joynr = require("joynr");
var testbase = require("test-base");
var PerformanceUtilities = require("./performanceutilities.js");
PerformanceUtilities.overrideRequire();

var error = testbase.logging.error;
var log = testbase.logging.log;
var options = PerformanceUtilities.getCommandLineOptionsOrDefaults(process.env);
var timeout = 600000;
var Benchmarks = require("./Benchmarks");
var benchmarks;

var testType = options.testType;

log("Using domain " + options.domain);
error("test runs: " + options.testRuns);
error("Using testType: " + testType);
var provisioning = testbase.provisioning_common;
provisioning.ccAddress.host = options.cchost;
provisioning.ccAddress.port = options.ccport;
joynr.selectRuntime("websocket.libjoynr");

provisioning.logging.configuration.loggers.root.level = "error";

var echoProxy;

joynr
    .load(provisioning)
    .then(function(loadedJoynr) {
        joynr = loadedJoynr;
        log("joynr started");
        return loadedJoynr;
    })
    .then(function(loadedJoynr) {
        var messagingQos = new loadedJoynr.messaging.MessagingQos({
            ttl: timeout
        });
        var EchoProxy = require("../generated-javascript/joynr/tests/performance/EchoProxy.js");
        return loadedJoynr.proxyBuilder.build(EchoProxy, {
            domain: options.domain,
            messagingQos: messagingQos
        });
    })
    .then(function(echoProxy1) {
        log("build proxy");
        echoProxy = echoProxy1;
        benchmarks = new Benchmarks(echoProxy, joynr);
        process.send({ msg: "initialized" });
    })
    .catch(function(e) {
        throw e;
    });

var cpuUsage;
var memoryIntervalId;
var measureMemory = options.measureMemory == "true";
var totalMemory = 0;
var totalMemoryMeasurements = 0;
var testData;
var handler = function(msg) {
    if (msg.msg === "terminate") {
        joynr.shutdown();
    } else if (msg.msg === "startMeasurement") {
        if (measureMemory) {
            memoryIntervalId = setInterval(function() {
                var memoryUsage = process.memoryUsage();
                totalMemory += memoryUsage.rss;
                totalMemoryMeasurements++;
            }, 1000);
        }
        cpuUsage = process.cpuUsage();
    } else if (msg.msg === "stopMeasurement") {
        var diff = process.cpuUsage(cpuUsage);
        if (measureMemory) {
            diff.averageMemory = totalMemory / totalMemoryMeasurements;
            clearInterval(memoryIntervalId);
        }
        process.send({ msg: "gotMeasurement", data: diff });
    } else if (msg.msg === "prepareBenchmark") {
        testData = [];
        var numRuns = msg.config.numRuns;
        for (var i = 0; i < numRuns; i++) {
            testData.push(benchmarks[msg.config.name].generateData(i));
        }
        process.send({ msg: "prepareBenchmarkFinished" });
    } else if (msg.msg === "executeBenchmark") {
        // TODO: add different types of tests -> one request after another, all parallel, etc.

        if (testType === "burst") {
            var promiseArray = testData.map(item => benchmarks[msg.config.name].testProcedure(item));
            Promise.all(promiseArray).then(() => process.send({ msg: "executeBenchmarkFinished" }));
        } else if (testType === "single") {
            testData
                .reduce((accumulator, item) => {
                    return accumulator.then(() => benchmarks[msg.config.name].testProcedure(item));
                }, Promise.resolve())
                .then(() => process.send({ msg: "executeBenchmarkFinished" }));
        } else {
            throw new Error("unknown testType");
        }
    }
};
process.on("message", handler);
