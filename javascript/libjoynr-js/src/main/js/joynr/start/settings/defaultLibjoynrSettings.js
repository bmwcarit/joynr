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

(function() {
    var setupDefaultLibjoynrSettings = function(defaultSettings, ProviderScope) {
        var discoveryCapability = {
            domain : "io.joynr",
            interfaceName : "system/Discovery",
            providerQos : {
                customParameters : [],
                priority : 1,
                scope : ProviderScope.LOCAL,
                onChangeSubscriptions : true
            },
            participantId : "CC.DiscoveryProvider.ParticipantId"
        };

        var routingCapability = {
            domain : "io.joynr",
            interfaceName : "system/Routing",
            providerQos : {
                customParameters : [],
                priority : 1,
                scope : ProviderScope.LOCAL,
                onChangeSubscriptions : true
            },
            participantId : "CC.RoutingProvider.ParticipantId"
        };

        defaultSettings.capabilities = [
            discoveryCapability,
            routingCapability
        ];
        return defaultSettings;
    };

    // AMD support
    if (typeof define === 'function' && define.amd) {
        define("joynr/start/settings/defaultLibjoynrSettings", [ "joynr/types/ProviderScope"
        ], function(ProviderScope) {
            return setupDefaultLibjoynrSettings({}, ProviderScope);
        });
    } else {
        window.joynr = window.joynr || {};
        window.joynr.start = window.joynr.start || {};
        window.joynr.start.defaultLibjoynrSettings =
                window.joynr.start.defaultLibjoynrSettings || {};
        setupDefaultLibjoynrSettings(
                window.joynr.start.defaultLibjoynrSettings,
                window.ProviderScope);
    }
}());
