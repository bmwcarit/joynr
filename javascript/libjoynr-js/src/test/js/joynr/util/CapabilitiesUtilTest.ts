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

import * as CapabilitiesUtil from "../../../../main/js/joynr/util/CapabilitiesUtil";
import DiscoveryEntry from "../../../../main/js/generated/joynr/types/DiscoveryEntry";
import DiscoveryEntryWithMetaInfo from "../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo";
import GlobalDiscoveryEntry from "../../../../main/js/generated/joynr/types/GlobalDiscoveryEntry";
import Version from "../../../../main/js/generated/joynr/types/Version";
import ProviderQos from "../../../../main/js/generated/joynr/types/ProviderQos";
import MqttAddress from "../../../../main/js/generated/joynr/system/RoutingTypes/MqttAddress";

describe("libjoynr-js.joynr.CapabilitiesUtil", () => {
    it("is of correct type and has all members", () => {
        expect(CapabilitiesUtil).toBeDefined();
        expect(CapabilitiesUtil).not.toBeNull();
        expect(typeof CapabilitiesUtil === "object").toBeTruthy();
        expect(CapabilitiesUtil.toDiscoveryEntry).toBeDefined();
        expect(typeof CapabilitiesUtil.toDiscoveryEntry === "function").toBeTruthy();
        expect(CapabilitiesUtil.toDiscoveryEntries).toBeDefined();
        expect(typeof CapabilitiesUtil.toDiscoveryEntries === "function").toBeTruthy();
        expect(CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry).toBeDefined();
        expect(typeof CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry === "function").toBeTruthy();
        expect(CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo).toBeDefined();
        expect(typeof CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo === "function").toBeTruthy();
        expect(CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray).toBeDefined();
        expect(typeof CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray === "function").toBeTruthy();
    });
});

describe("libjoynr-js.joynr.CapabilitiesUtil.discoveryEntryConversions", () => {
    let providerVersion: any;
    let providerDomain: any;
    let interfaceName: string;
    let providerParticipantId: any;
    let providerQos: ProviderQos;
    let now: any;
    let lastSeenDateMs: number;
    let expiryDateMs: number;
    let publicKeyId: any;
    let address: any;
    let serializedAddress: any;
    let discoveryEntry: DiscoveryEntry;
    let globalDiscoveryEntry: GlobalDiscoveryEntry;
    let localDiscoveryEntryWithMetaInfo: any;
    let globalDiscoveryEntryWithMetaInfo: any;

    beforeEach(() => {
        providerVersion = new Version({
            majorVersion: 47,
            minorVersion: 11
        });
        providerDomain = "providerDomain";
        interfaceName = "interfaceName";
        providerParticipantId = "providerParticipantId";
        providerQos = new ProviderQos(undefined as any);

        now = Date.now();
        lastSeenDateMs = now;
        expiryDateMs = now + 60000;
        publicKeyId = "testPublicKeyId";

        address = new MqttAddress(undefined as any);
        serializedAddress = JSON.stringify(address);

        discoveryEntry = new DiscoveryEntry({
            providerVersion,
            domain: providerDomain,
            interfaceName,
            participantId: providerParticipantId,
            qos: providerQos,
            lastSeenDateMs,
            expiryDateMs,
            publicKeyId
        });

        globalDiscoveryEntry = new GlobalDiscoveryEntry({
            providerVersion,
            domain: providerDomain,
            interfaceName,
            participantId: providerParticipantId,
            qos: providerQos,
            lastSeenDateMs,
            expiryDateMs,
            publicKeyId,
            address: serializedAddress
        });

        localDiscoveryEntryWithMetaInfo = new DiscoveryEntryWithMetaInfo({
            providerVersion,
            domain: providerDomain,
            interfaceName,
            participantId: providerParticipantId,
            qos: providerQos,
            lastSeenDateMs,
            expiryDateMs,
            publicKeyId,
            isLocal: true
        });

        globalDiscoveryEntryWithMetaInfo = new DiscoveryEntryWithMetaInfo({
            providerVersion,
            domain: providerDomain,
            interfaceName,
            participantId: providerParticipantId,
            qos: providerQos,
            lastSeenDateMs,
            expiryDateMs,
            publicKeyId,
            isLocal: false
        });
    });

    function compareDiscoveryEntries(expected: any, actual: any) {
        expect(actual._typeName).toEqual(expected._typeName);
        expect(actual.providerVersion).toEqual(expected.providerVersion);
        expect(actual.domain).toEqual(expected.domain);
        expect(actual.interfaceName).toEqual(expected.interfaceName);
        expect(actual.participantId).toEqual(expected.participantId);
        expect(actual.qos).toEqual(expected.qos);
        expect(actual.lastSeenDateMs).toEqual(expected.lastSeenDateMs);
        expect(actual.expiryDateMs).toEqual(expected.expiryDateMs);
        expect(actual.publicKeyId).toEqual(expected.publicKeyId);
    }

    function compareDiscoveryEntriesWithMetaInfo(isLocal: any, expected: any, actual: any) {
        compareDiscoveryEntries(expected, actual);
        expect(actual.isLocal).toEqual(isLocal);
    }

    function compareGlobalDiscoveryEntries(address: any, expected: any, actual: any) {
        compareDiscoveryEntries(expected, actual);
        expect(actual.address).toEqual(address);
    }

    it("convert to GlobalDiscoveryEntry", () => {
        let convertedDiscoveryEntry = CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, address);
        compareGlobalDiscoveryEntries(serializedAddress, globalDiscoveryEntry, convertedDiscoveryEntry);

        convertedDiscoveryEntry = CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry(
            localDiscoveryEntryWithMetaInfo,
            address
        );
        compareGlobalDiscoveryEntries(serializedAddress, globalDiscoveryEntry, convertedDiscoveryEntry);

        convertedDiscoveryEntry = CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry(
            globalDiscoveryEntryWithMetaInfo,
            address
        );
        compareGlobalDiscoveryEntries(serializedAddress, globalDiscoveryEntry, convertedDiscoveryEntry);
    });

    it("convert to local DiscoveryEntryWithMetaInfo", () => {
        let convertedDiscoveryEntry = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(true, discoveryEntry);
        compareDiscoveryEntriesWithMetaInfo(true, localDiscoveryEntryWithMetaInfo, convertedDiscoveryEntry);

        convertedDiscoveryEntry = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(true, globalDiscoveryEntry);
        compareDiscoveryEntriesWithMetaInfo(true, localDiscoveryEntryWithMetaInfo, convertedDiscoveryEntry);
    });

    it("convert to global DiscoveryEntryWithMetaInfo", () => {
        let convertedDiscoveryEntry = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(false, discoveryEntry);
        compareDiscoveryEntriesWithMetaInfo(false, localDiscoveryEntryWithMetaInfo, convertedDiscoveryEntry);

        convertedDiscoveryEntry = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(false, globalDiscoveryEntry);
        compareDiscoveryEntriesWithMetaInfo(false, localDiscoveryEntryWithMetaInfo, convertedDiscoveryEntry);
    });

    function createArrayOfDiscoveryEntries() {
        const discoveryEntries = [];
        discoveryEntries.push(discoveryEntry);
        discoveryEntries.push(
            new DiscoveryEntry({
                providerVersion,
                domain: "providerDomain2",
                interfaceName,
                participantId: "providerParticipantId2",
                qos: providerQos,
                lastSeenDateMs: 4711,
                expiryDateMs: 4712,
                publicKeyId: "testPublicKeyId2"
            })
        );
        return discoveryEntries;
    }

    function createArrayOfDiscoveryEntriesWithMetaInfo(isLocal: any) {
        const discoveryEntries = [];
        const discoveryEntry2 = new DiscoveryEntryWithMetaInfo({
            providerVersion,
            domain: "providerDomain2",
            interfaceName,
            participantId: "providerParticipantId2",
            qos: providerQos,
            lastSeenDateMs: 4711,
            expiryDateMs: 4712,
            publicKeyId: "testPublicKeyId2",
            isLocal: false
        });
        if (isLocal === true) {
            discoveryEntries.push(localDiscoveryEntryWithMetaInfo);
            discoveryEntry2.isLocal = true;
        } else {
            discoveryEntries.push(globalDiscoveryEntryWithMetaInfo);
        }
        discoveryEntries.push(discoveryEntry2);
        return discoveryEntries;
    }

    function compareArrayOfDiscoveryEntriesWithMetaInfo(isLocal: any, expected: any, actual: any) {
        let i: any;
        expect(actual.length).toEqual(expected.length);
        for (i = 0; i < expected.length; i++) {
            compareDiscoveryEntriesWithMetaInfo(isLocal, expected[i], actual[i]);
        }
    }

    it("convert to array of local DiscoveryEntryWithMetaInfo", () => {
        const discoveryEntryArray = createArrayOfDiscoveryEntries();
        const convertedDiscoveryEntryArray = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(
            true,
            discoveryEntryArray
        );
        const expected = createArrayOfDiscoveryEntriesWithMetaInfo(true);
        compareArrayOfDiscoveryEntriesWithMetaInfo(true, expected, convertedDiscoveryEntryArray);
    });

    it("convert to array of global DiscoveryEntryWithMetaInfo", () => {
        const discoveryEntryArray = createArrayOfDiscoveryEntries();
        const convertedDiscoveryEntryArray = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(
            false,
            discoveryEntryArray
        );
        const expected = createArrayOfDiscoveryEntriesWithMetaInfo(false);
        compareArrayOfDiscoveryEntriesWithMetaInfo(false, expected, convertedDiscoveryEntryArray);
    });
});
