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

import joynr from "joynr";

import testbase from "test-base";
const log = testbase.logging.log;
import TestInterfaceProvider from "../generated-javascript/joynr/interlanguagetest/TestInterfaceProvider";
import IltTestInterfaceProvider from "./IltProvider";
import IltStringBroadcastFilter from "./IltStringBroadcastFilter";
import WebSocketLibjoynrRuntime from "joynr/joynr/start/WebSocketLibjoynrRuntime";
import UdsLibJoynrRuntime from "joynr/joynr/start/UdsLibJoynrRuntime";

const provisioning = testbase.provisioning_common;

provisioning.logging.configuration = {
    appenders: {
        appender: [
            {
                type: "Console",
                name: "STDOUT",
                PatternLayout: {
                    pattern: "[%d{HH:mm:ss,SSS}][%c][%p] %m{2}"
                }
            }
        ]
    },
    loggers: {
        root: {
            level: "debug",
            AppenderRef: [
                {
                    ref: "STDOUT"
                }
            ]
        }
    }
};

if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(0);
}

// eslint-disable-next-line @typescript-eslint/no-non-null-assertion
const domain = process.env.domain!;
log(`domain: ${domain}`);

// eslint-disable-next-line @typescript-eslint/no-non-null-assertion
const runtime = process.env.runtime!;
log(`runtime: ${runtime}`);

if (runtime !== undefined) {
    if (runtime === "websocket") {
        // provisioning data are defined in test-base
        joynr.selectRuntime(WebSocketLibjoynrRuntime);
    } else if (runtime === "uds") {
        if (!process.env.udspath || !process.env.udsclientid || !process.env.udsconnectsleeptimems) {
            log("please pass udspath, udsclientid, udsconnectsleeptimems as argument");
            process.exit(1);
        }
        provisioning.uds = {
            socketPath: process.env.udspath,
            clientId: process.env.udsclientid,
            connectSleepTimeMs: Number(process.env.udsconnectsleeptimems)
        };
        // no selectRuntime: UdsLibJoynrRuntime is default
        joynr.selectRuntime(UdsLibJoynrRuntime);
    }
}

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
            TestInterfaceProvider.TestInterfaceProviderImplementation
        >(TestInterfaceProvider, IltTestInterfaceProvider);

        testInterfaceProvider.broadcastWithFiltering.addBroadcastFilter(new IltStringBroadcastFilter());

        return joynr.registration.registerProvider(domain, testInterfaceProvider, providerQos);
    })
    .then(() => {
        log("provider registered successfully");
    })
    .catch((error: any) => {
        log(`error registering provider: ${error.toString()}`);
    });
