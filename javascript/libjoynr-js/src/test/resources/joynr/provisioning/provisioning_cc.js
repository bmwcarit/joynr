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

(function() {
    var setupProvisionedData = function(provisioning) {
        var discoveryChannel = "discoverydirectory_channelid";
        provisioning.bounceProxyBaseUrl = "${joynr.provisioning.bounceProxyBaseUrl}";
        provisioning.bounceProxyUrl = provisioning.bounceProxyBaseUrl + "/bounceproxy/";

        provisioning.internalMessagingQos = {
            ttl : provisioning.ttl
        };

        provisioning.channelUrls = {};
        provisioning.channelUrls[discoveryChannel] = [ provisioning.bounceProxyBaseUrl + "/discovery/channels/" + discoveryChannel + "/" ];
        // joynr.provisioning.messaging.maxQueueSizeInKBytes = 10000;
        var globalCapDirCapability = {
            domain : "io.joynr",
            interfaceName : "infrastructure/GlobalCapabilitiesDirectory",
            providerQos : {
                qos : [],
                version : 0,
                priority : 1,
                scope : "GLOBAL",
                onChangeSubscriptions : true
            },
            channelId : discoveryChannel,
            participantId : "capabilitiesdirectory_participantid"
        };

        var channelUrlDirCapability = {
            domain : "io.joynr",
            interfaceName : "infrastructure/ChannelUrlDirectory",
            providerQos : {
                qos : [],
                version : 0,
                priority : 1,
                scope : "GLOBAL",
                onChangeSubscriptions : true
            },
            channelId : discoveryChannel,
            participantId : "channelurldirectory_participantid"
        };

        provisioning.capabilities = [ globalCapDirCapability, channelUrlDirCapability ];

        provisioning.logging = {
            configuration : {
                name : "test config",
                appenders : {
                    Console : {
                        name : "STDOUT",
                        PatternLayout : {
                            pattern : "%m%n"
                        }
                    }
                },
                loggers : {
                    root : {
                        level : "debug",
                        AppenderRef : {
                            ref : "STDOUT"
                        }
                    }
                }
            }
        };
        localStorage.setItem("joynr.participant.io.joynr.system.Discovery.", "CC.DiscoveryProvider.ParticipantId");
        localStorage.setItem("joynr.participant.io.joynr.system.Routing.", "CC.RoutingProvider.ParticipantId");
        return provisioning;
    };

    // AMD support
    if (typeof define === 'function' && define.amd) {
        define("joynr/provisioning/provisioning_cc",
            ["joynr/provisioning/provisioning_common"], function(provisioning) {
            return setupProvisionedData(provisioning);
        });
    } else {
        // expect that joynrprovisioning.common has been loaded before
        setupProvisionedData(window.joynr.provisioning);
    }
}());
