/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

module.exports = {
    benchmarks: [
        {
            name: "attributeString",
            enabled: "true",
            numRuns: 5000
        },
        {
            name: "echoComplexStruct",
            enabled: "true",
            numRuns: 1000
        },
        {
            name: "echoString",
            enabled: "true",
            numRuns: 5000
        },
        {
            name: "echoByteArray",
            enabled: "true",
            numRuns: 1000
        },
        {
            name: "registerPlentyOfConsumers",
            enabled: "false",
            numRuns: 500
        }
    ],
    global: {
        testRuns: 50,
        domain: "performance_test_domain",
        cc: {
            host: "localhost",
            port: "4242"
        },
        measureMemory: "false",
        stringLength: 10,
        timeout: 3600000,
        byteArraySize: 1000,
        testType: "burst" // burst, concurrency, single
    }
};
