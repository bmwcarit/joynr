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
import EchoProxy from "../generated-javascript/joynr/tests/performance/EchoProxy";
const ComplexStruct = require("../generated-javascript/joynr/tests/performance/Types/ComplexStruct");
const PerformanceUtilities = require("./performanceutilities");
const options = PerformanceUtilities.getCommandLineOptionsOrDefaults();

export interface Benchmark {
    generateData(i: number): { data: any };
    testProcedure(args: any): Promise<any>;
}
let builtEchoProxy: any;

export function setup(proxy: any): void {
    builtEchoProxy = proxy;
}
export const benchmarks: Record<string, Benchmark> = {
    echoString: {
        generateData(i: number) {
            return {
                data: `${PerformanceUtilities.createString(options.stringLength - 2, "x")}-${i}`
            };
        },
        async testProcedure(args) {
            const returnValues = await builtEchoProxy.echoString(args);
            if (args.data !== returnValues.responseData) {
                throw new Error(
                    `Echo ${JSON.stringify(returnValues.responseData)} does not match input data: ${JSON.stringify(
                        args.data
                    )}`
                );
            }
            return returnValues;
        }
    },

    attributeString: {
        generateData(i) {
            return {
                data: `${PerformanceUtilities.createString(options.stringLength - 2, "x")}-${i}`
            };
        },
        testProcedure(args) {
            return builtEchoProxy.simpleAttribute.set(args);
        }
    },

    echoComplexStruct: {
        generateData(i) {
            return {
                data: new ComplexStruct({
                    num32: PerformanceUtilities.createRandomNumber(100000),
                    num64: PerformanceUtilities.createRandomNumber(1000000),
                    data: PerformanceUtilities.createByteArray(options.byteArrayLength, 1),
                    str: `${PerformanceUtilities.createString(options.stringLength - 2, "x")}-${i}`
                })
            };
        },
        async testProcedure(args) {
            const returnValues = await builtEchoProxy.echoComplexStruct(args);
            if (
                args.data.num32 !== returnValues.responseData.num32 ||
                args.data.num64 !== returnValues.responseData.num64 ||
                args.data.data.length !== returnValues.responseData.data.length ||
                args.data.str.length !== returnValues.responseData.str.length
            ) {
                throw new Error(
                    `Echo ${JSON.stringify(returnValues.responseData)} does not match input data: ${JSON.stringify(
                        args.data
                    )}`
                );
            }
            return returnValues;
        }
    },
    echoByteArray: {
        generateData(i) {
            const byteArraySize = options.byteArrayLength;
            const args = {
                data: PerformanceUtilities.createByteArray(byteArraySize, 1)
            };
            const firstElement = i % 128;
            args.data[0] = firstElement;
            return args;
        },
        async testProcedure(args) {
            const returnValues = await builtEchoProxy.echoByteArray(args);
            const firstElement = args.data[0];
            if (
                args.data.length !== returnValues.responseData.length ||
                firstElement !== returnValues.responseData[0]
            ) {
                throw new Error(
                    `Echo ${JSON.stringify(returnValues.responseData)} does not match input data: ${JSON.stringify(
                        args.data
                    )}`
                );
            }

            return returnValues;
        }
    },
    registerPlentyOfConsumers: {
        generateData(i) {
            const data = {
                data: `${PerformanceUtilities.createString(options.stringLength - 2, "x")}-${i}`
            };
            return { data, number: i };
        },
        testProcedure(args) {
            const messagingQos = new joynr.messaging.MessagingQos({
                ttl: 3600000
            });
            return joynr.proxyBuilder
                .build<EchoProxy>(EchoProxy, {
                    domain: options.domain,
                    messagingQos
                })
                .then(proxy => proxy.simpleAttribute.set(args.data));
        }
    }
};
