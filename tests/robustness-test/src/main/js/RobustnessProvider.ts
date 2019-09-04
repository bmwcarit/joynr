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

import { ProviderReadWriteAttributeImpl } from "joynr/joynr/types/JoynrProvider";
import * as MySubscriptionContainer from "../generated-javascript/joynr/tests/robustness/MySubscriptionContainer";
import { TestInterfaceProviderImplementation } from "../generated-javascript/joynr/tests/robustness/TestInterfaceProvider";
import { prettyLog } from "./logging";
import joynr = require("joynr");

// Attributes
let attributeStringValue = "done";
let intervalTimer: NodeJS.Timer | undefined;

function sendBroadcast(): void {
    const stringOut = "boom";
    // eslint-disable-next-line @typescript-eslint/no-non-null-assertion,@typescript-eslint/no-use-before-define
    const outputParameters = implementation.broadcastWithSingleStringParameter!.createBroadcastOutputParameters();
    outputParameters.setStringOut(stringOut);
    // eslint-disable-next-line @typescript-eslint/no-non-null-assertion,@typescript-eslint/no-use-before-define
    implementation.broadcastWithSingleStringParameter!.fire(outputParameters);
}

function createGenericAttributeImpl<T>(): ProviderReadWriteAttributeImpl<T> {
    let store: T;
    return {
        get: () => store,
        set: (value: T) => {
            store = value;
        }
    };
}

export const implementation: TestInterfaceProviderImplementation = {
    // attribute getter and setter
    attributeString: {
        get() {
            prettyLog("RobustnessProvider.attributeString.get() called");
            return Promise.resolve(attributeStringValue);
        },
        set(value: string) {
            prettyLog(`RobustnessProvider.attributeString.set(${value}) called`);
            attributeStringValue = value;
            return Promise.resolve();
        }
    },

    methodWithStringParameters(opArgs) {
        prettyLog(`RobustnessProvider.methodWithStringParameters(${JSON.stringify(opArgs)}) called`);
        // eslint-disable-next-line promise/avoid-new
        return new Promise((resolve, reject) => {
            if (opArgs.stringArg === undefined || opArgs.stringArg === null) {
                reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "methodWithStringParameters: invalid argument stringArg"
                    })
                );
            } else {
                resolve({
                    stringOut: `received stringArg: ${opArgs.stringArg}`
                });
            }
        });
    },

    methodWithDelayedResponse(opArgs) {
        prettyLog(`RobustnessProvider.methodWithDelayedResponse(${JSON.stringify(opArgs)}) called`);
        // eslint-disable-next-line promise/avoid-new
        return new Promise((resolve, reject) => {
            if (opArgs.delayArg === undefined) {
                reject(
                    new joynr.exceptions.ProviderRuntimeException({
                        detailMessage: "methodWithDelayedResponse: invalid argument delayArg"
                    })
                );
            } else {
                // do the delay here
                setTimeout(() => {
                    resolve({
                        stringOut: "done"
                    });
                }, opArgs.delayArg);
            }
        });
    },

    async methodToFireBroadcastWithSingleStringParameter() {
        prettyLog("RobustnessProvider.methodToFireBroadcastWithSingleStringParameter() called");
        sendBroadcast();
    },

    startFireBroadcastWithSingleStringParameter() {
        prettyLog("RobustnessProvider.startFireBroadcastWithSingleStringParameter() called");
        if (intervalTimer) {
            clearTimeout(intervalTimer);
            intervalTimer = undefined;
        }
        let numberOfBroadcasts = 0;
        const periodMs = 250;
        const validityMs = 60000;
        intervalTimer = setInterval(() => {
            sendBroadcast();
            numberOfBroadcasts++;
            if (numberOfBroadcasts === validityMs / periodMs) {
                // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
                clearInterval(intervalTimer!);
            }
        }, periodMs);
        if (intervalTimer) {
            // intervalTimer successfully started
            return Promise.resolve();
        } else {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "startFireBroadcastWithSingleStringParameter: intervalTimer could not be started"
                })
            );
        }
    },

    stopFireBroadcastWithSingleStringParameter() {
        prettyLog("RobustnessProvider.stopFireBroadcastWithSingleStringParameter() called");
        if (intervalTimer) {
            clearInterval(intervalTimer);
            intervalTimer = undefined;
            return Promise.resolve();
        } else {
            return Promise.reject(
                new joynr.exceptions.ProviderRuntimeException({
                    detailMessage: "stopFireBroadcastWithSingleStringParameter: no intervalTimer running"
                })
            );
        }
    },
    // dummy implementation below
    attributeString1: createGenericAttributeImpl<string>(),
    attributeString2: createGenericAttributeImpl<string>(),
    methodWithStringParameters1: () => ({ stringOut: "string" }),
    methodWithStringParameters2: () => ({ stringOut: "string" }),
    ping: () => {},
    subContainer: createGenericAttributeImpl<MySubscriptionContainer>()
};
