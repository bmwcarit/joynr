/*global joynrTestRequire: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

joynrTestRequire("joynr/capabilities/TestCapabilityInformation", [
    "joynr/types/CapabilityInformation",
    "joynr/types/ProviderQos",
    "joynr/types/ProviderScope"
], function(CapabilityInformation, ProviderQos, ProviderScope) {

    var capInfo;
    beforeEach(function() {
        capInfo = new CapabilityInformation({
            domain : "vehicleADomain",
            interfaceName : "vehicle/cdsnavigation",
            providerQos : new ProviderQos({
                customParameters : [],
                priority : 1,
                scope : ProviderScope.GLOBAL,
                supportsOnChangeSubscriptions : true
            }),
            channelId : "vehicleAChannelId",
            participantId : "CDSNavigationParticipantId_vehicleA"
        });
    });

    describe("libjoynr-js.joynr.capabilities.CapabilityInformation", function() {
        it("is instantiable", function() {
            expect(capInfo).toBeDefined();
            expect(capInfo instanceof CapabilityInformation).toBeTruthy();
        });
    });

}); // require
