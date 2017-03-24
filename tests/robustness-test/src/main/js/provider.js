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

var log = require("./logging.js").log;

if (process.argv.length !== 3) {
    log("please pass a domain as argument");
    process.exit(0);
}
var domain = process.argv[2];
log("domain: " + domain);

var joynr = require("joynr");
var provisioning = require("./provisioning_common.js");
joynr.load(provisioning).then(function(loadedJoynr) {
    log("joynr started");
    joynr = loadedJoynr;

    var providerQos = new joynr.types.ProviderQos({
        customParameters : [],
        priority : Date.now(),
        scope : joynr.types.ProviderScope.GLOBAL,
        supportsOnChangeSubscriptions : true
    });

    var TestInterfaceProvider = require("../generated-javascript/joynr/tests/robustness/TestInterfaceProvider.js");
    var RobustnessTestInterfaceProvider = require("./RobustnessProvider.js");
    var testInterfaceProvider = joynr.providerBuilder.build(
            TestInterfaceProvider,
            RobustnessTestInterfaceProvider.implementation);
    RobustnessTestInterfaceProvider.setProvider(testInterfaceProvider);

    joynr.registration.registerProvider(domain, testInterfaceProvider, providerQos)
    .then(function() {
        log("provider registered successfully");
        return null;
    }).catch(function(error) {
        log("error registering provider: " + error.toString());
    });
    return loadedJoynr;
}).catch(function(error) {
    throw error;
});
