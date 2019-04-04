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
const ProviderScope = require("../../../generated/joynr/types/ProviderScope");
const UtilInternal = require("../../util/UtilInternal");
const RoutingProvider = require("../../../generated/joynr/system/RoutingProvider");
const DiscoveryProvider = require("../../../generated/joynr/system/DiscoveryProvider");
const defaultSettings = {};
const discoveryCapability = {
    providerVersion: {
        majorVersion: DiscoveryProvider.MAJOR_VERSION,
        minorVersion: DiscoveryProvider.MINOR_VERSION
    },
    domain: "io.joynr",
    interfaceName: "system/Discovery",
    participantId: "CC.DiscoveryProvider.ParticipantId",
    qos: {
        customParameters: [],
        priority: 1,
        scope: ProviderScope.LOCAL,
        supportsOnChangeSubscriptions: true
    },
    lastSeenDateMs: Date.now(),
    expiryDateMs: UtilInternal.getMaxLongValue(),
    publicKeyId: "",
    isLocal: true
};

const routingCapability = {
    providerVersion: {
        majorVersion: RoutingProvider.MAJOR_VERSION,
        minorVersion: RoutingProvider.MINOR_VERSION
    },
    domain: "io.joynr",
    interfaceName: "system/Routing",
    participantId: "CC.RoutingProvider.ParticipantId",
    qos: {
        customParameters: [],
        priority: 1,
        scope: ProviderScope.LOCAL,
        supportsOnChangeSubscriptions: true
    },
    lastSeenDateMs: Date.now(),
    expiryDateMs: UtilInternal.getMaxLongValue(),
    publicKeyId: "",
    isLocal: true
};

defaultSettings.persistencySettings = {
    routingTable: false,
    capabilities: true,
    publications: true
};

defaultSettings.shutdownSettings = {
    clearSubscriptionsEnabled: true,
    clearSubscriptionsTimeoutMs: 1000
};

defaultSettings.capabilities = [discoveryCapability, routingCapability];
module.exports = defaultSettings;
