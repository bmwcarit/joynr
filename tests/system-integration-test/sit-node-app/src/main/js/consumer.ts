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
import fs from "fs";

import SystemIntegrationTestProxy from "../generated-sources/joynr/test/SystemIntegrationTestProxy";

process.on("uncaughtException", e => {
    log(`SIT RESULT error: received uncaught Exception${e}`);
    process.exit(1);
});

const provisioning = testbase.provisioning_common;
const prefix = process.env.ccprotocol === "wss" ? "nodeTlsConsumer: " : "nodeConsumer: ";

function log(message: string): void {
    testbase.logging.log(prefix + message);
}

if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(1);
}

if (process.env.cchost === undefined) {
    log("please pass cchost as argument");
    process.exit(1);
}

if (process.env.ccport === undefined) {
    log("please pass ccport as argument");
    process.exit(1);
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
const gbids = process.env.gbids ? process.env.gbids.split(",") : undefined;
provisioning.ccAddress.host = process.env.cchost;
provisioning.ccAddress.port = process.env.ccport;
provisioning.ccAddress.protocol = process.env.ccprotocol;
const expectFailure = process.env.expectfailure ? process.env.expectfailure == "true" : undefined;

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
}

async function runTest(systemIntegrationTestProxy: SystemIntegrationTestProxy): Promise<void> {
    const addends = {
        addendA: 123,
        addendB: 321
    };
    const opArgs = await systemIntegrationTestProxy.add(addends);
    if (opArgs.result === undefined) {
        throw new Error("systemIntegrationTestProxy.add failed: result undefined");
    }
    if (opArgs.result !== addends.addendA + addends.addendB) {
        log(`${opArgs.result} != ${addends.addendA} + ${addends.addendB}`);
        throw new Error(
            `systemIntegrationTestProxy.add failed: ${opArgs.result} != ${addends.addendA} + ${addends.addendB}`
        );
    }
}

joynr.selectRuntime(WebSocketLibjoynrRuntime);
joynr
    .load(provisioning)
    .then(() => {
        log("joynr.load successfully");
        const messagingQos = new joynr.messaging.MessagingQos({
            ttl: 60000
        });

        if(expectFailure) {
            return joynr.proxyBuilder.build(SystemIntegrationTestProxy, {
                domain,
                messagingQos,
                discoveryQos: new joynr.proxy.DiscoveryQos({
                                  discoveryTimeoutMs: 20000 // 20 Seconds
                              }),
                gbids
            });
        } else {
            return joynr.proxyBuilder.build(SystemIntegrationTestProxy, {
                domain,
                messagingQos,
                discoveryQos: new joynr.proxy.DiscoveryQos({
                                  discoveryTimeoutMs: 120000 // 2 Mins
                              }),
                gbids
            });
        }
    })
    .then(systemIntegrationTestProxy => {
        return runTest(systemIntegrationTestProxy);
    })
    .then(() => {
        if(expectFailure) {
            log(`SIT RESULT error: node consumer did not fail as expected`);
            process.exit(1);
        } else {
            log(`SIT RESULT success: node consumer -> ${domain}`);
            process.exit(0);
        }
    })
    .catch(error => {
        if(expectFailure) {
            log(`SIT RESULT success: node consumer failed as expected`);
            process.exit(0);
        } else {
            log(`SIT RESULT error: node consumer -> ${domain} error: ${JSON.stringify(error)}`);
            process.exit(1);
        }
    })
    .catch(error => {
        throw error;
    });
