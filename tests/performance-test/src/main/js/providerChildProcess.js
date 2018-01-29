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

var PerformanceUtilities = require("./performanceutilities");

var options = PerformanceUtilities.getCommandLineOptionsOrDefaults(process.env);
PerformanceUtilities.overrideRequire();

var domain = options.domain;

var joynr = require("joynr");
joynr.selectRuntime("websocket.libjoynr");

var testbase = require("test-base");
var provisioning = testbase.provisioning_common;
var log = testbase.logging.log;
log("domain: " + domain);

provisioning.logging.configuration.loggers.root.level = "error";
joynr
    .load(provisioning)
    .then(function(loadedJoynr) {
        log("joynr started");
        joynr = loadedJoynr;

        var providerQos = new joynr.types.ProviderQos({
            customParameters: [],
            priority: Date.now(),
            scope: joynr.types.ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        });

        var EchoProvider = require("../generated-javascript/joynr/tests/performance/EchoProvider.js");
        var EchoProviderImpl = require("./EchoProviderImpl.js");
        var echoProvider = joynr.providerBuilder.build(EchoProvider, EchoProviderImpl.implementation);

        joynr.registration
            .registerProvider(domain, echoProvider, providerQos)
            .then(function() {
                log("provider registered successfully");
                process.send({
                    msg: "initialized"
                });
            })
            .catch(function(error) {
                log("error registering provider: " + error.toString());
            });

        return loadedJoynr;
    })
    .catch(function(error) {
        throw error;
    });

var cpuUsage;
var memoryIntervalId;
var measureMemory = options.measureMemory == "true";
var totalMemory = 0;
var totalMemoryMeasurements = 0;
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
    }
};
process.on("message", handler);
