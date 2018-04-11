/*jslint es5: true*/

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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

var ChildProcessUtils = require("./ChildProcessUtils");
ChildProcessUtils.overrideRequirePaths();

var joynr = require("joynr"),
    MultipleVersionsInterfaceProvider = require("joynr/tests/v1/MultipleVersionsInterfaceProvider"),
    Promise = require("../../classes/global/Promise"),
    provisioning = require("../joynr/provisioning/provisioning_cc.js");

var loadedJoynr, providerDomain, multipleVersionsInterfaceProvider;

var providerImplementation = {
    getTrue: function() {
        return true;
    }
};

function initializeTest(provisioningSuffix, providedDomain) {
    providerDomain = providedDomain;

    joynr.selectRuntime("inprocess");
    return joynr.load(provisioning).then(function(newJoynr) {
        loadedJoynr = newJoynr;
        var providerQos = new joynr.types.ProviderQos({
            customParameters: [],
            providerpriority: 5,
            scope: joynr.types.ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: false
        });

        multipleVersionsInterfaceProvider = joynr.providerBuilder.build(
            MultipleVersionsInterfaceProvider,
            providerImplementation
        );

        return loadedJoynr.registration
            .registerProvider(providerDomain, multipleVersionsInterfaceProvider, providerQos)
            .then(function() {
                return loadedJoynr;
            });
    });
}

function startTest() {
    // nothing to do here, everything is already performed in initialize
    return Promise.resolve();
}

function terminateTest() {
    return loadedJoynr.registration
        .unregisterProvider(providerDomain, multipleVersionsInterfaceProvider)
        .then(function() {
            loadedJoynr.shutdown();
        });
}

ChildProcessUtils.registerHandlers(initializeTest, startTest, terminateTest);
