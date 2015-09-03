/*jslint node: true */

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

var provisioning = {};
provisioning.ccAddress = {
    protocol : "ws",
    host : "localhost",
    port : 4242,
    path : ""
};
provisioning.bounceProxyBaseUrl = "http://localhost:8080";
provisioning.bounceProxyUrl = provisioning.bounceProxyBaseUrl + "/bounceproxy/";
var discoveryChannelId = "discoverydirectory_channelid";
provisioning.channelUrls = {};
provisioning.channelUrls[discoveryChannelId] =
        [ provisioning.bounceProxyBaseUrl + "/discovery/channels/" + discoveryChannelId + "/"
        ];
var globalCapDirCapability = {
    domain : "io.joynr",
    interfaceName : "infrastructure/globalcapabilitiesdirectory",
    providerQos : {
        qos : [],
        version : 0,
        priority : 1,
        isLocalOnly : false,
        onChangeSubscriptions : true
    },
    channelId : discoveryChannelId,
    participantId : "capabilitiesdirectory_participantid"
};
var channelUrlDirCapability = {
    domain : "io.joynr",
    interfaceName : "infrastructure/channelurldirectory",
    providerQos : {
        qos : [],
        version : 0,
        priority : 1,
        isLocalOnly : false,
        onChangeSubscriptions : true
    },
    channelId : discoveryChannelId,
    participantId : "channelurldirectory_participantid"
};
var discoveryCapability = {
    domain : "io.joynr",
    interfaceName : "system/discovery",
    providerQos : {
        qos : [],
        version : 0,
        priority : 1,
        isLocalOnly : false,
        onChangeSubscriptions : true
    },
    participantId : "CC.DiscoveryProvider.ParticipantId"
};
var routingCapability = {
    domain : "io.joynr",
    interfaceName : "system/routing",
    providerQos : {
        qos : [],
        version : 0,
        priority : 1,
        isLocalOnly : false,
        onChangeSubscriptions : true
    },
    participantId : "CC.RoutingProvider.ParticipantId"
};
provisioning.capabilities = [
    globalCapDirCapability,
    channelUrlDirCapability,
    discoveryCapability,
    routingCapability
];

provisioning.logging = {
    configuration : {
        loggers : {
            root : {
                level : "debug"
            }
        }
    }
};

module.exports = provisioning;