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
require("../../node-unit-test-helper");
var CapabilitiesStore = require("../../../../main/js/joynr/capabilities/CapabilitiesStore");
var DiscoveryEntry = require("../../../../main/js/generated/joynr/types/DiscoveryEntry");
var DiscoveryQos = require("../../../../main/js/generated/joynr/types/DiscoveryQos");
var DiscoveryScope = require("../../../../main/js/generated/joynr/types/DiscoveryScope");
var ProviderScope = require("../../../../main/js/generated/joynr/types/ProviderScope");
var ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");
var CustomParameter = require("../../../../main/js/generated/joynr/types/CustomParameter");
var Version = require("../../../../main/js/generated/joynr/types/Version");
var Date = require("../../../../test/js/global/Date");

describe("libjoynr-js.joynr.capabilities.CapabilitiesStore", function() {
    var fakeTime = 0,
        cacheMaxAge,
        directory,
        discoveryQos,
        discoveryEntry1;
    var discoveryEntry2, discoveryEntry3, discoveryEntry4;
    var settings = {
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
        participantId: "700"
    };

    function increaseFakeTime(time_ms) {
        fakeTime = fakeTime + time_ms;
        jasmine.clock().tick(time_ms);
    }

    /**
     * Called before each test.
     */
    beforeEach(function() {
        jasmine.clock().install();
        directory = new CapabilitiesStore();
        discoveryEntry1 = new DiscoveryEntry(settings);
        discoveryEntry2 = new DiscoveryEntry(settings);
        discoveryEntry3 = new DiscoveryEntry(settings);
        discoveryEntry4 = new DiscoveryEntry(settings);
        cacheMaxAge = 100;
        discoveryQos = new DiscoveryQos({
            cacheMaxAge: cacheMaxAge,
            discoveryScope: DiscoveryScope.LOCAL_ONLY
        });

        spyOn(Date, "now").and.callFake(function() {
            return fakeTime;
        });
    });

    afterEach(function() {
        jasmine.clock().uninstall();
    });

    function checkCapabilitiesDirectoryStateForUnregister(capabilityToRemove) {
        discoveryEntry2.participantId = "701";
        discoveryEntry3.participantId = "702";
        directory.add({
            discoveryEntries: [discoveryEntry1, discoveryEntry2, discoveryEntry3]
        });

        // unregister one capability
        directory.remove({
            participantId: discoveryEntry4.participantId
        });

        var result = directory.lookup({
            participantId: discoveryEntry1.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeUndefined();

        // check that it does not unregister other capabilities
        result = directory.lookup({
            participantId: discoveryEntry2.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeDefined();

        result = directory.lookup({
            domains: [discoveryEntry1.domain],
            interfaceName: discoveryEntry1.interfaceName,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result.length).toBe(2);
    }

    it("is instantiable", function() {
        expect(directory).toBeDefined();
    });

    it("can register a capability", function() {
        directory.add({
            discoveryEntry: discoveryEntry1
        });
        var result = directory.lookup({
            participantId: discoveryEntry1.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeDefined();
        expect(result.length).toBe(1);
        expect(result[0]).toEqual(discoveryEntry1);
    });

    it("register global entry, returned when looking for global entries", function() {
        discoveryEntry2.participantId = "701";
        directory.add({
            discoveryEntry: discoveryEntry1,
            remote: true
        });

        var result = directory.lookup({
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

    it("does not register duplicate capabilities", function() {
        directory.add({
            discoveryEntry: discoveryEntry1
        });
        directory.add({
            discoveryEntry: discoveryEntry1
        }); // register exact duplicate
        discoveryEntry2.qos.customParameters.blah = {
            value: "lala",
            type: "QosString"
        };
        directory.add({
            discoveryEntry: discoveryEntry2
        }); // register cap with different qos, this should be a duplicate

        var result = directory.lookup({
            participantId: discoveryEntry1.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeDefined();
    });

    it("registers an array of capabilities", function() {
        // different domain, interfaceName and participant Id
        discoveryEntry2.domain = "mySecondDomain";
        discoveryEntry2.interfaceName = "music";
        discoveryEntry2.participantId = "701";

        directory.add({
            discoveryEntries: [discoveryEntry1, discoveryEntry2]
        });

        var result = directory.lookup({
            participantId: discoveryEntry1.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeDefined();
        var result2 = directory.lookup({
            participantId: discoveryEntry2.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result2).toBeDefined();
        expect(result[0].domain).not.toBe(result2[0].domain);

        // TODO locally registered capabilities wont have a channelAddress....
        // expect(result[0].channelId).toBe(result[1].channelId);
    });

    it("gets capability for participant id", function() {
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
        var result = directory.lookup({
            participantId: discoveryEntry2.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeDefined();
        expect(result.length).toBe(1);
        expect(result[0]).toBe(discoveryEntry2);
    });

    it("gets capabilities for domain/interface", function() {
        // different provider qos
        discoveryEntry2.qos = {
            blah: "lala"
        };

        // different domain
        discoveryEntry3.domain = "mySecondDomain";

        // different participant id
        discoveryEntry4.participantId = "701";

        directory.add({
            discoveryEntries: [discoveryEntry1, discoveryEntry2, discoveryEntry3, discoveryEntry4]
        });
        var result = directory.lookup({
            domains: [discoveryEntry1.domain],
            interfaceName: discoveryEntry1.interfaceName,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result.length).toBe(2);
        expect(result[0].participantId).not.toBe(result[1].participantId);
    });

    it("can unregister capabilities with exact registered object", function() {
        checkCapabilitiesDirectoryStateForUnregister(discoveryEntry1);
    });

    it("can unregister capabilities with a copy of the registered capability", function() {
        checkCapabilitiesDirectoryStateForUnregister(discoveryEntry4);
    });

    it("can unregister multiple capabilities", function() {
        discoveryEntry2.participantId = "701";
        discoveryEntry3.participantId = "702";
        directory.add({
            discoveryEntries: [discoveryEntry1, discoveryEntry2, discoveryEntry3]
        });
        directory.remove({
            participantIds: [discoveryEntry2.participantId, discoveryEntry3.participantId]
        });
        var result = directory.lookup({
            participantId: discoveryEntry2.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeUndefined();
        result = directory.lookup({
            participantId: discoveryEntry3.participantId,
            cacheMaxAge: discoveryQos.cacheMaxAge
        });
        expect(result).toBeUndefined();
    });
});
