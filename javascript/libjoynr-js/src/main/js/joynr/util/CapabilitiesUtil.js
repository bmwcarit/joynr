/*global unescape: true, Blob: true, Array:true */
/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

define("joynr/util/CapabilitiesUtil", [ "joynr/types/DiscoveryEntry"
], function(DiscoveryEntry) {

    /**
     * @name CapabilitiesUtil
     * @class
     * @classdesc provides utility functions for dealing with capabilities
     */
    var CapabilitiesUtil = {};

    /**
     * This method transforms a capabilityInformation into an object of type "DiscoveryEntry"
     *
     * @function
     * @name CapabilitiesUtil#toDiscoveryEntry
     *
     * @param {CapabilityInformation}
     *            capabilityInformation the object to be transformed
     *
     * @returns {DiscoveryEntry} the transformed object
     */
    CapabilitiesUtil.toDiscoveryEntry = function toDiscoveryEntry(capabilityInformation) {
        return new DiscoveryEntry({
            providerVersion : capabilityInformation.providerVersion,
            domain : capabilityInformation.domain,
            interfaceName : capabilityInformation.interfaceName,
            qos : capabilityInformation.providerQos,
            participantId : capabilityInformation.participantId,
            lastSeenDateMs : Date.now()
        });
    };

    /**
     * This method transforms an array of capabilityInformations into an array of objects
     * of type DiscoveryEntry
     *
     * @function
     * @name CapabilitiesUtil#toDiscoveryEntries
     *
     * @param {Array}
     *            capabilityInformations array of capability information
     *
     * @returns {Array} array of transformed objects of type DiscoveryEntry
     */
    CapabilitiesUtil.toDiscoveryEntries =
            function toDiscoveryEntries(capabilityInformations) {
                var discoveryEntries = [], i;
                if (capabilityInformations) {
                    for (i = 0; i < capabilityInformations.length; i++) {
                        discoveryEntries.push(CapabilitiesUtil
                                .toDiscoveryEntry(capabilityInformations[i]));
                    }
                }
                return discoveryEntries;
            };

    return CapabilitiesUtil;

});
