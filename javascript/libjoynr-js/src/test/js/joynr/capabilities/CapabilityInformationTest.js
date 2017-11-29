/*jslint node: true */

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
var GlobalDiscoveryEntry = require("../../../classes/joynr/types/GlobalDiscoveryEntry");
var ProviderQos = require("../../../classes/joynr/types/ProviderQos");
var ProviderScope = require("../../../classes/joynr/types/ProviderScope");
var Version = require("../../../classes/joynr/types/Version");

var capInfo;
beforeEach(function() {
    capInfo = new GlobalDiscoveryEntry({
        providerVersion: new Version({
            majorVersion: 47,
            minorVersion: 11
        }),
        domain: "vehicleADomain",
        interfaceName: "vehicle/cdsnavigation",
        qos: new ProviderQos({
            customParameters: [],
            priority: 1,
            scope: ProviderScope.GLOBAL,
            supportsOnChangeSubscriptions: true
        }),
        address: "address",
        publicKeyId: "",
        participantId: "CDSNavigationParticipantId_vehicleA"
    });
});

describe("libjoynr-js.joynr.capabilities.GlobalDiscoveryEntry", function() {
    it("is instantiable", function() {
        expect(capInfo).toBeDefined();
        expect(capInfo instanceof GlobalDiscoveryEntry).toBeTruthy();
    });
    it("providerVersion is set", function() {
        expect(capInfo.providerVersion).toBeDefined();
        expect(capInfo.providerVersion instanceof Version).toBeTruthy();
    });
});
