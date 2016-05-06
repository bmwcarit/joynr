/*jslint nomen: true */
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
    var setupDefaultClusterControllerSettings =
            function(settings) {
                var defaultSettings = {};
                defaultSettings.discoveryChannel = "discoverydirectory_channelid";

                defaultSettings.getDefaultDiscoveryChannelUrl =
                        function() {
                            return settings.bounceProxyBaseUrl
                                + "/discovery/channels/"
                                + defaultSettings.discoveryChannel
                                + "/";
                        };

                var globalCapDirCapability = {
                    domain : "io.joynr",
                    interfaceName : "infrastructure/GlobalCapabilitiesDirectory",
                    providerQos : {
                        qos : [],
                        version : 0,
                        priority : 1,
                        isLocalOnly : false,
                        onChangeSubscriptions : true
                    },
                    address : JSON.stringify({
                        _typeName : "joynr.system.RoutingTypes.ChannelAddress",
                        channelId : defaultSettings.discoveryChannel,
                        messagingEndpointUrl : defaultSettings.getDefaultDiscoveryChannelUrl()
                    }),
                    publicKeyId : "",
                    participantId : "capabilitiesdirectory_participantid"
                };

                defaultSettings.capabilities = [ globalCapDirCapability
                ];
                return defaultSettings;
            };

    // AMD support
    if (typeof define === 'function' && define.amd) {
        define("joynr/start/settings/defaultClusterControllerSettings", [], function() {
            return setupDefaultClusterControllerSettings;
        });
    } else {
        window.joynr = window.joynr || {};
        window.joynr.start = window.joynr.start || {};
        window.joynr.start.defaultClusterControllerSettings = setupDefaultClusterControllerSettings;
    }
}());
