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


if (process.argv.length !== 3) {
    log("please pass a domain as argument");
    process.exit(0);
}
var domain = process.argv[2];

var joynr = require("joynr");
var testbase = require("test-base");
var log = testbase.logging.log;

//disable console log
console.log = function() {};
log("domain: " + domain);

testbase.provisioning_common.logging.configuration.loggers.root.level = "error";
joynr.load(testbase.provisioning_common).then(function(loadedJoynr) {
    log("joynr started");
    joynr = loadedJoynr;

    var providerQos = new joynr.types.ProviderQos({
        customParameters : [],
        priority : Date.now(),
        scope : joynr.types.ProviderScope.LOCAL,
        supportsOnChangeSubscriptions : true
    });

    var EchoProvider = require("../generated-javascript/joynr/tests/performance/EchoProvider.js");
    var EchoProviderImpl = require("./EchoProviderImpl.js");
    var echoProvider = joynr.providerBuilder.build(
            EchoProvider,
            EchoProviderImpl.implementation);

    joynr.registration.registerProvider(domain, echoProvider, providerQos).then(function() {
        log("provider registered successfully");
    }).catch(function(error) {
        log("error registering provider: " + error.toString());
    });
    return loadedJoynr;
}).catch(function(error) {
    throw error;
});
