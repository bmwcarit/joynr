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

import CapabilitiesStore from "../../../../main/js/joynr/capabilities/CapabilitiesStore";
import DiscoveryEntry from "../../../../main/js/generated/joynr/types/DiscoveryEntry";
import DiscoveryQos from "../../../../main/js/generated/joynr/types/DiscoveryQos";
import DiscoveryScope from "../../../../main/js/generated/joynr/types/DiscoveryScope";
import ProviderScope from "../../../../main/js/generated/joynr/types/ProviderScope";
import ProviderQos from "../../../../main/js/generated/joynr/types/ProviderQos";
import CustomParameter from "../../../../main/js/generated/joynr/types/CustomParameter";
import Version from "../../../../main/js/generated/joynr/types/Version";

const expiryDateMs = Date.now() + 1e10;

describe("libjoynr-js.joynr.capabilities.CapabilitiesStore", () => {
    let fakeTime = 0,
        cacheMaxAge: number,
        directory: CapabilitiesStore,
        discoveryQos: DiscoveryQos,
        discoveryEntry1: DiscoveryEntry;
    let discoveryEntry2: DiscoveryEntry, discoveryEntry3: DiscoveryEntry, discoveryEntry4: DiscoveryEntry;
    const settings = {
        providerVersion: new Version({
            majorVersion: 47,
            minorVersion: 11
        }),
        domain: "myDomain",
        interfaceName: "radio",
        qos: new ProviderQos({
            customParameters: [
                new CustomParameter({
                    name: "theName",
                    value: "theValue"
                })
            ],
            priority: 1234,
            scope: ProviderScope.LOCAL,
            supportsOnChangeSubscriptions: true
        }),
        channelId: "0001",
        publicKeyId: "",
        participantId: "700",
        expiryDateMs,
        lastSeenDateMs: Date.now()
    };

    function increaseFakeTime(timeMs: number) {
        fakeTime = fakeTime + timeMs;
        jest.advanceTimersByTime(timeMs);
    }

    /**
     * Called before each test.
     */
    beforeEach(() => {
        jest.useFakeTimers();
        directory = new CapabilitiesStore();
        discoveryEntry1 = new DiscoveryEntry(settings);
        discoveryEntry2 = new DiscoveryEntry(settings);
        discoveryEntry3 = new DiscoveryEntry(settings);
        discoveryEntry4 = new DiscoveryEntry(settings);
        cacheMaxAge = 100;
        discoveryQos = new DiscoveryQos({
            cacheMaxAge,
            discoveryScope: DiscoveryScope.LOCAL_ONLY
        } as any);

        jest.spyOn(Date, "now").mockImplementation(() => {
            return fakeTime;
        });
    });

    afterEach(() => {
        jest.useRealTimers();
    });

    it("is instantiable", () => {
        expect(directory).toBeDefined();
    });

    it("can register a capability", () => {
        directory.add({
            discoveryEntry: discoveryEntry1
        });
        const result = directory.lookup({
            participantId: discoveryEntry1.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeDefined();
        expect(result.length).toBe(1);
        expect(result[0]).toEqual(discoveryEntry1);
    });

    it("register global entry, returned when looking for global entries", () => {
        discoveryEntry2.participantId = "701";
        directory.add({
            discoveryEntry: discoveryEntry1,
            remote: true
        });

        let result = directory.lookup({
            domains: [settings.domain],
            interfaceName: settings.interfaceName,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });

        expect(result).toBeDefined();
        expect(result.length).toBe(1);
        expect(result[0]).toBe(discoveryEntry1);

        increaseFakeTime(discoveryQos.cacheMaxAge + 1);
        result = directory.lookup({
            domains: [settings.domain],
            interfaceName: settings.interfaceName,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });

        expect(result).toBeDefined();
        expect(result.length).toBe(0);
    });

    it("does not register duplicate capabilities", () => {
        directory.add({
            discoveryEntry: discoveryEntry1
        });
        directory.add({
            discoveryEntry: discoveryEntry1
        }); // register exact duplicate
        (discoveryEntry2.qos.customParameters as any).blah = {
            value: "lala",
            type: "QosString"
        };
        directory.add({
            discoveryEntry: discoveryEntry2
        }); // register cap with different qos, this should be a duplicate

        const result = directory.lookup({
            participantId: discoveryEntry1.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeDefined();
    });

    it("registers an array of capabilities", () => {
        // different domain, interfaceName and participant Id
        discoveryEntry2.domain = "mySecondDomain";
        discoveryEntry2.interfaceName = "music";
        discoveryEntry2.participantId = "701";

        directory.add({
            discoveryEntries: [discoveryEntry1, discoveryEntry2]
        });

        const result = directory.lookup({
            participantId: discoveryEntry1.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeDefined();
        const result2 = directory.lookup({
            participantId: discoveryEntry2.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result2).toBeDefined();
        expect(result[0].domain).not.toBe(result2[0].domain);

        // TODO locally registered capabilities wont have a channelAddress....
        // expect(result[0].channelId).toBe(result[1].channelId);
    });

    it("gets capability for participant id", () => {
        // same participant id
        discoveryEntry2.domain = "mySecondDomain";
        discoveryEntry2.interfaceName = "music";

        directory.remove({
            participantIds: ["700", "701", "702"]
        });

        // different participant id
        discoveryEntry3.participantId = "702";

        directory.add({
            discoveryEntries: [discoveryEntry1, discoveryEntry2, discoveryEntry3]
        });
        const result = directory.lookup({
            participantId: discoveryEntry2.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeDefined();
        expect(result.length).toBe(1);
        expect(result[0]).toBe(discoveryEntry2);
    });

    it("gets capabilities for domain/interface", () => {
        // different provider qos
        discoveryEntry2.qos = {
            blah: "lala"
        } as any;

        // different domain
        discoveryEntry3.domain = "mySecondDomain";

        // different participant id
        discoveryEntry4.participantId = "701";

        directory.add({
            discoveryEntries: [discoveryEntry1, discoveryEntry2, discoveryEntry3, discoveryEntry4]
        });
        const result = directory.lookup({
            domains: [discoveryEntry1.domain],
            interfaceName: discoveryEntry1.interfaceName,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result.length).toBe(2);
        expect(result[0].participantId).not.toBe(result[1].participantId);
    });

    it("can unregister capabilities with exact registered object", () => {
        discoveryEntry2.participantId = "701";
        discoveryEntry3.participantId = "702";
        directory.add({
            discoveryEntries: [discoveryEntry1, discoveryEntry2, discoveryEntry3]
        });

        // unregister one capability
        directory.remove({
            participantId: discoveryEntry4.participantId
        });

        let result = directory.lookup({
            participantId: discoveryEntry1.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result.length).toBe(0);

        // check that it does not unregister other capabilities
        result = directory.lookup({
            participantId: discoveryEntry2.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result.length).toBe(1);

        result = directory.lookup({
            domains: [discoveryEntry1.domain],
            interfaceName: discoveryEntry1.interfaceName,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result.length).toBe(2);
    });

    it("can unregister multiple capabilities", () => {
        discoveryEntry2.participantId = "701";
        discoveryEntry3.participantId = "702";
        directory.add({
            discoveryEntries: [discoveryEntry1, discoveryEntry2, discoveryEntry3]
        });
        directory.remove({
            participantIds: [discoveryEntry2.participantId, discoveryEntry3.participantId]
        });
        let result = directory.lookup({
            participantId: discoveryEntry2.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result.length).toBe(0);
        result = directory.lookup({
            participantId: discoveryEntry3.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result.length).toBe(0);
    });
});
