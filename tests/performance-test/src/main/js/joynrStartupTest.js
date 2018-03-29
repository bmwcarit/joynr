/*jslint node: true */

/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

var options = PerformanceUtilities.getCommandLineOptionsOrDefaults();
PerformanceUtilities.overrideRequire();

var domain = options.domain;
var testbase = require("test-base");
var provisioning = testbase.provisioning_common;

function logMemory() {
    let memory = process.memoryUsage();
    let format = str => {
        if (str.len < 6) {
            return str;
        }
        let ar = Array.from(`${str}`);
        ar.splice(ar.length - 6, 0, "'");
        return ar.join("");
    };
    console.log(
        new Date().toISOString(),
        Object.entries(memory)
            .map(([val, key]) => `${val} = ${format(key)}`)
            .join(", ")
    );
}

console.log("memory consumption before requiring jonyr");
logMemory();

var EchoProvider = require("../generated-javascript/joynr/tests/performance/EchoProvider.js");
var EchoProviderImpl = require("./EchoProviderImpl.js");

var joynr = require("joynr");
joynr.selectRuntime("websocket.libjoynr");

console.log("memory consumption after requiring jonyr and selecting runtime");
logMemory();

provisioning.logging.configuration.loggers.root.level = "error";
joynr
    .load(provisioning)
    .then(function(loadedJoynr) {
        console.log("memory consumption after loaded joynr and selecting runtime");
        logMemory();
        joynr = loadedJoynr;

        var providerQos = new joynr.types.ProviderQos({
            customParameters: [],
            priority: Date.now(),
            scope: joynr.types.ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        });

        var echoProvider = joynr.providerBuilder.build(EchoProvider, EchoProviderImpl.implementation);

        joynr.registration
            .registerProvider(domain, echoProvider, providerQos)
            .then(function() {
                console.log("memory consumption after registering provider");
                logMemory();
            })
            .catch(function(error) {
                console.log("error registering provider: " + error.toString());
            });

        return loadedJoynr;
    })
    .catch(function(error) {
        throw error;
    });
