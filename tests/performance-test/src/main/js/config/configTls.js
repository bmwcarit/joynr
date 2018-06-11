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
            numRuns: 8000
        },
        {
            name: "echoComplexStruct",
            enabled: "true",
            numRuns: 8000
        },
        {
            name: "echoString",
            enabled: "true",
            numRuns: 8000
        },
        {
            name: "echoByteArray",
            enabled: "true",
            numRuns: 8000
        },
        {
            name: "registerPlentyOfConsumers",
            enabled: "true",
            numRuns: 500
        },
        {
            name: "simpleBroadcast",
            enabled: "false",
            numRuns: 8000,
            type: "broadcast",
            numProxies: 4
        }
    ],
    global: {
        testRuns: 30,
        domain: "performance_test_domain",
        cc: {
            host: "localhost",
            port: "4243"
        },
        measureMemory: "false",
        heapSnapShot: "false",
        stringLength: 10,
        timeout: 3600000,
        byteArraySize: 20,
        testType: "burst" // burst, concurrency, single
    },
    tls: {
        certPath: "/data/ssl-data/certs/client.cert.pem",
        keyPath: "/data/ssl-data/private/client.key.pem",
        caPath: "/data/ssl-data/certs/ca.cert.pem",
        ownerId: "client"
    }
};
