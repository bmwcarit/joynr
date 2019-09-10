/* domain: true, interfaceNameDatatypes: true, globalCapDirCapability: true, channelUrlDirCapability: true */

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

import * as ChildProcessUtils from "../ChildProcessUtils";

import joynr from "../../../../main/js/joynr";
import provisioning from "../../../resources/joynr/provisioning/provisioning_cc";
import DatatypesProvider from "../../../generated/joynr/datatypes/DatatypesProvider";
import TestEnd2EndDatatypesTestData from "../TestEnd2EndDatatypesTestData";
import InProcessRuntime = require("joynr/joynr/start/InProcessRuntime");

let providerDomain: string;

// attribute values for provider
let currentAttributeValue: any;
let datatypesProvider: DatatypesProvider;
let providerQos;

function getObjectType(obj: any): string {
    if (obj === null || obj === undefined) {
        throw new Error("cannot determine the type of an undefined object");
    }
    return obj.constructor.name;
}

function getter(): any {
    return currentAttributeValue;
}

function setter(value: any): void {
    currentAttributeValue = value;
}

async function initializeTest(provisioningSuffix: string, providedDomain: string): Promise<void> {
    providerDomain = providedDomain;
    // @ts-ignore
    provisioning.persistency = "localStorage";
    // @ts-ignore
    provisioning.channelId = `End2EndDatatypesTestParticipantId${provisioningSuffix}`;

    joynr.selectRuntime(InProcessRuntime);
    await joynr.load(provisioning as any);
    providerQos = new joynr.types.ProviderQos({
        customParameters: [],
        priority: Date.now(),
        scope: joynr.types.ProviderScope.GLOBAL,
        supportsOnChangeSubscriptions: true
    });

    // build the provider
    datatypesProvider = joynr.providerBuilder.build(DatatypesProvider, {});

    let i;
    // there are so many attributes for testing different datatypes => register them
    // all by cycling over their names in the attribute
    for (i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
        // @ts-ignore
        const attribute = datatypesProvider[TestEnd2EndDatatypesTestData[i].attribute];
        attribute.registerGetter(getter);
        attribute.registerSetter(setter);
    }

    // registering operation functions
    datatypesProvider.getJavascriptType.registerOperation((opArgs: { arg: any }) => {
        return {
            javascriptType: getObjectType(opArgs.arg)
        };
    });
    datatypesProvider.getArgumentBack.registerOperation((opArgs: { arg: any }) => {
        return {
            returnValue: opArgs.arg
        };
    });
    datatypesProvider.multipleArguments.registerOperation((opArgs: any) => {
        return {
            serialized: JSON.stringify(opArgs)
        };
    });

    // register provider at the given domain
    await joynr.registration.registerProvider(providerDomain, datatypesProvider, providerQos);
}

function startTest(): Promise<void> {
    return Promise.resolve();
}

function terminateTest(): Promise<void> {
    return joynr.registration.unregisterProvider(providerDomain, datatypesProvider);
}

ChildProcessUtils.registerHandlers(initializeTest, startTest, terminateTest);
