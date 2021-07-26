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

import joynr = require("joynr");
import WebSocketLibjoynrRuntime = require("joynr/joynr/start/WebSocketLibjoynrRuntime");

import testbase from "test-base";
import SystemIntegrationTestProvider from "../generated-sources/joynr/test/SystemIntegrationTestProvider";
import { implementation } from "./SystemIntegrationTestProviderImpl";
import fs from "fs";
const log = testbase.logging.log;
const provisioning = testbase.provisioning_common;

if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(0);
}

if (process.env.cchost === undefined) {
    log("please pass cchost as argument");
    process.exit(0);
}

if (process.env.ccport === undefined) {
    log("please pass ccport as argument");
    process.exit(0);
}

log(`domain: ${process.env.domain}`);
log(`cchost: ${process.env.cchost}`);
log(`ccport: ${process.env.ccport}`);
log(`ccprotocol: ${process.env.ccprotocol}`);
log(`tlsCertPath: ${process.env.tlsCertPath}`);
log(`tlsKeyPath: ${process.env.tlsKeyPath}`);
log(`tlsCaPath: ${process.env.tlsCaPath}`);
log(`ownerId: ${process.env.ownerId}`);
log(`gbids: ${process.env.gbids}`);
log(`expectfailure: ${process.env.expectfailure}`);

// eslint-disable-next-line @typescript-eslint/no-non-null-assertion
const domain = process.env.domain!;
provisioning.ccAddress.host = process.env.cchost;
provisioning.ccAddress.port = process.env.ccport;
provisioning.ccAddress.protocol = process.env.ccprotocol;
const gbids = process.env.gbids ? process.env.gbids.split(",") : undefined;
const expectFailure = process.env.expectfailure ? process.env.expectfailure == "true" : undefined;

if (!provisioning.persistency) {
    provisioning.persistency = {};
}

if (process.env.tlsCertPath || process.env.tlsKeyPath || process.env.tlsCertPath || process.env.ownerId) {
    provisioning.keychain = {};

    if (process.env.tlsCertPath) {
        provisioning.keychain.tlsCert = fs.readFileSync(process.env.tlsCertPath, "utf8");
    }
    if (process.env.tlsKeyPath) {
        provisioning.keychain.tlsKey = fs.readFileSync(process.env.tlsKeyPath, "utf8");
    }
    if (process.env.tlsCaPath) {
        provisioning.keychain.tlsCa = fs.readFileSync(process.env.tlsCaPath, "utf8");
    }
    provisioning.keychain.ownerId = process.env.ownerId;
    provisioning.persistency.location = "./localStorageProviderTls";
} else {
    provisioning.persistency.location = "./localStorageProvider";
}

const timeoutPromise = new Promise((reject) => {
    let timeout = setTimeout(() => {
        clearTimeout(timeout);
        reject(new Error("Provider registration timed out"));
    }, 30000);
});

joynr.selectRuntime(WebSocketLibjoynrRuntime);
joynr
    .load(testbase.provisioning_common)
    .then(() => {
        log("joynr started");

        const providerQos = new joynr.types.ProviderQos({
            customParameters: [],
            priority: Date.now(),
            scope: joynr.types.ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        });

        const systemIntegrationTestProvider = joynr.providerBuilder.build(
            SystemIntegrationTestProvider,
            implementation
        );

        if (gbids) {
            return Promise.race([joynr.registration.register({ domain, providerQos, provider: systemIntegrationTestProvider, gbids }),
                                 timeoutPromise]);
        }
        return Promise.race([joynr.registration.registerInAllKnownBackends({
            domain,
            providerQos,
            provider: systemIntegrationTestProvider
        }), timeoutPromise]);
    })
    .then(() => {
        if(expectFailure){
            log("SIT RESULT FAILURE node provider registration did not fail as expected");
            process.exit(1);
        } else {
            log("provider registered successfully");
        }
    })
    .catch(error => {
        if(expectFailure){
            log("SIT RESULT success: node provider registration failed as expected");
            process.exit(0);
        } else {
            log(`error registering provider: ${error.toString()}`);
            process.exit(1);
        }
    });
