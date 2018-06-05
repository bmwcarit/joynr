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

const ChildProcessUtils = require("./ChildProcessUtils");
ChildProcessUtils.overrideRequirePaths();

const joynr = require("joynr");
const Promise = require("../../../main/js/global/Promise");
const provisioning = require("../../resources/joynr/provisioning/provisioning_cc.js");
const MultipleVersionsInterfaceProviderNameVersion = require("../../generated/joynr/tests/MultipleVersionsInterface2Provider");
const MultipleVersionsInterfaceProviderPackageVersion1 = require("../../generated/joynr/tests/v1/MultipleVersionsInterfaceProvider");
const MultipleVersionsInterfaceProviderPackageVersion2 = require("../../generated/joynr/tests/v2/MultipleVersionsInterfaceProvider");
const providerImplementation = require("./MultipleVersionsInterfaceProviderImplementation");

let loadedJoynr, providerDomain, multipleVersionsInterfaceProvider, MultipleVersionsInterfaceProvider;

function initializeTest(provisioningSuffix, providedDomain, versioning) {
    providerDomain = providedDomain;

    joynr.selectRuntime("inprocess");
    return joynr.load(provisioning).then(newJoynr => {
        loadedJoynr = newJoynr;
        const providerQos = new joynr.types.ProviderQos({
            customParameters: [],
            priority: 5,
            scope: joynr.types.ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: false
        });

        switch (versioning) {
            case "nameVersion2":
                MultipleVersionsInterfaceProvider = MultipleVersionsInterfaceProviderNameVersion;
                break;
            case "packageVersion1":
                MultipleVersionsInterfaceProvider = MultipleVersionsInterfaceProviderPackageVersion1;
                break;
            case "packageVersion2":
                MultipleVersionsInterfaceProvider = MultipleVersionsInterfaceProviderPackageVersion2;
                break;
            default:
                throw new Error("Please specify the versioning type used for provider generation!");
        }

        multipleVersionsInterfaceProvider = joynr.providerBuilder.build(
            MultipleVersionsInterfaceProvider,
            providerImplementation
        );

        return loadedJoynr.registration
            .registerProvider(providerDomain, multipleVersionsInterfaceProvider, providerQos)
            .then(() => {
                return loadedJoynr;
            });
    });
}

function startTest() {
    // nothing to do here, everything is already performed in initialize
    return Promise.resolve();
}

function terminateTest() {
    return loadedJoynr.registration.unregisterProvider(providerDomain, multipleVersionsInterfaceProvider).then(() => {
        loadedJoynr.shutdown();
    });
}

ChildProcessUtils.registerHandlers(initializeTest, startTest, terminateTest);
