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
import ProviderQos = require("../../../generated/joynr/types/ProviderQos");
import GlobalDiscoveryEntry from "../../../generated/joynr/types/GlobalDiscoveryEntry";
import ProviderScope from "../../../generated/joynr/types/ProviderScope";
import Version = require("../../../generated/joynr/types/Version");
import * as UtilInternal from "../../util/UtilInternal";
import GlobalCapabilitiesDirectoryProvider from "../../../generated/joynr/infrastructure/GlobalCapabilitiesDirectoryProvider";

export = function(settings: { bounceProxyBaseUrl: string; brokerUri: string }) {
    const discoveryChannel = "discoverydirectory_channelid";
    const defaultSettings = {
        discoveryChannel,
        getDefaultDiscoveryChannelUrl: () => {
            return `${settings.bounceProxyBaseUrl}/discovery/channels/${defaultSettings.discoveryChannel}/`;
        },
        capabilities: [
            new GlobalDiscoveryEntry({
                providerVersion: new Version({
                    majorVersion: GlobalCapabilitiesDirectoryProvider.MAJOR_VERSION,
                    minorVersion: GlobalCapabilitiesDirectoryProvider.MINOR_VERSION
                }),
                domain: "io.joynr",
                interfaceName: "infrastructure/GlobalCapabilitiesDirectory",
                participantId: "capabilitiesdirectory_participantid",
                qos: new ProviderQos({
                    customParameters: [],
                    priority: 1,
                    scope: ProviderScope.GLOBAL,
                    supportsOnChangeSubscriptions: true
                }),
                lastSeenDateMs: Date.now(),
                expiryDateMs: UtilInternal.getMaxLongValue(),
                publicKeyId: "",
                address: JSON.stringify({
                    _typeName: "joynr.system.RoutingTypes.MqttAddress",
                    topic: discoveryChannel,
                    brokerUri: settings.brokerUri
                })
            })
        ]
    };
    return defaultSettings;
};
