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

import * as path from "path";
import { log } from "./logging";
import { spawn } from "child_process";
import { waitForSpy } from "./testUtil";
import joynr = require("joynr");
import provisioning = require("./provisioning_common");
import TestInterfaceProxy = require("../generated-javascript/joynr/tests/robustness/TestInterfaceProxy");

// parse args
if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(1);
}
// eslint-disable-next-line @typescript-eslint/no-non-null-assertion
const domain = process.env.domain!;
// eslint-disable-next-line @typescript-eslint/no-non-null-assertion
const cmdPath = path.join(__dirname, "../../../");
// eslint-disable-next-line @typescript-eslint/no-non-null-assertion
const testcase = process.env.testcase!;
log(`domain: ${domain}`);
log(`testcase: ${testcase}`);
log(`cmdPath: ${cmdPath}`);

// test results

const killClusterController = function() {
    log("Entering killClusterController");
    log(`killClusterController: calling ${cmdPath}/kill-clustercontroller.sh`);
    const out = spawn(`${cmdPath}/kill-clustercontroller.sh`, []);
    out.stdout.on("data", (data: any) => {
        log(`${data}`);
    });
    out.stderr.on("data", (data: any) => {
        log(`${data}`);
    });
    out.on("close", (code: any) => {
        log("killClusterController: in close");
        log(`killClusterController: code = ${code}`);
        if (code === 0) {
            return Promise.resolve();
        } else {
            return Promise.reject(new Error(`killClusterController failed with code ${code}`));
        }
    });
};

const startClusterController = function() {
    // eslint-disable-next-line promise/avoid-new
    return new Promise((resolve: any, reject: any) => {
        log("Entering startClusterController");
        const out = spawn(`${cmdPath}/start-clustercontroller.sh`);
        out.stdout.on("data", (data: any) => {
            log(`${data}`);
        });
        out.stderr.on("data", (data: any) => {
            log(`${data}`);
        });
        out.on("close", (code: any) => {
            if (code === 0) {
                resolve();
            } else {
                reject(new Error(`startClusterController failed with code ${code}`));
            }
        });
    });
};

const killProvider = function() {
    // eslint-disable-next-line promise/avoid-new
    return new Promise((resolve: any, reject: any) => {
        log("Entering killProvider");
        log(`killProvider: calling ${cmdPath}/kill-provider.sh`);
        const out = spawn(`${cmdPath}/kill-provider.sh`, []);
        out.stdout.on("data", (data: any) => {
            log(`${data}`);
        });
        out.stderr.on("data", (data: any) => {
            log(`${data}`);
        });
        out.on("close", (code: any) => {
            log("killProvider: in close");
            log(`killProvider: code = ${code}`);
            if (code === 0) {
                resolve();
            } else {
                reject(new Error(`killProvider failed with code ${code}`));
            }
        });
    });
};

const startProviderJs = function() {
    // eslint-disable-next-line promise/avoid-new
    return new Promise((resolve: any, reject: any) => {
        log("Entering startProvider");
        const out = spawn(`${cmdPath}/start-provider.sh`, ["js", domain]);
        out.stdout.on("data", (data: any) => {
            log(`${data}`);
        });
        out.stderr.on("data", (data: any) => {
            log(`${data}`);
        });
        out.on("close", (code: any) => {
            if (code === 0) {
                resolve();
            } else {
                reject(new Error(`startProvider failed with code ${code}`));
            }
        });
    });
};

describe("Consumer test", () => {
    let testInterfaceProxy: TestInterfaceProxy;

    beforeAll(async () => {
        await joynr.load(provisioning);
        log("joynr started");
        const messagingQos = new joynr.messaging.MessagingQos({
            ttl: 60000
        });
        testInterfaceProxy = await joynr.proxyBuilder.build(TestInterfaceProxy, {
            domain,
            messagingQos
        });

        log("testInterface proxy build");
    });

    it("proxy is defined", () => {
        expect(testInterfaceProxy).toBeDefined();
    });

    if (testcase === "js_cpp_tests") {
        describe("Robustness test with C++ provider", () => {
            it("proxy is defined", () => {
                expect(testInterfaceProxy).toBeDefined();
            });

            it("callMethodBeforeClusterControllerHasBeenRestarted", async () => {
                const args = {
                    stringArg: "Hello"
                };
                await testInterfaceProxy.methodWithStringParameters(args);
            });

            it("callMethodAfterClusterControllerHasBeenRestarted", async () => {
                await killClusterController();
                await startClusterController();
                const args = {
                    stringArg: "Hello"
                };
                await testInterfaceProxy.methodWithStringParameters(args);
            });

            it("callMethodWithDelayedResponse", async () => {
                const delay = 30000; // in milliseconds, must be shorter than ttl

                // call function, on provider side the response is delayed
                // should be sent after the restart of clustercontroller has been finished
                const args = {
                    delayArg: delay
                };
                const promise = testInterfaceProxy.methodWithDelayedResponse(args);
                await killClusterController();
                await startClusterController();
                log("ClusterController was restarted successfully");

                await promise;
            });
        });
    } else {
        describe("Robustness test with JS provider", () => {
            it("proxy is defined", () => {
                expect(testInterfaceProxy).toBeDefined();
            });

            it("callMethodWithStringParameters", async () => {
                const args = {
                    stringArg: "Hello"
                };
                await testInterfaceProxy.methodWithStringParameters(args);
            });

            it("callMethodWithStringParametersAfterProviderHasBeenRestarted", async () => {
                await killProvider();
                await startProviderJs();
                const args = {
                    stringArg: "Hello"
                };
                await testInterfaceProxy.methodWithStringParameters(args);
            });

            it("callMethodWithStringParametersBeforeProviderHasBeenRestarted", async () => {
                // kill the provider before the request is sent
                await killProvider();
                const args = {
                    stringArg: "Hello"
                };
                const promise = testInterfaceProxy.methodWithStringParameters(args);

                await startProviderJs();
                await promise;
            });

            it("subscribeToAttributeString", async () => {
                const spy = {
                    onPublication: jest.fn(),
                    onPublicationError: jest.fn()
                };

                const qosSettings = {
                    minIntervalMs: 5000,
                    maxIntervalMs: 10000,
                    validityMs: 120000
                };
                const subscriptionQosOnChangeWithKeepAlive = new joynr.proxy.OnChangeWithKeepAliveSubscriptionQos(
                    qosSettings
                );
                log("subscribeToAttributeString");
                const onReceivePromise = waitForSpy(spy.onPublication);
                const subscriptionId = await testInterfaceProxy.attributeString.subscribe({
                    subscriptionQos: subscriptionQosOnChangeWithKeepAlive,
                    onReceive: spy.onPublication,
                    onError: spy.onPublicationError
                });

                let retObj = await onReceivePromise;
                expect(retObj).toBeDefined();

                // kill and restart the provider while the time period until the next
                // publication happens is passing; the time period must be long enough
                // so that no further publication is sent until the provider got killed
                await killProvider();
                await startProviderJs();
                retObj = await waitForSpy(spy.onPublication);
                expect(retObj).toBeDefined();

                await testInterfaceProxy.attributeString.unsubscribe({
                    subscriptionId
                });
            });

            it("subscribeToBroadcastWithSingleStringParameter", async () => {
                const spy = {
                    onPublication: jest.fn(),
                    onPublicationError: jest.fn()
                };
                const subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minIntervalMs: 50 });

                log("subscribeToBroadcastWithSingleStringParameter");
                const subscriptionId = await testInterfaceProxy.broadcastWithSingleStringParameter.subscribe({
                    subscriptionQos: subscriptionQosOnChange,
                    onReceive: spy.onPublication,
                    onError: spy.onPublicationError
                });
                log(`subscriptionId = ${subscriptionId}`);
                await killProvider();
                await startProviderJs();
                const onReceivePromise = waitForSpy(spy.onPublication);
                await testInterfaceProxy.startFireBroadcastWithSingleStringParameter();
                await onReceivePromise;

                await testInterfaceProxy.stopFireBroadcastWithSingleStringParameter();

                await testInterfaceProxy.broadcastWithSingleStringParameter.unsubscribe({
                    subscriptionId
                });
            });
        });
    }
});
