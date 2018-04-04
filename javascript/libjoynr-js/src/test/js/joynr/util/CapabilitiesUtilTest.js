/*jslint es5: true, node: true, nomen: true */
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
var Promise = require("../../../../main/js/global/Promise");
var CapabilitiesUtil = require("../../../../main/js/joynr/util/CapabilitiesUtil");
var DiscoveryEntry = require("../../../../main/js/generated/joynr/types/DiscoveryEntry");
var DiscoveryEntryWithMetaInfo = require("../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo");
var GlobalDiscoveryEntry = require("../../../../main/js/generated/joynr/types/GlobalDiscoveryEntry");
var Version = require("../../../../main/js/generated/joynr/types/Version");
var ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");
var MqttAddress = require("../../../../main/js/generated/joynr/system/RoutingTypes/MqttAddress");

describe("libjoynr-js.joynr.CapabilitiesUtil", function() {
    it("is of correct type and has all members", function() {
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

describe("libjoynr-js.joynr.CapabilitiesUtil.discoveryEntryConversions", function() {
    var providerVersion;
    var providerDomain;
    var interfaceName;
    var providerParticipantId;
    var providerQos;
    var now;
    var lastSeenDateMs;
    var expiryDateMs;
    var publicKeyId;
    var address;
    var serializedAddress;
    var discoveryEntry;
    var globalDiscoveryEntry;
    var localDiscoveryEntryWithMetaInfo;
    var globalDiscoveryEntryWithMetaInfo;

    beforeEach(function() {
        providerVersion = new Version({
            majorVersion: 47,
            minorVersion: 11
        });
        providerDomain = "providerDomain";
        interfaceName = "interfaceName";
        providerParticipantId = "providerParticipantId";
        providerQos = new ProviderQos();

        now = Date.now();
        lastSeenDateMs = now;
        expiryDateMs = now + 60000;
        publicKeyId = "testPublicKeyId";

        address = new MqttAddress();
        serializedAddress = JSON.stringify(address);

        discoveryEntry = new DiscoveryEntry({
            providerVersion: providerVersion,
            domain: providerDomain,
            interfaceName: interfaceName,
            participantId: providerParticipantId,
            qos: providerQos,
            lastSeenDateMs: lastSeenDateMs,
            expiryDateMs: expiryDateMs,
            publicKeyId: publicKeyId
        });

        globalDiscoveryEntry = new GlobalDiscoveryEntry({
            providerVersion: providerVersion,
            domain: providerDomain,
            interfaceName: interfaceName,
            participantId: providerParticipantId,
            qos: providerQos,
            lastSeenDateMs: lastSeenDateMs,
            expiryDateMs: expiryDateMs,
            publicKeyId: publicKeyId,
            address: serializedAddress
        });

        localDiscoveryEntryWithMetaInfo = new DiscoveryEntryWithMetaInfo({
            providerVersion: providerVersion,
            domain: providerDomain,
            interfaceName: interfaceName,
            participantId: providerParticipantId,
            qos: providerQos,
            lastSeenDateMs: lastSeenDateMs,
            expiryDateMs: expiryDateMs,
            publicKeyId: publicKeyId,
            isLocal: true
        });

        globalDiscoveryEntryWithMetaInfo = new DiscoveryEntryWithMetaInfo({
            providerVersion: providerVersion,
            domain: providerDomain,
            interfaceName: interfaceName,
            participantId: providerParticipantId,
            qos: providerQos,
            lastSeenDateMs: lastSeenDateMs,
            expiryDateMs: expiryDateMs,
            publicKeyId: publicKeyId,
            isLocal: false
        });
    });

    function compareDiscoveryEntries(expected, actual) {
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

    function compareDiscoveryEntriesWithMetaInfo(isLocal, expected, actual) {
        compareDiscoveryEntries(expected, actual);
        expect(actual.isLocal).toEqual(isLocal);
    }

    function compareGlobalDiscoveryEntries(address, expected, actual) {
        compareDiscoveryEntries(expected, actual);
        expect(actual.address).toEqual(address);
    }

    it("convert to GlobalDiscoveryEntry", function() {
        var convertedDiscoveryEntry = CapabilitiesUtil.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, address);
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

    it("convert to local DiscoveryEntryWithMetaInfo", function() {
        var convertedDiscoveryEntry = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(true, discoveryEntry);
        compareDiscoveryEntriesWithMetaInfo(true, localDiscoveryEntryWithMetaInfo, convertedDiscoveryEntry);

        convertedDiscoveryEntry = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(true, globalDiscoveryEntry);
        compareDiscoveryEntriesWithMetaInfo(true, localDiscoveryEntryWithMetaInfo, convertedDiscoveryEntry);
    });

    it("convert to global DiscoveryEntryWithMetaInfo", function() {
        var convertedDiscoveryEntry = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(false, discoveryEntry);
        compareDiscoveryEntriesWithMetaInfo(false, localDiscoveryEntryWithMetaInfo, convertedDiscoveryEntry);

        convertedDiscoveryEntry = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfo(false, globalDiscoveryEntry);
        compareDiscoveryEntriesWithMetaInfo(false, localDiscoveryEntryWithMetaInfo, convertedDiscoveryEntry);
    });

    function createArrayOfDiscoveryEntries() {
        var discoveryEntries = [];
        discoveryEntries.push(discoveryEntry);
        discoveryEntries.push(
            new DiscoveryEntry({
                providerVersion: providerVersion,
                domain: "providerDomain2",
                interfaceName: interfaceName,
                participantId: "providerParticipantId2",
                qos: providerQos,
                lastSeenDateMs: 4711,
                expiryDateMs: 4712,
                publicKeyId: "testPublicKeyId2"
            })
        );
        return discoveryEntries;
    }

    function createArrayOfDiscoveryEntriesWithMetaInfo(isLocal) {
        var discoveryEntries = [];
        var discoveryEntry2 = new DiscoveryEntryWithMetaInfo({
            providerVersion: providerVersion,
            domain: "providerDomain2",
            interfaceName: interfaceName,
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

    function compareArrayOfDiscoveryEntriesWithMetaInfo(isLocal, expected, actual) {
        var i;
        expect(actual.length).toEqual(expected.length);
        for (i = 0; i < expected.length; i++) {
            compareDiscoveryEntriesWithMetaInfo(isLocal, expected[i], actual[i]);
        }
    }

    it("convert to array of local DiscoveryEntryWithMetaInfo", function() {
        var discoveryEntryArray = createArrayOfDiscoveryEntries();
        var convertedDiscoveryEntryArray = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(
            true,
            discoveryEntryArray
        );
        var expected = createArrayOfDiscoveryEntriesWithMetaInfo(true);
        compareArrayOfDiscoveryEntriesWithMetaInfo(true, expected, convertedDiscoveryEntryArray);
    });

    it("convert to array of global DiscoveryEntryWithMetaInfo", function() {
        var discoveryEntryArray = createArrayOfDiscoveryEntries();
        var convertedDiscoveryEntryArray = CapabilitiesUtil.convertToDiscoveryEntryWithMetaInfoArray(
            false,
            discoveryEntryArray
        );
        var expected = createArrayOfDiscoveryEntriesWithMetaInfo(false);
        compareArrayOfDiscoveryEntriesWithMetaInfo(false, expected, convertedDiscoveryEntryArray);
    });
});
