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

import * as ChildProcessUtils from "../ChildProcessUtils";

import joynr from "joynr";
import provisioning from "../../../resources/joynr/provisioning/provisioning_cc";
import MultipleVersionsInterfaceProviderPackageVersion1 from "../../../generated/joynr/tests/v1/MultipleVersionsInterfaceProvider";
import MultipleVersionsInterfaceProviderPackageVersion2 from "../../../generated/joynr/tests/v2/MultipleVersionsInterfaceProvider";
import ProviderImplementation from "./MultipleVersionsInterfaceProviderImplementation";
import InProcessRuntime = require("../../../../main/js/joynr/start/InProcessRuntime");

let multipleVersionsInterfaceProvider: any, MultipleVersionsInterfaceProvider, providerDomain: string;

async function initializeTest(_provisioningSuffix: any, providedDomain: string, settings: any): Promise<void> {
    providerDomain = providedDomain;

    joynr.selectRuntime(InProcessRuntime);
    await joynr.load(provisioning as any);
    const providerQos = new joynr.types.ProviderQos({
        customParameters: [],
        priority: 5,
        scope: joynr.types.ProviderScope.GLOBAL,
        supportsOnChangeSubscriptions: false
    });

    switch (settings.versioning) {
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
        new ProviderImplementation()
    );

    await joynr.registration
        .registerProvider(providerDomain, multipleVersionsInterfaceProvider, providerQos)
        .catch((e: any) => {
            return joynr.shutdown().then(() => {
                throw e;
            });
        });
}

function terminateTest(): Promise<void> {
    return joynr.registration.unregisterProvider(providerDomain, multipleVersionsInterfaceProvider);
}

ChildProcessUtils.registerHandlers(initializeTest, () => Promise.resolve(), terminateTest);
