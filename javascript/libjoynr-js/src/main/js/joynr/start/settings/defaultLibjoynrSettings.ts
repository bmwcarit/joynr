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
import { DiscoveryEntryWithMetaInfoMembers } from "../../../generated/joynr/types/DiscoveryEntryWithMetaInfo";
import ProviderQos from "../../../generated/joynr/types/ProviderQos";
import ProviderScope from "../../../generated/joynr/types/ProviderScope";
import * as UtilInternal from "../../util/UtilInternal";
import RoutingProvider from "../../../generated/joynr/system/RoutingProvider";
import DiscoveryProvider from "../../../generated/joynr/system/DiscoveryProvider";
import Version = require("../../../generated/joynr/types/Version");
const discoveryCapability = {
    providerVersion: new Version({
        majorVersion: DiscoveryProvider.MAJOR_VERSION,
        minorVersion: DiscoveryProvider.MINOR_VERSION
    }),
    domain: "io.joynr",
    interfaceName: "system/Discovery",
    participantId: "CC.DiscoveryProvider.ParticipantId",
    qos: new ProviderQos({
        customParameters: [],
        priority: 1,
        scope: ProviderScope.LOCAL,
        supportsOnChangeSubscriptions: true
    }),
    lastSeenDateMs: Date.now(),
    expiryDateMs: UtilInternal.getMaxLongValue(),
    publicKeyId: "",
    isLocal: true
};

const routingCapability = {
    providerVersion: new Version({
        majorVersion: RoutingProvider.MAJOR_VERSION,
        minorVersion: RoutingProvider.MINOR_VERSION
    }),
    domain: "io.joynr",
    interfaceName: "system/Routing",
    participantId: "CC.RoutingProvider.ParticipantId",
    qos: new ProviderQos({
        customParameters: [],
        priority: 1,
        scope: ProviderScope.LOCAL,
        supportsOnChangeSubscriptions: true
    }),
    lastSeenDateMs: Date.now(),
    expiryDateMs: UtilInternal.getMaxLongValue(),
    publicKeyId: "",
    isLocal: true
};
const capabilities: DiscoveryEntryWithMetaInfoMembers[] = [discoveryCapability, routingCapability];
const defaultLibJoynrSettings = {
    capabilities,
    shutdownSettings: {
        clearSubscriptionsEnabled: true,
        clearSubscriptionsTimeoutMs: 1000
    },
    persistencySettings: {
        capabilities: true
    }
};
export = defaultLibJoynrSettings;
