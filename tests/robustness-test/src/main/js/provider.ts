/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import { log } from "./logging";
import joynr from "joynr";
import provisioning from "./provisioning_common";
import TestInterfaceProvider = require("../generated-javascript/joynr/tests/robustness/TestInterfaceProvider");
import { TestInterfaceProviderImplementation } from "../generated-javascript/joynr/tests/robustness/TestInterfaceProvider";
import * as RobustnessTestInterfaceProvider from "./RobustnessProvider";

if (process.argv.length !== 3) {
    log("please pass a domain as argument");
    process.exit(0);
}
const domain = process.argv[2];
log(`domain: ${domain}`);

joynr
    .load(provisioning)
    .then(() => {
        log("joynr started");

        const providerQos = new joynr.types.ProviderQos({
            customParameters: [],
            priority: Date.now(),
            scope: joynr.types.ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        });

        const testInterfaceProvider = joynr.providerBuilder.build<
            typeof TestInterfaceProvider,
            TestInterfaceProviderImplementation
        >(TestInterfaceProvider, RobustnessTestInterfaceProvider.implementation);

        return joynr.registration.registerProvider(domain, testInterfaceProvider, providerQos);
    })
    .then(() => {
        log("provider registered successfully");
    })
    .catch((error: any) => {
        log(`error registering provider: ${error.toString()}`);
    });
