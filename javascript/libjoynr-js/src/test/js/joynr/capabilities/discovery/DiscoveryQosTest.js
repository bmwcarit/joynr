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
require("../../../node-unit-test-helper");
var DiscoveryQos = require("../../../../../main/js/joynr/proxy/DiscoveryQos");
var ArbitrationStrategyCollection = require("../../../../../main/js/joynr/types/ArbitrationStrategyCollection");
var DiscoveryScope = require("../../../../../main/js/generated/joynr/types/DiscoveryScope");

describe("libjoynr-js.joynr.capabilities.arbitration.DiscoveryQos", function() {
    it("is instantiable", function() {
        expect(new DiscoveryQos()).toBeDefined();
        expect(
            new DiscoveryQos({
                discoveryTimeoutMs: 5000,
                discoveryRetryDelayMs: 0,
                arbitrationStrategy: ArbitrationStrategyCollection.HighestPriority,
                cacheMaxAgeMs: 0,
                discoveryScope: DiscoveryScope.LOCAL_AND_GLOBAL,
                additionalParameters: {}
            })
        ).toBeDefined();
    });

    it("is of correct type", function() {
        var emptyDiscoveryQos = new DiscoveryQos();
        expect(emptyDiscoveryQos).toBeDefined();
        expect(emptyDiscoveryQos).not.toBeNull();
        expect(typeof emptyDiscoveryQos === "object").toBeTruthy();
        expect(emptyDiscoveryQos instanceof DiscoveryQos).toBeTruthy();

        var defaultDiscoveryQos = new DiscoveryQos({
            discoveryTimeoutMs: 30000,
            discoveryRetryDelayMs: 1000,
            arbitrationStrategy: ArbitrationStrategyCollection.HighestPriority,
            cacheMaxAgeMs: 0,
            discoveryScope: DiscoveryScope.LOCAL_THEN_GLOBAL,
            additionalParameters: {}
        });
        expect(defaultDiscoveryQos).toBeDefined();
        expect(defaultDiscoveryQos).not.toBeNull();
        expect(typeof defaultDiscoveryQos === "object").toBeTruthy();
        expect(defaultDiscoveryQos instanceof DiscoveryQos).toEqual(true);
    });

    it("constructs correct default object", function() {
        expect(new DiscoveryQos()).toEqual(
            new DiscoveryQos({
                discoveryTimeoutMs: 600000,
                discoveryRetryDelayMs: 10000,
                arbitrationStrategy: ArbitrationStrategyCollection.LastSeen,
                cacheMaxAgeMs: 0,
                discoveryScope: DiscoveryScope.LOCAL_THEN_GLOBAL,
                additionalParameters: {}
            })
        );
    });

    it("adapts to new default values", function() {
        DiscoveryQos.setDefaultSettings({
            discoveryTimeoutMs: 30000,
            discoveryRetryDelayMs: 1000,
            arbitrationStrategy: ArbitrationStrategyCollection.HighestPriority,
            cacheMaxAgeMs: 0,
            discoveryScope: DiscoveryScope.LOCAL_THEN_GLOBAL,
            additionalParameters: {}
        });
        expect(new DiscoveryQos()).toEqual(
            new DiscoveryQos({
                discoveryTimeoutMs: 30000,
                discoveryRetryDelayMs: 1000,
                arbitrationStrategy: ArbitrationStrategyCollection.HighestPriority,
                cacheMaxAgeMs: 0,
                discoveryScope: DiscoveryScope.LOCAL_THEN_GLOBAL,
                additionalParameters: {}
            })
        );
    });

    it("constructs with correct member values", function() {
        var discoveryQos = new DiscoveryQos({
            discoveryTimeoutMs: 12345,
            discoveryRetryDelayMs: 123456,
            arbitrationStrategy: ArbitrationStrategyCollection.HighestPriority,
            cacheMaxAgeMs: 1234,
            discoveryScope: DiscoveryScope.LOCAL_AND_GLOBAL,
            additionalParameters: {
                testKey: "testValue"
            }
        });
        expect(discoveryQos.discoveryTimeoutMs).toEqual(12345);
        expect(discoveryQos.discoveryRetryDelayMs).toEqual(123456);
        expect(discoveryQos.arbitrationStrategy).toEqual(ArbitrationStrategyCollection.HighestPriority);
        expect(discoveryQos.cacheMaxAgeMs).toEqual(1234);
        expect(discoveryQos.discoveryScope).toEqual(DiscoveryScope.LOCAL_AND_GLOBAL);
        expect(discoveryQos.additionalParameters).toEqual({
            testKey: "testValue"
        });
    });
});
