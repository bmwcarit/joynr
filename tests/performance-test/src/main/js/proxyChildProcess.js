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
var Promise = require("bluebird").Promise;

var error = testbase.logging.error;
var log = testbase.logging.log;
var options = PerformanceUtilities.getCommandLineOptionsOrDefaults(process.env);
var timeout = 600000;
var Benchmarks = require("./Benchmarks");
var benchmarks;

log("Using domain " + options.domain);
error("Performing " + options.numRuns + " runs");
error("test runs: " + options.testRuns);
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

var numRuns = Number.parseInt(options.numRuns);
var cpuUsage;
var testData;
var handler = function(msg) {
    if (msg.msg === "terminate") {
        joynr.shutdown();
    } else if (msg.msg === "startMeasurement") {
        cpuUsage = process.cpuUsage();
    } else if (msg.msg === "stopMeasurement") {
        var diff = process.cpuUsage(cpuUsage);
        process.send({ msg: "gotMeasurement", data: diff });
    } else if (msg.msg === "prepareBenchmark") {
        testData = [];
        for (var i = 0; i < numRuns; i++) {
            testData.push(benchmarks[msg.name].generateData(i));
        }
        process.send({ msg: "prepareBenchmarkFinished" });
    } else if (msg.msg === "executeBenchmark") {
        // TODO: add different types of tests -> one request after another, all parallel, etc.

        // burstTest
        var promiseArray = testData.map(item => benchmarks[msg.name].testProcedure(item));
        Promise.all(promiseArray).then(() => process.send({ msg: "executeBenchmarkFinished" }));
    }
};
process.on("message", handler);
