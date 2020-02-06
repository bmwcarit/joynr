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
import LocalDiscoveryAggregator = require("../../../../../main/js/joynr/capabilities/discovery/LocalDiscoveryAggregator");

describe(`libjoynr-js.joynr.capabilities.discovery.LocalDiscoveryAggregator`, () => {
    const lookupResult = { result: "someDiscoveryEntriesWithMetaInfo" };
    const discoveryProxy = {
        add: jest.fn(),
        lookup: jest.fn().mockResolvedValue(lookupResult),
        remove: jest.fn(),
        addToAll: jest.fn()
    };
    const awaitGlobalRegistration = true;
    const gbids = ["joynrdefaultgbid"];
    const discoveryEntry: any = {};
    const discoveryQos: any = {};
    const domains = ["joynr"];
    const interfaceName = "radio";
    const participantId = "participantId";
    let localDiscoveryAggregator: LocalDiscoveryAggregator;

    beforeEach(() => {
        jest.clearAllMocks();
        localDiscoveryAggregator = new LocalDiscoveryAggregator();
        localDiscoveryAggregator.setDiscoveryProxy(discoveryProxy as any);
    });

    it(`add calls DiscoveryProxy`, () => {
        localDiscoveryAggregator.add(discoveryEntry, awaitGlobalRegistration, gbids);
        expect(discoveryProxy.add).toHaveBeenCalledWith({ discoveryEntry, awaitGlobalRegistration, gbids });
    });

    it(`lookup calls DiscoveryProxy`, async () => {
        const result = await localDiscoveryAggregator.lookup(domains, interfaceName, discoveryQos, gbids);
        expect(discoveryProxy.lookup).toHaveBeenCalledWith({ domains, interfaceName, discoveryQos, gbids });
        expect(result).toEqual(lookupResult.result);
    });

    it(`lookupByParticipantId calls DiscoveryProxy`, async () => {
        const result = await localDiscoveryAggregator.lookupByParticipantId(participantId, discoveryQos, gbids);
        expect(discoveryProxy.lookup).toHaveBeenCalledWith({ participantId, discoveryQos, gbids });
        expect(result).toEqual(lookupResult.result);
    });

    it(`addToAll calls DiscoveryProxy`, () => {
        localDiscoveryAggregator.addToAll(discoveryEntry, awaitGlobalRegistration);
        expect(discoveryProxy.addToAll).toHaveBeenCalledWith({ discoveryEntry, awaitGlobalRegistration });
    });

    it(`remove calls DiscoveryProxy`, () => {
        localDiscoveryAggregator.remove(participantId);
        expect(discoveryProxy.remove).toHaveBeenCalledWith({ participantId });
    });
});
