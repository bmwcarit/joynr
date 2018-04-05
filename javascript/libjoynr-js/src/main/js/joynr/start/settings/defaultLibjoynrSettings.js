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
var ProviderScope = require("../../../joynr/types/ProviderScope");
var Util = require("../../util/UtilInternal");
var defaultSettings = {};
var discoveryCapability = {
    providerVersion: {
        majorVersion: 0,
        minorVersion: 1
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
    expiryDateMs: Util.getMaxLongValue(),
    publicKeyId: "",
    isLocal: true
};

var routingCapability = {
    providerVersion: {
        majorVersion: 0,
        minorVersion: 1
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
    expiryDateMs: Util.getMaxLongValue(),
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
